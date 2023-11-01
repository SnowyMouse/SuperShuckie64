use std::net::{UdpSocket, SocketAddr};

#[derive(Copy, Clone, PartialEq)]
pub enum Request {
    ReadCoreMemory(u64, u64),
    WriteCoreMemory(u64, u8),
}

/// Handles UDP commands, supporting RetroArch's commands.
pub struct UDPCommandHandler {
    socket: UdpSocket,
    queue: Vec<(SocketAddr, Request)>
}

pub fn decode_hex(s: &str) -> Result<u64, ()> {
    if s.len() > 8 {
        return Err(());
    }

    let bytes = (0..s.len())
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

    Ok(val)
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
        let mut buf = [0u8; 65536];
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
                    if splitted.len() != 3 {
                        continue
                    }

                    let addr = match decode_hex(splitted[1]) {
                        Ok(n) => n,
                        Err(_) => continue
                    };

                    let byte = match decode_num(splitted[2]) {
                        Ok(n) => n,
                        Err(_) => continue
                    };

                    self.queue.push((from, Request::WriteCoreMemory(addr, byte as u8)))
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

                    self.queue.push((from, Request::ReadCoreMemory(addr, amount)))
                },
                n => {
                    eprintln!("Unrecognized command {n}");
                    continue
                }
            }
        }
    }

    pub fn first(&self) -> Option<&(SocketAddr,Request)> {
        self.queue.first()
    }

    pub fn remove_first(&mut self) {
        self.queue.remove(0);
    }
}

#[test]
fn do_test() {
    let mut handler = UDPCommandHandler::new().unwrap();
    loop {
        handler.refresh();

        loop {
            let first = handler.first();
            if first.is_none() {
                break;
            }

            let (addr, req) = first.unwrap();

            handler.remove_first();
        }
    }
}
