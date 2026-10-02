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


fn __sn_runtime_error_0(message: &'static str) -> ! {
    #[cfg(windows)]
    {
    crate::__sn_write_stderr_bytes(message.as_bytes());
    crate::__sn_write_stderr_bytes(b"\n");
    }
    #[cfg(not(windows))]
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_0<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_0(message),
    }
}

fn __sn_checked_div_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn main() {
    println!("0x{:02X}", ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } as u32));
    println!("0x{:02X}", (-(1 as i32) as u32));
    println!("0x{:02X}", (!(1 as i32) as u32));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } == 0));
    let mut stored: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("0x{:02X}", (stored as u32));
}
