#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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

#[allow(non_camel_case_types)]
trait __SnArrayText_0 {
    fn __sn_array_text_0(&self) -> String;
    fn __sn_join_text_0(&self) -> String;
}

macro_rules! __sn_integer_array_text_0 {
    ($($type:ty),+ $(,)?) => {$(
        impl __SnArrayText_0 for $type {
            fn __sn_array_text_0(&self) -> String { self.to_string() }
            fn __sn_join_text_0(&self) -> String { self.to_string() }
        }
    )+};
}

__sn_integer_array_text_0!(i64, i32, u64, u32);

impl __SnArrayText_0 for u8 {
    fn __sn_array_text_0(&self) -> String { format!("0x{:02X}", self) }
    fn __sn_join_text_0(&self) -> String { format!("0x{:02X}", self) }
}

impl __SnArrayText_0 for bool {
    fn __sn_array_text_0(&self) -> String { self.to_string() }
    fn __sn_join_text_0(&self) -> String { self.to_string() }
}

impl __SnArrayText_0 for char {
    fn __sn_array_text_0(&self) -> String { format!("'{}'", self) }
    fn __sn_join_text_0(&self) -> String { self.to_string() }
}

impl __SnArrayText_0 for String {
    fn __sn_array_text_0(&self) -> String { format!("\"{}\"", self) }
    fn __sn_join_text_0(&self) -> String { self.clone() }
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
            fn __sn_array_text_0(&self) -> String { __sn_float_array_text_0(*self as f64) }
            fn __sn_join_text_0(&self) -> String { format!("{:.5}", self) }
        }
    )+};
}

__sn_float_array_text_impl_0!(f64, f32);

impl<T: __SnArrayText_0> __SnArrayText_0 for Vec<T> {
    fn __sn_array_text_0(&self) -> String { __sn_array_to_string_0(self.as_slice()) }
    // sn_array_join renders each nested-array element as "?"; recursion is
    // reserved for full array formatting (print and interpolation).
    fn __sn_join_text_0(&self) -> String { "?".to_string() }
}

fn __sn_array_to_string_0<T: __SnArrayText_0>(array: &[T]) -> String {
    let mut result = String::from("[");
    for (index, value) in array.iter().enumerate() {
        if index != 0 { result.push_str(", "); }
        result.push_str(&value.__sn_array_text_0());
    }
    result.push(']');
    result
}

fn __sn_array_join_1<T: __SnArrayText_0>(array: &[T], separator: &str) -> String {
    let mut result = String::new();
    for (index, value) in array.iter().enumerate() {
        if index != 0 { result.push_str(separator); }
        result.push_str(&value.__sn_join_text_0());
    }
    result
}


fn __sn_array_join() -> i64 {
    return 11;
}

fn __sn_array_join_0() -> i64 {
    return 21;
}

fn main() {
    let mut __sn_array: Vec<i64> = vec![1, 2, 3];
    let mut __sn_array_0: Vec<i64> = vec![4];
    let mut __sn_separator: String = "-".to_string();
    let mut __sn_separator_0: String = ",".to_string();
    println!("{}", { let __sn_array_1 = &(__sn_array); let __sn_separator_1 = &(__sn_separator); __sn_array_join_1(__sn_array_1.as_slice(), __sn_separator_1.as_str()) });
    println!("{}", { let __sn_array_1 = &(__sn_array_0); let __sn_separator_1 = &(__sn_separator_0); __sn_array_join_1(__sn_array_1.as_slice(), __sn_separator_1.as_str()) });
    println!("{}", __sn_array_join());
    println!("{}", __sn_array_join_0());
}
