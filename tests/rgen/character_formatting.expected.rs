#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_string_substring(value: &str, start: i64, end: i64) -> String {
    let length = value.len() as i64;
    let start = start.max(0);
    let end = end.min(length);
    if start >= end {
        return String::new();
    }
    value.get(start as usize..end as usize)
        .expect("substring index is not a UTF-8 boundary")
        .to_string()
}

fn __sn_string_replace(value: &str, old: &str, new: &str) -> String {
    if old.is_empty() {
        value.to_string()
    } else {
        value.replace(old, new)
    }
}

fn __sn_string_char_at(value: &str, index: i64) -> char {
    if index < 0 {
        return '\0';
    }
    value.as_bytes().get(index as usize).copied().map(char::from).unwrap_or('\0')
}

fn __sn_string_index_of(value: &str, needle: &str) -> i64 {
    value.find(needle).map(|index| index as i64).unwrap_or(-1)
}

fn __sn_format_string_width(value: &str, width: usize, left_align: bool) -> String {
    let padding = width.saturating_sub(value.len());
    if left_align {
        format!("{}{}", value, " ".repeat(padding))
    } else {
        format!("{}{}", " ".repeat(padding), value)
    }
}

fn __sn_format_character(value: char, width: usize, left_align: bool) -> String {
    if !value.is_ascii() {
        panic!("Rust target cannot represent non-ASCII C character interpolation");
    }
    let padding = width.saturating_sub(1);
    if value == '\0' {
        return if left_align { String::new() } else { " ".repeat(padding) };
    }
    if left_align {
        format!("{}{}", value, " ".repeat(padding))
    } else {
        format!("{}{}", " ".repeat(padding), value)
    }
}

fn __sn_format_scientific(value: f64, precision: usize, uppercase: bool,
                          width: usize, left_align: bool, force_sign: bool,
                          space_sign: bool, zero_pad: bool) -> String {
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
    let mut letter: char = '\u{41}';
    let mut tab: char = '\u{9}';
    let mut nul: char = '\u{0}';
    let mut word: String = "byte".to_string();
    let mut dynamic: char = __sn_string_char_at(&(word), 1);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("plain=|"); __sn_interpolated.push_str(&__sn_format_character(letter, 0, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&__sn_format_character(letter, 5, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&__sn_format_character(letter, 5, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("dynamic=|"); __sn_interpolated.push_str(&__sn_format_character(dynamic, 3, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("tab=|"); __sn_interpolated.push_str(&__sn_format_character(tab, 0, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("nul=|"); __sn_interpolated.push_str(&__sn_format_character(nul, 0, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("nul-width=|"); __sn_interpolated.push_str(&__sn_format_character(nul, 5, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("nul-left=|"); __sn_interpolated.push_str(&__sn_format_character(nul, 5, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
