#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
    let mut value: i64 = 26;
    let mut zero: i64 = 0;
    let mut negative: i64 = (-1);
    let mut narrow_negative: i32 = (-1);
    let mut unsigned: u32 = 4294967295;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("plain="); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 0, false, false) }); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:X}", __sn_value), __sn_value == 0, true, false, 0, false, false) }); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:o}", __sn_value), __sn_value == 0, false, true, 0, false, false) }); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero="); __sn_interpolated.push_str(&{ let __sn_value = zero; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 0, false, false) }); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&{ let __sn_value = zero; __sn_format_integer_alternate(&format!("{:X}", __sn_value), __sn_value == 0, true, false, 0, false, false) }); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&{ let __sn_value = zero; __sn_format_integer_alternate(&format!("{:o}", __sn_value), __sn_value == 0, false, true, 0, false, false) }); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 8, false, false) }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:o}", __sn_value), __sn_value == 0, false, true, 8, false, false) }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:X}", __sn_value), __sn_value == 0, true, false, 8, true, false) }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:o}", __sn_value), __sn_value == 0, false, true, 8, true, false) }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero-pad=|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 8, false, true) }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:o}", __sn_value), __sn_value == 0, false, true, 8, false, true) }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("negative="); __sn_interpolated.push_str(&{ let __sn_value = negative; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 0, false, false) }); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&{ let __sn_value = narrow_negative; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 0, false, false) }); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unsigned="); __sn_interpolated.push_str(&{ let __sn_value = unsigned; __sn_format_integer_alternate(&format!("{:X}", __sn_value), __sn_value == 0, true, false, 0, false, false) }); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("space=|"); __sn_interpolated.push_str(&{ let __sn_value = value; __sn_format_integer_alternate(&format!("{:x}", __sn_value), __sn_value == 0, false, false, 8, false, false) }); __sn_interpolated.push_str("|"); __sn_interpolated });
}
