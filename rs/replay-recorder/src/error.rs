//! Functionality for error handling.

/// Error type that can be returned in a failure case.
#[derive(Debug)]
pub enum Error {
    IO(std::io::Error),
    ParseError(&'static str),
    NeedsDecompressed
}

/// General result type.
pub type LoadResult<T> = Result<T, Error>;
