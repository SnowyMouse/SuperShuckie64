//! Functionality for generating a stream of a replay.

use crate::header::*;
use crate::error::*;
use crate::packet::*;

use std::io::Cursor;

/// Writer object for generating replays.
#[derive(Clone, PartialEq, Debug)]
pub struct ReplayWriter {
    current_delay: u8,
    current_stream: Vec<u8>,
    header: ReplayHeader,
}

impl ReplayWriter {
    /// Initialize a new ReplayWriter structure with a set header.
    pub fn new(header: ReplayHeader) -> Self {
        Self {
            current_stream: header.as_bytes().to_vec(),
            header,
            current_delay: 0,
        }
    }

    /// Initialize a new ReplayWriter structure from an existing stream.
    pub fn from_stream(stream: &[u8]) -> LoadResult<Self> {
        let mut cursor = Cursor::new(stream);
        let header = ReplayHeader::from_stream(&mut cursor)?;

        Ok(Self {
            current_stream: stream.to_vec(),
            current_delay: 0,
            header
        })
    }

    /// Get the stream data in its current state.
    pub fn get_stream(&self) -> &[u8] {
        &self.current_stream
    }

    /// Advance the internal frame counter to the next frame.
    ///
    /// You should call this on vblank for the game, itself.
    pub fn next_frame(&mut self) {
        self.current_delay += 1;
        if self.current_delay == 255 {
            self.write_packet(&NoOp::default());
        }
    }

    /// Get the metadata that is placed at the start of the stream.
    pub fn get_header(&self) -> &ReplayHeader {
        &self.header
    }

    /// Write a packet to the stream.
    pub fn write_packet<P: Packet>(&mut self, packet: &P) {
        self.current_stream.push(self.current_delay);
        self.current_delay = 0;

        self.current_stream.push(P::get_packet_type() as u8);
        packet.write(&mut self.current_stream).unwrap();
    }
}
