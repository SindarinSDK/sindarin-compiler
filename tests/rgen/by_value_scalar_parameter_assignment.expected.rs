#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
    use std::io::Write;
    std::io::stdout().lock().write_all(bytes).expect("failed to write stdout");
}

fn __sn_print_string(value: &SnString) { __sn_write_bytes(value.as_bytes()); }

fn __sn_println_string(value: &SnString) {
    __sn_write_bytes(value.as_bytes());
    __sn_write_bytes(b"\n");
}

fn __sn_print_char(value: char) { __sn_write_bytes(&[value as u32 as u8]); }

fn __sn_println_char(value: char) {
    __sn_print_char(value);
    __sn_write_bytes(b"\n");
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


fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
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
struct ScalarAssignments {
    marker: i64,
}

impl ScalarAssignments {
    fn assignInt32(mut value: i32, untouched: i32) -> i32 {
        { value = __sn_checked_0((value).checked_add(4), "Runtime error: integer overflow in addition"); value };
        return __sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition");
    }
    fn assignByte(mut value: u8, untouched: u8) -> u8 {
        let mut assigned: u8 = { value = __sn_checked_0((value).checked_add(5), "Runtime error: integer overflow in addition"); value };
        return __sn_checked_0((__sn_checked_0((assigned).checked_add(value), "Runtime error: integer overflow in addition")).checked_add(untouched), "Runtime error: integer overflow in addition");
    }
    fn assignUint32(mut value: u32, untouched: u32) -> u32 {
        return { value = __sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition"); value };
    }
    fn assignUint(&self, mut value: u64, untouched: u64) -> u64 {
        { value = __sn_checked_0((value).checked_add(7), "Runtime error: integer overflow in addition"); value };
        return __sn_checked_0((__sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition")).checked_add(((self).marker as u64)
 ), "Runtime error: integer overflow in addition");
    }
    fn assignFloat(&self, mut value: f32, untouched: f32) -> f32 {
        let mut assigned: f32 = { value = (value + 1.5); value };
        return ((assigned + value) + untouched);
    }
    fn assignDouble(&self, mut value: f64, untouched: f64) -> f64 {
        return { value = (value + untouched); value };
    }
}

fn observeInt(calls: &mut i64, value: i64) -> i64 {
    (*(calls) = __sn_checked_0((*(calls)).checked_add(1), "Runtime error: integer overflow in addition"));
    return value;
}

fn assignBool(mut value: bool, untouched: bool) -> bool {
    { value = (!value); value };
    return (value && untouched);
}

fn assignInt(mut value: i64, calls: &mut i64, untouched: i64) -> i64 {
    { value = observeInt(&mut *(calls), __sn_checked_0((value).checked_add(2), "Runtime error: integer overflow in addition"))
; value };
    return __sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition");
}

fn assignLong(mut value: i64, untouched: i64) -> i64 {
    let mut assigned: i64 = { value = __sn_checked_0((value).checked_add(3), "Runtime error: integer overflow in addition"); value };
    return __sn_checked_0((__sn_checked_0((assigned).checked_add(value), "Runtime error: integer overflow in addition")).checked_add(untouched), "Runtime error: integer overflow in addition");
}

fn helperNames(mut __sn_rhs: i64, __sn_place: i64, __sn_next: i64) -> i64 {
    let mut assigned: i64 = { __sn_rhs = __sn_checked_0((__sn_rhs).checked_add(__sn_place), "Runtime error: integer overflow in addition"); __sn_rhs };
    if true {
        let mut __sn_next: i64 = 4;
        (__sn_next = 5);
    }
    return __sn_checked_0((__sn_checked_0((assigned).checked_add(__sn_rhs), "Runtime error: integer overflow in addition")).checked_add(__sn_next), "Runtime error: integer overflow in addition");
}

fn statementOrder(mut value: i64, delta: i64) -> i64 {
    if true {
        let mut readBefore: i64 = __sn_checked_0((value).checked_add(delta), "Runtime error: integer overflow in addition");
        { value = __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition"); value };
        let mut value: i64 = readBefore;
        (value = __sn_checked_0((value).checked_add(10), "Runtime error: integer overflow in addition"));
    }
    return value;
}

fn main() {
    let mut boolCaller: bool = false;
    let mut intCaller: i64 = 10;
    let mut longCaller: i64 = 20;
    let mut int32Caller: i32 = 30;
    let mut byteCaller: u8 = 40;
    let mut uint32Caller: u32 = 50;
    let mut uintCaller: u64 = 60;
    let mut floatCaller: f32 = 2.0;
    let mut doubleCaller: f64 = 3.0;
    let mut calls: i64 = 0;
    let mut ops: ScalarAssignments = ScalarAssignments { marker: 1 };
    let mut boolResult: bool = assignBool(boolCaller, true)
;
    let mut intResult: i64 = assignInt(intCaller, &mut (calls), 1)
;
    let mut longResult: i64 = assignLong(longCaller, 1)
;
    let mut int32Result: i32 = ScalarAssignments::assignInt32(int32Caller, 1);
    let mut byteResult: u8 = ScalarAssignments::assignByte(byteCaller, 1);
    let mut uint32Result: u32 = ScalarAssignments::assignUint32(uint32Caller, 2);
    let mut uintResult: u64 = (ops).assignUint(uintCaller, 2)
;
    let mut floatResult: f32 = (ops).assignFloat(floatCaller, 0.5)
;
    let mut doubleResult: f64 = (ops).assignDouble(doubleCaller, 0.25)
;
    let mut helperResult: i64 = helperNames(1, 2, 3)
;
    let mut orderCaller: i64 = 4;
    let mut orderResult: i64 = statementOrder(orderCaller, 2)
;
    println!("{}", (boolResult && (!boolCaller)))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", intResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", intCaller)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", longResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", longCaller)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", int32Result)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", int32Caller)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", byteResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", byteCaller)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", uint32Result)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", uint32Caller)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", uintResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", uintCaller)); __sn_interpolated }
))
;
    println!("{}", ((floatResult == 7.5) && (floatCaller == 2.0)))
;
    println!("{}", ((doubleResult == 3.25) && (doubleCaller == 3.0)))
;
    println!("{}", helperResult)
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", orderResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", orderCaller)); __sn_interpolated }
))
;
}
