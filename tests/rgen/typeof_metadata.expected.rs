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

#[allow(non_snake_case)]
#[derive(Clone, Debug, PartialEq)]
struct FieldInfo {
    name: SnString,
    typeName: SnString,
    typeId: i64,
}

#[allow(non_snake_case)]
#[derive(Clone, Debug, PartialEq)]
struct TypeInfo {
    name: SnString,
    fields: Vec<FieldInfo>,
    fieldCount: i64,
    typeId: i64,
}


#[derive(Clone, Copy, Debug, PartialEq)]
struct Inner {
    code: i32,
}
#[derive(Clone, Debug, PartialEq)]
struct Record {
    name: SnString,
    count: i64,
    flags: Vec<bool>,
    inner: Inner,
    ratio: f32,
}

fn touch(counter: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return *(counter);
}

fn reflectedRecord() -> TypeInfo {
    let mut value: Record = Record { name: SnString::from_slice(&[0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x65, 0x64]), count: 2, flags: vec![true, false], inner: Inner { code: 7 }, ratio: 1.5 };
    let mut info: TypeInfo = TypeInfo { name: SnString::from_slice(&[0x52, 0x65, 0x63, 0x6f, 0x72, 0x64]), fields: vec![FieldInfo { name: SnString::from_slice(&[0x6e, 0x61, 0x6d, 0x65]), typeName: SnString::from_slice(&[0x73, 0x74, 0x72]), typeId: 1112265104 }, FieldInfo { name: SnString::from_slice(&[0x63, 0x6f, 0x75, 0x6e, 0x74]), typeName: SnString::from_slice(&[0x69, 0x6e, 0x74]), typeId: 367623774 }, FieldInfo { name: SnString::from_slice(&[0x66, 0x6c, 0x61, 0x67, 0x73]), typeName: SnString::from_slice(&[0x61, 0x72, 0x72, 0x61, 0x79]), typeId: 173583654 }, FieldInfo { name: SnString::from_slice(&[0x69, 0x6e, 0x6e, 0x65, 0x72]), typeName: SnString::from_slice(&[0x49, 0x6e, 0x6e, 0x65, 0x72]), typeId: 2124115655 }, FieldInfo { name: SnString::from_slice(&[0x72, 0x61, 0x74, 0x69, 0x6f]), typeName: SnString::from_slice(&[0x66, 0x6c, 0x6f, 0x61, 0x74]), typeId: 650403205 }], fieldCount: 5, typeId: 524641772 }
;
    return info;
}

fn main() {
    let mut integer: i64 = 1;
    let mut long_value: i64 = 2;
    let mut int32_value: i32 = 3;
    let mut uint_value: u64 = 4;
    let mut uint32_value: u32 = 5;
    let mut double_value: f64 = 6.0;
    let mut float_value: f32 = 7.0;
    let mut bool_value: bool = true;
    let mut char_value: char = '\u{78}';
    let mut byte_value: u8 = 8;
    let mut string_value: SnString = SnString::from_slice(&[0x74, 0x65, 0x78, 0x74]);
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x69, 0x6e, 0x74]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 367623774)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x6c, 0x6f, 0x6e, 0x67]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 1122819923)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x69, 0x6e, 0x74, 0x33, 0x32]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 2078204607)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x75, 0x69, 0x6e, 0x74]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 1268266657)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x75, 0x69, 0x6e, 0x74, 0x33, 0x32]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 848563180)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x64, 0x6f, 0x75, 0x62, 0x6c, 0x65]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 552275720)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x66, 0x6c, 0x6f, 0x61, 0x74]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 650403205)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x62, 0x6f, 0x6f, 0x6c]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 1217697085)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x63, 0x68, 0x61, 0x72]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 676070173)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x62, 0x79, 0x74, 0x65]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 1683620383)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(SnString::from_slice(&[0x73, 0x74, 0x72]))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 1112265104)); __sn_interpolated }));
    let mut numbers: Vec<i64> = vec![1, 2, 3];
    let mut words: Vec<SnString> = vec![SnString::from_slice(&[0x61]), SnString::from_slice(&[0x62])];
    let mut number_info: TypeInfo = TypeInfo { name: SnString::from_slice(&[0x61, 0x72, 0x72, 0x61, 0x79]), fields: vec![], fieldCount: 0, typeId: 173583654 }
;
    let mut word_info: TypeInfo = TypeInfo { name: SnString::from_slice(&[0x61, 0x72, 0x72, 0x61, 0x79]), fields: vec![], fieldCount: 0, typeId: 173583654 }
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((number_info).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (number_info).fieldCount)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (number_info).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((word_info).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (word_info).fieldCount)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (word_info).typeId)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((number_info).typeId == (word_info).typeId))); __sn_interpolated }));
    let mut inner: Inner = Inner { code: 9 };
    let mut record: Record = Record { name: SnString::from_slice(&[0x72, 0x65, 0x63, 0x6f, 0x72, 0x64]), count: 3, flags: vec![true], inner: inner, ratio: 2.5 };
    let mut info: TypeInfo = TypeInfo { name: SnString::from_slice(&[0x52, 0x65, 0x63, 0x6f, 0x72, 0x64]), fields: vec![FieldInfo { name: SnString::from_slice(&[0x6e, 0x61, 0x6d, 0x65]), typeName: SnString::from_slice(&[0x73, 0x74, 0x72]), typeId: 1112265104 }, FieldInfo { name: SnString::from_slice(&[0x63, 0x6f, 0x75, 0x6e, 0x74]), typeName: SnString::from_slice(&[0x69, 0x6e, 0x74]), typeId: 367623774 }, FieldInfo { name: SnString::from_slice(&[0x66, 0x6c, 0x61, 0x67, 0x73]), typeName: SnString::from_slice(&[0x61, 0x72, 0x72, 0x61, 0x79]), typeId: 173583654 }, FieldInfo { name: SnString::from_slice(&[0x69, 0x6e, 0x6e, 0x65, 0x72]), typeName: SnString::from_slice(&[0x49, 0x6e, 0x6e, 0x65, 0x72]), typeId: 2124115655 }, FieldInfo { name: SnString::from_slice(&[0x72, 0x61, 0x74, 0x69, 0x6f]), typeName: SnString::from_slice(&[0x66, 0x6c, 0x6f, 0x61, 0x74]), typeId: 650403205 }], fieldCount: 5, typeId: 524641772 }
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((info).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (info).fieldCount)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (info).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 0)]).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 0)]).typeName)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 0)]).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 1)]).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 1)]).typeName)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 1)]).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 2)]).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 2)]).typeName)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 2)]).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 3)]).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 3)]).typeName)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 3)]).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 4)]).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((((info).fields)[__sn_index(((info).fields).len(), 4)]).typeName)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 4)]).typeId)); __sn_interpolated }));
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", ((((info).fields)[__sn_index(((info).fields).len(), 3)]).typeId == 2124115655))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (524641772 == (info).typeId))); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (524641772 != 367623774))); __sn_interpolated }));
    let mut assigned: TypeInfo = info.clone();
    ((assigned).fields).clear();
    let mut copied: TypeInfo = (info).clone();
    ((copied).fields).clear();
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((info).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((info).fields).len() as i64)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((assigned).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((assigned).fields).len() as i64)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((copied).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", ((copied).fields).len() as i64)); __sn_interpolated }));
    let mut returned: TypeInfo = reflectedRecord();
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&((returned).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", (returned).fieldCount)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((((returned).fields)[__sn_index(((returned).fields).len(), 3)]).typeName)); __sn_interpolated }));
    let mut counter: i64 = 0;
    let mut unevaluated: TypeInfo = TypeInfo { name: SnString::from_slice(&[0x69, 0x6e, 0x74]), fields: vec![], fieldCount: 0, typeId: 367623774 }
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", counter)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&((unevaluated).name)); __sn_interpolated.push_str(&(SnString::from_slice(&[0x3a]))); __sn_interpolated.push_str(&format!("{}", 0)); __sn_interpolated }));
}
