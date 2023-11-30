//! # Replay Recorder
//!
//! This library is used for recording replays for playback on an emulator, utilizing player input and (optionally) save
//! states. The idea is that, if an emulator is deterministic, then a replay of a run should result in the same outcome.
//! There are a few use cases for something like this:
//!
//! * You can inspect the internal state of a recorded run as it is being played with software such as GameHook.
//! * You can return to any point in the replay, making it more powerful than just creating one save state.
//! * The size of a run recorded like this can be much smaller than a video of it without any loss in quality.
//!   * Example: For a run being played at 240 FPS with an 8-bit input change every 4th frame (3 bytes per frame), the
//!     bitrate is 5760 kbps, or 2.592 MB (2.472 MiB) per hour.
//! * Recording a replay is less CPU intensive than encoding a video of the run. Note, however, that it is not a
//!   substitute for recording to video. However, you can choose to record a video after the run has been performed,
//!   optionally recording it at a slower speed for better quality and CPU usage.
//!
//! ## Using save states
//!
//! You can insert save states as keyframes, allowing for you to navigate to any arbitrary point in the replay quickly
//! for easy scrubbing. Note, however, that this will greatly increase the size of the stream. It is highly recommended
//! that you use some form of compression if you choose to embed save states periodically.
//!
//! For example, a 128 KiB save state inserted every 10 seconds will add 47.19 MB (46.08 MiB) per hour. This may make
//! it difficult to transfer your replay as-is if you have a strict upload limit. However, a 10 KiB save state inserted
//! every 5 seconds will only add up to 7.37 MB (7.03 MiB) per hour and give you double the keyframes.
//!
//! For consoles that require larger save states, you may want to opt to not save them in the replay stream, but instead
//! do a pass of loading the entire stream and generating the save states as the replay is being played back.
//!
//! ## Replay structure
//!
//! A recording contains a 512-byte header as well as packets stored sequentially. The header contains information about
//! the emulator, ROM, and BIOS. It is OK for the emulator to be different (although differences in emulation quality
//! may lead to inconsistent results). However, the reader should fail if the ROM and BIOS checksums do not match.
//!
//! The packets are read as a stream, thus the only context that exists is what has been played in the past (e.g. for
//! loading save states). Each packet starts with a delay (in frames) which the emulator must wait before executing the
//! packet, followed by a packet type and the payload.
//!
//! This crate has a writer and reader for encoding and decoding replays, respectively.

extern crate sha256;
extern crate zstd;

macro_rules! do_to_packet {
    ($packet: expr, $var: tt, $fn: block ) => {{
        match $packet.get_packet_type() {
            PacketType::NoOp => { let $var = $packet.as_any().downcast_ref::<NoOp>().unwrap(); $fn },
            PacketType::CustomData => { let $var = $packet.as_any().downcast_ref::<CustomData>().unwrap(); $fn },
            PacketType::LoadSRAM => { let $var = $packet.as_any().downcast_ref::<LoadSRAM>().unwrap(); $fn },
            PacketType::ChangeGameSpeed => { let $var = $packet.as_any().downcast_ref::<ChangeGameSpeed>().unwrap(); $fn },
            PacketType::Bookmark => { let $var = $packet.as_any().downcast_ref::<Bookmark>().unwrap(); $fn },
            PacketType::SetInput8 => { let $var = $packet.as_any().downcast_ref::<SetInput8>().unwrap(); $fn },
            PacketType::SetInput16 => { let $var = $packet.as_any().downcast_ref::<SetInput16>().unwrap(); $fn },
            PacketType::SetInput32 => { let $var = $packet.as_any().downcast_ref::<SetInput32>().unwrap(); $fn },
            PacketType::SetInput64 => { let $var = $packet.as_any().downcast_ref::<SetInput64>().unwrap(); $fn },
            PacketType::SetInputData8 => { let $var = $packet.as_any().downcast_ref::<SetInputData8>().unwrap(); $fn },
            PacketType::SetInputData16 => { let $var = $packet.as_any().downcast_ref::<SetInputData16>().unwrap(); $fn },
            PacketType::SetInputData32 => { let $var = $packet.as_any().downcast_ref::<SetInputData32>().unwrap(); $fn },
            PacketType::SetInputData64 => { let $var = $packet.as_any().downcast_ref::<SetInputData64>().unwrap(); $fn },
            PacketType::AddSaveState => { let $var = $packet.as_any().downcast_ref::<AddSaveState>().unwrap(); $fn },
            PacketType::LoadSaveState => { let $var = $packet.as_any().downcast_ref::<LoadSaveState>().unwrap(); $fn },
            PacketType::WriteRAMByteAddr32 => { let $var = $packet.as_any().downcast_ref::<WriteRAMByteAddr32>().unwrap(); $fn },
            PacketType::WriteRAMByteAddr64 => { let $var = $packet.as_any().downcast_ref::<WriteRAMByteAddr64>().unwrap(); $fn },
            PacketType::WriteROMByteOffset32 => { let $var = $packet.as_any().downcast_ref::<WriteROMByteOffset32>().unwrap(); $fn },
            PacketType::WriteROMByteOffset64 => { let $var = $packet.as_any().downcast_ref::<WriteROMByteOffset64>().unwrap(); $fn },
            PacketType::ResetSystem => { let $var = $packet.as_any().downcast_ref::<ResetSystem>().unwrap(); $fn },
        }
    }};
}

pub mod header;
pub mod packet;
pub mod reader;
pub mod error;
pub mod writer;

#[cfg(test)]
mod test;
