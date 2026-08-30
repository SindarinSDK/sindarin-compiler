#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_format_string(value: &str, width: usize, left_align: bool,
                      has_precision: bool, precision: usize) -> String {
    let c_length = value.as_bytes().iter().position(|byte| *byte == 0)
        .unwrap_or(value.len());
    let length = if has_precision {
        c_length.min(precision)
    } else {
        c_length
    };
    if !value.is_char_boundary(length) {
        panic!("Rust target cannot represent C string precision that splits a UTF-8 code point");
    }
    let value = &value[..length];
    let padding = width.saturating_sub(length);
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
    let mut large: f64 = 1234.5;
    let mut small: f64 = 0.00125;
    let mut negative: f64 = (-42.0);
    let mut zero: f64 = 0.0;
    let mut rounded: f64 = 9.9990000000000006;
    let mut single: f32 = 12.5;
    let mut divisor: f64 = 0.0;
    let mut infinity: f64 = (1.0 / divisor);
    let mut negative_infinity: f64 = ((-1.0) / divisor);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("default="); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 6, false, 0, false, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("precision="); __sn_interpolated.push_str(&__sn_format_scientific((small) as f64, 2, false, 0, false, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("uppercase="); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, true, 0, false, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero="); __sn_interpolated.push_str(&__sn_format_scientific((zero) as f64, 3, false, 0, false, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("rounded="); __sn_interpolated.push_str(&__sn_format_scientific((rounded) as f64, 2, false, 0, false, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("float="); __sn_interpolated.push_str(&__sn_format_scientific((single) as f64, 3, true, 0, false, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, false, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, true, false, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("sign=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 0, false, true, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("positive-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, true, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("negative-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, false, 14, false, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("combined=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, true, 14, true, true, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("infinity=|"); __sn_interpolated.push_str(&__sn_format_scientific((infinity) as f64, 2, false, 10, false, true, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("negative-infinity=|"); __sn_interpolated.push_str(&__sn_format_scientific((negative_infinity) as f64, 2, true, 10, true, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
