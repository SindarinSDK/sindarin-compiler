#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

struct __sn_concurrency0_Cell<T> {
    value: std::sync::Mutex<T>,
    gate: std::sync::Mutex<()>,
}
impl<T> __sn_concurrency0_Cell<T> {
    fn new(value: T) -> Self { Self { value: std::sync::Mutex::new(value), gate: std::sync::Mutex::new(()) } }
    fn lock(&self) -> std::sync::LockResult<std::sync::MutexGuard<'_, T>> { self.value.lock() }
    fn guard(&self) -> std::sync::MutexGuard<'_, ()> { self.gate.lock().unwrap_or_else(|e| e.into_inner()) }
}

struct __sn_concurrency0_Join<T>(Option<std::thread::JoinHandle<T>>);
impl<T: Send + 'static> __sn_concurrency0_Join<T> {
    fn spawn<F: FnOnce() -> T + Send + 'static>(call: F) -> Self {
        Self(Some(std::thread::spawn(call)))
    }
}
impl<T> __sn_concurrency0_Join<T> {
    fn join(mut self) -> T {
        match self.0.take().expect("thread already joined").join() {
            Ok(value) => value,
            Err(error) => std::panic::resume_unwind(error),
        }
    }
    fn detach(mut self) { self.0.take(); }
}
impl<T> Drop for __sn_concurrency0_Join<T> {
    fn drop(&mut self) {
        if let Some(handle) = self.0.take() { let _ = handle.join(); }
    }
}

static __sn_concurrency0_global_value: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(1));
static __sn_concurrency0_global_rhs_calls: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));

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


fn worker() -> i64 {
    { let mut __sn_concurrency0_value_guard = { (*__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner())).clone() }; let __sn_concurrency0_numeric_rhs = (1).clone(); let __sn_concurrency0_value = { { let (__sn_rhs, __sn_place): (i64, &mut i64) = (__sn_concurrency0_numeric_rhs, &mut (__sn_concurrency0_value_guard)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next } }; *__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value_guard; __sn_concurrency0_value };
    return 2;
}

fn joined_rhs() -> i64 {
    { let mut __sn_concurrency0_value_guard = { (*__sn_concurrency0_global_rhs_calls.lock().unwrap_or_else(|e| e.into_inner())).clone() }; let __sn_concurrency0_numeric_rhs = (1).clone(); let __sn_concurrency0_value = { { let (__sn_rhs, __sn_place): (i64, &mut i64) = (__sn_concurrency0_numeric_rhs, &mut (__sn_concurrency0_value_guard)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next } }; *__sn_concurrency0_global_rhs_calls.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value_guard; __sn_concurrency0_value };
    let mut result: i64 = 0; let mut __sn_concurrency0_handle_result: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || worker()) }
);
    return { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_result.take() { result = __sn_concurrency0_handle.join(); } result.clone() }
;
}

fn observe(result: i64) -> i64 {
    { let mut __sn_concurrency0_value_guard = { (*__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner())).clone() }; let __sn_concurrency0_numeric_rhs = (1).clone(); let __sn_concurrency0_value = { { let (__sn_rhs, __sn_place): (i64, &mut i64) = (__sn_concurrency0_numeric_rhs, &mut (__sn_concurrency0_value_guard)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next } }; *__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value_guard; __sn_concurrency0_value };
    println!("{}", { let value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value });
    return result;
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_value);
    std::sync::LazyLock::force(&__sn_concurrency0_global_rhs_calls);
    println!("{}", observe({ let mut __sn_concurrency0_value_guard = { (*__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner())).clone() }; let __sn_concurrency0_numeric_rhs = (joined_rhs()).clone(); let __sn_concurrency0_value = { { let (__sn_rhs, __sn_place): (i64, &mut i64) = (__sn_concurrency0_numeric_rhs, &mut (__sn_concurrency0_value_guard)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next } }; *__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value_guard; __sn_concurrency0_value }));
    println!("{}", { let value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value });
    println!("{}", { let value = __sn_concurrency0_global_rhs_calls.lock().unwrap_or_else(|e| e.into_inner()).clone(); value });
}
