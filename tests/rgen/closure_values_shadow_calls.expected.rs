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

struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
fn action(x: i64) -> i64 {
    return __sn_checked_0((x).checked_add(1), "Runtime error: integer overflow in addition");
}

fn parameter(action: __SnClosure<dyn Fn(i64) -> i64>) -> i64 {
    return ((action.clone()).0)(2);
}

fn main() {
    println!("{}", action(1));
    if true {
        let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((x).checked_add(10), "Runtime error: integer overflow in addition")})) }
;
        println!("{}", ((action.clone()).0)(1));
        println!("{}", parameter(action.clone()));
        if true {
        let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((x).checked_add(20), "Runtime error: integer overflow in addition")})) }
;
        println!("{}", ((action.clone()).0)(1));
    }
        println!("{}", ((action.clone()).0)(1));
    }
    println!("{}", action(1));
    let mut values: Vec<__SnClosure<dyn Fn(i64) -> i64>> = vec![{ self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((x).checked_add(30), "Runtime error: integer overflow in addition")})) }
];
    for mut action in (values).iter().cloned() {
        println!("{}", ((action.clone()).0)(1));
    }
    println!("{}", action(1));
    if true {
        let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((x).checked_add(40), "Runtime error: integer overflow in addition")})) }
;
        let mut result: i64 = match (1 as i64) {
        1 => {
            (((action.clone()).0)(1) as i64)
        },
        _ => {
            (((action.clone()).0)(2) as i64)
        },
    };
        println!("{}", result);
    }
    println!("{}", action(1));
    let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((x).checked_add(50), "Runtime error: integer overflow in addition")})) }
;
    println!("{}", ((action.clone()).0)(1));
}
