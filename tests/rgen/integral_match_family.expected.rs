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

fn observeLong(calls: &mut i64, order: &mut i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(1), "Runtime error: integer overflow in addition"));
    return value;
}

fn observeUint(calls: &mut i64, value: u64) -> u64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn observeFloat(calls: &mut i64, order: &mut i64, marker: i64, value: f32) -> f32 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn observeDouble(calls: &mut i64, order: &mut i64, marker: i64, value: f64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn main() {
    let mut __sn_match_subject: i64 = 41;
    let mut __sn_match_result: i64 = 42;
    let mut subject_calls: i64 = 0;
    let mut result_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = 0;
    match (observeLong(&mut (subject_calls), &mut (order), 2)
 as i64) {
        1 | 2 | 2 | 2 | 2 => {
            (selected = 10);
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(2), "Runtime error: integer overflow in addition"));
        },
        2 => {
            (selected = 20);
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(8), "Runtime error: integer overflow in addition"));
        },
        _ => {
            (selected = 30);
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(9), "Runtime error: integer overflow in addition"));
        },
    };
    let mut int32_selected: i64 = 0;
    let mut int32_value: i32 = (-7);
    match (int32_value as i32) {
        (-6) | (-7) | (-7) => {
            (int32_selected = 1);
        },
        _ => {
            (int32_selected = 2);
        },
    };
    let mut uint32_selected: i64 = 0;
    let mut uint32_value: u32 = 4;
    match (uint32_value as u32) {
        1 | 2 | 3 | 4 | 5 => {
            (uint32_selected = 1);
        },
        _ => {
            (uint32_selected = 2);
        },
    };
    let mut uint_selected: i64 = 0;
    let mut uint_value: u64 = 5;
    match (uint_value as u64) {
        1 | 2 | 3 | 4 | 5 => {
            (uint_selected = 1);
        },
        _ => {
            (uint_selected = 2);
        },
    };
    let mut byte_selected: i64 = 0;
    let mut byte_value: u8 = 255;
    match (byte_value as u8) {
        1 | 255 => {
            (byte_selected = 1);
        },
        _ => {
            (byte_selected = 2);
        },
    };
    let mut no_match: i64 = 7;
    match (9 as u8) {
        8 => {
            (no_match = 99);
        },
        _ => {},
    };
    let mut boundary_hits: i64 = 0;
    match ((-9223372036854775807) as i64) {
        (-9223372036854775807) => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (9223372036854775807 as i64) {
        9223372036854775807 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    let mut int32_min: i32 = (-2147483647);
    { let __sn_rhs = 1; let __sn_place = &mut (int32_min); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    match (int32_min as i32) {
        (-2147483648) => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (2147483647 as i32) {
        2147483647 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (0 as u32) {
        0 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (4294967295 as u32) {
        4294967295 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (0 as u64) {
        0 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (9223372036854775807 as u64) {
        9223372036854775807 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (0 as u8) {
        0 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (255 as u8) {
        255 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    let mut nested_statement: i64 = 0;
    match (1 as u32) {
        1 => {
            match (2 as u8) {
        2 => {
            (nested_statement = 12);
        },
        _ => {},
    };
        },
        _ => {},
    };
    let mut bool_result: bool = match (1 as i64) {
        1 => {
            (true)
        },
        _ => {
            (false)
        },
    };
    let mut int_result: i64 = match ((-2) as i32) {
        (-2) => {
            (20 as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut long_result: i64 = match (4294967295 as u32) {
        4294967295 => {
            ((-30) as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut int32_result: i32 = match (3 as u64) {
        3 => {
            ((-40) as i32)
        },
        _ => {
            (0 as i32)
        },
    };
    let mut uint32_result: u32 = match (4 as u8) {
        4 => {
            (50 as u32)
        },
        _ => {
            (0 as u32)
        },
    };
    let mut uint_result: u64 = match (5 as i64) {
        5 => {
            (60 as u64)
        },
        _ => {
            (0 as u64)
        },
    };
    let mut byte_result: u8 = match (6 as i32) {
        6 => {
            (70 as u8)
        },
        _ => {
            (0 as u8)
        },
    };
    let mut bool_subject_int_result: i64 = match (true) {
        true => {
            (80 as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut int_subject_bool_result: bool = match (1 as i64) {
        1 => {
            (true)
        },
        _ => {
            (false)
        },
    };
    let mut float_result: f32 = match (7 as u32) {
        7 => {
            (observeFloat(&mut (result_calls), &mut (order), 3, 6.25)
 as f32)
        },
        _ => {
            (observeFloat(&mut (result_calls), &mut (order), 8, 0.0)
 as f32)
        },
    };
    let mut double_result: f64 = match (observeUint(&mut (subject_calls), 8)
 as u64) {
        8 => {
            (observeDouble(&mut (result_calls), &mut (order), 4, 7.5)
 as f64)
        },
        _ => {
            (observeDouble(&mut (result_calls), &mut (order), 9, 0.0)
 as f64)
        },
    };
    let mut nested_value: i64 = match (1 as u8) {
        1 => {
            (match (2 as u32) {
        2 => {
            (77 as i64)
        },
        _ => {
            (0 as i64)
        },
    } as i64)
        },
        _ => {
            ((-1) as i64)
        },
    };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_subject)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", result_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int32_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint32_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", byte_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", no_match)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", boundary_hits)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested_statement)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", bool_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", long_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int32_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint32_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", byte_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", bool_subject_int_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int_subject_bool_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (float_result == 6.25))); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (double_result == 7.5))); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested_value)); __sn_interpolated }
))
;
}
