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

fn observeInt(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn chooseDouble(subject_calls: &mut i64, body_calls: &mut i64, order: &mut i64) -> f64 {
    return {
     let __sn_match_subject_0: f64 = observeDouble(&mut *(subject_calls), &mut *(order), 7, 9.5);
     if (__sn_match_subject_0 == 9.5) {
         (observeDouble(&mut *(body_calls), &mut *(order), 8, 42.25) as f64)
     }
     else {
         (observeDouble(&mut *(body_calls), &mut *(order), 9, 0.0) as f64)
     }
 };
}

fn main() {
    let mut __sn_match_subject: i64 = 41;
    let mut __sn_match_result: i64 = 42;
    let mut subject_calls: i64 = 0;
    let mut body_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = 0;
    {
    let __sn_match_subject_1: f32 = observeFloat(&mut (subject_calls), &mut (order), 1, 2.5);
    if (__sn_match_subject_1 == 1.0 || __sn_match_subject_1 == 2.5 || __sn_match_subject_1 == (-2.5)) {
        (selected = 10);
        observeInt(&mut (body_calls), &mut (order), 2, 0);
    }
    else if (__sn_match_subject_1 == 2.5) {
        (selected = 20);
        observeInt(&mut (body_calls), &mut (order), 8, 0);
    }
    else {
        (selected = 30);
        observeInt(&mut (body_calls), &mut (order), 9, 0);
    }
};
    {
    let __sn_match_subject_2: f64 = observeDouble(&mut (subject_calls), &mut (order), 3, (-4.5));
    if (__sn_match_subject_2 == 4.5 || __sn_match_subject_2 == (-4.5) || __sn_match_subject_2 == (-1.0)) {
        { let __sn_rhs = 20; let __sn_place = &mut (selected); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        observeInt(&mut (body_calls), &mut (order), 4, 0);
    }
    else if (__sn_match_subject_2 == (-4.5)) {
        (selected = 200);
        observeInt(&mut (body_calls), &mut (order), 8, 0);
    }
    else {
        (selected = 300);
        observeInt(&mut (body_calls), &mut (order), 9, 0);
    }
};
    let mut nested_statement: i64 = 0;
    {
    let __sn_match_subject_4: f32 = 1.0;
    if (__sn_match_subject_4 == 1.0) {
        {
    let __sn_match_subject_3: f64 = 2.0;
    if (__sn_match_subject_3 == 2.0) {
        (nested_statement = 12);
    }
};
    }
};
    let mut nan_statement_hits: i64 = 0;
    {
    let __sn_match_subject_5: f32 = (0.0 / 0.0);
    if (__sn_match_subject_5 == 0.0) {
        { let __sn_place = &mut (nan_statement_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    let mut zero_hits: i64 = 0;
    {
    let __sn_match_subject_6: f32 = (-0.0);
    if (__sn_match_subject_6 == 0.0) {
        { let __sn_place = &mut (zero_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    {
    let __sn_match_subject_7: f64 = 0.0;
    if (__sn_match_subject_7 == (-0.0)) {
        { let __sn_place = &mut (zero_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    let mut first_value: i64 = {
    let __sn_match_subject_8: f32 = observeFloat(&mut (subject_calls), &mut (order), 5, (-2.5));
    if (__sn_match_subject_8 == (-2.5) || __sn_match_subject_8 == (-1.0)) {
        (observeInt(&mut (body_calls), &mut (order), 6, 100) as i64)
    }
    else if (__sn_match_subject_8 == (-2.5)) {
        (observeInt(&mut (body_calls), &mut (order), 8, 200) as i64)
    }
    else {
        (observeInt(&mut (body_calls), &mut (order), 9, 300) as i64)
    }
};
    let mut nan_value: bool = {
    let __sn_match_subject_9: f64 = (0.0 / 0.0);
    if (__sn_match_subject_9 == 0.0) {
        (false)
    }
    else {
        (true)
    }
};
    let mut float_value: f32 = {
    let __sn_match_subject_10: f32 = 7.25;
    if (__sn_match_subject_10 == 7.25) {
        ((-3.5) as f32)
    }
    else {
        (0.0 as f32)
    }
};
    let mut double_value: f64 = {
    let __sn_match_subject_11: f64 = (-6.5);
    if (__sn_match_subject_11 == (-6.5) || __sn_match_subject_11 == 1.0) {
        (6.75 as f64)
    }
    else {
        (0.0 as f64)
    }
};
    let mut returned: f64 = chooseDouble(&mut (subject_calls), &mut (body_calls), &mut (order));
    let mut nested_value: i64 = {
    let __sn_match_subject_13: f32 = (-1.0);
    if (__sn_match_subject_13 == (-1.0)) {
        ({
    let __sn_match_subject_12: f64 = 2.0;
    if (__sn_match_subject_12 == 2.0) {
        (77 as i64)
    }
    else {
        (0 as i64)
    }
} as i64)
    }
    else {
        ((-1) as i64)
    }
};
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_subject)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", __sn_match_result)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", body_calls)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", nested_statement)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", nan_statement_hits)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", zero_hits)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", first_value)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", nan_value)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", (float_value == (-3.5)))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", (double_value == 6.75))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", (returned == 42.25))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x2c]))); __sn_interpolated.push_str(&format!("{}", nested_value)); __sn_interpolated }));
}
