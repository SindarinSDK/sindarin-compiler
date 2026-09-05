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


fn __sn_format_string(value: &SnString, width: usize, left_align: bool,
                      has_precision: bool, precision: usize) -> SnString {
    let c_length = value.as_bytes().iter().position(|byte| *byte == 0)
        .unwrap_or(value.len());
    let length = if has_precision {
        c_length.min(precision)
    } else {
        c_length
    };
    let padding = width.saturating_sub(length);
    let mut result = SnString::new();
    if left_align {
        result.0.extend_from_slice(&value.as_bytes()[..length]);
        result.0.extend(std::iter::repeat_n(b' ', padding));
    } else {
        result.0.extend(std::iter::repeat_n(b' ', padding));
        result.0.extend_from_slice(&value.as_bytes()[..length]);
    }
    result
}

fn __sn_format_character(value: char, width: usize, left_align: bool) -> SnString {
    let padding = width.saturating_sub(1);
    if value == '\0' {
        return if left_align { SnString::new() }
            else { SnString::from_bytes(vec![b' '; padding]) };
    }
    let mut result = SnString::new();
    if left_align {
        result.push_char(value);
        result.0.extend(std::iter::repeat_n(b' ', padding));
    } else {
        result.0.extend(std::iter::repeat_n(b' ', padding));
        result.push_char(value);
    }
    result
}

fn __sn_format_integer_alternate(digits: &str, is_zero: bool, uppercase: bool,
                                 octal: bool, width: usize, left_align: bool,
                                 zero_pad: bool) -> String {
    let prefix = if is_zero {
        ""
    } else if octal {
        "0"
    } else if uppercase {
        "0X"
    } else {
        "0x"
    };
    let padding = width.saturating_sub(prefix.len() + digits.len());
    if left_align {
        format!("{}{}{}", prefix, digits, " ".repeat(padding))
    } else if zero_pad {
        format!("{}{}{}", prefix, "0".repeat(padding), digits)
    } else {
        format!("{}{}{}", " ".repeat(padding), prefix, digits)
    }
}

fn __sn_format_fixed_alternate(value: f64, precision: usize, width: usize,
                               left_align: bool, force_sign: bool,
                               space_sign: bool, zero_pad: bool) -> String {
    let is_special = value.is_nan() || value.is_infinite();
    let magnitude = if value.is_nan() {
        "nan".to_string()
    } else if value.is_infinite() {
        "inf".to_string()
    } else {
        let mut rendered = format!("{:.*}", precision, value.abs());
        if precision == 0 {
            rendered.push('.');
        }
        rendered
    };
    let sign = if value.is_sign_negative() {
        "-"
    } else if force_sign {
        "+"
    } else if space_sign {
        " "
    } else {
        ""
    };
    let padding = width.saturating_sub(sign.len() + magnitude.len());
    if left_align {
        format!("{}{}{}", sign, magnitude, " ".repeat(padding))
    } else if zero_pad && !is_special {
        format!("{}{}{}", sign, "0".repeat(padding), magnitude)
    } else {
        format!("{}{}{}", " ".repeat(padding), sign, magnitude)
    }
}

fn __sn_format_scientific(value: f64, precision: usize, uppercase: bool,
                          width: usize, left_align: bool, force_sign: bool,
                          space_sign: bool, zero_pad: bool,
                          alternate: bool) -> String {
    let is_special = value.is_nan() || value.is_infinite();
    let magnitude = if value.is_nan() {
        if uppercase { "NAN" } else { "nan" }.to_string()
    } else if value.is_infinite() {
        if uppercase { "INF" } else { "inf" }.to_string()
    } else {
        let rendered = if uppercase {
            format!("{:.*E}", precision, value.abs())
        } else {
            format!("{:.*e}", precision, value.abs())
        };
        let marker = if uppercase { 'E' } else { 'e' };
        let (mantissa, exponent) = rendered.rsplit_once(marker)
            .expect("scientific formatting must contain an exponent");
        let mantissa = if alternate && precision == 0 {
            format!("{}.", mantissa)
        } else {
            mantissa.to_string()
        };
        let exponent: i32 = exponent.parse().expect("scientific exponent must be numeric");
        format!("{}{}{:+03}", mantissa, marker, exponent)
    };
    let sign = if value.is_sign_negative() {
        "-"
    } else if force_sign {
        "+"
    } else if space_sign {
        " "
    } else {
        ""
    };
    let padding = width.saturating_sub(sign.len() + magnitude.len());

    if left_align {
        format!("{}{}{}", sign, magnitude, " ".repeat(padding))
    } else if zero_pad && !is_special {
        format!("{}{}{}", sign, "0".repeat(padding), magnitude)
    } else {
        format!("{}{}{}", " ".repeat(padding), sign, magnitude)
    }
}


fn main() {
    let mut large: f64 = 1234.5;
    let mut small: f64 = 0.00125;
    let mut negative: f64 = (-42.0);
    let mut zero: f64 = 0.0;
    let mut rounded: f64 = 9.9990000000000006;
    let mut single: f32 = 12.5;
    let mut divisor: f64 = 0.0;
    let mut infinity: f64 = (1.0 / divisor);
    let mut negative_infinity: f64 = ((-1.0) / divisor);
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("default="); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 6, false, 0, false, false, false, false, false)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("precision="); __sn_interpolated.push_str(&__sn_format_scientific((small) as f64, 2, false, 0, false, false, false, false, false)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("uppercase="); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, true, 0, false, false, false, false, false)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("zero="); __sn_interpolated.push_str(&__sn_format_scientific((zero) as f64, 3, false, 0, false, false, false, false, false)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("rounded="); __sn_interpolated.push_str(&__sn_format_scientific((rounded) as f64, 2, false, 0, false, false, false, false, false)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("float="); __sn_interpolated.push_str(&__sn_format_scientific((single) as f64, 3, true, 0, false, false, false, false, false)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, false, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, true, false, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("sign=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 0, false, true, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("positive-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, true, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("negative-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, false, 14, false, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("combined=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, true, 14, true, true, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("infinity=|"); __sn_interpolated.push_str(&__sn_format_scientific((infinity) as f64, 2, false, 10, false, true, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("negative-infinity=|"); __sn_interpolated.push_str(&__sn_format_scientific((negative_infinity) as f64, 2, true, 10, true, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated }
))
;
}
