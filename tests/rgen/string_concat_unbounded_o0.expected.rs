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


fn main() {
    let mut left33: bool = {
    let __sn_match_subject_0: SnString = SnString::from("L33:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|L33:p006|L33:p007|L33:p008|L33:p009|L33:p010|L33:p011|L33:p012|L33:p013|L33:p014|L33:p015|L33:p016|L33:p017|L33:p018|L33:p019|L33:p020|L33:p021|L33:p022|L33:p023|L33:p024|L33:p025|L33:p026|L33:p027|L33:p028|L33:p029|L33:p030|L33:p031|L33:boundary32|");
    if (__sn_match_subject_0 == SnString::from("L33:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|L33:p006|L33:p007|L33:p008|L33:p009|L33:p010|L33:p011|L33:p012|L33:p013|L33:p014|L33:p015|L33:p016|L33:p017|L33:p018|L33:p019|L33:p020|L33:p021|L33:p022|L33:p023|L33:p024|L33:p025|L33:p026|L33:p027|L33:p028|L33:p029|L33:p030|L33:p031|L33:boundary32|")) {
        (true)
    }
    else {
        (false)
    }
};
    let mut right33: bool = {
    let __sn_match_subject_1: SnString = SnString::from("R33:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|R33:p006|R33:p007|R33:p008|R33:p009|R33:p010|R33:p011|R33:p012|R33:p013|R33:p014|R33:p015|R33:p016|R33:p017|R33:p018|R33:p019|R33:p020|R33:p021|R33:p022|R33:p023|R33:p024|R33:p025|R33:p026|R33:p027|R33:p028|R33:p029|R33:p030|R33:p031|R33:boundary32|");
    if (__sn_match_subject_1 == SnString::from("R33:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|R33:p006|R33:p007|R33:p008|R33:p009|R33:p010|R33:p011|R33:p012|R33:p013|R33:p014|R33:p015|R33:p016|R33:p017|R33:p018|R33:p019|R33:p020|R33:p021|R33:p022|R33:p023|R33:p024|R33:p025|R33:p026|R33:p027|R33:p028|R33:p029|R33:p030|R33:p031|R33:boundary32|")) {
        (true)
    }
    else {
        (false)
    }
};
    let mut left40: bool = {
    let __sn_match_subject_2: SnString = SnString::from("L40:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|L40:p006|L40:p007|L40:p008|L40:p009|L40:p010|L40:p011|L40:p012|L40:p013|L40:p014|L40:p015|L40:p016|L40:p017|L40:p018|L40:p019|L40:p020|L40:p021|L40:p022|L40:p023|L40:p024|L40:p025|L40:p026|L40:p027|L40:p028|L40:p029|L40:p030|L40:p031|L40:boundary32|L40:p033|L40:p034|L40:p035|L40:p036|L40:p037|L40:p038|L40:boundary39|");
    if (__sn_match_subject_2 == SnString::from("L40:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|L40:p006|L40:p007|L40:p008|L40:p009|L40:p010|L40:p011|L40:p012|L40:p013|L40:p014|L40:p015|L40:p016|L40:p017|L40:p018|L40:p019|L40:p020|L40:p021|L40:p022|L40:p023|L40:p024|L40:p025|L40:p026|L40:p027|L40:p028|L40:p029|L40:p030|L40:p031|L40:boundary32|L40:p033|L40:p034|L40:p035|L40:p036|L40:p037|L40:p038|L40:boundary39|")) {
        (true)
    }
    else {
        (false)
    }
};
    let mut right40: bool = {
    let __sn_match_subject_3: SnString = SnString::from("R40:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|R40:p006|R40:p007|R40:p008|R40:p009|R40:p010|R40:p011|R40:p012|R40:p013|R40:p014|R40:p015|R40:p016|R40:p017|R40:p018|R40:p019|R40:p020|R40:p021|R40:p022|R40:p023|R40:p024|R40:p025|R40:p026|R40:p027|R40:p028|R40:p029|R40:p030|R40:p031|R40:boundary32|R40:p033|R40:p034|R40:p035|R40:p036|R40:p037|R40:p038|R40:boundary39|");
    if (__sn_match_subject_3 == SnString::from("R40:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|R40:p006|R40:p007|R40:p008|R40:p009|R40:p010|R40:p011|R40:p012|R40:p013|R40:p014|R40:p015|R40:p016|R40:p017|R40:p018|R40:p019|R40:p020|R40:p021|R40:p022|R40:p023|R40:p024|R40:p025|R40:p026|R40:p027|R40:p028|R40:p029|R40:p030|R40:p031|R40:boundary32|R40:p033|R40:p034|R40:p035|R40:p036|R40:p037|R40:p038|R40:boundary39|")) {
        (true)
    }
    else {
        (false)
    }
};
    let mut left96: bool = {
    let __sn_match_subject_4: SnString = SnString::from("L96:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|L96:p006|L96:p007|L96:p008|L96:p009|L96:p010|L96:p011|L96:p012|L96:p013|L96:p014|L96:p015|L96:p016|L96:p017|L96:p018|L96:p019|L96:p020|L96:p021|L96:p022|L96:p023|L96:p024|L96:p025|L96:p026|L96:p027|L96:p028|L96:p029|L96:p030|L96:p031|L96:boundary32|L96:p033|L96:p034|L96:p035|L96:p036|L96:p037|L96:p038|L96:boundary39|L96:p040|L96:p041|L96:p042|L96:p043|L96:p044|L96:p045|L96:p046|L96:p047|L96:p048|L96:p049|L96:p050|L96:p051|L96:p052|L96:p053|L96:p054|L96:p055|L96:p056|L96:p057|L96:p058|L96:p059|L96:p060|L96:p061|L96:p062|L96:p063|L96:p064|L96:p065|L96:p066|L96:p067|L96:p068|L96:p069|L96:p070|L96:p071|L96:p072|L96:p073|L96:p074|L96:p075|L96:p076|L96:p077|L96:p078|L96:p079|L96:p080|L96:p081|L96:p082|L96:p083|L96:p084|L96:p085|L96:p086|L96:p087|L96:p088|L96:p089|L96:p090|L96:p091|L96:p092|L96:p093|L96:p094|L96:last95|");
    if (__sn_match_subject_4 == SnString::from("L96:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|L96:p006|L96:p007|L96:p008|L96:p009|L96:p010|L96:p011|L96:p012|L96:p013|L96:p014|L96:p015|L96:p016|L96:p017|L96:p018|L96:p019|L96:p020|L96:p021|L96:p022|L96:p023|L96:p024|L96:p025|L96:p026|L96:p027|L96:p028|L96:p029|L96:p030|L96:p031|L96:boundary32|L96:p033|L96:p034|L96:p035|L96:p036|L96:p037|L96:p038|L96:boundary39|L96:p040|L96:p041|L96:p042|L96:p043|L96:p044|L96:p045|L96:p046|L96:p047|L96:p048|L96:p049|L96:p050|L96:p051|L96:p052|L96:p053|L96:p054|L96:p055|L96:p056|L96:p057|L96:p058|L96:p059|L96:p060|L96:p061|L96:p062|L96:p063|L96:p064|L96:p065|L96:p066|L96:p067|L96:p068|L96:p069|L96:p070|L96:p071|L96:p072|L96:p073|L96:p074|L96:p075|L96:p076|L96:p077|L96:p078|L96:p079|L96:p080|L96:p081|L96:p082|L96:p083|L96:p084|L96:p085|L96:p086|L96:p087|L96:p088|L96:p089|L96:p090|L96:p091|L96:p092|L96:p093|L96:p094|L96:last95|")) {
        (true)
    }
    else {
        (false)
    }
};
    let mut right96: bool = {
    let __sn_match_subject_5: SnString = SnString::from("R96:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|R96:p006|R96:p007|R96:p008|R96:p009|R96:p010|R96:p011|R96:p012|R96:p013|R96:p014|R96:p015|R96:p016|R96:p017|R96:p018|R96:p019|R96:p020|R96:p021|R96:p022|R96:p023|R96:p024|R96:p025|R96:p026|R96:p027|R96:p028|R96:p029|R96:p030|R96:p031|R96:boundary32|R96:p033|R96:p034|R96:p035|R96:p036|R96:p037|R96:p038|R96:boundary39|R96:p040|R96:p041|R96:p042|R96:p043|R96:p044|R96:p045|R96:p046|R96:p047|R96:p048|R96:p049|R96:p050|R96:p051|R96:p052|R96:p053|R96:p054|R96:p055|R96:p056|R96:p057|R96:p058|R96:p059|R96:p060|R96:p061|R96:p062|R96:p063|R96:p064|R96:p065|R96:p066|R96:p067|R96:p068|R96:p069|R96:p070|R96:p071|R96:p072|R96:p073|R96:p074|R96:p075|R96:p076|R96:p077|R96:p078|R96:p079|R96:p080|R96:p081|R96:p082|R96:p083|R96:p084|R96:p085|R96:p086|R96:p087|R96:p088|R96:p089|R96:p090|R96:p091|R96:p092|R96:p093|R96:p094|R96:last95|");
    if (__sn_match_subject_5 == SnString::from("R96:start|quote:\"|slash:\\|line:\n|tab:\t|utf8:héllo-世界-🙂|R96:p006|R96:p007|R96:p008|R96:p009|R96:p010|R96:p011|R96:p012|R96:p013|R96:p014|R96:p015|R96:p016|R96:p017|R96:p018|R96:p019|R96:p020|R96:p021|R96:p022|R96:p023|R96:p024|R96:p025|R96:p026|R96:p027|R96:p028|R96:p029|R96:p030|R96:p031|R96:boundary32|R96:p033|R96:p034|R96:p035|R96:p036|R96:p037|R96:p038|R96:boundary39|R96:p040|R96:p041|R96:p042|R96:p043|R96:p044|R96:p045|R96:p046|R96:p047|R96:p048|R96:p049|R96:p050|R96:p051|R96:p052|R96:p053|R96:p054|R96:p055|R96:p056|R96:p057|R96:p058|R96:p059|R96:p060|R96:p061|R96:p062|R96:p063|R96:p064|R96:p065|R96:p066|R96:p067|R96:p068|R96:p069|R96:p070|R96:p071|R96:p072|R96:p073|R96:p074|R96:p075|R96:p076|R96:p077|R96:p078|R96:p079|R96:p080|R96:p081|R96:p082|R96:p083|R96:p084|R96:p085|R96:p086|R96:p087|R96:p088|R96:p089|R96:p090|R96:p091|R96:p092|R96:p093|R96:p094|R96:last95|")) {
        (true)
    }
    else {
        (false)
    }
};
    println!("{}", left33)
;
    println!("{}", right33)
;
    println!("{}", left40)
;
    println!("{}", right40)
;
    println!("{}", left96)
;
    println!("{}", right96)
;
    println!("{}", (((((left33 && right33) && left40) && right40) && left96) && right96))
;
}
