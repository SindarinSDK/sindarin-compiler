#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn markByte(trace: &mut i64, marker: i64, value: u8) -> u8 {
    (*(trace) = ((*(trace) * 10) + marker));
    return value;
}

fn markInt(trace: &mut i64, marker: i64, value: i64) -> i64 {
    (*(trace) = ((*(trace) * 10) + marker));
    return value;
}

fn main() {
    let mut trace: i64 = 0;
    let mut mixed: i64 = { let (__sn_left, __sn_right): (i64, i64) = ((markByte(&mut (trace), 1, 255)) as i64, (markInt(&mut (trace), 2, 1000)) as i64); __sn_left + __sn_right };
    println!("{}", mixed);
    println!("{}", trace);
    (trace = 0);
    let mut stored: i64 = (({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (markByte(&mut (trace), 1, 200) as i32, markByte(&mut (trace), 2, 200) as i32); __sn_byte_left * __sn_byte_right }) as i64);
    println!("{}", stored);
    println!("{}", trace);
    let mut total: i64 = 0;
    let mut value: u8 = 255;
    (total = { let (__sn_left, __sn_right): (i64, i64) = ((total) as i64, (value) as i64); __sn_left + __sn_right });
    println!("{}", total);
}
