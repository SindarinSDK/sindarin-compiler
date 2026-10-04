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


#[allow(non_camel_case_types)]
#[derive(Clone, Debug, PartialEq, Eq)]
struct __sn_nullable_array_0<T> {
    values: Vec<T>,
    nil: bool,
}

impl<T> __sn_nullable_array_0<T> {
    fn nil() -> Self { Self { values: Vec::new(), nil: true } }
    fn from_vec(values: Vec<T>) -> Self { Self { values, nil: false } }
    fn is_nil(&self) -> bool { self.nil }
}

impl<T: Clone> __sn_nullable_array_0<T> {
    fn concat_nullable(&self, other: &Self) -> Self {
        if self.nil && other.nil { return Self::nil(); }
        Self::from_vec([self.values.as_slice(), other.values.as_slice()].concat())
    }
}

impl<T> Default for __sn_nullable_array_0<T> {
    fn default() -> Self { Self::nil() }
}

impl<T> std::ops::Deref for __sn_nullable_array_0<T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Self::Target { &self.values }
}

impl<T> std::ops::DerefMut for __sn_nullable_array_0<T> {
    fn deref_mut(&mut self) -> &mut Self::Target { &mut self.values }
}

impl<T> IntoIterator for __sn_nullable_array_0<T> {
    type Item = T;
    type IntoIter = std::vec::IntoIter<T>;
    fn into_iter(self) -> Self::IntoIter { self.values.into_iter() }
}

impl<'a, T> IntoIterator for &'a __sn_nullable_array_0<T> {
    type Item = &'a T;
    type IntoIter = std::slice::Iter<'a, T>;
    fn into_iter(self) -> Self::IntoIter { self.values.iter() }
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

#[derive(Clone, Debug, PartialEq)]
struct Holder {
    values: __sn_nullable_array_0<i64>,
}

impl Holder {
    fn missing(&self) -> bool {
        return { ((self).values.clone()).is_nil() }
;
    }
    fn missingArgument(&self, values: &mut __sn_nullable_array_0<i64>) -> bool {
        return { (values).is_nil() }
;
    }
    fn missingStatic(values: &mut __sn_nullable_array_0<i64>) -> bool {
        return { (values).is_nil() }
;
    }
    fn absent() -> __sn_nullable_array_0<i64> {
        return __sn_nullable_array_0::nil();
    }
}

fn missing(values: &mut __sn_nullable_array_0<i64>) -> bool {
    return { (values).is_nil() }
;
}

fn main() {
    let mut holder: Holder = Holder { values: __sn_nullable_array_0::nil() };
    println!("{}", (holder).missing());
    ((holder).values = __sn_nullable_array_0::from_vec(vec![]));
    println!("{}", (holder).missing());
    ((holder).values = __sn_nullable_array_0::nil());
    println!("{}", { ((holder).values).is_nil() }
);
    println!("{}", missing(&mut (__sn_nullable_array_0::nil())));
    println!("{}", (holder).missingArgument(&mut (__sn_nullable_array_0::nil())));
    println!("{}", Holder::missingStatic(&mut (__sn_nullable_array_0::nil())));
    let mut result: __sn_nullable_array_0<i64> = Holder::absent();
    println!("{}", { (result).is_nil() }
);
    let mut nested: __sn_nullable_array_0<__sn_nullable_array_0<i64>> = __sn_nullable_array_0::from_vec(vec![__sn_nullable_array_0::from_vec(vec![]), __sn_nullable_array_0::from_vec(vec![])]);
    { let __sn_array_index = __sn_index((nested).len(), 0); (nested)[__sn_array_index] = __sn_nullable_array_0::nil(); };
    let mut copied: __sn_nullable_array_0<__sn_nullable_array_0<i64>> = (nested).clone();
    println!("{}", { ((copied)[__sn_index((copied).len(), 0)]).is_nil() }
);
    println!("{}", { ((copied)[__sn_index((copied).len(), 1)]).is_nil() }
);
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
