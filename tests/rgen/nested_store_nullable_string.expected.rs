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


#[derive(Clone,  PartialEq, Eq, PartialOrd, Ord, Hash)]
struct SnString(Vec<u8>, bool);

impl Default for SnString {
    fn default() -> Self { Self::nil() }
}

trait SnBytes {
    fn sn_bytes(&self) -> &[u8];
}

impl SnBytes for SnString {
    fn sn_bytes(&self) -> &[u8] { &self.0 }
}

impl SnBytes for str {
    fn sn_bytes(&self) -> &[u8] { self.as_bytes() }
}

impl SnBytes for String {
    fn sn_bytes(&self) -> &[u8] { self.as_bytes() }
}

impl SnString {
    fn nil() -> Self { Self(Vec::new(), true) }

    fn is_nil(&self) -> bool { self.1 }

    fn new() -> Self { Self(Vec::new(), false) }

    fn from_bytes(bytes: Vec<u8>) -> Self { Self(bytes, false) }

    fn from_slice(bytes: &[u8]) -> Self { Self(bytes.to_vec(), false) }

    fn from_c_bytes(bytes: &[u8]) -> Self {
        let end = bytes.iter().position(|byte| *byte == 0).unwrap_or(bytes.len());
        Self(bytes[..end].to_vec(), false)
    }

    fn as_bytes(&self) -> &[u8] { &self.0 }

    fn len(&self) -> usize { self.0.len() }

    fn is_empty(&self) -> bool { self.0.is_empty() }

    fn push_str<T: SnBytes + ?Sized>(&mut self, value: &T) {
        self.1 = false;
        self.0.extend_from_slice(value.sn_bytes());
    }

    fn push_char(&mut self, value: char) {
        if value != '\0' { self.0.push(value as u32 as u8); }
    }

    fn contains(&self, needle: &Self) -> bool {
        __sn_find_bytes(&self.0, &needle.0).is_some()
    }

    fn starts_with(&self, prefix: &Self) -> bool { self.0.starts_with(&prefix.0) }

    fn ends_with(&self, suffix: &Self) -> bool { self.0.ends_with(&suffix.0) }

    fn trim_ascii(&self) -> Self {
        let mut start = 0;
        let mut end = self.0.len();
        while start < end && self.0[start].is_ascii_whitespace() { start += 1; }
        while end > start && self.0[end - 1].is_ascii_whitespace() { end -= 1; }
        Self(self.0[start..end].to_vec(), false)
    }

    fn to_ascii_uppercase(&self) -> Self {
        Self(self.0.iter().map(u8::to_ascii_uppercase).collect(), false)
    }

    fn to_ascii_lowercase(&self) -> Self {
        Self(self.0.iter().map(u8::to_ascii_lowercase).collect(), false)
    }
}

impl From<&str> for SnString {
    fn from(value: &str) -> Self { Self(value.as_bytes().to_vec(), false) }
}

impl From<String> for SnString {
    fn from(value: String) -> Self { Self(value.into_bytes(), false) }
}

#[cfg(unix)]
fn __sn_args() -> Vec<SnString> {
    use std::os::unix::ffi::OsStrExt;
    std::env::args_os()
        .map(|value| SnString::from_slice(value.as_os_str().as_bytes()))
        .collect()
}

#[cfg(windows)]
fn __sn_push_wtf8(bytes: &mut Vec<u8>, value: u32) {
    if value <= 0x7f {
        bytes.push(value as u8);
    } else if value <= 0x7ff {
        bytes.push((0xc0 | (value >> 6)) as u8);
        bytes.push((0x80 | (value & 0x3f)) as u8);
    } else if value <= 0xffff {
        bytes.push((0xe0 | (value >> 12)) as u8);
        bytes.push((0x80 | ((value >> 6) & 0x3f)) as u8);
        bytes.push((0x80 | (value & 0x3f)) as u8);
    } else {
        bytes.push((0xf0 | (value >> 18)) as u8);
        bytes.push((0x80 | ((value >> 12) & 0x3f)) as u8);
        bytes.push((0x80 | ((value >> 6) & 0x3f)) as u8);
        bytes.push((0x80 | (value & 0x3f)) as u8);
    }
}

#[cfg(windows)]
fn __sn_args() -> Vec<SnString> {
    use std::os::windows::ffi::OsStrExt;
    std::env::args_os().map(|value| {
        let mut bytes = Vec::new();
        let mut units = value.as_os_str().encode_wide().peekable();
        while let Some(unit) = units.next() {
            let scalar = if (0xd800..=0xdbff).contains(&unit) {
                match units.peek().copied() {
                    Some(low) if (0xdc00..=0xdfff).contains(&low) => {
                        units.next();
                        0x10000 + (((unit as u32 - 0xd800) << 10) |
                                   (low as u32 - 0xdc00))
                    }
                    _ => unit as u32,
                }
            } else {
                unit as u32
            };
            __sn_push_wtf8(&mut bytes, scalar);
        }
        SnString::from_bytes(bytes)
    }).collect()
}

#[cfg(not(any(unix, windows)))]
compile_error!("Sindarin Rust argv byte transport supports Unix and Windows targets");

impl std::fmt::Debug for SnString {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("\"")?;
        for byte in &self.0 {
            match *byte {
                b'\\' => f.write_str("\\\\")?,
                b'\"' => f.write_str("\\\"")?,
                b'\n' => f.write_str("\\n")?,
                b'\r' => f.write_str("\\r")?,
                b'\t' => f.write_str("\\t")?,
                0x20..=0x7e => f.write_str(&char::from(*byte).to_string())?,
                _ => write!(f, "\\x{:02x}", byte)?,
            }
        }
        f.write_str("\"")
    }
}

fn __sn_find_bytes(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    if needle.is_empty() { return Some(0); }
    haystack.windows(needle.len()).position(|window| window == needle)
}

fn __sn_write_bytes(bytes: &[u8]) {
    crate::__sn_write_stdout_bytes(bytes);
}

fn __sn_print_string(value: &SnString) { __sn_write_bytes(value.as_bytes()); }

fn __sn_println_string(value: &SnString) {
    __sn_write_bytes(&[value.as_bytes(), b"\n"].concat());
}

fn __sn_print_char(value: char) { __sn_write_bytes(&[value as u32 as u8]); }

fn __sn_println_char(value: char) {
    __sn_write_bytes(&[value as u32 as u8, b'\n']);
}

fn __sn_string_join(values: &[SnString], delimiter: &SnString) -> SnString {
    let capacity = values.iter().map(SnString::len).sum::<usize>()
        + delimiter.len().saturating_mul(values.len().saturating_sub(1));
    let mut result = SnString(Vec::with_capacity(capacity), false);
    for (index, value) in values.iter().enumerate() {
        if index != 0 { result.push_str(delimiter); }
        result.push_str(value);
    }
    result
}

fn __sn_byte_array_to_string(values: &[u8]) -> SnString {
    SnString::from_c_bytes(values)
}

fn __sn_string_to_bytes(value: &SnString) -> Vec<u8> { value.as_bytes().to_vec() }

fn __sn_string_append(value: &SnString, suffix: &SnString) -> SnString {
    let mut result = SnString(Vec::with_capacity(value.len() + suffix.len()), false);
    result.push_str(value);
    result.push_str(suffix);
    result
}

unsafe extern "C" {
    fn strtoll(value: *const std::ffi::c_char,
               end: *mut *mut std::ffi::c_char, base: std::ffi::c_int) -> i64;
    fn strtod(value: *const std::ffi::c_char,
              end: *mut *mut std::ffi::c_char) -> f64;
}

fn __sn_nul_terminated(value: &SnString) -> Vec<u8> {
    let mut bytes = value.as_bytes().to_vec();
    bytes.push(0);
    bytes
}

fn __sn_string_to_int(value: &SnString) -> i64 {
    let bytes = __sn_nul_terminated(value);
    unsafe { strtoll(bytes.as_ptr().cast(), std::ptr::null_mut(), 10) }
}

fn __sn_string_to_double(value: &SnString) -> f64 {
    let bytes = __sn_nul_terminated(value);
    unsafe { strtod(bytes.as_ptr().cast(), std::ptr::null_mut()) }
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

fn change(index: &mut i64) -> SnString {
    (*(index) = 1);
    return SnString::from_slice(&[0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x64]);
}

fn main() {
    let mut missing: __sn_nullable_array_0<__sn_nullable_array_0<SnString>> = __sn_nullable_array_0::nil();
    let mut nilText: SnString = SnString::nil();
    println!("{}", { (missing).is_nil() }
);
    println!("{}", (nilText == SnString::nil()));
    let mut grid: __sn_nullable_array_0<__sn_nullable_array_0<SnString>> = __sn_nullable_array_0::from_vec(vec![__sn_nullable_array_0::from_vec(vec![SnString::from_slice(&[0x61]), SnString::from_slice(&[0x62])]), __sn_nullable_array_0::from_vec(vec![SnString::from_slice(&[0x63]), SnString::from_slice(&[0x64])])]);
    let mut copy: __sn_nullable_array_0<__sn_nullable_array_0<SnString>> = (grid).clone();
    let mut selected: i64 = 0;
    { let __sn_place_raw_index_0 = 0; let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (copy).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_place_raw_index_1 = selected; let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { let __sn_place_raw_index_2 = 0; let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (copy).len() as i64 } else { __sn_place_raw_index_2 }; __sn_place_raw_index_1 + ((copy)[__sn_place_index_2 as usize]).len() as i64 } else { __sn_place_raw_index_1 }; let __sn_nullable_store_value = change(&mut (selected)); *(&mut (((copy)[__sn_place_index_0 as usize])[__sn_place_index_1 as usize])) = __sn_nullable_store_value; };
    let mut last: i64 = (-1);
    { let __sn_place_raw_index_3 = last; let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (copy).len() as i64 } else { __sn_place_raw_index_3 }; let __sn_place_raw_index_4 = last; let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { let __sn_place_raw_index_5 = last; let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (copy).len() as i64 } else { __sn_place_raw_index_5 }; __sn_place_raw_index_4 + ((copy)[__sn_place_index_5 as usize]).len() as i64 } else { __sn_place_raw_index_4 }; let __sn_nullable_store_value_1 = SnString::from_slice(&[0x74, 0x61, 0x69, 0x6c]); *(&mut (((copy)[__sn_place_index_3 as usize])[__sn_place_index_4 as usize])) = __sn_nullable_store_value_1; };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(((grid)[__sn_index((grid).len(), 0)])[__sn_index(((grid)[__sn_index((grid).len(), 0)]).len(), 0)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&(((grid)[__sn_index((grid).len(), 0)])[__sn_index(((grid)[__sn_index((grid).len(), 0)]).len(), 1)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&(((grid)[__sn_index((grid).len(), 1)])[__sn_index(((grid)[__sn_index((grid).len(), 1)]).len(), 0)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&(((grid)[__sn_index((grid).len(), 1)])[__sn_index(((grid)[__sn_index((grid).len(), 1)]).len(), 1)])); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(((copy)[__sn_index((copy).len(), 0)])[__sn_index(((copy)[__sn_index((copy).len(), 0)]).len(), 0)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&(((copy)[__sn_index((copy).len(), 0)])[__sn_index(((copy)[__sn_index((copy).len(), 0)]).len(), 1)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&(((copy)[__sn_index((copy).len(), 1)])[__sn_index(((copy)[__sn_index((copy).len(), 1)]).len(), 0)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&(((copy)[__sn_index((copy).len(), 1)])[__sn_index(((copy)[__sn_index((copy).len(), 1)]).len(), 1)])); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated }));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
