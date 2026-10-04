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

fn main() {
    let mut absent: __sn_nullable_array_0<f64> = __sn_nullable_array_0::nil();
    let mut empty: __sn_nullable_array_0<f64> = __sn_nullable_array_0::from_vec(vec![]);
    let mut absentCopy: __sn_nullable_array_0<f64> = (absent).clone();
    let mut emptyCopy: __sn_nullable_array_0<f64> = (empty).clone();
    println!("{}", { let __sn_float_eq_right: &__sn_nullable_array_0<f64> = &(absentCopy); let __sn_float_eq_left: &__sn_nullable_array_0<f64> = &(absent); (__sn_float_eq_left.is_nil() == __sn_float_eq_right.is_nil() && __sn_float_eq_left.len() == __sn_float_eq_right.len() && __sn_float_eq_left.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_1: &__sn_nullable_array_0<f64> = &(empty); let __sn_float_eq_left_1: &__sn_nullable_array_0<f64> = &(absent); (__sn_float_eq_left_1.is_nil() == __sn_float_eq_right_1.is_nil() && __sn_float_eq_left_1.len() == __sn_float_eq_right_1.len() && __sn_float_eq_left_1.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_1.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_1.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_2: &__sn_nullable_array_0<f64> = &(absent); let __sn_float_eq_left_2: &__sn_nullable_array_0<f64> = &(empty); (__sn_float_eq_left_2.is_nil() == __sn_float_eq_right_2.is_nil() && __sn_float_eq_left_2.len() == __sn_float_eq_right_2.len() && __sn_float_eq_left_2.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_2.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_2.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_3: &__sn_nullable_array_0<f64> = &(emptyCopy); let __sn_float_eq_left_3: &__sn_nullable_array_0<f64> = &(empty); (__sn_float_eq_left_3.is_nil() == __sn_float_eq_right_3.is_nil() && __sn_float_eq_left_3.len() == __sn_float_eq_right_3.len() && __sn_float_eq_left_3.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_3.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_3.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_4: &__sn_nullable_array_0<f64> = &(empty); let __sn_float_eq_left_4: &__sn_nullable_array_0<f64> = &(absent); !(__sn_float_eq_left_4.is_nil() == __sn_float_eq_right_4.is_nil() && __sn_float_eq_left_4.len() == __sn_float_eq_right_4.len() && __sn_float_eq_left_4.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_4.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_4.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { (absent).is_nil() }
);
    println!("{}", { (empty).is_nil() }
);
    let mut positive: __sn_nullable_array_0<f64> = __sn_nullable_array_0::from_vec(vec![0.0]);
    let mut negative: __sn_nullable_array_0<f64> = __sn_nullable_array_0::from_vec(vec![(-0.0)]);
    println!("{}", { let __sn_float_eq_right_5: &__sn_nullable_array_0<f64> = &(negative); let __sn_float_eq_left_5: &__sn_nullable_array_0<f64> = &(positive); (__sn_float_eq_left_5.is_nil() == __sn_float_eq_right_5.is_nil() && __sn_float_eq_left_5.len() == __sn_float_eq_right_5.len() && __sn_float_eq_left_5.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_5.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_5.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_6: &__sn_nullable_array_0<f64> = &(negative); let __sn_float_eq_left_6: &__sn_nullable_array_0<f64> = &(positive); !(__sn_float_eq_left_6.is_nil() == __sn_float_eq_right_6.is_nil() && __sn_float_eq_left_6.len() == __sn_float_eq_right_6.len() && __sn_float_eq_left_6.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_6.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_6.len() * std::mem::size_of::<f64>()))) }
);
    let mut bothAbsent: __sn_nullable_array_0<f64> = { let __sn_array_left = &(absent); let __sn_array_right = &(absentCopy); __sn_array_left.concat_nullable(__sn_array_right) };
    println!("{}", { (bothAbsent).is_nil() }
);
    let mut joined: __sn_nullable_array_0<f64> = { let __sn_array_left = &(absent); let __sn_array_right = &(empty); __sn_array_left.concat_nullable(__sn_array_right) };
    println!("{}", { (joined).is_nil() }
);
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
