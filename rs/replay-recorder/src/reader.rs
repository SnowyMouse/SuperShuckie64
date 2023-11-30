//! Functionality for parsing a stream of a replay.

use std::io::Read;
use super::packet::*;
use super::header::*;
use super::error::*;

/// Reader object for reading replays.
pub struct ReplayReader<R: Read> {
    header: ReplayHeader,
    stream: R,
    done: bool
}

impl<R: Read> ReplayReader<R> {
    /// Initialize the reader with the given input stream.
    pub fn new(mut stream: R) -> Result<Self, Error> {
        let header = ReplayHeader::from_stream(&mut stream)?;
        if header.flags.is_compressed {
            return Err(Error::NeedsDecompressed)
        }
        Ok(Self {
            header,
            stream,
            done: false
        })
    }

    /// Get the header to read metadata.
    pub fn get_header(&self) -> &ReplayHeader {
        &self.header
    }
}


impl<R: Read> Iterator for ReplayReader<R> {
    type Item = Result<ReplayReaderItem, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done {
            return None
        }

        let mut delay = [0];
        if self.stream.read_exact(&mut delay).is_err() {
            self.done = true;
            return None
        }

        let mut packet_type = [0];
        match self.stream.read_exact(&mut packet_type) {
            Ok(()) => (),
            Err(e) => {
                self.done = true;
                return Some(Err(Error::IO(e)))
            }
        }

        let packet_type: PacketType = match packet_type[0].try_into() {
            Ok(n) => n,
            Err(_) => {
                self.done = true;
                return Some(Err(Error::ParseError("invalid or unknown packet type in stream")));
            }
        };

        let packet = match packet_type {
            PacketType::NoOp => read_packet_to_box::<NoOp, R>(&mut self.stream),
            PacketType::CustomData => read_packet_to_box::<CustomData, R>(&mut self.stream),
            PacketType::LoadSRAM => read_packet_to_box::<LoadSRAM, R>(&mut self.stream),
            PacketType::ChangeGameSpeed => read_packet_to_box::<ChangeGameSpeed, R>(&mut self.stream),
            PacketType::Bookmark => read_packet_to_box::<Bookmark, R>(&mut self.stream),
            PacketType::SetInput8 => read_packet_to_box::<SetInput8, R>(&mut self.stream),
            PacketType::SetInput16 => read_packet_to_box::<SetInput16, R>(&mut self.stream),
            PacketType::SetInput32 => read_packet_to_box::<SetInput32, R>(&mut self.stream),
            PacketType::SetInput64 => read_packet_to_box::<SetInput64, R>(&mut self.stream),
            PacketType::SetInputData8 => read_packet_to_box::<SetInputData8, R>(&mut self.stream),
            PacketType::SetInputData16 => read_packet_to_box::<SetInputData16, R>(&mut self.stream),
            PacketType::SetInputData32 => read_packet_to_box::<SetInputData32, R>(&mut self.stream),
            PacketType::SetInputData64 => read_packet_to_box::<SetInputData64, R>(&mut self.stream),
            PacketType::AddSaveState => read_packet_to_box::<AddSaveState, R>(&mut self.stream),
            PacketType::LoadSaveState => read_packet_to_box::<LoadSaveState, R>(&mut self.stream),
            PacketType::WriteRAMByteAddr32 => read_packet_to_box::<WriteRAMByteAddr32, R>(&mut self.stream),
            PacketType::WriteRAMByteAddr64 => read_packet_to_box::<WriteRAMByteAddr64, R>(&mut self.stream),
            PacketType::WriteROMByteOffset32 => read_packet_to_box::<WriteROMByteOffset32, R>(&mut self.stream),
            PacketType::WriteROMByteOffset64 => read_packet_to_box::<WriteROMByteOffset64, R>(&mut self.stream),
            PacketType::ResetSystem => read_packet_to_box::<ResetSystem, R>(&mut self.stream)
        };

        Some(packet.map(|packet| ReplayReaderItem { delay: delay[0], packet_type, packet} ))
    }
}

/// Item obtained from a `ReplayReader`
pub struct ReplayReaderItem {
    delay: u8,
    packet_type: PacketType,
    packet: Box<dyn Packet>
}

impl ReplayReaderItem {
    /// Get the packet if it matches the desired type.
    pub fn get_packet_if_matches<P: Packet>(&self) -> Option<&P> {
        self.packet.as_any().downcast_ref::<P>()
    }

    /// Get the number of frames to wait before executing this packet.
    pub fn get_delay(&self) -> u8 {
        self.delay
    }

    /// Get the packet type.
    pub fn get_packet_type(&self) -> PacketType {
        self.packet_type
    }

    /// Get the reference to the packet
    pub fn get_packet(&self) -> &dyn Packet {
        self.packet.as_ref()
    }
}

fn read_packet_to_box<P: Packet + 'static, R: Read>(stream: &mut R) -> Result<Box<dyn Packet>, Error> where P: Sized {
    P::read(stream)
        .map(|o| -> Box<dyn Packet> { Box::new(o) })
}
