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

#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn longOps(value: &mut i64) -> i64 {
        let mut add: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        let mut sub: i64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        let mut mul: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
        let mut div: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut rem: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        return __sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((add).checked_add(sub), "Runtime error: integer overflow in addition")).checked_add(mul), "Runtime error: integer overflow in addition")).checked_add(div), "Runtime error: integer overflow in addition")).checked_add(rem), "Runtime error: integer overflow in addition")).checked_add(old_inc), "Runtime error: integer overflow in addition")).checked_add(old_dec), "Runtime error: integer overflow in addition")).checked_add(*(value)), "Runtime error: integer overflow in addition");
    }
    fn intPostfix(&self, value: &mut i64) -> i64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    }
    fn longPostfix(&self, value: &mut i64) -> i64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
    fn intCompound(&self, value: &mut i64) -> i64 {
        return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
    fn longCompound(&self, value: &mut i64) -> i64 {
        return { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut i64) -> i64 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
}

fn intOps(value: &mut i64) -> i64 {
    let mut add: i64 = { let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    let mut sub: i64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    let mut mul: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    let mut div: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut rem: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    return __sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0((add).checked_add(sub), "Runtime error: integer overflow in addition")).checked_add(mul), "Runtime error: integer overflow in addition")).checked_add(div), "Runtime error: integer overflow in addition")).checked_add(rem), "Runtime error: integer overflow in addition")).checked_add(old_inc), "Runtime error: integer overflow in addition")).checked_add(old_dec), "Runtime error: integer overflow in addition")).checked_add(*(value)), "Runtime error: integer overflow in addition");
}

fn main() {
    let mut integer: i64 = 2;
    let mut long_value: i64 = 10;
    let mut ops: RefOps = RefOps {  };
    println!("{}", intOps(&mut (integer)));
    println!("{}", integer);
    println!("{}", RefOps::longOps(&mut (long_value)));
    println!("{}", long_value);
    println!("{}", (ops).intPostfix(&mut (integer)));
    println!("{}", integer);
    println!("{}", (ops).longPostfix(&mut (long_value)));
    println!("{}", long_value);
    println!("{}", (ops).intCompound(&mut (integer)));
    println!("{}", integer);
    println!("{}", (ops).longCompound(&mut (long_value)));
    println!("{}", long_value);
}
