#![allow(dead_code, unused_mut, unused_variables, unused_parens)]
#[derive(Debug)]
struct __sn_concurrency0_Field<T>(std::sync::Arc<std::sync::Mutex<T>>);
impl<T> __sn_concurrency0_Field<T> {
    fn new(value: T) -> Self { Self(std::sync::Arc::new(std::sync::Mutex::new(value))) }
    fn share(&self) -> Self { Self(self.0.clone()) }
    fn lock(&self) -> std::sync::MutexGuard<'_ , T> { self.0.lock().unwrap_or_else(|e| e.into_inner()) }
    fn set(&self, value: T) { *self.0.lock().unwrap_or_else(|e| e.into_inner()) = value; }
}
impl<T: Clone> __sn_concurrency0_Field<T> {
    fn read(&self) -> T { self.0.lock().unwrap_or_else(|e| e.into_inner()).clone() }
}
impl<T: Clone> Clone for __sn_concurrency0_Field<T> {
    fn clone(&self) -> Self { Self::new(self.read()) }
}
impl<T: Clone + PartialEq> PartialEq for __sn_concurrency0_Field<T> {
    fn eq(&self, other: &Self) -> bool { self.read() == other.read() }
}
#[derive(Debug)]
struct __sn_concurrency0_ReferenceField<T>(Option<std::sync::Arc<std::sync::Mutex<T>>>);
impl<T> __sn_concurrency0_ReferenceField<T> {
    fn new(value: T) -> Self { Self(Some(std::sync::Arc::new(std::sync::Mutex::new(value)))) }
    fn nil() -> Self { Self(None) }
    fn share(&self) -> Self { Self(self.0.clone()) }
    fn owner(&self) -> __sn_concurrency0_Field<T> {
        __sn_concurrency0_Field(self.0.as_ref().expect("nil record field access").clone())
    }
    fn lock(&self) -> std::sync::MutexGuard<'_, T> { self.0.as_ref().expect("nil record field access").lock().unwrap_or_else(|e| e.into_inner()) }
    fn set(&self, value: T) { *self.lock() = value; }
}
impl<T: Clone> __sn_concurrency0_ReferenceField<T> {
    fn read(&self) -> T { self.lock().clone() }
}
impl<T: Clone> Clone for __sn_concurrency0_ReferenceField<T> {
    fn clone(&self) -> Self { if self.0.is_none() { Self::nil() } else { Self::new(self.read()) } }
}

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
#[derive( Debug)]
struct State {
    __sn_concurrency0_record_identity: Option<std::sync::Arc<()>>,
    value: __sn_concurrency0_ReferenceField<i64>,
}
impl State {
    fn __sn_concurrency0_share(&self) -> Self {
        Self { __sn_concurrency0_record_identity: self.__sn_concurrency0_record_identity.clone(), value: self.value.share(),  }
    }
}
impl State {
    fn __sn_concurrency0_nil() -> Self {
        Self { __sn_concurrency0_record_identity: None, value: __sn_concurrency0_ReferenceField::nil(),  }
    }

    fn __sn_concurrency0_snapshot(&self) -> Self {
        if self.__sn_concurrency0_record_identity.is_none() { return Self::__sn_concurrency0_nil(); }
        Self { __sn_concurrency0_record_identity: Some(std::sync::Arc::new(())), value: self.value.clone(),  }
    }
}
impl PartialEq for State {
    fn eq(&self, other: &Self) -> bool {
        match (&self.__sn_concurrency0_record_identity, &other.__sn_concurrency0_record_identity) {
            (Some(left), Some(right)) => std::sync::Arc::ptr_eq(left, right),
            (None, None) => true,
            _ => false,
        }
    }
}
impl Clone for State {
    fn clone(&self) -> Self { self.__sn_concurrency0_share() }
}


fn main() {
    let mut state: State = State { __sn_concurrency0_record_identity: Some(std::sync::Arc::new(())), value: __sn_concurrency0_ReferenceField::new(1) };
    let mut action: __SnClosure<dyn Fn() -> i64> = { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { (state).value.read().clone()})) }
;
    println!("{}", ((action.clone()).0)());
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
