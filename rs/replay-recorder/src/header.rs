//! Functionality for reading metadata of a replay.

use std::io::{Cursor, Read, Write};
use super::error::*;

const MAGIC: u64 = 0x47616D655245433E;
const VERSION: u64 = 0;

const HEADER_SIZE: usize = 512;

/// Header containing metadata about a replay that can be verified.
#[derive(Copy, Clone, PartialEq, Debug, Default)]
pub struct ReplayHeader {
    /// Emulator-specific info (i.e. name and version). This should be encoded in UTF-8 and padded with 00's.
    pub emulator_info: [u8;32],

    /// Timestamp the replay was recorded (milliseconds since epoch)
    pub timestamp_start: u128,

    /// Emulator-specific info. This should be encoded in UTF-8 and padded with 00's.
    pub rom_name: [u8;32],

    /// ROM checksum bytes (SHA-256)
    pub rom_checksum: [u8;32],

    /// BIOS checksum bytes (SHA-256)
    pub bios_checksum: [u8;32],
}

impl ReplayHeader {
    /// Instantiate from the given data, returning `None` if the strings are too large.
    /// 
    /// `rom_data` and `bios_data` will be checksum'd using SHA-256.
    pub fn new_from_strs(emulator_info: &str, rom_name: &str, rom_data: &[u8], bios_data: &[u8]) -> Option<Self> {
        let mut emulator_info = emulator_info.as_bytes().to_owned();
        let mut rom_name = rom_name.as_bytes().to_owned();

        if emulator_info.len() > 32 || rom_name.len() > 32 {
            return None
        }

        emulator_info.resize(32, 0);
        rom_name.resize(32, 0);

        Some(Self::new_from_data(
            emulator_info.as_slice().try_into().unwrap(), 
            rom_name.as_slice().try_into().unwrap(), 
            rom_data, 
            bios_data
        ))
    }

    /// Instantiate from the given data.
    /// 
    /// `rom_data` and `bios_data` will be checksum'd using SHA-256.
    pub fn new_from_data(emulator_info: &[u8; 32], rom_name: &[u8; 32], rom_data: &[u8], bios_data: &[u8]) -> Self {
        ReplayHeader {
            emulator_info: emulator_info.to_owned(),
            rom_name: rom_name.to_owned(),
            timestamp_start: std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap_or_default().as_millis(),
            rom_checksum: checksum(rom_data),
            bios_checksum: checksum(bios_data)
        }
    }

    /// Parse the header of the input.
    pub fn from_stream<R: Read>(stream: &mut R) -> LoadResult<Self> {
        // Read all bytes of the header upfront
        let mut bytes = [0u8; HEADER_SIZE];
        stream.read_exact(&mut bytes).map_err(|e| Error::IO(e))?;

        // We can now read this as a stream
        let mut byte_reader = Cursor::new(bytes.as_slice());

        let mut magic_bytes = [0u8; 8];
        byte_reader.read_exact(&mut magic_bytes).unwrap();
        if u64::from_be_bytes(magic_bytes) != MAGIC {
            return Err(Error::ParseError("invalid stream - magic mismatch"));
        }

        let mut version_bytes = [0u8; 8];
        byte_reader.read_exact(&mut version_bytes).unwrap();
        if u64::from_be_bytes(version_bytes) != VERSION {
            return Err(Error::ParseError("invalid stream - version mismatch"));
        }

        let mut timestamp_bytes = [0u8; 16];
        let mut rom_name = [0u8; 32];
        let mut emulator_info = [0u8; 32];
        let mut rom_checksum = [0u8; 32];
        let mut bios_checksum = [0u8; 32];

        byte_reader.read_exact(&mut timestamp_bytes).unwrap();
        byte_reader.read_exact(&mut rom_name).unwrap();
        byte_reader.read_exact(&mut emulator_info).unwrap();
        byte_reader.read_exact(&mut rom_checksum).unwrap();
        byte_reader.read_exact(&mut bios_checksum).unwrap();

        Ok(Self {
            emulator_info,
            timestamp_start: u128::from_be_bytes(timestamp_bytes),
            rom_checksum,
            bios_checksum,
            rom_name
        })
    }

    /// Get the header as bytes.
    pub fn as_bytes(&self) -> [u8; 512] {
        let mut new_header = [0u8; 512];
        let mut cursor = Cursor::new(new_header.as_mut_slice());

        cursor.write_all(&MAGIC.to_be_bytes()).unwrap();
        cursor.write_all(&VERSION.to_be_bytes()).unwrap();
        cursor.write_all(&self.timestamp_start.to_be_bytes()).unwrap();
        cursor.write_all(&self.rom_name).unwrap();
        cursor.write_all(&self.emulator_info).unwrap();
        cursor.write_all(&self.rom_checksum).unwrap();
        cursor.write_all(&self.bios_checksum).unwrap();

        new_header
    }
}

fn checksum(what: &[u8]) -> [u8;32] {
    let digest = sha256::digest(what);
    let digest_bytes = digest.as_bytes();
    let mut result = [0u8;32];

    for i in 0..result.len() {
        let parsedigit = |d: u8| {
            match d as char {
                '0' => 0x0,
                '1' => 0x1,
                '2' => 0x2,
                '3' => 0x3,
                '4' => 0x4,
                '5' => 0x5,
                '6' => 0x6,
                '7' => 0x7,
                '8' => 0x8,
                '9' => 0x9,
                'a' => 0xA,
                'b' => 0xB,
                'c' => 0xC,
                'd' => 0xD,
                'e' => 0xE,
                'f' => 0xF,
                _ => unreachable!()
            }
        };

        result[i] = parsedigit(digest_bytes[i * 2]) << 4 | parsedigit(digest_bytes[i * 2 + 1])
    }

    result
}