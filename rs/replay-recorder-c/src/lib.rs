//! C bindings for the replay recorder

macro_rules! to_slice {
    ($ptr:expr, $len:expr) => {
        std::slice::from_raw_parts($ptr, $len)
    }
}

extern crate replay_recorder;
pub mod writer;
pub mod reader;
