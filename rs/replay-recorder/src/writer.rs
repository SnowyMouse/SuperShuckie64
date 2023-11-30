//! Functionality for generating a stream of a replay.

use crate::header::*;
use crate::reader::*;
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
        let mut cursor = Cursor::new(&stream);
        let header = ReplayHeader::from_stream(&mut cursor)?;

        if header.flags.is_compressed {
            return Self::from_stream_vec(Self::decompress_stream(&stream)?);
        }

        return Self::from_stream_vec(stream.to_vec())
    }

    /// Initialize a new ReplayWriter structure from an existing stream without reallocation (unless it's compressed!).
    pub fn from_stream_vec(stream: Vec<u8>) -> LoadResult<Self> {
        let mut cursor = Cursor::new(&stream);
        let header = ReplayHeader::from_stream(&mut cursor)?;

        if header.flags.is_compressed {
            return Self::from_stream_vec(Self::decompress_stream(&stream)?);
        }

        Ok(Self {
            current_stream: stream,
            current_delay: 0,
            header
        })
    }

    /// Decompress the stream.
    pub fn decompress_stream(stream: &[u8]) -> LoadResult<Vec<u8>> {
        let mut cursor = Cursor::new(&stream);
        let mut header = ReplayHeader::from_stream(&mut cursor)?;

        if header.flags.is_compressed {
            header.flags.is_compressed = false;

            let header_bytes = header.as_bytes();
            let capacity = match zstd::bulk::Decompressor::upper_bound(&stream[HEADER_SIZE..]) {
                Some(n) => n,
                None => return Err(Error::ParseError("invalid zstd stream")),
            };
            let mut result = match zstd::bulk::decompress(&stream[HEADER_SIZE..], capacity) {
                Ok(n) => n,
                Err(_) => return Err(Error::ParseError("invalid zstd stream")),
            };

            let mut final_stream = Vec::with_capacity(result.len() + header_bytes.len());
            final_stream.extend_from_slice(&header_bytes);
            final_stream.append(&mut result);
            return Ok(final_stream)
        }

        return Ok(stream.to_vec());
    }

    /// Initialize a new ReplayWriter structure from a slice of packets.
    pub fn from_reader_items(items: &[ReplayReaderItem], header: ReplayHeader) -> Self {
        let mut stream = Self {
            current_stream: header.as_bytes().to_vec(),
            header,
            current_delay: 0
        };

        for i in items {
            do_to_packet!(i.get_packet(), packet, {
                stream.write_packet(packet);
            });
        }

        stream
    }

    /// Get the stream data in its current state.
    pub fn get_stream(&self) -> &[u8] {
        &self.current_stream
    }

    /// Compress the stream and return it
    pub fn compress_stream(&self) -> Vec<u8> {
        let mut compressed = zstd::bulk::compress(&self.current_stream[HEADER_SIZE..], 8).unwrap();
        let mut header = self.header;

        header.flags.is_compressed = true;
        let header_bytes = header.as_bytes();

        let mut result = Vec::with_capacity(header_bytes.len() + compressed.len());
        result.extend_from_slice(&header_bytes);
        result.append(&mut compressed);
        result
    }

    /// Advance the internal frame counter to the next frame.
    ///
    /// You should call this on vblank for the game, itself.
    pub fn next_frame(&mut self) {
        self.current_delay += 1;
        if self.current_delay == u8::MAX {
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

        self.current_stream.push(packet.get_packet_type() as u8);
        packet.write(&mut self.current_stream).unwrap();
    }
}
