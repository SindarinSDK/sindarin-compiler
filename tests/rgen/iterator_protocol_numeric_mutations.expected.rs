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


#[derive(Clone, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
struct SnString(Vec<u8>);

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
    fn new() -> Self { Self(Vec::new()) }

    fn from_bytes(bytes: Vec<u8>) -> Self { Self(bytes) }

    fn from_slice(bytes: &[u8]) -> Self { Self(bytes.to_vec()) }

    fn from_c_bytes(bytes: &[u8]) -> Self {
        let end = bytes.iter().position(|byte| *byte == 0).unwrap_or(bytes.len());
        Self(bytes[..end].to_vec())
    }

    fn as_bytes(&self) -> &[u8] { &self.0 }

    fn len(&self) -> usize { self.0.len() }

    fn is_empty(&self) -> bool { self.0.is_empty() }

    fn push_str<T: SnBytes + ?Sized>(&mut self, value: &T) {
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
        Self(self.0[start..end].to_vec())
    }

    fn to_ascii_uppercase(&self) -> Self {
        Self(self.0.iter().map(u8::to_ascii_uppercase).collect())
    }

    fn to_ascii_lowercase(&self) -> Self {
        Self(self.0.iter().map(u8::to_ascii_lowercase).collect())
    }
}

impl From<&str> for SnString {
    fn from(value: &str) -> Self { Self(value.as_bytes().to_vec()) }
}

impl From<String> for SnString {
    fn from(value: String) -> Self { Self(value.into_bytes()) }
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
    let mut result = SnString(Vec::with_capacity(capacity));
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
    let mut result = SnString(Vec::with_capacity(value.len() + suffix.len()));
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

#[derive(Clone, Copy, Debug, PartialEq)]
struct IntSequence {
    value: i64,
    remaining: i64,
    has_next_calls: i64,
    next_calls: i64,
}

impl IntSequence {
    fn iter(&self) -> IntSequence {
        return self.clone();
    }
    fn hasNext(&mut self) -> bool {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).has_next_calls); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        return ((self).next_calls < (self).remaining);
    }
    fn next(&mut self) -> i64 {
        let mut result: i64 = __sn_checked_0((__sn_checked_0((__sn_checked_0(((self).has_next_calls).checked_mul(100), "Runtime error: integer overflow in multiplication")).checked_add(__sn_checked_0(((self).next_calls).checked_mul(10), "Runtime error: integer overflow in multiplication")), "Runtime error: integer overflow in addition")).checked_add((self).value), "Runtime error: integer overflow in addition");
        { let __sn_rhs = 1; let __sn_place = &mut ((self).next_calls); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        return result;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct LongSequence {
    value: i64,
    remaining: i64,
}

impl LongSequence {
    fn iter(&self) -> LongSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> i64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Int32Sequence {
    value: i32,
    remaining: i64,
}

impl Int32Sequence {
    fn iter(&self) -> Int32Sequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> i32 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct ByteSequence {
    value: u8,
    remaining: i64,
}

impl ByteSequence {
    fn iter(&self) -> ByteSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u8 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Uint32Sequence {
    value: u32,
    remaining: i64,
}

impl Uint32Sequence {
    fn iter(&self) -> Uint32Sequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u32 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct UintSequence {
    value: u64,
    remaining: i64,
}

impl UintSequence {
    fn iter(&self) -> UintSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct FloatSequence {
    value: f32,
    remaining: i64,
}

impl FloatSequence {
    fn iter(&self) -> FloatSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> f32 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct DoubleSequence {
    value: f64,
    remaining: i64,
}

impl DoubleSequence {
    fn iter(&self) -> DoubleSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> f64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}

fn rhsInt(calls: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return 2;
}

fn rhsFloat(calls: &mut i64) -> f32 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return 2.0;
}

fn selectInt(calls: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return 0;
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut __sn_rhs: i64 = 41;
    let mut __sn_place: i64 = 42;
    let mut __sn_next: i64 = 43;
    let mut __sn_previous: i64 = 44;
    let mut __sn_iter_0: i64 = 45;
    let mut iterable_calls: i64 = 0;
    let mut int_rhs_calls: i64 = 0;
    let mut float_rhs_calls: i64 = 0;
    let mut int_sources: Vec<IntSequence> = vec![IntSequence { value: 8, remaining: 2, has_next_calls: 0, next_calls: 0 }];
    {
    let mut __sn_iter_1 = ((int_sources)[__sn_index((int_sources).len(), selectInt(&mut (iterable_calls)))]).iter();
    while __sn_iter_1.hasNext() {
        let mut value = __sn_iter_1.next();
        let mut original: i64 = value;
        let mut compound: i64 = { let __sn_rhs = rhsInt(&mut (int_rhs_calls)); let __sn_place = &mut (value); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x69, 0x6e, 0x74, 0x20]))); __sn_interpolated.push_str(&format!("{}", original)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
        if (original == 108) {
        continue;
    }
    }
}
    let mut longs: LongSequence = LongSequence { value: 20, remaining: 2 };
    {
    let mut __sn_iter_2 = (longs).iter();
    while __sn_iter_2.hasNext() {
        let mut value = __sn_iter_2.next();
        let mut compound: i64 = { let __sn_rhs = 3; let __sn_place = &mut (value); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x6c, 0x6f, 0x6e, 0x67, 0x20]))); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
    }
}
    let mut int32s: Int32Sequence = Int32Sequence { value: 6, remaining: 2 };
    {
    let mut __sn_iter_3 = (int32s).iter();
    while __sn_iter_3.hasNext() {
        let mut value = __sn_iter_3.next();
        let mut compound: i32 = { let __sn_rhs = 2; let __sn_place = &mut (value); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: i32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x69, 0x6e, 0x74, 0x33, 0x32, 0x20]))); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
    }
}
    let mut bytes: ByteSequence = ByteSequence { value: 20, remaining: 2 };
    {
    let mut __sn_iter_4 = (bytes).iter();
    while __sn_iter_4.hasNext() {
        let mut value = __sn_iter_4.next();
        let mut compound: u8 = { let __sn_rhs = 2; let __sn_place = &mut (value); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut postfix: u8 = { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x62, 0x79, 0x74, 0x65, 0x20]))); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
    }
}
    let mut uint32s: Uint32Sequence = Uint32Sequence { value: 10, remaining: 2 };
    {
    let mut __sn_iter_5 = (uint32s).iter();
    while __sn_iter_5.hasNext() {
        let mut value = __sn_iter_5.next();
        let mut compound: u32 = { let __sn_rhs = 6; let __sn_place = &mut (value); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut postfix: u32 = { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x75, 0x69, 0x6e, 0x74, 0x33, 0x32, 0x20]))); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
    }
}
    let mut uints: UintSequence = UintSequence { value: 10, remaining: 2 };
    {
    let mut __sn_iter_6 = (uints).iter();
    while __sn_iter_6.hasNext() {
        let mut value = __sn_iter_6.next();
        let mut compound: u64 = { let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (3, &mut (value)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
        let mut postfix: u64 = { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x75, 0x69, 0x6e, 0x74, 0x20]))); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
    }
}
    let mut floats: FloatSequence = FloatSequence { value: 4.0, remaining: 2 };
    {
    let mut __sn_iter_7 = (floats).iter();
    while __sn_iter_7.hasNext() {
        let mut value = __sn_iter_7.next();
        let mut added: f32 = { let (__sn_rhs, __sn_place) = (rhsFloat(&mut (float_rhs_calls)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut subtracted: f32 = { let (__sn_rhs, __sn_place) = (1.0, &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut postfix: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x66, 0x6c, 0x6f, 0x61, 0x74, 0x20]))); __sn_interpolated.push_str(&format!("{}", (added == 6.0))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (subtracted == 5.0))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (postfix == 5.0))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (value == 6.0))); __sn_interpolated }));
    }
}
    let mut doubles: DoubleSequence = DoubleSequence { value: 8.0, remaining: 2 };
    {
    let mut __sn_iter_8 = (doubles).iter();
    while __sn_iter_8.hasNext() {
        let mut value = __sn_iter_8.next();
        let mut multiplied: f64 = { let (__sn_rhs, __sn_place) = (0.5, &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut divided: f64 = { let (__sn_rhs, __sn_place) = (2.0, &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut postfix: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x64, 0x6f, 0x75, 0x62, 0x6c, 0x65, 0x20]))); __sn_interpolated.push_str(&format!("{}", (multiplied == 4.0))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (divided == 2.0))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (postfix == 2.0))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (value == 1.0))); __sn_interpolated }));
    }
}
    let mut nested_outer: IntSequence = IntSequence { value: 1, remaining: 2, has_next_calls: 0, next_calls: 0 };
    let mut nested_inner: ByteSequence = ByteSequence { value: 5, remaining: 2 };
    {
    let mut __sn_iter_10 = (nested_outer).iter();
    while __sn_iter_10.hasNext() {
        let mut value = __sn_iter_10.next();
        let mut outer_compound: i64 = { let __sn_rhs = 1; let __sn_place = &mut (value); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        {
    let mut __sn_iter_9 = (nested_inner).iter();
    while __sn_iter_9.hasNext() {
        let mut value = __sn_iter_9.next();
        let mut inner_postfix: u8 = { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x6e, 0x65, 0x73, 0x74, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x20]))); __sn_interpolated.push_str(&format!("{}", inner_postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
        break;
    }
}
        let mut outer_postfix: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x6e, 0x65, 0x73, 0x74, 0x65, 0x64, 0x20, 0x6f, 0x75, 0x74, 0x65, 0x72, 0x20]))); __sn_interpolated.push_str(&format!("{}", outer_compound)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", outer_postfix)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }));
        continue;
    }
}
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x73, 0x74, 0x61, 0x74, 0x65, 0x20]))); __sn_interpolated.push_str(&format!("{}", iterable_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", int_rhs_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", float_rhs_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", ((int_sources)[__sn_index((int_sources).len(), 0)]).value)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", ((int_sources)[__sn_index((int_sources).len(), 0)]).has_next_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", ((int_sources)[__sn_index((int_sources).len(), 0)]).next_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (longs).value)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", (bytes).value)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x68, 0x65, 0x6c, 0x70, 0x65, 0x72, 0x73, 0x20]))); __sn_interpolated.push_str(&format!("{}", __sn_rhs)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", __sn_place)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", __sn_next)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", __sn_previous)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", __sn_iter_0)); __sn_interpolated }));
}
