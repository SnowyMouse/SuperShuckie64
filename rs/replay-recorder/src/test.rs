use crate::writer::*;
use crate::reader::*;
use crate::packet::*;
use crate::header::*;

#[test]
fn rw_round_trip() {
    let rom = [0,1,2,3];
    let bios = [5,6,7,8];

    let mut run = ReplayWriter::new(ReplayHeader::new_from_strs("My Emulator", "MYROM", &rom, &bios).unwrap());
    run.write_packet(&SetInput8 { input: 123 });
    run.next_frame();
    run.write_packet(&AddSaveState { data: vec![1,2,3,4,5,6,7,8], index: 0 });
    run.next_frame();
    run.next_frame();
    run.next_frame();
    run.write_packet(&SetInput8 { input: 3 });
    run.next_frame();
    run.write_packet(&SetInput8 { input: 4 });
    run.write_packet(&SetInputData8 { input: vec![1,2,3,4] });
    assert_eq!(run, ReplayWriter::from_stream(run.get_stream()).unwrap());

    let reader = ReplayReader::new(run.get_stream()).unwrap();
    let all_packets: Vec<ReplayReaderItem> = reader.map(|f| f.unwrap()).collect();
    assert_eq!(5, all_packets.len());
    let packet0: &SetInput8 = all_packets[0].get_packet_if_matches().unwrap();
    assert_eq!(123, packet0.input);
    let packet1: &AddSaveState = all_packets[1].get_packet_if_matches().unwrap();
    assert_eq!(vec![1,2,3,4,5,6,7,8], packet1.data);
    let packet2: &SetInput8 = all_packets[2].get_packet_if_matches().unwrap();
    assert_eq!(3, packet2.input);
    let packet3: &SetInput8 = all_packets[3].get_packet_if_matches().unwrap();
    assert_eq!(4, packet3.input);
    let packet4: &SetInputData8 = all_packets[4].get_packet_if_matches().unwrap();
    assert_eq!(vec![1,2,3,4], packet4.input);
}

#[test]
fn set_speed_works() {
    let speed = ChangeGameSpeed::from_float(1.0).unwrap();
    assert_eq!(0x100, speed.speed);
    assert_eq!(speed, ChangeGameSpeed::from_float(speed.to_float()).unwrap());
}
