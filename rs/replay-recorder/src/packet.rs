//! Functionality for parsing packets in a structure.

use std::any::Any;
use std::io::{Write, Read, Result as IOResult};
use std::convert::TryFrom;

use super::error::*;

macro_rules! io_to_load_err {
    ($what:expr) => {
        ($what).map_err(|e| Error::IO(e))
    }
}

/// Read bytes to an integer
macro_rules! read_bytes {
    ($reader:expr, $type:ty) => {{
        let f = (|| -> IOResult<$type> {
            let mut bytes = (0 as $type).to_be_bytes();
            ($reader.read_exact(&mut bytes)).map(|_| <$type>::from_be_bytes(bytes))
        })();
        io_to_load_err!(f)
    }}
}

/// Write a vector with a length
macro_rules! write_data {
    ($data:expr, $writer:expr, $length_width:tt) => {{
        let f = (|| -> IOResult<()> {
            $writer.write_all(&($data.len() as $length_width).to_be_bytes())?;
            $writer.write_all(&$data)?;
            Ok(())
        })();
        io_to_load_err!(f)
    }};
}

/// Read a vector with a length
macro_rules! read_data {
    ($reader:expr, $length_width:tt) => {{
        let f = (|| -> IOResult<Vec<u8>> {
            let size = match read_bytes!($reader, $length_width) {
                Ok(n) => n as usize,
                Err(Error::IO(e)) => return Err(e),
                _ => unreachable!()
            };
            let mut data = Vec::new();
            data.reserve_exact(size);
            unsafe { data.set_len(size); }

            $reader.read_exact(&mut data)?;

            Ok(data)
        })();
        io_to_load_err!(f)
    }};
}

/// Determines a packet type
#[repr(u8)]
#[derive(Copy, Clone, PartialEq)]
pub enum PacketType {
    CustomData = 0,
    LoadSRAM = 1,
    ChangeGameSpeed = 2,
    Bookmark = 3,
    SetInput8 = 4,
    SetInput16 = 5,
    SetInput32 = 6,
    SetInput64 = 7,
    SetInputData8 = 8,
    SetInputData16 = 9,
    SetInputData32 = 10,
    SetInputData64 = 11,
    AddSaveState = 12,
    LoadSaveState = 13,
    NoOp = 255, // will generally be accompanied by another 0xFF byte, so it might be compression bait
}

impl TryFrom<u8> for PacketType {
    type Error = ();
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
              0 => Ok(PacketType::CustomData),
              1 => Ok(PacketType::LoadSRAM),
              2 => Ok(PacketType::ChangeGameSpeed),
              3 => Ok(PacketType::Bookmark),
              4 => Ok(PacketType::SetInput8),
              5 => Ok(PacketType::SetInput16),
              6 => Ok(PacketType::SetInput32),
              7 => Ok(PacketType::SetInput64),
              8 => Ok(PacketType::SetInputData8),
              9 => Ok(PacketType::SetInputData16),
             10 => Ok(PacketType::SetInputData32),
             11 => Ok(PacketType::SetInputData64),
             12 => Ok(PacketType::AddSaveState),
             13 => Ok(PacketType::LoadSaveState),
            255 => Ok(PacketType::NoOp),
            _ => Err(())
        }
    }
}

/// Defines a packet
pub trait Packet: Any {
    /// Get the packet type enum
    fn get_packet_type() -> PacketType where Self: Sized;

    /// Write the packet out to the given stream.
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> where Self: Sized;

    /// Read the packet from the stream.
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> where Self: Sized;

    /// Convert to Any
    fn as_any(&self) -> &dyn Any;
}

/// Do nothing.
///
/// This is generally used for delaying for more than 255 frames.
#[derive(Copy, Clone, PartialEq, Default, Debug)]
pub struct NoOp {}
impl Packet for NoOp {
    fn get_packet_type() -> PacketType { PacketType::NoOp }
    fn write<W: Write>(&self, _: &mut W) -> LoadResult<()> { Ok(()) }
    fn read<R: Read>(_: &mut R) -> LoadResult<Self> { Ok(Self {}) }
    fn as_any(&self) -> &dyn Any { self }
}

/// Load save data. This should be used at the beginning of the stream if SRAM is desired.
#[derive(Clone, PartialEq, Default, Debug)]
pub struct LoadSRAM {
    pub data: Vec<u8>
}
impl Packet for LoadSRAM {
    fn get_packet_type() -> PacketType { PacketType::LoadSRAM }
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
        write_data!(&self.data, w, u64)
    }
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
        Ok(Self {
            data: read_data!(r, u64)?
        })
    }
    fn as_any(&self) -> &dyn Any { self }
}

/// Bookmark for the current position of the stream.
///
/// This can be used for pointing out important parts of a replay, such as when a game begins (e.g. new game) and ends (e.g. end credits).
#[derive(Copy, Clone, PartialEq, Default, Debug)]
pub struct Bookmark {
    pub name: [u8; 32]
}
impl Packet for Bookmark {
    fn get_packet_type() -> PacketType { PacketType::Bookmark }
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
        io_to_load_err!(w.write_all(&self.name))?;
        Ok(())
    }
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
        let mut d = Self::default();
        io_to_load_err!(r.read_exact(&mut d.name))?;
        Ok(d)
    }
    fn as_any(&self) -> &dyn Any { self }
}

/// Context-specific custom data to be parsed by the reader.
///
/// For example, this can be a command for an emulator to do something special (e.g. increment a death counter).
///
/// The maximum length for data that can be stored is [`u64::MAX`].
#[derive(Clone, PartialEq, Default, Debug)]
pub struct CustomData {
    pub name: [u8; 32],
    pub data: Vec<u8>
}
impl Packet for CustomData {
    fn get_packet_type() -> PacketType { PacketType::CustomData }
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
        io_to_load_err!(w.write_all(&self.name))?;
        write_data!(&self.data, w, u64)?;
        Ok(())
    }
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
        let mut d = Self::default();
        io_to_load_err!(r.read_exact(&mut d.name))?;
        d.data = read_data!(r, u64)?;
        Ok(d)
    }
    fn as_any(&self) -> &dyn Any { self }
}

/// Set the current game speed.
///
/// Speed is stored as an 8.8 fixed-width number to save space.
///
/// The max speed is `255 + 255/256`, and the minimum speed is `0`. To convert it to a float, divide by `256`.
///
/// You can use the `from_float` and `to_float` convenience functions for this.
///
/// Note that emulators are not required to respect values that are out of their range (for performance, etc.).
#[derive(Copy, Clone, PartialEq, Default, Debug)]
pub struct ChangeGameSpeed {
    pub speed: u16
}
impl Packet for ChangeGameSpeed {
    fn get_packet_type() -> PacketType { PacketType::ChangeGameSpeed }
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
        io_to_load_err!(w.write_all(&self.speed.to_be_bytes()))?;
        Ok(())
    }
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
        Ok(Self {
            speed: read_bytes!(r, u16)?
        })
    }
    fn as_any(&self) -> &dyn Any { self }
}
impl ChangeGameSpeed {
    /// Convert the speed to a floating-point number.
    pub fn to_float(self) -> f64 {
        self.speed as f64 / 256.0
    }

    /// Convert a floating point number to a speed value.
    ///
    /// If the number is out of bounds, `None` is returned.
    pub fn from_float(f: f64) -> Option<Self> {
        if f < 0.0 || f >= 256.0 {
            return None
        }
        Some(ChangeGameSpeed { speed: (f * 256.0) as u16 })
    }
}

macro_rules! make_set_input_int {
    ($name:tt, $width:tt, $doc:tt) => {
        #[doc = $doc]
        #[derive(Copy, Clone, PartialEq, Default, Debug)]
        pub struct $name {
            pub input: $width
        }
        impl Packet for $name {
            fn get_packet_type() -> PacketType { PacketType::$name }
            fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
                io_to_load_err!(w.write_all(&self.input.to_be_bytes()))?;
                Ok(())
            }
            fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
                Ok(Self {
                    input: read_bytes!(r, $width)?
                })
            }
            fn as_any(&self) -> &dyn Any { self }
        }
    };
}

make_set_input_int!(SetInput8,  u8,  "Set 8-bit input.\n\nIf setting inputs for multiple input sources, treat the first input in a frame as the first player, the second as the second player, and so on.\n\nFor consoles that store inputs in 8 bits (e.g. 1-8 buttons), this is what you use.");
make_set_input_int!(SetInput16, u16, "Set 16-bit input.\n\nIf setting inputs for multiple input sources, treat the first input in a frame as the first player, the second as the second player, and so on.\n\nFor consoles that store inputs in 16 bits (e.g. 9-16 buttons, or two 8-bit axis), this is what you use.");
make_set_input_int!(SetInput32, u32, "Set 32-bit input.\n\nIf setting inputs for multiple input sources, treat the first input in a frame as the first player, the second as the second player, and so on.\n\nFor consoles that store inputs in 32 bits (e.g. 17-32 buttons, or four 8-bit axis), this is what you use.");
make_set_input_int!(SetInput64, u64, "Set 64-bit input.\n\nIf setting inputs for multiple input sources, treat the first input in a frame as the first player, the second as the second player, and so on.\n\nFor consoles that store inputs in 64 bits (e.g. 33-64 buttons, or eight 8-bit axis), this is what you use.\n\nNote: If you need 33-48 bits, it is more economical to use [`SetInputData8`] and pass in 5-6 bytes.");

macro_rules! make_set_input_data {
    ($name:tt, $length_width:tt, $doc_append:tt) => {
        /// Set a custom data input,
        #[doc = $doc_append]
        ///
        /// If setting inputs for multiple input sources, treat the first input in a frame as the first player, the second as the second player, and so on.
        ///
        /// For consoles that store inputs in a custom data structure, this is what you use.
        #[derive(Clone, PartialEq, Default, Debug)]
        pub struct $name {
            pub input: Vec<u8>
        }
        impl Packet for $name {
            fn get_packet_type() -> PacketType { PacketType::$name }
            fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
                write_data!(&self.input, w, $length_width)?;
                Ok(())
            }
            fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
                Ok(Self {
                    input: read_data!(r, $length_width)?
                })
            }
            fn as_any(&self) -> &dyn Any { self }
        }
    };
}

make_set_input_data!(SetInputData8, u8, "where the input is sized with an 8-bit integer.");
make_set_input_data!(SetInputData16, u16, "where the input is sized with a 16-bit integer.");
make_set_input_data!(SetInputData32, u32, "where the input is sized with a 32-bit integer.");
make_set_input_data!(SetInputData64, u64, "where the input is sized with a 64-bit integer.");

/// Add a save state to the context with a given index.
///
/// This save state should accurately represent the current in-game state at this point to allow for scrubbing or for loading with [LoadSaveState].
///
/// The index does not have to be in order, nor does it have to be unique, but the most recent save state with a given index must be used if [LoadSaveState] is used.
///
/// The maximum length for data that can be stored is [`u64::MAX`].
#[derive(Clone, PartialEq, Default, Debug)]
pub struct AddSaveState {
    pub data: Vec<u8>,
    pub index: u32
}
impl Packet for AddSaveState {
    fn get_packet_type() -> PacketType { PacketType::AddSaveState }
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
        write_data!(&self.data, w, u64)?;
        io_to_load_err!(w.write_all(&self.index.to_be_bytes()))
    }
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
        Ok(Self {
            data: read_data!(r, u64)?,
            index: read_bytes!(r, u32)?
        })
    }
    fn as_any(&self) -> &dyn Any { self }
}

/// Load a save state added with [AddSaveState] with a given index.
///
/// If there are multiple save states with the same index, the most recent save state of that index must be used.
#[derive(Copy, Clone, PartialEq, Default, Debug)]
pub struct LoadSaveState {
    pub index: u32
}
impl Packet for LoadSaveState {
    fn get_packet_type() -> PacketType { PacketType::LoadSaveState }
    fn write<W: Write>(&self, w: &mut W) -> LoadResult<()> {
        io_to_load_err!(w.write_all(&self.index.to_be_bytes()))
    }
    fn read<R: Read>(r: &mut R) -> LoadResult<Self> {
        Ok(Self {
            index: read_bytes!(r, u32)?
        })
    }
    fn as_any(&self) -> &dyn Any { self }
}
