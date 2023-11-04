use std::net::{UdpSocket, SocketAddr};
use std::fmt::Write;

#[derive(Clone, PartialEq)]
pub enum Request {
    // Addr, Amount
    ReadCoreMemory(u64, u64),

    // Addr, Bytes
    WriteCoreMemory(u64, Vec<u8>),
}

#[repr(u8)]
pub enum RequestType {
    Invalid = 0,
    ReadCoreMemory,
    WriteCoreMemory
}

impl Request {
    pub fn get_request_type(&self) -> RequestType {
        match *self {
            Request::ReadCoreMemory(_, _) => RequestType::ReadCoreMemory,
            Request::WriteCoreMemory(_, _) => RequestType::WriteCoreMemory
        }
    }
}

pub struct QueuedRequest {
    pub from: SocketAddr,
    pub request: Request,
    pub socket: UdpSocket
}
impl QueuedRequest {
    pub fn handle_read_core_memory(&self, bytes: &[u8]) {
        let (addr, _amount) = match self.request {
            Request::ReadCoreMemory(n, m) => (n, m),
            _ => panic!()
        };

        let mut result = String::with_capacity(bytes.len() * 3 + 256);
        let _dontcare = write!(&mut result, "READ_CORE_MEMORY {addr:x}");
        for b in bytes {
            let _dontcare = write!(&mut result, " {b:02X}");
        }
        let _dontcare = self.socket.send_to(result.as_bytes(), self.from);
    }
}

/// Handles UDP commands, supporting RetroArch's commands.
pub struct UDPCommandHandler {
    socket: UdpSocket,
    queue: Vec<QueuedRequest>
}

pub fn decode_hex(s: &str) -> Result<u64, ()> {
    if s.len() > 8 {
        return Err(());
    }

    let (start, adding) = if s.len() % 2 == 1 {
        (1, (u8::from_str_radix(&s[0..1], 16).map_err(|_| ())? as u64) << ((s.len() - 1) * 4))
    }
    else {
        (0, 0)
    };

    let bytes = (start..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16));

    let mut val = 0u64;
    for i in bytes {
        let b = match i {
            Ok(n) => n,
            Err(_) => return Err(())
        };
        val = (val << 8) | (b as u64);
    }

    Ok(val | adding)
}

pub fn decode_num(s: &str) -> Result<u64, ()> {
    if s.starts_with("0x") {
        decode_hex(&s[2..])
    }
    else {
        s.parse().map_err(|_| ())
    }
}

impl UDPCommandHandler {
    pub fn get_socket(&self) -> &UdpSocket {
        &self.socket
    }

    pub fn new() -> std::io::Result<Self> {
        let socket = UdpSocket::bind("127.0.0.1:55355")?;
        socket.set_read_timeout(Some(std::time::Duration::from_millis(1)))?;
        socket.set_write_timeout(Some(std::time::Duration::from_millis(1)))?;
        Ok(Self { socket, queue: Vec::new() })
    }

    pub fn refresh(&mut self) {
        let mut buf = [0u8; 1024];
        'outer_loop:
        loop {
            let (data, from) = match self.socket.recv_from(&mut buf) {
                Ok((size, addr)) => (&buf[0..size], addr),
                Err(_) => return
            };

            let string = std::str::from_utf8(data);
            if string.is_err() {
                continue
            }

            let splitted: Vec<&str> = string.unwrap().split_ascii_whitespace().collect();
            if splitted.len() < 1 {
                continue
            }

            let cmd = splitted[0];

            match cmd {
                "WRITE_CORE_MEMORY" => {
                    if splitted.len() < 2 {
                        continue
                    }

                    let addr = match decode_hex(splitted[1]) {
                        Ok(n) => n,
                        Err(_) => continue
                    };

                    let mut bytes = Vec::with_capacity(splitted.len());
                    for i in &splitted[2..] {
                        let byte: u8 = match decode_num(i) {
                            Ok(n) => n as u8,
                            Err(_) => continue 'outer_loop
                        };
                        bytes.push(byte);
                    }

                    self.queue.push(QueuedRequest { from, request: Request::WriteCoreMemory(addr, bytes), socket: self.socket.try_clone().expect("can't clone socket") })
                },
                "READ_CORE_MEMORY" => {
                    if splitted.len() != 3 {
                        continue
                    }

                    let addr = match decode_hex(splitted[1]) {
                        Ok(n) => n,
                        Err(_) => continue
                    };

                    let amount = match decode_num(splitted[2]) {
                        Ok(n) => n,
                        Err(_) => continue
                    };
                    self.queue.push(QueuedRequest { from, request: Request::ReadCoreMemory(addr, amount), socket: self.socket.try_clone().expect("can't clone socket") })
                },
                n => {
                    eprintln!("Unrecognized command {n}");
                    continue
                }
            }
        }
    }

    pub fn first(&self) -> Option<&QueuedRequest> {
        self.queue.first()
    }

    pub fn remove_first(&mut self) {
        self.queue.remove(0);
    }
}

#[no_mangle]
pub extern "C" fn RA_UDP_CMD_new() -> *mut UDPCommandHandler {
    let b = Box::new(match UDPCommandHandler::new() {
        Ok(n) => n,
        Err(_) => return std::ptr::null_mut()
    });
    Box::into_raw(b)
}

#[no_mangle]
pub unsafe extern "C" fn RA_UDP_CMD_free(handler: *mut UDPCommandHandler) {
    if !handler.is_null() {
        drop(Box::from_raw(handler));
    }
}

#[repr(C)]
pub struct SizedPtr {
    byteptr: *const u8,
    size: u64
}

#[no_mangle]
pub extern "C" fn RA_UDP_CMD_get_request_data(handler: &UDPCommandHandler, rtype: &mut RequestType, addr: &mut u64, param: &mut SizedPtr) {
    let params = match handler.first() {
        None => {
            *rtype = RequestType::Invalid;
            return;
        },
        Some(n) => {
            *rtype = n.request.get_request_type();
            match &n.request {
                Request::ReadCoreMemory(addr, length) => (addr, SizedPtr { size: *length, byteptr: std::ptr::null() }),
                Request::WriteCoreMemory(addr, bytes) => (addr, SizedPtr { size: bytes.len() as u64, byteptr: bytes.as_ptr() })
            }
        }
    };
    *addr = *params.0;
    *param = params.1;
}

#[no_mangle]
pub extern "C" fn RA_UDP_CMD_pop_request(handler: &mut UDPCommandHandler) {
    handler.remove_first()
}

#[no_mangle]
pub extern "C" fn RA_UDP_CMD_refresh(handler: &mut UDPCommandHandler) {
    handler.refresh()
}

#[no_mangle]
pub unsafe extern "C" fn RA_UDP_CMD_handle_read_request(handler: &UDPCommandHandler, bytes: *const u8) {
    let request = handler.first().unwrap();
    let len = match request.request {
        Request::ReadCoreMemory(_addr, len) => len,
        _ => unreachable!()
    } as usize;
    request.handle_read_core_memory(std::slice::from_raw_parts(bytes, len))
}
