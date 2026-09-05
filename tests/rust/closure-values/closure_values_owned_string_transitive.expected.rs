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


struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
fn make(seed: SnString) -> __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(SnString) -> SnString>> {
    let value: std::rc::Rc<std::cell::RefCell<SnString>> = std::rc::Rc::new(std::cell::RefCell::new(seed));
    return { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> __SnClosure<dyn Fn(SnString) -> SnString>>(std::rc::Rc::new(move || -> __SnClosure<dyn Fn(SnString) -> SnString> { { let (__sn_string_part, __sn_cell) = ((SnString::from("-middle")).clone(), &value); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn(SnString) -> SnString>(std::rc::Rc::new(move |suffix: SnString| -> SnString { { let (__sn_string_part, __sn_cell) = ((suffix).clone(), &value); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.borrow().clone();})) }
 ;})) }
;
}

fn makeDirect(seed: SnString) -> __SnClosure<dyn Fn(SnString) -> SnString> {
    let seed: std::rc::Rc<std::cell::RefCell<SnString>> = std::rc::Rc::new(std::cell::RefCell::new(seed));
    return { let (seed, ) = (seed.clone(), ); self::__SnClosure::<dyn Fn(SnString) -> SnString>(std::rc::Rc::new(move |suffix: SnString| -> SnString { { let (__sn_string_part, __sn_cell) = ((suffix).clone(), &seed); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return seed.borrow().clone();})) }
;
}

fn main() {
    let mut factory: __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(SnString) -> SnString>> = make(SnString::from("root"))
;
    let mut first: __SnClosure<dyn Fn(SnString) -> SnString> = ((factory.clone()).0)();
    let mut second: __SnClosure<dyn Fn(SnString) -> SnString> = ((factory.clone()).0)();
    __sn_println_string(&(((first.clone()).0)(SnString::from("-one"))))
;
    __sn_println_string(&(((first.clone()).0)(SnString::from("-two"))))
;
    __sn_println_string(&(((second.clone()).0)(SnString::from("-three"))))
;
    let mut other: __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(SnString) -> SnString>> = make(SnString::from("other"))
;
    let mut third: __SnClosure<dyn Fn(SnString) -> SnString> = ((other.clone()).0)();
    __sn_println_string(&(((third.clone()).0)(SnString::from("-four"))))
;
    let mut direct: __SnClosure<dyn Fn(SnString) -> SnString> = makeDirect(SnString::from("param"))
;
    __sn_println_string(&(((direct.clone()).0)(SnString::from("-one"))))
;
    __sn_println_string(&(((direct.clone()).0)(SnString::from("-two"))))
;
}
