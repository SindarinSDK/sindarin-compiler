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

#[derive(Clone, Debug, PartialEq)]
struct Leaf {
    text: SnString,
}
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    direct: SnString,
    leaf: Leaf,
}

impl Holder {
    fn selfMatches(&self, subject: SnString) -> bool {
        return {
     let __sn_match_subject_1: SnString = subject.clone();
     if (__sn_match_subject_1 == (((self).leaf).text)) {
         (true)
     }
     else {
         (false)
     }
 };
    }
}

fn parameterMatch(subject: SnString, pattern: SnString) -> SnString {
    return {
     let __sn_match_subject_2: SnString = subject.clone();
     if (__sn_match_subject_2 == SnString::from_slice(&[0x6d, 0x69, 0x73, 0x73]) || __sn_match_subject_2 == (pattern) || __sn_match_subject_2 == (pattern)) {
         (SnString::from_slice(&[0x6d, 0x61, 0x74, 0x63, 0x68, 0x65, 0x64]))
     }
     else {
         (SnString::from_slice(&[0x6f, 0x74, 0x68, 0x65, 0x72]))
     }
 };
}

fn main() {
    let mut empty: SnString = SnString::from_slice(&[]);
    let mut utf8: SnString = SnString::from_slice(&[0x68, 0xc3, 0xa9, 0x6c, 0x6c, 0x6f, 0x2d, 0xe4, 0xb8, 0x96, 0xe7, 0x95, 0x8c, 0x2d, 0xf0, 0x9f, 0x99, 0x82]);
    let mut escaped: SnString = SnString::from_slice(&[0x71, 0x75, 0x6f, 0x74, 0x65, 0x3a, 0x22, 0x20, 0x73, 0x6c, 0x61, 0x73, 0x68, 0x3a, 0x5c, 0x20, 0x6c, 0x69, 0x6e, 0x65, 0x3a, 0x0a, 0x20, 0x74, 0x61, 0x62, 0x3a, 0x09]);
    let mut leaf: Leaf = Leaf { text: SnString::from_slice(&[0x6e, 0x65, 0x73, 0x74, 0x65, 0x64]) };
    let mut holder: Holder = Holder { direct: SnString::from_slice(&[0x64, 0x69, 0x72, 0x65, 0x63, 0x74]), leaf: leaf.clone() };
    let mut statementHits: i64 = 0;
    {
    let __sn_match_subject_3: SnString = SnString::from_slice(&[0x6e, 0x65, 0x73, 0x74, 0x65, 0x64]);
    if (__sn_match_subject_3 == SnString::from_slice(&[0x6d, 0x69, 0x73, 0x73]) || __sn_match_subject_3 == ((holder).direct)) {
        (statementHits = 10);
    }
    else if (__sn_match_subject_3 == (((holder).leaf).text) || __sn_match_subject_3 == (((holder).leaf).text)) {
        { let __sn_place = &mut (statementHits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (statementHits == 1));
    let mut scalar: i64 = {
    let __sn_match_subject_4: SnString = SnString::from_slice(&[0x68, 0xc3, 0xa9, 0x6c, 0x6c, 0x6f, 0x2d, 0xe4, 0xb8, 0x96, 0xe7, 0x95, 0x8c, 0x2d, 0xf0, 0x9f, 0x99, 0x82]);
    if (__sn_match_subject_4 == (empty)) {
        (0 as i64)
    }
    else if (__sn_match_subject_4 == (utf8) || __sn_match_subject_4 == (utf8)) {
        (7 as i64)
    }
    else {
        ((-1) as i64)
    }
};
    println!("{}", (scalar == 7));
    let mut prefix: i64 = 0;
    let mut stringResult: SnString = {
    let __sn_match_subject_5: SnString = escaped.clone();
    if (__sn_match_subject_5 == (escaped)) {
        { let __sn_place = &mut (prefix); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        (SnString::from_slice(&[0x6f, 0x6b]))
    }
    else {
        (SnString::from_slice(&[0x62, 0x61, 0x64]))
    }
};
    println!("{}", ((stringResult == SnString::from_slice(&[0x6f, 0x6b])) && (prefix == 1)));
    (utf8 = SnString::from_slice(&[0x63, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64]));
    let mut later: bool = {
    let __sn_match_subject_6: SnString = SnString::from_slice(&[0x63, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64]);
    if (__sn_match_subject_6 == (utf8)) {
        (true)
    }
    else {
        (false)
    }
};
    { let (__sn_string_part, __sn_string_place) = ((SnString::from_slice(&[0x21])).clone(), &mut (utf8)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
    println!("{}", (later && (utf8 == SnString::from_slice(&[0x63, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64, 0x21]))));
    let mut nestedResult: bool = {
    let __sn_match_subject_8: SnString = SnString::from_slice(&[0x6f, 0x75, 0x74, 0x65, 0x72]);
    if (__sn_match_subject_8 == SnString::from_slice(&[0x6f, 0x75, 0x74, 0x65, 0x72])) {
        ({
    let __sn_match_subject_7: SnString = SnString::from_slice(&[0x64, 0x69, 0x72, 0x65, 0x63, 0x74]);
    if (__sn_match_subject_7 == ((holder).direct)) {
        (true)
    }
    else {
        (false)
    }
})
    }
    else {
        (false)
    }
};
    println!("{}", nestedResult);
    let mut __sn_match_subject_0: SnString = SnString::from_slice(&[0x68, 0x65, 0x6c, 0x70, 0x65, 0x72]);
    let mut __sn_match_array_0: SnString = SnString::from_slice(&[0x68, 0x65, 0x6c, 0x70, 0x65, 0x72]);
    let mut __sn_match_index_0: SnString = SnString::from_slice(&[0x68, 0x65, 0x6c, 0x70, 0x65, 0x72]);
    {
    let __sn_match_subject_9: SnString = SnString::from_slice(&[0x68, 0x65, 0x6c, 0x70, 0x65, 0x72]);
    if (__sn_match_subject_9 == (__sn_match_subject_0) || __sn_match_subject_9 == (__sn_match_array_0) || __sn_match_subject_9 == (__sn_match_index_0)) {
        { let __sn_place = &mut (prefix); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (prefix == 2));
    println!("{}", (parameterMatch(SnString::from_slice(&[]), empty.clone()) == SnString::from_slice(&[0x6d, 0x61, 0x74, 0x63, 0x68, 0x65, 0x64])));
    println!("{}", (holder).selfMatches(SnString::from_slice(&[0x6e, 0x65, 0x73, 0x74, 0x65, 0x64])));
    (((holder).leaf).text = SnString::from_slice(&[0x61, 0x66, 0x74, 0x65, 0x72]));
    println!("{}", ((((holder).leaf).text == SnString::from_slice(&[0x61, 0x66, 0x74, 0x65, 0x72])) && (empty == SnString::from_slice(&[]))));
}
