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
#[derive(Clone, Copy, Debug, PartialEq)]
struct Dispatcher {
    bias: i64,
}

impl Dispatcher {
    fn apply(callback: __SnClosure<dyn Fn(i64) -> i64>, value: i64) -> i64 {
        return ((callback.clone()).0)(value.clone());
    }
    fn same(left: __SnClosure<dyn Fn(i64) -> i64>, right: __SnClosure<dyn Fn(i64) -> i64>) -> bool {
        return (left.clone() == right.clone());
    }
    fn make(offset: i64) -> __SnClosure<dyn Fn(i64) -> i64> {
        return { let (offset, ) = (offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((offset.clone()).checked_add(value), "Runtime error: integer overflow in addition")})) }
;
    }
    fn composeStatic(callback: __SnClosure<dyn Fn(i64) -> i64>, offset: i64) -> __SnClosure<dyn Fn(i64) -> i64> {
        return { let (callback, offset, ) = (callback.clone(), offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((((callback.clone()).0)(value.clone())).checked_add(offset.clone()), "Runtime error: integer overflow in addition")})) }
;
    }
    fn run(&self, callback: __SnClosure<dyn Fn(i64) -> i64>, value: i64) -> i64 {
        return __sn_checked_0((((callback.clone()).0)(value.clone())).checked_add((self).bias), "Runtime error: integer overflow in addition");
    }
    fn compose(&self, callback: __SnClosure<dyn Fn(i64) -> i64>) -> __SnClosure<dyn Fn(i64) -> i64> {
        let mut bias: i64 = (self).bias;
        return { let (callback, bias, ) = (callback.clone(), bias.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((((callback.clone()).0)(value.clone())).checked_add(bias.clone()), "Runtime error: integer overflow in addition")})) }
;
    }
}

fn markedValue(order: &mut i64, marker: i64, value: i64) -> i64 {
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn makeDispatcher(order: &mut i64, marker: i64, callback: __SnClosure<dyn Fn(i64) -> i64>) -> Dispatcher {
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    ((callback.clone()).0)(marker.clone());
    return Dispatcher { bias: marker };
}

fn returnComposed(dispatcher: Dispatcher, callback: __SnClosure<dyn Fn(i64) -> i64>) -> __SnClosure<dyn Fn(i64) -> i64> {
    return (dispatcher).compose(callback.clone());
}

fn main() {
    let mut order: i64 = 0;
    let mut source: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition")})) }
;
    let mut dispatcher: Dispatcher = Dispatcher { bias: 10 };
    let mut staticApplied: i64 = Dispatcher::apply(source.clone(), 2);
    let mut instanceApplied: i64 = (dispatcher).run(source.clone(), 3);
    let mut same: bool = Dispatcher::same(source.clone(), source.clone());
    let mut made: __SnClosure<dyn Fn(i64) -> i64> = Dispatcher::make(20);
    let mut staticComposed: __SnClosure<dyn Fn(i64) -> i64> = Dispatcher::composeStatic(source.clone(), 30);
    let mut instanceComposed: __SnClosure<dyn Fn(i64) -> i64> = (dispatcher).compose(source.clone());
    let mut returned: __SnClosure<dyn Fn(i64) -> i64> = returnComposed(dispatcher, source.clone());
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", staticApplied)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", instanceApplied)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", same)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((made.clone()).0)(4))); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", ((staticComposed.clone()).0)(5))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((instanceComposed.clone()).0)(6))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(7))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((source.clone()).0)(8))); __sn_interpolated }));
    { source = Dispatcher::make(100); source.clone() };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", ((staticComposed.clone()).0)(9))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((instanceComposed.clone()).0)(10))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(11))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((source.clone()).0)(1))); __sn_interpolated }));
    let mut ordered: i64 = (makeDispatcher(&mut (order), 1, source.clone())).run(source.clone(), markedValue(&mut (order), 2, 5));
    let mut temporary: __SnClosure<dyn Fn(i64) -> i64> = Dispatcher::make(40);
    let mut temporaryApplied: i64 = Dispatcher::apply(temporary.clone(), 2);
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", ordered)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", temporaryApplied)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((source.clone()).0)(12))); __sn_interpolated }));
}
