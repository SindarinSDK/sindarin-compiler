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

fn referenceNext(value: &mut char) -> char {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; *__sn_place = (__sn_previous as u32 as u8).wrapping_add(1) as char; __sn_previous };
}

fn valueNext(mut value: char) -> char {
    let mut old: char = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; *__sn_place = (__sn_previous as u32 as u8).wrapping_add(1) as char; __sn_previous };
    return old;
}

fn referencePrevious(value: &mut char) -> char {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; *__sn_place = (__sn_previous as u32 as u8).wrapping_sub(1) as char; __sn_previous };
}

fn main() {
    let mut values: Vec<char> = vec!['\u{0}', '\u{1}', '\u{2}', '\u{3}', '\u{4}', '\u{5}', '\u{6}', '\u{7}', '\u{8}', '\u{9}', '\u{a}', '\u{b}', '\u{c}', '\u{d}', '\u{e}', '\u{f}', '\u{10}', '\u{11}', '\u{12}', '\u{13}', '\u{14}', '\u{15}', '\u{16}', '\u{17}', '\u{18}', '\u{19}', '\u{1a}', '\u{1b}', '\u{1c}', '\u{1d}', '\u{1e}', '\u{1f}', '\u{20}', '\u{21}', '\u{22}', '\u{23}', '\u{24}', '\u{25}', '\u{26}', '\u{27}', '\u{28}', '\u{29}', '\u{2a}', '\u{2b}', '\u{2c}', '\u{2d}', '\u{2e}', '\u{2f}', '\u{30}', '\u{31}', '\u{32}', '\u{33}', '\u{34}', '\u{35}', '\u{36}', '\u{37}', '\u{38}', '\u{39}', '\u{3a}', '\u{3b}', '\u{3c}', '\u{3d}', '\u{3e}', '\u{3f}', '\u{40}', '\u{41}', '\u{42}', '\u{43}', '\u{44}', '\u{45}', '\u{46}', '\u{47}', '\u{48}', '\u{49}', '\u{4a}', '\u{4b}', '\u{4c}', '\u{4d}', '\u{4e}', '\u{4f}', '\u{50}', '\u{51}', '\u{52}', '\u{53}', '\u{54}', '\u{55}', '\u{56}', '\u{57}', '\u{58}', '\u{59}', '\u{5a}', '\u{5b}', '\u{5c}', '\u{5d}', '\u{5e}', '\u{5f}', '\u{60}', '\u{61}', '\u{62}', '\u{63}', '\u{64}', '\u{65}', '\u{66}', '\u{67}', '\u{68}', '\u{69}', '\u{6a}', '\u{6b}', '\u{6c}', '\u{6d}', '\u{6e}', '\u{6f}', '\u{70}', '\u{71}', '\u{72}', '\u{73}', '\u{74}', '\u{75}', '\u{76}', '\u{77}', '\u{78}', '\u{79}', '\u{7a}', '\u{7b}', '\u{7c}', '\u{7d}', '\u{7e}', '\u{7f}', '\u{80}', '\u{81}', '\u{82}', '\u{83}', '\u{84}', '\u{85}', '\u{86}', '\u{87}', '\u{88}', '\u{89}', '\u{8a}', '\u{8b}', '\u{8c}', '\u{8d}', '\u{8e}', '\u{8f}', '\u{90}', '\u{91}', '\u{92}', '\u{93}', '\u{94}', '\u{95}', '\u{96}', '\u{97}', '\u{98}', '\u{99}', '\u{9a}', '\u{9b}', '\u{9c}', '\u{9d}', '\u{9e}', '\u{9f}', '\u{a0}', '\u{a1}', '\u{a2}', '\u{a3}', '\u{a4}', '\u{a5}', '\u{a6}', '\u{a7}', '\u{a8}', '\u{a9}', '\u{aa}', '\u{ab}', '\u{ac}', '\u{ad}', '\u{ae}', '\u{af}', '\u{b0}', '\u{b1}', '\u{b2}', '\u{b3}', '\u{b4}', '\u{b5}', '\u{b6}', '\u{b7}', '\u{b8}', '\u{b9}', '\u{ba}', '\u{bb}', '\u{bc}', '\u{bd}', '\u{be}', '\u{bf}', '\u{c0}', '\u{c1}', '\u{c2}', '\u{c3}', '\u{c4}', '\u{c5}', '\u{c6}', '\u{c7}', '\u{c8}', '\u{c9}', '\u{ca}', '\u{cb}', '\u{cc}', '\u{cd}', '\u{ce}', '\u{cf}', '\u{d0}', '\u{d1}', '\u{d2}', '\u{d3}', '\u{d4}', '\u{d5}', '\u{d6}', '\u{d7}', '\u{d8}', '\u{d9}', '\u{da}', '\u{db}', '\u{dc}', '\u{dd}', '\u{de}', '\u{df}', '\u{e0}', '\u{e1}', '\u{e2}', '\u{e3}', '\u{e4}', '\u{e5}', '\u{e6}', '\u{e7}', '\u{e8}', '\u{e9}', '\u{ea}', '\u{eb}', '\u{ec}', '\u{ed}', '\u{ee}', '\u{ef}', '\u{f0}', '\u{f1}', '\u{f2}', '\u{f3}', '\u{f4}', '\u{f5}', '\u{f6}', '\u{f7}', '\u{f8}', '\u{f9}', '\u{fa}', '\u{fb}', '\u{fc}', '\u{fd}', '\u{fe}', '\u{ff}'];
    let mut original: i64 = 0;
    let mut advanced: i64 = 0;
    let mut isolated: i64 = 0;
    let mut restored: i64 = 0;
    {
        let mut i: i64 = 0;

        while (i < (values).len() as i64) {
            let mut expected: char = (values)[__sn_index((values).len(), i)];
            let mut value: char = expected;
            let mut old: char = referenceNext(&mut (value));
            if (old == expected) {
        { let __sn_place = &mut (original); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
            if (value == (values)[__sn_index((values).len(), { let __sn_left = __sn_checked_0((i).checked_add(1), "Runtime error: integer overflow in addition"); let __sn_right = (values).len() as i64; __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) })]) {
        { let __sn_place = &mut (advanced); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
            let mut localOld: char = valueNext(expected);
            if ((localOld == (values)[__sn_index((values).len(), i)]) && (expected == (values)[__sn_index((values).len(), i)])) {
        { let __sn_place = &mut (isolated); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
            let mut previous: char = referencePrevious(&mut (value));
            if ((previous == (values)[__sn_index((values).len(), { let __sn_left = __sn_checked_0((i).checked_add(1), "Runtime error: integer overflow in addition"); let __sn_right = (values).len() as i64; __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) })]) && (value == expected)) {
        { let __sn_place = &mut (restored); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
            { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", original)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", advanced)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", isolated)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x20]))); __sn_interpolated.push_str(&format!("{}", restored)); __sn_interpolated }));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
