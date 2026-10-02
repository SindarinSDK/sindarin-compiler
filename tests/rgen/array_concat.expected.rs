#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[cfg(windows)]
fn __sn_write_windows_text<W: std::io::Write>(writer: &mut W, bytes: &[u8]) {
    let mut start = 0usize;
    for (index, byte) in bytes.iter().enumerate() {
        if *byte == b'\n' {
            writer.write_all(&bytes[start..index]).expect("failed to write output");
            writer.write_all(b"\r\n").expect("failed to write output newline");
            start = index + 1;
        }
    }
    writer.write_all(&bytes[start..]).expect("failed to write output");
    writer.flush().expect("failed to flush output");
}

#[cfg(windows)]
fn __sn_write_stdout_bytes(bytes: &[u8]) {
    __sn_write_windows_text(&mut std::io::stdout().lock(), bytes);
}

#[cfg(windows)]
fn __sn_write_stderr_bytes(bytes: &[u8]) {
    __sn_write_windows_text(&mut std::io::stderr().lock(), bytes);
}

#[cfg(windows)]
fn __sn_print_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    __sn_write_stdout_bytes(rendered.as_bytes());
}

#[cfg(windows)]
fn __sn_println_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    rendered.push('\n');
    __sn_write_stdout_bytes(rendered.as_bytes());
}

#[cfg(windows)]
macro_rules! print {
    ($($arg:tt)*) => { crate::__sn_print_format(format_args!($($arg)*)) };
}

#[cfg(windows)]
macro_rules! println {
    () => { crate::__sn_println_format(format_args!("")) };
    ($($arg:tt)*) => { crate::__sn_println_format(format_args!($($arg)*)) };
}


fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

fn main() {
    let mut left: Vec<i64> = vec![1, 2];
    let mut right: Vec<i64> = vec![3, 4];
    let mut combined: Vec<i64> = { let __sn_array_left = &(left); let __sn_array_right = &(right); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    (combined).push(5);
    println!("{}", (left).len() as i64);
    println!("{}", (right).len() as i64);
    println!("{}", (combined).len() as i64);
    println!("{}", (combined)[__sn_index((combined).len(), 0)]);
    println!("{}", (combined)[__sn_index((combined).len(), (-1))]);
    let mut self_concat: Vec<i64> = { let __sn_array_left = &(left); let __sn_array_right = &(left); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (self_concat).len() as i64);
    println!("{}", (self_concat)[__sn_index((self_concat).len(), 2)]);
    let mut empty: Vec<i64> = vec![];
    let mut with_empty: Vec<i64> = { let __sn_array_left = &(empty); let __sn_array_right = &(right); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (with_empty).len() as i64);
    let mut first_flags: Vec<bool> = vec![true];
    let mut second_flags: Vec<bool> = vec![false];
    let mut flags: Vec<bool> = { let __sn_array_left = &(first_flags); let __sn_array_right = &(second_flags); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (flags)[__sn_index((flags).len(), 0)]);
    println!("{}", (flags)[__sn_index((flags).len(), 1)]);
}
