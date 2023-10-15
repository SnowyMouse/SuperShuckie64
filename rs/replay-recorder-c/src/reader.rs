use replay_recorder::reader::*;
use replay_recorder::packet::*;

pub struct ReplayReaderContainer {
    reader: ReplayReader<&'static [u8]>,
    last_item: Option<ReplayReaderItem>
}

/// Pointer must be freed with RR_ReplayReader_free
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayReader_new(
    stream_data: *const u8,
    stream_data_len: usize
) -> *mut ReplayReaderContainer {
    let contents = to_slice!(stream_data, stream_data_len);
    if let Ok(reader) = ReplayReader::new(contents) {
        Box::into_raw(Box::new(
            ReplayReaderContainer {
                reader,
                last_item: None
            }
        ))
    }
    else {
        std::ptr::null_mut()
    }
}

/// Frees a ReplayReader(Container); no-op if null
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayReader_free(
    reader: *mut ReplayReaderContainer
) {
    if !reader.is_null() {
        drop(Box::from_raw(reader))
    }
}

/// Gets the next item. The last item will no longer be usable. Returns `null` if unavailable.
#[no_mangle]
pub extern "C" fn RR_ReplayReader_next(
    reader: &mut ReplayReaderContainer,
    error: &mut bool
) -> *const ReplayReaderItem {
    reader.last_item = None;

    if let Some(n) = reader.reader.next() {
        if let Ok(n) = n {
            reader.last_item = Some(n);
            *error = false;
            reader.last_item.as_ref().unwrap()
        }
        else {
            *error = true;
            std::ptr::null()
        }
    }
    else {
        *error = false;
        std::ptr::null()
    }
}

pub type ReplayReaderItemCollection = Vec<ReplayReaderItem>;

/// Get all items into a container. Returns `null` if unavailable. The resulting pointer must be freed with RR_ReplayReaderItemCollection_free
#[no_mangle]
pub extern "C" fn RR_ReplayReader_collect(
    reader: &mut ReplayReaderContainer,
    error: &mut bool
) -> *mut ReplayReaderItemCollection {
    let mut v: Vec<ReplayReaderItem> = Vec::new();

    for i in &mut reader.reader {
        if let Ok(i) = i {
            v.push(i);
        }
        else {
            *error = true;
            return std::ptr::null_mut();
        }
    }

    Box::into_raw(Box::new(v))
}

/// Frees a ReplayReaderItemCollection; no-op if null
#[no_mangle]
pub unsafe extern "C" fn RR_ReplayReaderItemCollection_free(
    collection: *mut ReplayReaderItemCollection
) {
    if !collection.is_null() {
        drop(Box::from_raw(collection))
    }
}

/// Count the number of items in the collection.
#[no_mangle]
pub extern "C" fn RR_ReplayReaderItemCollection_len(
    collection: &ReplayReaderItemCollection
) -> usize {
    collection.len()
}

/// Get the nth item in the collection. Returns `null` if nonexistent.
#[no_mangle]
pub extern "C" fn RR_ReplayReaderItemCollection_get_n(
    collection: &ReplayReaderItemCollection,
    n: usize
) -> *const ReplayReaderItem {
    if let Some(n) = collection.get(n) {
        n
    }
    else {
        std::ptr::null()
    }
}

/// [ReplayReaderItem::get_packet_type]
#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_get_packet_type(
    item: &ReplayReaderItem
) -> PacketType {
    item.get_packet_type()
}

/// [ReplayReaderItem::get_delay]
#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_get_delay(
    item: &ReplayReaderItem
) -> u8 {
    item.get_delay()
}

macro_rules! get_packet_or_bail {
    ($type:tt, $item:tt) => {
        $item.get_packet_if_matches::<$type>().unwrap()
    }
}

macro_rules! make_set_input_int {
    ($fn_name:tt, $input_type:tt, $packet_type:tt) => {
        #[no_mangle]
        pub extern "C" fn $fn_name(
            item: &ReplayReaderItem,
            input: &mut $input_type
        ) {
            *input = get_packet_or_bail!($packet_type, item).input
        }
    }
}

make_set_input_int!(RR_ReplayReaderItem_read_SetInput8,  u8,  SetInput8);
make_set_input_int!(RR_ReplayReaderItem_read_SetInput16, u16, SetInput16);
make_set_input_int!(RR_ReplayReaderItem_read_SetInput32, u32, SetInput32);
make_set_input_int!(RR_ReplayReaderItem_read_SetInput64, u64, SetInput64);

macro_rules! make_set_input_int {
    ($fn_name:tt, $packet_type:tt) => {
        #[no_mangle]
        pub extern "C" fn $fn_name(
            item: &ReplayReaderItem,
            input: &mut *const u8,
            input_size: &mut usize
        ) {
            let packet = get_packet_or_bail!($packet_type, item);
            *input = packet.input.as_ptr();
            *input_size = packet.input.len();
        }
    }
}

make_set_input_int!(RR_ReplayReaderItem_read_SetInputData8,  SetInputData8);
make_set_input_int!(RR_ReplayReaderItem_read_SetInputData16, SetInputData16);
make_set_input_int!(RR_ReplayReaderItem_read_SetInputData32, SetInputData32);
make_set_input_int!(RR_ReplayReaderItem_read_SetInputData64, SetInputData64);

#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_read_LoadSRAM(
    item: &ReplayReaderItem,
    data: &mut *const u8,
    data_size: &mut usize
) {
    let packet = get_packet_or_bail!(LoadSRAM, item);
    *data = packet.data.as_ptr();
    *data_size = packet.data.len();
}

#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_read_AddSaveState(
    item: &ReplayReaderItem,
    index: &mut u32,
    data: &mut *const u8,
    data_size: &mut usize
) {
    let packet = get_packet_or_bail!(AddSaveState, item);
    *index = packet.index;
    *data = packet.data.as_ptr();
    *data_size = packet.data.len();
}

#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_read_LoadSaveState(
    item: &ReplayReaderItem,
    index: &mut u32
) {
    let packet = get_packet_or_bail!(LoadSaveState, item);
    *index = packet.index;
}

#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_read_Bookmark(
    item: &ReplayReaderItem,
    name: &mut [u8; 32]
) {
    let packet = get_packet_or_bail!(Bookmark, item);
    *name = packet.name;
}

#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_read_ChangeGameSpeed(
    item: &ReplayReaderItem,
    speed: &mut u16
) {
    let packet = get_packet_or_bail!(ChangeGameSpeed, item);
    *speed = packet.speed;
}

#[no_mangle]
pub extern "C" fn RR_ReplayReaderItem_read_CustomData(
    item: &ReplayReaderItem,
    name: &mut [u8; 32],
    data: &mut *const u8,
    data_size: &mut usize
) {
    let packet = get_packet_or_bail!(CustomData, item);
    *name = packet.name;
    *data = packet.data.as_ptr();
    *data_size = packet.data.len();
}
