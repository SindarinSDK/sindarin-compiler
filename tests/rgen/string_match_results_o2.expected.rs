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

#[derive(Clone, Debug, PartialEq)]
struct ResultBox {
    label: SnString,
    rows: Vec<Vec<SnString>>,
}

impl ResultBox {
    fn memberResult(&self, calls: &mut i64) -> SnString {
        { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
        return ((self).label).to_ascii_uppercase()
 
;
    }
    fn staticResult(calls: &mut i64) -> SnString {
        { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
        return SnString::from("static");
    }
}

fn selectSubject(calls: &mut i64) -> i64 {
    { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
    return 2;
}

fn ownedResult(calls: &mut i64, value: SnString) -> SnString {
    { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
    return { let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("<"); __sn_interpolated.push_str(&(value)); __sn_interpolated.push_str(">"); __sn_interpolated }
;
}

fn chooseForReturn(value: bool, fallback: SnString) -> SnString {
    return match (value) {
         true => {
             (SnString::from("returned"))
         },
         _ => {
             (fallback.clone())
         },
     };
}

fn main() {
    let mut variableResult: SnString = SnString::from("variable");
    let mut fallbackResult: SnString = SnString::from("fallback");
    let mut r#box: ResultBox = ResultBox { label: SnString::from("member"), rows: vec![vec![SnString::from("zero"), SnString::from("one")], vec![SnString::from("two"), SnString::from("three")]] };
    let mut localRows: Vec<Vec<SnString>> = vec![vec![SnString::from("local-zero")], vec![SnString::from("local-one")]];
    let mut escapedBorrowedSource: SnString = SnString::from("borrowed\n\tquote:\" slash:\\");
    let mut escapedRows: Vec<Vec<SnString>> = vec![vec![SnString::from("indexed\n\tquote:\" slash:\\")]];
    let mut subjectCalls: i64 = 0;
    let mut selectedCalls: i64 = 0;
    let mut selected: SnString = match (selectSubject(&mut (subjectCalls))
 as i64) {
        1 => {
            (ownedResult(&mut (selectedCalls), SnString::from("wrong"))
)
        },
        2 => {
            (ownedResult(&mut (selectedCalls), SnString::from("selected"))
)
        },
        2 => {
            (ownedResult(&mut (selectedCalls), SnString::from("duplicate"))
)
        },
        _ => {
            (ownedResult(&mut (selectedCalls), SnString::from("else"))
)
        },
    };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(selected)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", subjectCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", selectedCalls)); __sn_interpolated }
))
;
    let mut literal: SnString = match (true) {
        true => {
            (SnString::from("literal"))
        },
        _ => {
            (SnString::from("wrong"))
        },
    };
    let mut variable: SnString = {
    let __sn_match_subject_0: f32 = 1.0;
    if (__sn_match_subject_0 == 1.0) {
        (variableResult.clone())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut member: SnString = {
    let __sn_match_subject_1: f64 = 1.0;
    if (__sn_match_subject_1 == 1.0) {
        ((r#box).label.clone())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut localIndexed: SnString = match (7 as i64) {
        7 => {
            (((localRows)[__sn_index((localRows).len(), 1)])[__sn_index(((localRows)[__sn_index((localRows).len(), 1)]).len(), 0)].clone())
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut memberIndexed: SnString = {
    let __sn_match_subject_2: SnString = SnString::from("key");
    if (__sn_match_subject_2 == SnString::from("key")) {
        ((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)]).len(), 1)].clone())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut multiIndexed: SnString = match (false) {
        true => {
            (fallbackResult.clone())
        },
        _ => {
            ((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)]).len(), 1)].clone())
        },
    };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(literal)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(variable)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(member)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(localIndexed)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(memberIndexed)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(multiIndexed)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(variableResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&((r#box).label)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(((localRows)[__sn_index((localRows).len(), 1)])[__sn_index(((localRows)[__sn_index((localRows).len(), 1)]).len(), 0)])); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)]).len(), 1)])); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)]).len(), 1)])); __sn_interpolated }
))
;
    let mut concatenated: SnString = match (10 as i64) {
        10 => {
            ({ let mut __sn_string = SnString::new(); __sn_string.push_str(&(SnString::from("con"))); __sn_string.push_str(&(variableResult)); __sn_string })
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut interpolated: SnString = match (10 as i32) {
        10 => {
            ({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str("inter-"); __sn_interpolated.push_str(&(variableResult)); __sn_interpolated }
)
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut freeCalls: i64 = 0;
    let mut freeCalled: SnString = match (10 as u64) {
        10 => {
            (ownedResult(&mut (freeCalls), SnString::from("free"))
)
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut staticCalls: i64 = 0;
    let mut staticCalled: SnString = match (10 as u32) {
        10 => {
            (ResultBox::staticResult(&mut (staticCalls)))
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut memberCalls: i64 = 0;
    let mut memberCalled: SnString = match (10 as u8) {
        10 => {
            ((r#box).memberResult(&mut (memberCalls))
)
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut stringMemberCalled: SnString = {
    let __sn_match_subject_3: SnString = SnString::from("upper");
    if (__sn_match_subject_3 == SnString::from("upper")) {
        ((variableResult).to_ascii_uppercase()

)
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut joined: SnString = match (3 as i64) {
        3 => {
            ({ let __sn_array = &(((r#box).rows)[__sn_index(((r#box).rows).len(), 0)]); let __sn_separator = &(SnString::from("+")); __sn_string_join(__sn_array, __sn_separator) }

)
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(concatenated)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(interpolated)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(freeCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(staticCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(memberCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(stringMemberCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(joined)); __sn_interpolated }
))
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&format!("{}", freeCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", staticCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", memberCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(variableResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&((r#box).label)); __sn_interpolated }
))
;
    let mut nestedCalls: i64 = 0;
    let mut nested: SnString = {
    let __sn_match_subject_4: SnString = SnString::from("outer");
    if (__sn_match_subject_4 == SnString::from("outer")) {
        (match (4 as i64) {
        4 => {
            (ownedResult(&mut (nestedCalls), SnString::from("nested"))
)
        },
        _ => {
            (SnString::from("inner-else"))
        },
    })
    }
    else {
        (SnString::from("outer-else"))
    }
};
    let mut fallbackCalls: i64 = 0;
    let mut fallback: SnString = match (99 as i64) {
        1 => {
            (ownedResult(&mut (fallbackCalls), SnString::from("ordinary"))
)
        },
        _ => {
            (ownedResult(&mut (fallbackCalls), SnString::from("fallback"))
)
        },
    };
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(nested)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", nestedCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(fallback)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", fallbackCalls)); __sn_interpolated }
))
;
    let mut returned: SnString = chooseForReturn(false, fallbackResult.clone())
;
    __sn_println_string(&({ let mut __sn_interpolated = SnString::new(); __sn_interpolated.push_str(&(returned)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&(fallbackResult)); __sn_interpolated }
))
;
    let mut escapedDirect: SnString = match (true) {
        true => {
            (SnString::from("direct\n\tquote:\" slash:\\"))
        },
        _ => {
            (SnString::from("wrong"))
        },
    };
    let mut escapedBorrowed: SnString = match (1 as i64) {
        1 => {
            (escapedBorrowedSource.clone())
        },
        _ => {
            (SnString::from("wrong"))
        },
    };
    let mut escapedIndexed: SnString = match (false) {
        true => {
            (SnString::from("wrong"))
        },
        _ => {
            (((escapedRows)[__sn_index((escapedRows).len(), 0)])[__sn_index(((escapedRows)[__sn_index((escapedRows).len(), 0)]).len(), 0)].clone())
        },
    };
    let mut escapedNested: SnString = match (2 as i64) {
        2 => {
            ({
    let __sn_match_subject_5: SnString = SnString::from("nested");
    if (__sn_match_subject_5 == SnString::from("nested")) {
        (SnString::from("nested\n\tquote:\" slash:\\"))
    }
    else {
        (SnString::from("wrong-inner"))
    }
})
        },
        _ => {
            (SnString::from("wrong-outer"))
        },
    };
    __sn_print_string(&(SnString::from("direct[")))
;
    __sn_print_string(&(escapedDirect))
;
    __sn_println_string(&(SnString::from("]")))
;
    __sn_print_string(&(SnString::from("borrowed[")))
;
    __sn_print_string(&(escapedBorrowed))
;
    __sn_println_string(&(SnString::from("]")))
;
    __sn_print_string(&(SnString::from("indexed[")))
;
    __sn_print_string(&(escapedIndexed))
;
    __sn_println_string(&(SnString::from("]")))
;
    __sn_print_string(&(SnString::from("nested[")))
;
    __sn_print_string(&(escapedNested))
;
    __sn_println_string(&(SnString::from("]")))
;
}
