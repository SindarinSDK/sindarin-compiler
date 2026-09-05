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


fn __sn_string_substring(value: &SnString, start: i64, end: i64) -> SnString {
    let length = value.len() as i64;
    let start = start.max(0);
    let end = end.min(length);
    if start >= end { return SnString::new(); }
    SnString::from_bytes(value.as_bytes()[start as usize..end as usize].to_vec())
}

fn __sn_string_replace(value: &SnString, old: &SnString, new: &SnString) -> SnString {
    if old.is_empty() { return value.clone(); }
    let mut result = SnString::new();
    let mut remaining = value.as_bytes();
    while let Some(index) = __sn_find_bytes(remaining, old.as_bytes()) {
        result.0.extend_from_slice(&remaining[..index]);
        result.push_str(new);
        remaining = &remaining[index + old.len()..];
    }
    result.0.extend_from_slice(remaining);
    result
}

fn __sn_string_char_at(value: &SnString, index: i64) -> char {
    if index < 0 { return '\0'; }
    value.as_bytes().get(index as usize).copied().map(char::from).unwrap_or('\0')
}

fn __sn_string_index_of(value: &SnString, needle: &SnString) -> i64 {
    __sn_find_bytes(value.as_bytes(), needle.as_bytes())
        .map(|index| index as i64).unwrap_or(-1)
}


fn decorate(value: SnString) -> SnString {
    return { let mut __sn_string = SnString::new(); __sn_string.push_str(&(value)); __sn_string.push_str(&(SnString::from("!"))); __sn_string };
}

fn main() {
    let mut source: SnString = SnString::from("  Hello World  ");
    let mut assigned: SnString = source.clone();
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("assignedCopy="); __sn_interpolated.push_str(&(assigned)); __sn_interpolated }
))
;
    (assigned = SnString::from("changed"));
    let mut explicit_copy: SnString = (source).clone();
    { let (__sn_string_part, __sn_string_place) = ((SnString::from(" copy")).clone(), &mut (explicit_copy)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
    let mut decorated: SnString = decorate(source.clone())
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("source="); __sn_interpolated.push_str(&(source)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("assigned="); __sn_interpolated.push_str(&(assigned)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("copy="); __sn_interpolated.push_str(&(explicit_copy)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("decorated="); __sn_interpolated.push_str(&(decorated)); __sn_interpolated }
))
;
    let mut hello: SnString = (source).trim_ascii()

;
    let mut joined: SnString = { let mut __sn_string = SnString::new(); __sn_string.push_str(&(hello)); __sn_string.push_str(&(SnString::from(" from"))); __sn_string.push_str(&(SnString::from(" Rust"))); __sn_string }
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("joined="); __sn_interpolated.push_str(&(joined)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("length="); __sn_interpolated.push_str(&format!("{}", (hello).len() as i64)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", (hello).len() as i64)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("equal="); __sn_interpolated.push_str(&format!("{}", (hello == SnString::from("Hello World")))); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", (hello != source))); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("contains="); __sn_interpolated.push_str(&format!("{}", (hello).contains(&(SnString::from("lo Wo")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("starts="); __sn_interpolated.push_str(&format!("{}", (hello).starts_with(&(SnString::from("Hell")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("ends="); __sn_interpolated.push_str(&format!("{}", (hello).ends_with(&(SnString::from("World")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("upper="); __sn_interpolated.push_str(&((hello).to_ascii_uppercase()

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("lower="); __sn_interpolated.push_str(&((hello).to_ascii_lowercase()

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("substring="); __sn_interpolated.push_str(&(__sn_string_substring(&(hello), 6, 11)

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("replace="); __sn_interpolated.push_str(&(__sn_string_replace(&(hello), &(SnString::from("World")), &(SnString::from("Rust")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("emptyReplace="); __sn_interpolated.push_str(&(__sn_string_replace(&(hello), &(SnString::from("")), &(SnString::from("ignored")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("char="); __sn_interpolated.push_char(__sn_string_char_at(&(hello), 1)

); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("index="); __sn_interpolated.push_str(&format!("{}", __sn_string_index_of(&(hello), &(SnString::from("World")))

)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", __sn_string_index_of(&(hello), &(SnString::from("missing")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("chain="); __sn_interpolated.push_str(&(__sn_string_replace(&(((source).trim_ascii()

).to_ascii_lowercase()

), &(SnString::from("world")), &(SnString::from("rust")))

)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("sourceAgain="); __sn_interpolated.push_str(&(source)); __sn_interpolated }
))
;
}
