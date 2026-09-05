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

#[derive(Clone, Debug, PartialEq)]
struct Matcher {
    text: SnString,
}

impl Matcher {
    fn instanceValue(&self) -> i64 {
        let mut result: i64 = {
    let __sn_match_subject_1: SnString = (self).text.clone();
    if (__sn_match_subject_1 == SnString::from("north")) {
        (10 as i64)
    }
    else if (__sn_match_subject_1 == SnString::from("north")) {
        (20 as i64)
    }
    else {
        (0 as i64)
    }
};
        return result;
    }
    fn staticValue(value: SnString) -> bool {
        return {
     let __sn_match_subject_2: SnString = value.clone();
     if (__sn_match_subject_2 == SnString::from("")) {
         (false)
     }
     else if (__sn_match_subject_2 == SnString::from("héllo") || __sn_match_subject_2 == SnString::from("hello")) {
         (true)
     }
     else {
         (false)
     }
 };
    }
}

fn makeSubject(calls: &mut i64) -> SnString {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return { let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("to"))); __sn_string.push_str(&(SnString::from("ken"))); __sn_string };
}

fn makeLabels(calls: &mut i64) -> Vec<SnString> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![SnString::from("indexed")];
}

fn makeRows(calls: &mut i64) -> Vec<Vec<SnString>> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![vec![SnString::from("nested")]];
}

fn recordIndex(trace: &mut i64, marker: i64) -> i64 {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return (-1);
}

fn makeCube(trace: &mut i64) -> Vec<Vec<Vec<SnString>>> {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(1), "Runtime error: integer overflow in addition"));
    return vec![vec![vec![SnString::from("deep")]]];
}

fn selectedValue(calls: &mut i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn parameterValue(value: SnString) -> i64 {
    let mut result: i64 = {
    let __sn_match_subject_3: SnString = value.clone();
    if (__sn_match_subject_3 == SnString::from("token")) {
        (7 as i64)
    }
    else {
        (0 as i64)
    }
};
    return result;
}

fn main() {
    let mut subject: SnString = SnString::from("token");
    let mut statementCalls: i64 = 0;
    {
    let __sn_match_subject_4: SnString = subject.clone();
    if (__sn_match_subject_4 == SnString::from("miss")) {
        (statementCalls = 100);
    }
    else if (__sn_match_subject_4 == SnString::from("token") || __sn_match_subject_4 == SnString::from("token")) {
        { let __sn_place = &mut (statementCalls); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
    else {
        (statementCalls = 200);
    }
};
    println!("{}", ((statementCalls == 1) && (subject == SnString::from("token"))))
;
    let mut subjectCalls: i64 = 0;
    let mut armCalls: i64 = 0;
    let mut chosen: i64 = {
    let __sn_match_subject_5: SnString = makeSubject(&mut (subjectCalls))
;
    if (__sn_match_subject_5 == SnString::from("miss")) {
        (selectedValue(&mut (armCalls), 1)
 as i64)
    }
    else if (__sn_match_subject_5 == SnString::from("token")) {
        (selectedValue(&mut (armCalls), 7)
 as i64)
    }
    else if (__sn_match_subject_5 == SnString::from("token")) {
        (selectedValue(&mut (armCalls), 9)
 as i64)
    }
    else {
        (selectedValue(&mut (armCalls), 0)
 as i64)
    }
};
    println!("{}", (((chosen == 7) && (subjectCalls == 1)) && (armCalls == 1)))
;
    let mut noElseCalls: i64 = 0;
    {
    let __sn_match_subject_6: SnString = SnString::from("absent");
    if (__sn_match_subject_6 == SnString::from("present")) {
        { let __sn_place = &mut (noElseCalls); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (noElseCalls == 0))
;
    let mut literalResult: bool = {
    let __sn_match_subject_7: SnString = SnString::from("");
    if (__sn_match_subject_7 == SnString::from("")) {
        (true)
    }
    else {
        (false)
    }
};
    println!("{}", literalResult)
;
    let mut nested: bool = {
    let __sn_match_subject_9: SnString = SnString::from("outer");
    if (__sn_match_subject_9 == SnString::from("outer")) {
        ({
    let __sn_match_subject_8: SnString = subject.clone();
    if (__sn_match_subject_8 == SnString::from("token")) {
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
    println!("{}", nested)
;
    let mut __sn_match_subject: SnString = SnString::from("source-subject");
    let mut __sn_match_array: i64 = 0;
    let mut __sn_match_index: i64 = 41;
    let mut __sn_match_subject_0: SnString = SnString::from("candidate-subject");
    let mut __sn_match_array_0: i64 = 7;
    let mut __sn_match_index_0: i64 = 8;
    let mut hygieneCalls: i64 = 0;
    let mut hygieneRows: Vec<Vec<SnString>> = vec![vec![SnString::from("nested")]];
    {
    let __sn_match_array_12 = &(makeRows(&mut (hygieneCalls))
);
    let __sn_match_index_12 = __sn_index(__sn_match_array_12.len(), 0);
    let __sn_match_array_12 = &(__sn_match_array_12[__sn_match_index_12]);
    let __sn_match_index_12 = __sn_index(__sn_match_array_12.len(), 0);
    let __sn_match_subject_12: SnString = __sn_match_array_12[__sn_match_index_12].clone();
    if (__sn_match_subject_12 == SnString::from("nested")) {
        (__sn_match_subject = __sn_match_subject.clone());
        (__sn_match_array = __sn_checked_0((__sn_match_index).checked_add(1), "Runtime error: integer overflow in addition"));
        (__sn_match_subject_0 = __sn_match_subject_0.clone());
        (__sn_match_array_0 = __sn_checked_0((__sn_match_index_0).checked_add(1), "Runtime error: integer overflow in addition"));
        {
    let __sn_match_array_10 = &(hygieneRows);
    let __sn_match_index_10 = __sn_index(__sn_match_array_10.len(), 0);
    let __sn_match_array_10 = &(__sn_match_array_10[__sn_match_index_10]);
    let __sn_match_index_10 = __sn_index(__sn_match_array_10.len(), 0);
    let __sn_match_subject_10: SnString = __sn_match_array_10[__sn_match_index_10].clone();
    if (__sn_match_subject_10 == SnString::from("nested")) {
        (__sn_match_index = __sn_checked_0((__sn_match_array).checked_sub(1), "Runtime error: integer overflow in subtraction"));
        (__sn_match_index_0 = __sn_checked_0((__sn_match_array_0).checked_sub(1), "Runtime error: integer overflow in subtraction"));
    }
};
        {
    let __sn_match_subject_11: f32 = 1.0;
    if (__sn_match_subject_11 == 1.0) {
        (__sn_match_subject = __sn_match_subject.clone());
    }
};
    }
};
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(__sn_match_subject)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_array)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_index)); __sn_interpolated }
))
;
    println!("{}", ((((hygieneCalls == 1) && (__sn_match_subject_0 == SnString::from("candidate-subject"))) && (__sn_match_array_0 == 9)) && (__sn_match_index_0 == 8)))
;
    let mut labels: Vec<SnString> = vec![SnString::from("indexed")];
    let mut indexed: bool = {
    let __sn_match_array_13 = &(labels);
    let __sn_match_index_13 = __sn_index(__sn_match_array_13.len(), 0);
    let __sn_match_subject_13: SnString = __sn_match_array_13[__sn_match_index_13].clone();
    if (__sn_match_subject_13 == SnString::from("indexed")) {
        (true)
    }
    else {
        (false)
    }
};
    println!("{}", (((indexed && ((labels)[__sn_index((labels).len(), 0)] == SnString::from("indexed"))) && (__sn_match_array == 42)) && (__sn_match_index == 41)))
;
    let mut indexedReceiverCalls: i64 = 0;
    let mut indexedReceiverResult: i64 = {
    let __sn_match_array_14 = &(makeLabels(&mut (indexedReceiverCalls))
);
    let __sn_match_index_14 = __sn_index(__sn_match_array_14.len(), 0);
    let __sn_match_subject_14: SnString = __sn_match_array_14[__sn_match_index_14].clone();
    if (__sn_match_subject_14 == SnString::from("indexed")) {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", indexedReceiverResult)
;
    println!("{}", indexedReceiverCalls)
;
    let mut nestedReceiverCalls: i64 = 0;
    let mut nestedReceiverResult: i64 = {
    let __sn_match_array_15 = &(makeRows(&mut (nestedReceiverCalls))
);
    let __sn_match_index_15 = __sn_index(__sn_match_array_15.len(), 0);
    let __sn_match_array_15 = &(__sn_match_array_15[__sn_match_index_15]);
    let __sn_match_index_15 = __sn_index(__sn_match_array_15.len(), 0);
    let __sn_match_subject_15: SnString = __sn_match_array_15[__sn_match_index_15].clone();
    if (__sn_match_subject_15 == SnString::from("nested")) {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", nestedReceiverResult)
;
    println!("{}", nestedReceiverCalls)
;
    let mut nestedOrder: i64 = 0;
    let mut deepIndexedResult: i64 = {
    let __sn_match_array_16 = &(makeCube(&mut (nestedOrder))
);
    let __sn_match_index_16 = __sn_index(__sn_match_array_16.len(), recordIndex(&mut (nestedOrder), 2)
);
    let __sn_match_array_16 = &(__sn_match_array_16[__sn_match_index_16]);
    let __sn_match_index_16 = __sn_index(__sn_match_array_16.len(), recordIndex(&mut (nestedOrder), 3)
);
    let __sn_match_array_16 = &(__sn_match_array_16[__sn_match_index_16]);
    let __sn_match_index_16 = __sn_index(__sn_match_array_16.len(), recordIndex(&mut (nestedOrder), 4)
);
    let __sn_match_subject_16: SnString = __sn_match_array_16[__sn_match_index_16].clone();
    if (__sn_match_subject_16 == SnString::from("deep")) {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", ((((deepIndexedResult == 1) && (nestedOrder == 1234)) && (__sn_match_array == 42)) && (__sn_match_index == 41)))
;
    let mut concatenated: i64 = {
    let __sn_match_subject_17: SnString = { let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("con"))); __sn_string.push_str(&(SnString::from("tent"))); __sn_string };
    if (__sn_match_subject_17 == SnString::from("content")) {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", (concatenated == 1))
;
    let mut matcher: Matcher = Matcher { text: SnString::from("north") };
    println!("{}", (((matcher).instanceValue()
 == 10) && ((matcher).text == SnString::from("north"))))
;
    let mut greeting: SnString = SnString::from("héllo");
    println!("{}", (Matcher::staticValue(greeting.clone()) && (greeting == SnString::from("héllo"))))
;
    println!("{}", ((parameterValue(subject.clone())
 == 7) && (subject == SnString::from("token"))))
;
}
