#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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


enum __SnScalarStorage<'a, T> {
    Borrowed(std::cell::RefCell<&'a mut T>),
    Shared(std::sync::Arc<std::sync::Mutex<T>>),
}
struct __SnScalarRef<'a, T>(std::rc::Rc<__SnScalarStorage<'a, T>>);
impl<'a, T: Copy> __SnScalarRef<'a, T> {
    fn new(value: &'a mut T) -> Self { Self(std::rc::Rc::new(__SnScalarStorage::Borrowed(std::cell::RefCell::new(value)))) }
    fn shared(value: std::sync::Arc<std::sync::Mutex<T>>) -> Self { Self(std::rc::Rc::new(__SnScalarStorage::Shared(value))) }
    fn get(&self) -> T { match &*self.0 {
        __SnScalarStorage::Borrowed(value) => **value.borrow(),
        __SnScalarStorage::Shared(value) => *value.lock().unwrap_or_else(|e| e.into_inner()),
    } }
    fn set(&self, next: T) { match &*self.0 {
        __SnScalarStorage::Borrowed(value) => **value.borrow_mut() = next,
        __SnScalarStorage::Shared(value) => *value.lock().unwrap_or_else(|e| e.into_inner()) = next,
    } }
    fn same_place(&self, other: &__SnScalarRef<'_, T>) -> bool {
        if std::rc::Rc::as_ptr(&self.0) as *const () == std::rc::Rc::as_ptr(&other.0) as *const () { return true; }
        match (&*self.0, &*other.0) {
            (__SnScalarStorage::Shared(a), __SnScalarStorage::Shared(b)) => std::sync::Arc::ptr_eq(a, b),
            _ => false,
        }
    }
    fn borrow_mut(&self) -> __SnScalarGuard<'_, 'a, T> { match &*self.0 {
        __SnScalarStorage::Borrowed(value) => __SnScalarGuard::Borrowed(value.borrow_mut()),
        __SnScalarStorage::Shared(value) => __SnScalarGuard::Shared(value.lock().unwrap_or_else(|e| e.into_inner())),
    } }
}
impl<T> Clone for __SnScalarRef<'_, T> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
enum __SnScalarGuard<'r, 'a, T> {
    Borrowed(std::cell::RefMut<'r, &'a mut T>),
    Shared(std::sync::MutexGuard<'r, T>),
}
impl<T> std::ops::Deref for __SnScalarGuard<'_, '_, T> {
    type Target = T;
    fn deref(&self) -> &T { match self { Self::Borrowed(value) => &***value, Self::Shared(value) => &**value } }
}
impl<T> std::ops::DerefMut for __SnScalarGuard<'_, '_, T> {
    fn deref_mut(&mut self) -> &mut T { match self { Self::Borrowed(value) => &mut ***value, Self::Shared(value) => &mut **value } }
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
fn main() {
    let mut action: __SnClosure<dyn Fn(__SnScalarRef<'_, i64>) -> i64> = { self::__SnClosure::<dyn Fn(__SnScalarRef<'_, i64>) -> i64>(std::rc::Rc::new(move |value: __SnScalarRef<'_, i64>| -> i64 { value.get().clone()})) }
;
    let mut value: i64 = 1;
    println!("{}", { let __sn_closure_scalar_cell = __SnScalarRef::new(&mut (value)); ((action.clone()).0)(__sn_closure_scalar_cell.clone()) });
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
