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

fn __sn_array_join_1<T: __SnArrayText_0>(array: &[T], separator: &str) -> String {
    let mut result = String::new();
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
    fn mutateSeparator(&mut self) -> String {
        ((self).values).push(3);
        return "-".to_string();
    }
    fn render(&mut self) -> String {
        return { let __sn_separator_1 = &((self).mutateSeparator()); __sn_array_join_1(((self).values).as_slice(), __sn_separator_1.as_str()) };
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
    let mut __sn_separator: String = "-".to_string();
    let mut __sn_separator_0: String = ",".to_string();
    println!("{}", { let __sn_separator_1 = &(__sn_separator); __sn_array_join_1((__sn_array).as_slice(), __sn_separator_1.as_str()) });
    println!("{}", { let __sn_separator_1 = &(__sn_separator_0); __sn_array_join_1((__sn_array_0).as_slice(), __sn_separator_1.as_str()) });
    println!("{}", __sn_array_join());
    println!("{}", __sn_array_join_0());
    let mut negative: i64 = (-1);
    let mut unsigned: Vec<u64> = vec![0, 42, 9223372036854775807, (negative as u64)];
    println!("{}", { let __sn_separator_1 = &(",".to_string()); __sn_array_join_1((unsigned).as_slice(), __sn_separator_1.as_str()) });
    println!("{}", __sn_array_to_string_0(&(unsigned)));
    let mut bag: JoinBag = JoinBag { values: vec![1, 2] };
    println!("{}", (bag).render());
    println!("{}", __sn_array_to_string_0(&((bag).values)));
    let mut __sn_join_index_0: i64 = 41;
    let mut receiverIndexCalls: i64 = 0;
    let mut nested: Vec<Vec<JoinBag>> = vec![vec![JoinBag { values: vec![1, 2] }]];
    println!("{}", { let __sn_join_index_3 = __sn_index((nested).len(), { let __sn_place = &mut (receiverIndexCalls); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous }); let __sn_join_index_4 = __sn_index(((nested)[__sn_join_index_3]).len(), 0); let __sn_separator_1 = &({ let __sn_join_index_1 = __sn_index((nested).len(), 0); let __sn_join_index_2 = __sn_index(((nested)[__sn_join_index_1]).len(), 0); (((nested)[__sn_join_index_1])[__sn_join_index_2]).mutateSeparator() }); __sn_array_join_1(((((nested)[__sn_join_index_3])[__sn_join_index_4]).values).as_slice(), __sn_separator_1.as_str()) });
    println!("{}", receiverIndexCalls);
    println!("{}", __sn_array_to_string_0(&((((nested)[__sn_index((nested).len(), 0)])[__sn_index(((nested)[__sn_index((nested).len(), 0)]).len(), 0)]).values)));
    println!("{}", __sn_join_index_0);
    let mut __sn_join_owner_0: i64 = 42;
    let mut producerCalls: i64 = 0;
    println!("{}", { let __sn_join_owner_1 = produceNested(&mut (producerCalls)); let __sn_join_index_5 = __sn_index((__sn_join_owner_1).len(), 0); let __sn_join_index_6 = __sn_index(((__sn_join_owner_1)[__sn_join_index_5]).len(), 0); let __sn_separator_1 = &("/".to_string()); __sn_array_join_1(((((__sn_join_owner_1)[__sn_join_index_5])[__sn_join_index_6]).values).as_slice(), __sn_separator_1.as_str()) });
    println!("{}", producerCalls);
    println!("{}", __sn_join_owner_0);
    let mut bytes: Vec<u8> = vec![65, 0, 66, 255];
    let mut byteText: String = { let __sn_array_1 = &(bytes); String::from_utf8_lossy(__sn_array_1.split(|value| *value == 0).next().unwrap_or(&[])).into_owned() };
    println!("{}", byteText);
    println!("{}", (byteText).len() as i64);
}
