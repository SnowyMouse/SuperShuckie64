use replay_recorder::header::ReplayHeader;
use replay_recorder::writer::ReplayWriter;
use replay_recorder::packet::*;

use std::ffi::CStr;

use crate::reader::ReplayReaderItemCollection;

unsafe fn create_header(
    emulator_info: *const i8,
    rom_name: *const i8,
    rom_data: *const u8,
    rom_data_size: usize,
    bios_data: *const u8,
    bios_data_size: usize
) -> Option<ReplayHeader> {
    let rom_name = CStr::from_ptr(rom_name).to_string_lossy();
    let emulator_info = CStr::from_ptr(emulator_info).to_string_lossy();
    let rom_data = to_slice!(rom_data, rom_data_size);
    let bios_data = to_slice!(bios_data, bios_data_size);

    replay_recorder::header::ReplayHeader::new_from_strs(
        emulator_info.as_ref(),
        rom_name.as_ref(),
        rom_data,
        bios_data
    )
}

/// Pointer must be freed with RR_ReplayWriter_free
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_new(
    emulator_info: *const i8,
    rom_name: *const i8,
    rom_data: *const u8,
    rom_data_size: usize,
    bios_data: *const u8,
    bios_data_size: usize
) -> *mut ReplayWriter {
    let header = create_header(emulator_info, rom_name, rom_data, rom_data_size, bios_data, bios_data_size);

    if let Some(header) = header {
        Box::into_raw(Box::new(ReplayWriter::new(header)))
    }
    else {
        std::ptr::null_mut()
    }
}

/// Pointer must be freed with RR_ReplayWriter_free
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_new_from_collection(
    emulator_info: *const i8,
    rom_name: *const i8,
    rom_data: *const u8,
    rom_data_size: usize,
    bios_data: *const u8,
    bios_data_size: usize,
    collection: &mut ReplayReaderItemCollection,
    from: usize,
    to: usize
) -> *mut ReplayWriter {
    let header = create_header(emulator_info, rom_name, rom_data, rom_data_size, bios_data, bios_data_size);

    if let Some(header) = header {
        Box::into_raw(Box::new(ReplayWriter::from_reader_items(&collection[from..to], header)))
    }
    else {
        std::ptr::null_mut()
    }
}

/// Pointer must be freed with RR_ReplayWriter_free
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_new_from_stream(
    stream: *const u8,
    stream_size: usize,
) -> *mut ReplayWriter {
    let result = ReplayWriter::from_stream(std::slice::from_raw_parts(stream, stream_size));

    if let Ok(result) = result {
        Box::into_raw(Box::new(result))
    }
    else {
        std::ptr::null_mut()
    }
}


/// Frees a ReplayWriter; no-op if null
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_free(
    writer: *mut ReplayWriter
) {
    if !writer.is_null() {
        drop(Box::from_raw(writer));
    }
}

/// [ReplayWriter::get_stream]
#[no_mangle]
pub extern "C" fn RR_ReplayWriter_get_stream(
    writer: &ReplayWriter,
    stream: &mut *const u8,
    length: &mut usize
) {
    *stream = writer.get_stream().as_ptr();
    *length = writer.get_stream().len();
}

/// [ReplayWriter::next_frame]
#[no_mangle]
pub extern "C" fn RR_ReplayWriter_next_frame(
    writer: &mut ReplayWriter
) {
    writer.next_frame();
}

macro_rules! make_set_input_int {
    ($fn_name:tt, $input_type:tt, $packet_type:tt) => {
        #[no_mangle]
        pub extern "C" fn $fn_name(
            writer: &mut ReplayWriter,
            input: $input_type
        ) {
            writer.write_packet(&$packet_type { input });
        }
    }
}

make_set_input_int!(RR_ReplayWriter_write_SetInput8,  u8,  SetInput8);
make_set_input_int!(RR_ReplayWriter_write_SetInput16, u16, SetInput16);
make_set_input_int!(RR_ReplayWriter_write_SetInput32, u32, SetInput32);
make_set_input_int!(RR_ReplayWriter_write_SetInput64, u64, SetInput64);

macro_rules! make_set_input_data {
    ($fn_name:tt, $packet_type:tt) => {
        #[no_mangle]
        pub unsafe extern "C" fn $fn_name(
            writer: &mut ReplayWriter,
            input: *const u8,
            input_length: usize
        ) {
            writer.write_packet(&$packet_type { input: to_slice!(input, input_length).to_owned() });
        }
    }
}

make_set_input_data!(RR_ReplayWriter_write_SetInputData8,  SetInputData8);
make_set_input_data!(RR_ReplayWriter_write_SetInputData16, SetInputData16);
make_set_input_data!(RR_ReplayWriter_write_SetInputData32, SetInputData32);
make_set_input_data!(RR_ReplayWriter_write_SetInputData64, SetInputData64);

#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_write_LoadSRAM(
    writer: &mut ReplayWriter,
    data: *const u8,
    data_length: usize
) {
    writer.write_packet(&LoadSRAM { data: to_slice!(data, data_length).to_owned() });
}

#[no_mangle]
pub extern "C" fn RR_ReplayWriter_write_Bookmark(
    writer: &mut ReplayWriter,
    name: &[u8; 32]
) {
    writer.write_packet(&Bookmark { name: *name });
}

#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_write_CustomData(
    writer: &mut ReplayWriter,
    name: &[u8; 32],
    data: *const u8,
    data_length: usize
) {
    writer.write_packet(&CustomData { name: *name, data: to_slice!(data, data_length).to_owned() });
}

#[no_mangle]
pub extern "C" fn RR_ReplayWriter_write_ChangeGameSpeed(
    writer: &mut ReplayWriter,
    speed: u16
) {
    writer.write_packet(&ChangeGameSpeed { speed });
}

#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_write_AddSaveState(
    writer: &mut ReplayWriter,
    index: u32,
    data: *const u8,
    data_length: usize
) {
    writer.write_packet(&AddSaveState { index, data: to_slice!(data, data_length).to_owned() });
}

#[no_mangle]
pub extern "C" fn RR_ReplayWriter_write_LoadSaveState(
    writer: &mut ReplayWriter,
    index: u32
) {
    writer.write_packet(&LoadSaveState { index });
}

macro_rules! make_write_byte_to_addr {
    ($fn_name:tt, $packet_type:tt, $addr_width:tt, $addr_name:tt) => {
        #[no_mangle]
        pub extern "C" fn $fn_name(
            writer: &mut ReplayWriter,
            byte: u8,
            $addr_name: $addr_width
        ) {
            writer.write_packet(&$packet_type { byte, $addr_name });
        }
    }
}

make_write_byte_to_addr!(RR_ReplayWriter_write_WriteRAMByteAddr32,   WriteRAMByteAddr32,   u32, addr);
make_write_byte_to_addr!(RR_ReplayWriter_write_WriteRAMByteAddr64,   WriteRAMByteAddr64,   u64, addr);
make_write_byte_to_addr!(RR_ReplayWriter_write_WriteROMByteOffset32, WriteROMByteOffset32, u32, offset);
make_write_byte_to_addr!(RR_ReplayWriter_write_WriteROMByteOffset64, WriteROMByteOffset64, u64, offset);

#[no_mangle]
pub unsafe extern "C" fn RR_ReplayWriter_write_ResetSystem(
    writer: &mut ReplayWriter
) {
    writer.write_packet(&ResetSystem { });
}
