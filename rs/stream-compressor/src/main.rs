use std::fs::{read, write};
use std::io::Cursor;
use std::io::Write;
use replay_recorder::reader::ReplayReaderItem;

fn main() {
    let mut a = std::env::args();
    let path = a.next().unwrap();

    let p = std::path::PathBuf::from(&path).canonicalize().unwrap().parent().unwrap().to_owned();

    let mut success = 0;
    let mut fail = 0;

    let mut log = p.join("log.txt");
    let mut log_file = std::fs::File::create(&mut log).unwrap();

    for i in std::fs::read_dir(p).unwrap() {
        let Ok(i) = i else {
            continue
        };

        let path = i.path();

        if path.extension() != Some("replay".as_ref()) {
            continue;
        }

        let file = match read(&path) {
            Ok(n) => n,
            Err(e) => {
                writeln!(&mut log_file, "Failed to open {}: {e:?}", path.display()).unwrap();
                fail += 1;
                continue;
            }
        };

        let mut cursor = Cursor::new(file);
        let header = match replay_recorder::header::ReplayHeader::from_stream(&mut cursor) {
            Ok(n) => n,
            Err(e) => {
                writeln!(&mut log_file, "Failed to open {}: {e:?}", path.display()).unwrap();
                fail += 1;
                continue;
            }
        };

        if header.flags.is_compressed {
            continue;
        }

        let file = cursor.into_inner();

        let get_items = |v: &[u8]| -> Result<Vec<ReplayReaderItem>, replay_recorder::error::Error> {
            let v = replay_recorder::writer::ReplayWriter::decompress_stream(v)?;
            let reader = replay_recorder::reader::ReplayReader::new(Cursor::new(v))?;
            let mut items = Vec::new();
            for i in reader.into_iter() {
                items.push(i?);
            }
            Ok(items)
        };

        let old_items = match get_items(file.as_slice()) {
            Ok(n) => n,
            Err(e) => {
                writeln!(&mut log_file, "Failed to read {}: {e:?}", path.display()).unwrap();
                fail += 1;
                continue;
            }
        };

        let stream = match replay_recorder::writer::ReplayWriter::from_stream_vec(file) {
            Ok(n) => n,
            Err(e) => {
                writeln!(&mut log_file, "Failed to read {}: {e:?}", path.display()).unwrap();
                fail += 1;
                continue;
            }
        };

        let mut uncompressed_path = path.clone();
        let new_filename = uncompressed_path.file_name().unwrap().to_str().unwrap().to_owned() + ".bak";
        uncompressed_path.set_file_name(new_filename);

        if let Err(e) = std::fs::rename(&path, &uncompressed_path) {
            writeln!(&mut log_file, "Failed to backup {}: {e:?}", path.display()).unwrap();
            fail += 1;
            continue;
        }

        if let Err(e) = write(&path, stream.compress_stream()) {
            writeln!(&mut log_file, "Failed to save {}: {e:?}", path.display()).unwrap();
            fail += 1;
            continue;
        }

        // try to read the data we just got
        let file_again = match read(&path) {
            Ok(n) => n,
            Err(e) => {
                writeln!(&mut log_file, "Failed to re-read {} after compressing: {e:?}", path.display()).unwrap();
                fail += 1;
                continue;
            }
        };

        let new_items = match get_items(file_again.as_slice()) {
            Ok(n) => n,
            Err(e) => {
                writeln!(&mut log_file, "Failed to verify {}: {e:?}", path.display()).unwrap();
                fail += 1;
                continue;
            }
        };

        if new_items != old_items {
            writeln!(&mut log_file, "Failed to verify {}: items did not match...", path.display()).unwrap();
            fail += 1;
            continue;
        }

        success += 1;
    }

    writeln!(&mut log_file, "Compressed {success} replays successfully!").unwrap();
    if fail > 0 {
        writeln!(&mut log_file, "Failed to compress {fail}, however...").unwrap();
    }
}
