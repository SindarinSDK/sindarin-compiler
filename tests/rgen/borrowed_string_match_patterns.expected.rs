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
     if (__sn_match_subject_2 == SnString::from("miss") || __sn_match_subject_2 == (pattern) || __sn_match_subject_2 == (pattern)) {
         (SnString::from("matched"))
     }
     else {
         (SnString::from("other"))
     }
 };
}

fn main() {
    let mut empty: SnString = SnString::from("");
    let mut utf8: SnString = SnString::from("héllo-世界-🙂");
    let mut escaped: SnString = SnString::from("quote:\" slash:\\ line:\n tab:\t");
    let mut leaf: Leaf = Leaf { text: SnString::from("nested") };
    let mut holder: Holder = Holder { direct: SnString::from("direct"), leaf: leaf.clone() };
    let mut statementHits: i64 = 0;
    {
    let __sn_match_subject_3: SnString = SnString::from("nested");
    if (__sn_match_subject_3 == SnString::from("miss") || __sn_match_subject_3 == ((holder).direct)) {
        (statementHits = 10);
    }
    else if (__sn_match_subject_3 == (((holder).leaf).text) || __sn_match_subject_3 == (((holder).leaf).text)) {
        { let __sn_place = &mut (statementHits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (statementHits == 1))
;
    let mut scalar: i64 = {
    let __sn_match_subject_4: SnString = SnString::from("héllo-世界-🙂");
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
    println!("{}", (scalar == 7))
;
    let mut prefix: i64 = 0;
    let mut stringResult: SnString = {
    let __sn_match_subject_5: SnString = escaped.clone();
    if (__sn_match_subject_5 == (escaped)) {
        { let __sn_place = &mut (prefix); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        (SnString::from("ok"))
    }
    else {
        (SnString::from("bad"))
    }
};
    println!("{}", ((stringResult == SnString::from("ok")) && (prefix == 1)))
;
    (utf8 = SnString::from("changed"));
    let mut later: bool = {
    let __sn_match_subject_6: SnString = SnString::from("changed");
    if (__sn_match_subject_6 == (utf8)) {
        (true)
    }
    else {
        (false)
    }
};
    { let (__sn_string_part, __sn_string_place) = ((SnString::from("!")).clone(), &mut (utf8)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
    println!("{}", (later && (utf8 == SnString::from("changed!"))))
;
    let mut nestedResult: bool = {
    let __sn_match_subject_8: SnString = SnString::from("outer");
    if (__sn_match_subject_8 == SnString::from("outer")) {
        ({
    let __sn_match_subject_7: SnString = SnString::from("direct");
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
    println!("{}", nestedResult)
;
    let mut __sn_match_subject_0: SnString = SnString::from("helper");
    let mut __sn_match_array_0: SnString = SnString::from("helper");
    let mut __sn_match_index_0: SnString = SnString::from("helper");
    {
    let __sn_match_subject_9: SnString = SnString::from("helper");
    if (__sn_match_subject_9 == (__sn_match_subject_0) || __sn_match_subject_9 == (__sn_match_array_0) || __sn_match_subject_9 == (__sn_match_index_0)) {
        { let __sn_place = &mut (prefix); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (prefix == 2))
;
    println!("{}", (parameterMatch(SnString::from(""), empty.clone())
 == SnString::from("matched")))
;
    println!("{}", (holder).selfMatches(SnString::from("nested"))
)
;
    (((holder).leaf).text = SnString::from("after"));
    println!("{}", ((((holder).leaf).text == SnString::from("after")) && (empty == SnString::from(""))))
;
}
