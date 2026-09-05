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
fn main() {
    let mut i: i64 = 10;
    let mut l: i64 = 11;
    let mut i32: i32 = 12;
    let mut b: u8 = 13;
    let mut u32: u32 = 14;
    let mut u: u64 = 15;
    let mut f: f32 = 1.5;
    let mut d: f64 = 2.5;
    let mut yes: bool = true;
    let mut letter: char = '\u{51}';
    let mut fi: __SnClosure<dyn Fn(i64) -> i64> = { let (i, ) = (i.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((i.clone()).checked_add(x), "Runtime error: integer overflow in addition")})) }
;
    let mut fl: __SnClosure<dyn Fn(i64) -> i64> = { let (l, ) = (l.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked_0((l.clone()).checked_add(x), "Runtime error: integer overflow in addition")})) }
;
    let mut fi32: __SnClosure<dyn Fn(i32) -> i32> = { let (i32, ) = (i32.clone(), ); self::__SnClosure::<dyn Fn(i32) -> i32>(std::rc::Rc::new(move |x: i32| -> i32 { __sn_checked_0((i32.clone()).checked_add(x), "Runtime error: integer overflow in addition")})) }
;
    let mut fb: __SnClosure<dyn Fn(u8) -> u8> = { let (b, ) = (b.clone(), ); self::__SnClosure::<dyn Fn(u8) -> u8>(std::rc::Rc::new(move |x: u8| -> u8 { __sn_checked_0((b.clone()).checked_add(x), "Runtime error: integer overflow in addition")})) }
;
    let mut fu32: __SnClosure<dyn Fn(u32) -> u32> = { let (u32, ) = (u32.clone(), ); self::__SnClosure::<dyn Fn(u32) -> u32>(std::rc::Rc::new(move |x: u32| -> u32 { __sn_checked_0((u32.clone()).checked_add(x), "Runtime error: integer overflow in addition")})) }
;
    let mut fu: __SnClosure<dyn Fn(u64) -> u64> = { let (u, ) = (u.clone(), ); self::__SnClosure::<dyn Fn(u64) -> u64>(std::rc::Rc::new(move |x: u64| -> u64 { __sn_checked_0((u.clone()).checked_add(x), "Runtime error: integer overflow in addition")})) }
;
    let mut ff: __SnClosure<dyn Fn(f32) -> f32> = { let (f, ) = (f.clone(), ); self::__SnClosure::<dyn Fn(f32) -> f32>(std::rc::Rc::new(move |x: f32| -> f32 { (f.clone() + x)})) }
;
    let mut fd: __SnClosure<dyn Fn(f64) -> f64> = { let (d, ) = (d.clone(), ); self::__SnClosure::<dyn Fn(f64) -> f64>(std::rc::Rc::new(move |x: f64| -> f64 { (d.clone() + x)})) }
;
    let mut fy: __SnClosure<dyn Fn(bool) -> bool> = { let (yes, ) = (yes.clone(), ); self::__SnClosure::<dyn Fn(bool) -> bool>(std::rc::Rc::new(move |x: bool| -> bool { (yes.clone() && x)})) }
;
    let mut fc: __SnClosure<dyn Fn(char) -> char> = { let (letter, ) = (letter.clone(), ); self::__SnClosure::<dyn Fn(char) -> char>(std::rc::Rc::new(move |x: char| -> char { letter.clone().clone()})) }
;
    (i = 100);
    (l = 100);
    (i32 = 100);
    (b = 100);
    (u32 = 100);
    (u = 100);
    (f = 100.0);
    (d = 100.0);
    (yes = false);
    (letter = '\u{5a}');
    __sn_print_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", ((fi.clone()).0)(1))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((fl.clone()).0)(1))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((fi32.clone()).0)(1))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((fb.clone()).0)(1))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((fu32.clone()).0)(1))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((fu.clone()).0)(1))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x0a]))); __sn_interpolated }));
    __sn_print_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{:.5}", ((ff.clone()).0)(0.5))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{:.5}", ((fd.clone()).0)(0.5))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((fy.clone()).0)(true))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_char(((fc.clone()).0)('\u{58}')); __sn_interpolated.push_str(&(SnString::from_slice(&[0x0a]))); __sn_interpolated }));
    let mut many: __SnClosure<dyn Fn(i64, i64, i32, u8, u32, u64, f32, f64, bool, char) -> bool> = { self::__SnClosure::<dyn Fn(i64, i64, i32, u8, u32, u64, f32, f64, bool, char) -> bool>(std::rc::Rc::new(move |a: i64, z: i64, c: i32, e: u8, g: u32, h: u64, j: f32, k: f64, m: bool, n: char| -> bool { ((((((((((a == 1) && (z == 2)) && (c == 3)) && (e == 4)) && (g == 5)) && (h == 6)) && (j == 7.0)) && (k == 8.0)) && m) && (n == '\u{4e}'))})) }
;
    __sn_print_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", ((many.clone()).0)(1, 2, 3, 4, 5, 6, 7.0, 8.0, true, '\u{4e}'))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x0a]))); __sn_interpolated }));
    let mut empty: __SnClosure<dyn Fn() -> ()> = { self::__SnClosure::<dyn Fn() -> ()>(std::rc::Rc::new(move || -> () { __sn_print_string(&(SnString::from_slice(&[0x76, 0x6f, 0x69, 0x64, 0x0a])));})) }
;
    ((empty.clone()).0)();
}
