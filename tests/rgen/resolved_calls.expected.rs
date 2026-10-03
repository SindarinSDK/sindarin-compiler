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
struct Point {
    value: i64,
}

impl Point {
    fn op_eq(&self, other: Point) -> bool {
        return ((self).value == (other).value);
    }
    fn op_lt(&self, other: Point) -> bool {
        return ((self).value < (other).value);
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Explicit {
    value: i64,
}

impl Explicit {
    fn op_ne(&self, other: Explicit) -> bool {
        return ((self).value != (other).value);
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Holder {
    point: Point,
}
#[derive(Clone, Debug, PartialEq)]
struct OwnedPoint {
    label: SnString,
    values: Vec<i64>,
}

impl OwnedPoint {
    fn op_eq(&self, other: &mut OwnedPoint) -> bool {
        return (((self).label.clone() == (other).label.clone()) && (((self).values.clone()).len() as i64 == ((other).values.clone()).len() as i64));
    }
    fn op_lt(&self, other: &mut OwnedPoint) -> bool {
        return (((self).values.clone())[__sn_index(((self).values.clone()).len(), 0)] < ((other).values.clone())[__sn_index(((other).values.clone()).len(), 0)]);
    }
}
#[derive(Clone, Debug, PartialEq)]
struct CallValues {
    label: SnString,
}

impl CallValues {
    fn labelMatches(value: SnString) -> bool {
        return (value == SnString::from_slice(&[0x73, 0x6f, 0x75, 0x72, 0x63, 0x65]));
    }
    fn makeLabel() -> SnString {
        return { let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x73, 0x6f, 0x75, 0x72, 0x63, 0x65]))); __sn_interpolated };
    }
    fn makeNumbers() -> Vec<i64> {
        return vec![1, 2];
    }
    fn joinLabel(&self, value: SnString) -> SnString {
        return { let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((self).label.clone())); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&(value)); __sn_interpolated };
    }
    fn makeNumbersAgain(&self) -> Vec<i64> {
        return vec![3, 4];
    }
}

fn markedPoint(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> Point {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return Point { value: value };
}

fn markedOwnedPoint(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> OwnedPoint {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return OwnedPoint { label: { let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x76, 0x61, 0x6c, 0x75, 0x65, 0x2d]))); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }, values: vec![value] };
}

fn markedPoints(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> Vec<Point> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    let mut points: Vec<Point> = vec![];
    (points).push(Point { value: value });
    return points;
}

fn markedIndex(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn acceptBool(value: bool) -> bool {
    return value;
}

fn returnedComparison(left: Point, right: Point) -> bool {
    return (left).op_eq(right.clone());
}

fn main() {
    let mut left: Point = Point { value: 7 };
    let mut same: Point = Point { value: 7 };
    let mut greater: Point = Point { value: 9 };
    let mut holder: Holder = Holder { point: Point { value: 7 } };
    let mut points: Vec<Point> = vec![];
    (points).push(left);
    (points).push(greater);
    (left).op_eq(same.clone());
    let mut initialized: bool = (left).op_eq(same.clone());
    let mut argument: bool = acceptBool((!(left).op_eq(greater.clone())));
    let mut returned: bool = returnedComparison(left, same);
    let mut memberReceiver: bool = ((holder).point).op_eq(same.clone());
    let mut indexedReceiver: bool = ((points)[__sn_index((points).len(), 0)]).op_lt((points)[__sn_index((points).len(), 1)].clone());
    let mut explicitNe: bool = (Explicit { value: 1 }).op_ne(Explicit { value: 2 });
    let mut derivedGe: bool = (!(greater).op_lt(left.clone()));
    let mut derivedLe: bool = (!(greater).op_lt(left.clone()));
    let mut matched: bool = match (true) {
        true => {
            (!(left).op_eq(greater.clone()));
            ((left).op_eq(same.clone()))
        },
        _ => {
            (false)
        },
    };
    let mut calls: i64 = 0;
    let mut order: i64 = 0;
    let mut __sn_resolved_arg_0: i64 = 40;
    let mut __sn_resolved_receiver_0: i64 = 2;
    let mut directOrder: bool = (markedPoint(&mut (calls), &mut (order), 1, 1)).op_lt(markedPoint(&mut (calls), &mut (order), 2, 2));
    let mut swappedOrder: bool = (markedPoint(&mut (calls), &mut (order), 4, 4)).op_lt(markedPoint(&mut (calls), &mut (order), 3, 3));
    let mut ownedLeft: OwnedPoint = OwnedPoint { label: SnString::from_slice(&[0x6f, 0x77, 0x6e, 0x65, 0x64]), values: vec![5, 6] };
    let mut ownedSame: OwnedPoint = ownedLeft.clone();
    let mut ownedEqual: bool = (ownedLeft).op_eq(&mut (ownedSame));
    let mut ownedSwapped: bool = (markedOwnedPoint(&mut (calls), &mut (order), 6, 6)).op_lt(&mut (markedOwnedPoint(&mut (calls), &mut (order), 5, 5)));
    let mut nestedReceiver: bool = { let mut __sn_resolved_owner_0: Vec<Point> = markedPoints(&mut (calls), &mut (order), 8, 8);let __sn_resolved_receiver_1 = & ((__sn_resolved_owner_0)[__sn_index((__sn_resolved_owner_0).len(), markedIndex(&mut (calls), &mut (order), 9, 0))]); (__sn_resolved_receiver_1).op_lt(markedPoint(&mut (calls), &mut (order), 7, 7)) };
    let mut negativeNestedReceiver: bool = { let mut __sn_resolved_owner_2: Vec<Point> = markedPoints(&mut (calls), &mut (order), 2, 10);let __sn_resolved_receiver_3 = & ((__sn_resolved_owner_2)[__sn_index((__sn_resolved_owner_2).len(), markedIndex(&mut (calls), &mut (order), 3, (-1)))]); (__sn_resolved_receiver_3).op_lt(markedPoint(&mut (calls), &mut (order), 1, 9)) };
    ((ownedSame).values).push(7);
    let mut sourceLabel: SnString = SnString::from_slice(&[0x73, 0x6f, 0x75, 0x72, 0x63, 0x65]);
    let mut callValues: CallValues = CallValues { label: SnString::from_slice(&[0x70, 0x72, 0x65, 0x66, 0x69, 0x78]) };
    let mut staticMatch: bool = CallValues::labelMatches(sourceLabel.clone());
    let mut staticLabel: SnString = CallValues::makeLabel();
    let mut instanceLabel: SnString = (callValues).joinLabel(sourceLabel.clone());
    let mut staticNumbers: Vec<i64> = CallValues::makeNumbers();
    let mut instanceNumbers: Vec<i64> = (callValues).makeNumbersAgain();
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", initialized)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", argument)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", returned)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", memberReceiver)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", indexedReceiver)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", explicitNe)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", derivedGe)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", derivedLe)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", matched)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", directOrder)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", swappedOrder)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", ownedEqual)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", ownedSwapped)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", nestedReceiver)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", negativeNestedReceiver)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", __sn_checked_0((__sn_resolved_arg_0).checked_add(__sn_resolved_receiver_0), "Runtime error: integer overflow in addition"))); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((ownedLeft).label)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", ((ownedLeft).values).len() as i64)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", ((ownedSame).values).len() as i64)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", staticMatch)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&(staticLabel)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&(instanceLabel)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&(sourceLabel)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", (staticNumbers).len() as i64)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", (instanceNumbers).len() as i64)); __sn_interpolated }));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
