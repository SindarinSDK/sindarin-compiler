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
    let mut large: f64 = 1234.5;
    let mut small: f64 = 0.00125;
    let mut negative: f64 = (-42.0);
    let mut zero: f64 = 0.0;
    let mut rounded: f64 = 9.9990000000000006;
    let mut single: f32 = 12.5;
    let mut divisor: f64 = 0.0;
    let mut infinity: f64 = (1.0 / divisor);
    let mut negative_infinity: f64 = ((-1.0) / divisor);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("default="); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 6, false, 0, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("precision="); __sn_interpolated.push_str(&__sn_format_scientific((small) as f64, 2, false, 0, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("uppercase="); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, true, 0, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero="); __sn_interpolated.push_str(&__sn_format_scientific((zero) as f64, 3, false, 0, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("rounded="); __sn_interpolated.push_str(&__sn_format_scientific((rounded) as f64, 2, false, 0, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("float="); __sn_interpolated.push_str(&__sn_format_scientific((single) as f64, 3, true, 0, false, false, false, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, true, false, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("sign=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 0, false, true, false, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, false, false, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("positive-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, false, 14, false, true, false, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("negative-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, false, 14, false, false, false, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("combined=|"); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 2, true, 14, true, true, false, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("infinity=|"); __sn_interpolated.push_str(&__sn_format_scientific((infinity) as f64, 2, false, 10, false, true, false, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("negative-infinity=|"); __sn_interpolated.push_str(&__sn_format_scientific((negative_infinity) as f64, 2, true, 10, true, false, false, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
