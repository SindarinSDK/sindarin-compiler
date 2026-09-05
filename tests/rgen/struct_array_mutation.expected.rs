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

__sn_integer_array_text_0!(i64, i32, u32);

impl __SnArrayText_0 for u64 {
    fn __sn_array_text_0(&self) -> String { (*self as i64).to_string() }
    fn __sn_join_text_0(&self) -> String { (*self as i64).to_string() }
}

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

fn __sn_array_join_0<T: __SnArrayText_0>(array: &[T], separator: &str) -> String {
    let mut result = String::new();
    for (index, value) in array.iter().enumerate() {
        if index != 0 { result.push_str(separator); }
        result.push_str(&value.__sn_join_text_0());
    }
    result
}


#[derive(Clone, Debug, PartialEq)]
struct Bag {
    values: Vec<i64>,
}

impl Bag {
    fn add(&mut self, value: i64) {
        ((self).values).push(value);
    }
    fn addPair(&mut self, first: i64, second: i64) {
        (self).add(first);
        (self).add(second);
    }
    fn reverse(&mut self) {
        ((self).values).reverse();
    }
    fn removeMiddle(&mut self) {
        { let __sn_array_index = __sn_index(((self).values).len(), 1); ((self).values).remove(__sn_array_index) };
    }
    fn size(&self) -> i64 {
        return ((self).values.clone()).len() as i64;
    }
}

fn main() {
    let mut bag: Bag = Bag { values: vec![] };
    (bag).add(1);
    (bag).addPair(2, 3);
    (bag).reverse();
    (bag).removeMiddle();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("values="); __sn_interpolated.push_str(&__sn_array_to_string_0(&((bag).values))); __sn_interpolated.push_str("; size="); __sn_interpolated.push_str(&format!("{}", (bag).size())); __sn_interpolated });
}
