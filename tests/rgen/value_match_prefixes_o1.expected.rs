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
struct Prefixes {
    marker: i64,
}

impl Prefixes {
    fn staticMark(calls: &mut i64, order: &mut i64, marker: i64) -> i64 {
        return markInt(&mut *(calls), &mut *(order), marker, 0);
    }
    fn instanceMark(&self, calls: &mut i64, order: &mut i64, marker: i64) -> i64 {
        return markInt(&mut *(calls), &mut *(order), marker, (self).marker);
    }
    fn chooseBool(value: bool, calls: &mut i64, order: &mut i64) -> bool {
        return match (value) {
         true => {
             { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
             (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(1), "Runtime error: integer overflow in addition"));
             markInt(&mut *(calls), &mut *(order), 2, 0);
             (markBool(&mut *(calls), &mut *(order), 3, true))
         },
         false => {
             markInt(&mut *(calls), &mut *(order), 4, 0);
             (false)
         },
         _ => {
             markInt(&mut *(calls), &mut *(order), 5, 0);
             (false)
         },
     };
    }
    fn chooseDouble(&self, value: f64, calls: &mut i64, order: &mut i64) -> f64 {
        return {
     let __sn_match_subject_0: f64 = value;
     if (__sn_match_subject_0 == 1.5) {
         markInt(&mut *(calls), &mut *(order), 4, 0);
         (markDouble(&mut *(calls), &mut *(order), 5, 2.5) as f64)
     }
     else {
         markInt(&mut *(calls), &mut *(order), 6, 0);
         (3.5 as f64)
     }
 };
    }
}

fn markInt(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn markBool(calls: &mut i64, order: &mut i64, marker: i64, value: bool) -> bool {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn markDouble(calls: &mut i64, order: &mut i64, marker: i64, value: f64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn markString(calls: &mut i64, order: &mut i64, marker: i64, value: SnString) -> SnString {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn acceptInt(value: i64) -> i64 {
    return value;
}

fn chooseInt(value: i64, calls: &mut i64, order: &mut i64) -> i64 {
    let mut prefixes: Prefixes = Prefixes { marker: 0 };
    return match (markInt(&mut *(calls), &mut *(order), 1, value) as i64) {
         1 => {
             markInt(&mut *(calls), &mut *(order), 7, 0);
             (10 as i64)
         },
         2 => {
             markInt(&mut *(calls), &mut *(order), 2, 0);
             Prefixes::staticMark(&mut *(calls), &mut *(order), 3);
             (prefixes).instanceMark(&mut *(calls), &mut *(order), 4);
             match (true) {
         true => {
             markInt(&mut *(calls), &mut *(order), 5, 0);
         },
         _ => {
             markInt(&mut *(calls), &mut *(order), 9, 0);
         },
     };
             (markInt(&mut *(calls), &mut *(order), 6, 20) as i64)
         },
         2 => {
             markInt(&mut *(calls), &mut *(order), 7, 0);
             (markInt(&mut *(calls), &mut *(order), 8, 30) as i64)
         },
         _ => {
             markInt(&mut *(calls), &mut *(order), 9, 0);
             (40 as i64)
         },
     };
}

fn scalarFamilies(calls: &mut i64) -> bool {
    let mut order: i64 = 0;
    let mut longResult: i64 = match (1 as u8) {
        1 => {
            markInt(&mut *(calls), &mut (order), 1, 0);
            (2 as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut int32Result: i32 = match (2 as u64) {
        2 => {
            markInt(&mut *(calls), &mut (order), 2, 0);
            (3 as i32)
        },
        _ => {
            (0 as i32)
        },
    };
    let mut uint32Result: u32 = match (3 as i32) {
        3 => {
            markInt(&mut *(calls), &mut (order), 3, 0);
            (4 as u32)
        },
        _ => {
            (0 as u32)
        },
    };
    let mut uintResult: u64 = match (4 as u32) {
        4 => {
            markInt(&mut *(calls), &mut (order), 4, 0);
            (5 as u64)
        },
        _ => {
            (0 as u64)
        },
    };
    let mut byteResult: u8 = match (5 as i64) {
        5 => {
            markInt(&mut *(calls), &mut (order), 5, 0);
            (6 as u8)
        },
        _ => {
            (0 as u8)
        },
    };
    let mut floatResult: f32 = {
    let __sn_match_subject_1: f32 = 6.0;
    if (__sn_match_subject_1 == 6.0) {
        markInt(&mut *(calls), &mut (order), 6, 0);
        (7.0 as f32)
    }
    else {
        (0.0 as f32)
    }
};
    return (((((((longResult == 2) && (int32Result == 3)) && (uint32Result == 4)) && (uintResult == 5)) && (byteResult == 6)) && (floatResult == 7.0)) && (order == 123456));
}

fn main() {
    let mut calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = chooseInt(2, &mut (calls), &mut (order));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated }));
    (calls = 0);
    (order = 0);
    let mut boolResult: bool = Prefixes::chooseBool(true, &mut (calls), &mut (order));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", boolResult)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated }));
    (calls = 0);
    (order = 0);
    let mut prefixes: Prefixes = Prefixes { marker: 0 };
    let mut doubleResult: f64 = (prefixes).chooseDouble(1.5, &mut (calls), &mut (order));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{:.5}", doubleResult)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated }));
    (calls = 0);
    (order = 0);
    let mut borrowed: SnString = SnString::from_slice(&[0x62, 0x6f, 0x72, 0x72, 0x6f, 0x77, 0x65, 0x64]);
    let mut borrowedResult: SnString = {
    let __sn_match_subject_2: SnString = markString(&mut (calls), &mut (order), 1, SnString::from_slice(&[0x6b, 0x65, 0x79]));
    if (__sn_match_subject_2 == SnString::from_slice(&[0x6b, 0x65, 0x79])) {
        markString(&mut (calls), &mut (order), 2, SnString::from_slice(&[0x64, 0x69, 0x73, 0x63, 0x61, 0x72, 0x64, 0x2d, 0x6f, 0x77, 0x6e, 0x65, 0x64]));
        match (1 as i64) {
        1 => {
            markInt(&mut (calls), &mut (order), 3, 0);
        },
        _ => {
            markInt(&mut (calls), &mut (order), 9, 0);
        },
    };
        (borrowed.clone())
    }
    else if (__sn_match_subject_2 == SnString::from_slice(&[0x6b, 0x65, 0x79])) {
        markString(&mut (calls), &mut (order), 8, SnString::from_slice(&[0x64, 0x75, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x65]));
        (SnString::from_slice(&[0x77, 0x72, 0x6f, 0x6e, 0x67]))
    }
    else {
        markString(&mut (calls), &mut (order), 9, SnString::from_slice(&[0x65, 0x6c, 0x73, 0x65]));
        (SnString::from_slice(&[0x77, 0x72, 0x6f, 0x6e, 0x67]))
    }
};
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(borrowedResult)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&(borrowed)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated }));
    (calls = 0);
    (order = 0);
    let mut ownedResult: SnString = {
    let __sn_match_subject_3: SnString = SnString::from_slice(&[0x6f, 0x77, 0x6e, 0x65, 0x64]);
    if (__sn_match_subject_3 == SnString::from_slice(&[0x6f, 0x77, 0x6e, 0x65, 0x64])) {
        markString(&mut (calls), &mut (order), 5, SnString::from_slice(&[0x64, 0x69, 0x73, 0x63, 0x61, 0x72, 0x64, 0x2d, 0x6f, 0x77, 0x6e, 0x65, 0x64]));
        (markString(&mut (calls), &mut (order), 6, SnString::from_slice(&[0x6f, 0x77, 0x6e, 0x65, 0x64, 0x2d, 0x66, 0x69, 0x6e, 0x61, 0x6c])))
    }
    else {
        (borrowed.clone())
    }
};
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(ownedResult)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&(borrowed)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated }));
    (calls = 0);
    (order = 0);
    let mut argumentResult: i64 = acceptInt(70);
    let mut nestedResult: i64 = match (false) {
        true => {
            (0 as i64)
        },
        _ => {
            markInt(&mut (calls), &mut (order), 8, 0);
            (match (1 as i64) {
        1 => {
            markInt(&mut (calls), &mut (order), 9, 0);
            (90 as i64)
        },
        _ => {
            (0 as i64)
        },
    } as i64)
        },
    };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", argumentResult)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", nestedResult)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated }));
    (calls = 0);
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", scalarFamilies(&mut (calls)))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x7c]))); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated }));
}
