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


fn main() {
    println!("0x{:02X}", ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as u32));
    println!("0x{:02X}", (-(1 as i32) as u32));
    println!("0x{:02X}", (!(1 as i32) as u32));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as i32 == 0 as i32));
    let mut stored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as i32, 2 as i32); __sn_byte_left / __sn_byte_right } as u8;
    println!("0x{:02X}", (stored as u32));
}
