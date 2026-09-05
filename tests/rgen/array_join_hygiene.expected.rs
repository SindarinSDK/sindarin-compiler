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

#[allow(non_camel_case_types)]
trait __SnArrayText_0 {
    fn __sn_array_text_0(&self) -> SnString;
    fn __sn_join_text_0(&self) -> SnString;
    fn __sn_struct_text_0(&self) -> SnString;
}

macro_rules! __sn_integer_array_text_0 {
    ($($type:ty),+ $(,)?) => {$(
        impl __SnArrayText_0 for $type {
            fn __sn_array_text_0(&self) -> SnString { SnString::from(self.to_string()) }
            fn __sn_join_text_0(&self) -> SnString { SnString::from(self.to_string()) }
            fn __sn_struct_text_0(&self) -> SnString { SnString::from(self.to_string()) }
        }
    )+};
}

__sn_integer_array_text_0!(i64, i32, u32);

impl __SnArrayText_0 for u64 {
    fn __sn_array_text_0(&self) -> SnString { SnString::from((*self as i64).to_string()) }
    fn __sn_join_text_0(&self) -> SnString { SnString::from((*self as i64).to_string()) }
    fn __sn_struct_text_0(&self) -> SnString { SnString::from((*self as i64).to_string()) }
}

impl __SnArrayText_0 for u8 {
    fn __sn_array_text_0(&self) -> SnString { SnString::from(format!("0x{:02X}", self)) }
    fn __sn_join_text_0(&self) -> SnString { SnString::from(format!("0x{:02X}", self)) }
    fn __sn_struct_text_0(&self) -> SnString { SnString::from(self.to_string()) }
}

impl __SnArrayText_0 for bool {
    fn __sn_array_text_0(&self) -> SnString { SnString::from(self.to_string()) }
    fn __sn_join_text_0(&self) -> SnString { SnString::from(self.to_string()) }
    fn __sn_struct_text_0(&self) -> SnString { SnString::from(self.to_string()) }
}

impl __SnArrayText_0 for char {
    fn __sn_array_text_0(&self) -> SnString {
        let mut result = SnString::from_slice(b"'");
        result.push_char(*self);
        result.push_str("'");
        result
    }
    fn __sn_join_text_0(&self) -> SnString {
        let mut result = SnString::new();
        result.push_char(*self);
        result
    }
    fn __sn_struct_text_0(&self) -> SnString {
        self.__sn_array_text_0()
    }
}

impl __SnArrayText_0 for SnString {
    fn __sn_array_text_0(&self) -> SnString {
        let mut result = SnString::from_slice(b"\"");
        result.push_str(self);
        result.push_str("\"");
        result
    }
    fn __sn_join_text_0(&self) -> SnString { self.clone() }
    fn __sn_struct_text_0(&self) -> SnString {
        self.__sn_array_text_0()
    }
}

fn __sn_float_array_text_0(value: f64) -> String {
    if value.is_nan() { return "nan".to_string(); }
    if value == f64::INFINITY { return "inf".to_string(); }
    if value == f64::NEG_INFINITY { return "-inf".to_string(); }

    let scientific = format!("{:.5e}", value);
    let (mantissa, exponent_text) = scientific.split_once('e').unwrap();
    let exponent: i32 = exponent_text.parse().unwrap();
    if exponent < -4 || exponent >= 6 {
        let mantissa = mantissa.trim_end_matches('0').trim_end_matches('.');
        return format!("{}e{:+03}", mantissa, exponent);
    }

    let precision = (5 - exponent).max(0) as usize;
    let fixed = format!("{:.*}", precision, value);
    if fixed.contains('.') {
        fixed.trim_end_matches('0').trim_end_matches('.').to_string()
    } else {
        fixed
    }
}

macro_rules! __sn_float_array_text_impl_0 {
    ($($type:ty),+ $(,)?) => {$(
        impl __SnArrayText_0 for $type {
            fn __sn_array_text_0(&self) -> SnString { SnString::from(__sn_float_array_text_0(*self as f64)) }
            fn __sn_join_text_0(&self) -> SnString { SnString::from(format!("{:.5}", self)) }
            fn __sn_struct_text_0(&self) -> SnString { SnString::from(format!("{:.5}", self)) }
        }
    )+};
}

__sn_float_array_text_impl_0!(f64, f32);

impl<T: __SnArrayText_0> __SnArrayText_0 for Vec<T> {
    fn __sn_array_text_0(&self) -> SnString { __sn_array_to_string_0(self.as_slice()) }
    // sn_array_join renders each nested-array element as "?"; recursion is
    // reserved for full array formatting (print and interpolation).
    fn __sn_join_text_0(&self) -> SnString { SnString::from_slice(b"?") }
    fn __sn_struct_text_0(&self) -> SnString { __sn_array_to_string_0(self.as_slice()) }
}

fn __sn_array_to_string_0<T: __SnArrayText_0>(array: &[T]) -> SnString {
    let mut result = SnString::from_slice(b"[");
    for (index, value) in array.iter().enumerate() {
        if index != 0 { result.push_str(", "); }
        result.push_str(&value.__sn_array_text_0());
    }
    result.push_str("]");
    result
}

fn __sn_array_join_1<T: __SnArrayText_0>(array: &[T], separator: &SnString) -> SnString {
    let mut result = SnString::new();
    for (index, value) in array.iter().enumerate() {
        if index != 0 { result.push_str(separator); }
        result.push_str(&value.__sn_join_text_0());
    }
    result
}


#[derive(Clone, Debug, PartialEq)]
struct JoinBag {
    values: Vec<i64>,
}

impl JoinBag {
    fn mutateSeparator(&mut self) -> SnString {
        ((self).values).push(3);
        return SnString::from_slice(&[0x2d]);
    }
    fn render(&mut self) -> SnString {
        return { let __sn_separator_1 = &((self).mutateSeparator()); __sn_array_join_1(((self).values).as_slice(), __sn_separator_1) };
    }
}

impl __SnArrayText_0 for JoinBag {
    fn __sn_array_text_0(&self) -> SnString {
        let mut result = SnString::from_slice(b"JoinBag { ");
        result.push_str("values: ");
        result.push_str(&self.values.__sn_struct_text_0());
        result.push_str(" }");
        result
    }

    fn __sn_join_text_0(&self) -> SnString {
        self.__sn_array_text_0()
    }

    fn __sn_struct_text_0(&self) -> SnString {
        self.__sn_array_text_0()
    }
}

fn __sn_array_join() -> i64 {
    return 11;
}

fn __sn_array_join_0() -> i64 {
    return 21;
}

fn produceNested(calls: &mut i64) -> Vec<Vec<JoinBag>> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![vec![JoinBag { values: vec![4, 5] }]];
}

fn main() {
    let mut __sn_array: Vec<i64> = vec![1, 2, 3];
    let mut __sn_array_0: Vec<i64> = vec![4];
    let mut __sn_separator: SnString = SnString::from_slice(&[0x2d]);
    let mut __sn_separator_0: SnString = SnString::from_slice(&[0x2c]);
    __sn_println_string(&({ let __sn_separator_1 = &(__sn_separator); __sn_array_join_1((__sn_array).as_slice(), __sn_separator_1) }));
    __sn_println_string(&({ let __sn_separator_1 = &(__sn_separator_0); __sn_array_join_1((__sn_array_0).as_slice(), __sn_separator_1) }));
    println!("{}", __sn_array_join());
    println!("{}", __sn_array_join_0());
    let mut negative: i64 = (-1);
    let mut unsigned: Vec<u64> = vec![0, 42, 9223372036854775807, (negative as u64)];
    __sn_println_string(&({ let __sn_separator_1 = &(SnString::from_slice(&[0x2c])); __sn_array_join_1((unsigned).as_slice(), __sn_separator_1) }));
    __sn_println_string(&__sn_array_to_string_0(&(unsigned)));
    let mut bag: JoinBag = JoinBag { values: vec![1, 2] };
    __sn_println_string(&((bag).render()));
    __sn_println_string(&__sn_array_to_string_0(&((bag).values)));
    let mut __sn_join_index_0: i64 = 41;
    let mut receiverIndexCalls: i64 = 0;
    let mut nested: Vec<Vec<JoinBag>> = vec![vec![JoinBag { values: vec![1, 2] }]];
    __sn_println_string(&({ let __sn_join_index_3 = __sn_index((nested).len(), { let __sn_place = &mut (receiverIndexCalls); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous }); let __sn_join_index_4 = __sn_index(((nested)[__sn_join_index_3]).len(), 0); let __sn_separator_1 = &({ let __sn_join_index_1 = __sn_index((nested).len(), 0); let __sn_join_index_2 = __sn_index(((nested)[__sn_join_index_1]).len(), 0); (((nested)[__sn_join_index_1])[__sn_join_index_2]).mutateSeparator() }); __sn_array_join_1(((((nested)[__sn_join_index_3])[__sn_join_index_4]).values).as_slice(), __sn_separator_1) }));
    println!("{}", receiverIndexCalls);
    __sn_println_string(&__sn_array_to_string_0(&((((nested)[__sn_index((nested).len(), 0)])[__sn_index(((nested)[__sn_index((nested).len(), 0)]).len(), 0)]).values)));
    println!("{}", __sn_join_index_0);
    let mut __sn_join_owner_0: i64 = 42;
    let mut producerCalls: i64 = 0;
    __sn_println_string(&({ let __sn_join_owner_1 = produceNested(&mut (producerCalls)); let __sn_join_index_5 = __sn_index((__sn_join_owner_1).len(), 0); let __sn_join_index_6 = __sn_index(((__sn_join_owner_1)[__sn_join_index_5]).len(), 0); let __sn_separator_1 = &(SnString::from_slice(&[0x2f])); __sn_array_join_1(((((__sn_join_owner_1)[__sn_join_index_5])[__sn_join_index_6]).values).as_slice(), __sn_separator_1) }));
    println!("{}", producerCalls);
    println!("{}", __sn_join_owner_0);
    let mut bytes: Vec<u8> = vec![65, 0, 66, 255];
    let mut byteText: SnString = { let __sn_array_1 = &(bytes); __sn_byte_array_to_string(__sn_array_1.as_slice()) };
    __sn_println_string(&(byteText));
    println!("{}", (byteText).len() as i64);
}
