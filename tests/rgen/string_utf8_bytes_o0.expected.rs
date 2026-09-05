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

fn main() {
    let mut direct: SnString = SnString::from("X\u{1f}AY");
    let mut longGreedy: SnString = SnString::from("X\u{1f}Ab09Y");
    let mut lower: SnString = SnString::from("x\u{1f}ay");
    let mut expectedFour: SnString = { let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("X\u{1f}"))); __sn_string.push_str(&(SnString::from("A"))); __sn_string.push_str(&(SnString::from("Y"))); __sn_string }
;
    let mut expectedSeven: SnString = { let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("X\u{1f}"))); __sn_string.push_str(&(SnString::from("A"))); __sn_string.push_str(&(SnString::from("b"))); __sn_string.push_str(&(SnString::from("0"))); __sn_string.push_str(&(SnString::from("9"))); __sn_string.push_str(&(SnString::from("Y"))); __sn_string }
;
    let mut expectedLower: SnString = { let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("x\u{1f}"))); __sn_string.push_str(&(SnString::from("a"))); __sn_string.push_str(&(SnString::from("y"))); __sn_string }
;
    let mut borrowedSource: SnString = SnString::from("X\u{1f}Ab09Y");
    let mut rows: Vec<SnString> = vec![SnString::from("X\u{1f}Ab09Y")];
    let mut borrowed: SnString = match (1 as i64) {
        1 => {
            (borrowedSource.clone())
        },
        _ => {
            (SnString::from("wrong"))
        },
    };
    let mut indexed: SnString = match (2 as i64) {
        2 => {
            ((rows)[__sn_index((rows).len(), 0)].clone())
        },
        _ => {
            (SnString::from("wrong"))
        },
    };
    let mut nested: SnString = match (true) {
        true => {
            ({
    let __sn_match_subject_0: SnString = SnString::from("X\u{1f}AY");
    if (__sn_match_subject_0 == SnString::from("X\u{1f}AY")) {
        (SnString::from("X\u{1f}Ab09Y"))
    }
    else {
        (SnString::from("wrong-inner"))
    }
})
        },
        _ => {
            (SnString::from("wrong-outer"))
        },
    };
    let mut result: SnString = match (false) {
        true => {
            (SnString::from("wrong"))
        },
        _ => {
            (SnString::from("X\u{1f}AY"))
        },
    };
    let mut concatenated: SnString = { let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("X\u{1f}"))); __sn_string.push_str(&(SnString::from("Ab09Y"))); __sn_string };
    let mut interpolated: SnString = { let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("X\u{1f}Ab09Y"); __sn_interpolated }
;
    let mut boundaries: SnString = SnString::from("߿ࠀ퟿￿𐀀􏿿");
    let mut controls: SnString = SnString::from("\n\t\r\"\\");
    let mut unicode: SnString = SnString::from("é世界🙂");
    println!("{}", (direct == expectedFour))
;
    println!("{}", (longGreedy == expectedSeven))
;
    println!("{}", (lower == expectedLower))
;
    println!("{}", (((borrowed == expectedSeven) && (indexed == expectedSeven)) && (nested == expectedSeven)))
;
    println!("{}", (result == expectedFour))
;
    println!("{}", ((concatenated == expectedSeven) && (interpolated == expectedSeven)))
;
    println!("{}", (((boundaries).len() as i64 == 24) && ((controls).len() as i64 == 5)))
;
    println!("{}", ((unicode == SnString::from("é世界🙂")) && ((unicode).len() as i64 == 12)))
;
    __sn_println_string(&(direct))
;
    __sn_println_string(&(longGreedy))
;
    __sn_println_string(&(lower))
;
    __sn_println_string(&(unicode))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", (boundaries).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (controls).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (unicode).len() as i64)); __sn_interpolated }
))
;
}
