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

// Arc owns capture identity. Reads clone under the lock and release it before
// evaluating the next source operand; mutable operations borrow the same cell.
struct __sn_concurrency0_Capture<T>(std::sync::Mutex<T>, std::sync::Mutex<()>);
impl<T> __sn_concurrency0_Capture<T> {
    fn new(value: T) -> Self { Self(std::sync::Mutex::new(value), std::sync::Mutex::new(())) }
    fn lock(&self) -> std::sync::LockResult<std::sync::MutexGuard<'_, T>> { self.0.lock() }
    fn guard(&self) -> std::sync::MutexGuard<'_, ()> { self.1.lock().unwrap_or_else(|e| e.into_inner()) }
    fn borrow_mut(&self) -> std::sync::MutexGuard<'_, T> { self.0.lock().unwrap_or_else(|e| e.into_inner()) }
    fn set(&self, value: T) { *self.borrow_mut() = value; }
    fn replace(&self, value: T) -> T { std::mem::replace(&mut *self.borrow_mut(), value) }
}
impl<T: Clone> __sn_concurrency0_Capture<T> {
    fn borrow(&self) -> T { self.borrow_mut().clone() }
    fn get(&self) -> T { self.borrow() }
}

static __sn_concurrency0_global_value: std::sync::LazyLock<__sn_concurrency0_Cell<char>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new('\u{41}'));

extern "C" {
    #[link_name = "fwrite"]
    fn __sn_stdio_fwrite(_: *const std::ffi::c_void, _: usize, _: usize, _: *mut std::ffi::c_void) -> usize;
    #[link_name = "fflush"]
    fn __sn_stdio_fflush(_: *mut std::ffi::c_void) -> std::ffi::c_int;
    #[link_name = "exit"]
    fn __sn_stdio_c_exit(_: std::ffi::c_int) -> !;
    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_stdio_iob(_: u32) -> *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stdoutp"]
    static mut __sn_stdio_stdout: *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stderrp"]
    static mut __sn_stdio_stderr: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stdout"]
    static mut __sn_stdio_stdout: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stderr"]
    static mut __sn_stdio_stderr: *mut std::ffi::c_void;
}

fn __sn_stdio_write(bytes: &[u8], stderr: bool) {
    unsafe {
        #[cfg(windows)]
        let stream = __sn_stdio_iob(if stderr { 2 } else { 1 });
        #[cfg(not(windows))]
        let stream = if stderr { __sn_stdio_stderr } else { __sn_stdio_stdout };
        __sn_stdio_fwrite(bytes.as_ptr().cast(), 1, bytes.len(), stream);
    }
}

fn __sn_stdio_exit(status: i32) -> ! {
    unsafe { __sn_stdio_c_exit(status) }
}

struct __SnStdioGuard;
impl Drop for __SnStdioGuard {
    fn drop(&mut self) {
        unsafe { __sn_stdio_fflush(std::ptr::null_mut()); }
    }
}


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

fn __sn_write_stdout_bytes(bytes: &[u8]) {
    __sn_stdio_write(bytes, false);
}

fn __sn_write_stderr_bytes(bytes: &[u8]) {
    __sn_stdio_write(bytes, true);
}

fn __sn_print_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    __sn_write_stdout_bytes(rendered.as_bytes());
}

fn __sn_println_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    rendered.push('\n');
    __sn_write_stdout_bytes(rendered.as_bytes());
}

macro_rules! print {
    ($($arg:tt)*) => { crate::__sn_print_format(format_args!($($arg)*)) };
}

macro_rules! println {
    () => { crate::__sn_println_format(format_args!("")) };
    ($($arg:tt)*) => { crate::__sn_println_format(format_args!($($arg)*)) };
}


fn __sn_runtime_error_0(message: &'static str) -> ! {
    crate::__sn_write_stderr_bytes(&[message.as_bytes(), b"\n"].concat());
    crate::__sn_stdio_exit(1);
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

fn step() -> i64 {
    {
        let mut index: i64 = 0;

        while (index < 100) {
            { let __sn_concurrency0_gate = __sn_concurrency0_global_value.guard(); let mut __sn_concurrency0_value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()); let __sn_concurrency0_previous = *__sn_concurrency0_value; *__sn_concurrency0_value = ((*__sn_concurrency0_value as u32 as u8).wrapping_add(1)) as char; drop(__sn_concurrency0_value); drop(__sn_concurrency0_gate); { let value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } };
            { let __sn_place = &mut (index); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    return 100;
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_value);
    let mut a: i64 = 0; let mut __sn_concurrency0_handle_a: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || step()) }
);
    let mut b: i64 = 0; let mut __sn_concurrency0_handle_b: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || step()) }
);
    let mut c: i64 = 0; let mut __sn_concurrency0_handle_c: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || step()) }
);
    let mut d: i64 = 0; let mut __sn_concurrency0_handle_d: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || step()) }
);
    let mut e: i64 = 0; let mut __sn_concurrency0_handle_e: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || step()) }
);
    println!("{}", __sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_checked_0(({ if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_a.take() { a = __sn_concurrency0_handle.join(); } a.clone() }
).checked_add({ if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_b.take() { b = __sn_concurrency0_handle.join(); } b.clone() }
), "Runtime error: integer overflow in addition")).checked_add({ if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_c.take() { c = __sn_concurrency0_handle.join(); } c.clone() }
), "Runtime error: integer overflow in addition")).checked_add({ if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_d.take() { d = __sn_concurrency0_handle.join(); } d.clone() }
), "Runtime error: integer overflow in addition")).checked_add({ if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_e.take() { e = __sn_concurrency0_handle.join(); } e.clone() }
), "Runtime error: integer overflow in addition"));
    println!("{}", ({ let value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } == '\u{35}'));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
