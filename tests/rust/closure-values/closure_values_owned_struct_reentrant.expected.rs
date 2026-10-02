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
#[derive(Clone, Copy, Debug, PartialEq)]
struct Counter {
    value: i64,
}

fn main() {
    let mut state: Counter = Counter { value: 10 };
    let depth: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut action: __SnClosure<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64> = { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64>(std::rc::Rc::new(move |callback: __SnClosure<dyn Fn() -> i64>| -> i64 { let mut state = state.clone(); let mut result: i64 = ((callback.clone()).0)();{ let __sn_value = __sn_checked_0((result).checked_add((state.clone()).value), "Runtime error: integer overflow in addition"); state.value = __sn_value.clone(); __sn_value };return (state.clone()).value;})) }
;
    let mut alias: __SnClosure<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64> = action.clone();
    let mut callback: __SnClosure<dyn Fn() -> i64> = { let (depth, alias, ) = (depth.clone(), alias.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let (__sn_rhs, __sn_cell) = (1, &depth); let __sn_previous = __sn_cell.get(); let __sn_next = __sn_checked_0(__sn_previous.checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); __sn_cell.set(__sn_next); __sn_next };if (depth.get() == 1) {
        return ((alias.clone()).0)({ self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { 1})) }
 );
    }return 2;})) }
;
    println!("{}", ((action.clone()).0)(callback.clone().clone()));
    println!("{}", (state).value);
}
