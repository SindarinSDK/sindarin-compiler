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
    let mut positive: i64 = 42;
    let mut negative: i64 = (-42);
    let mut zero_int: i64 = 0;
    let mut unsigned: u64 = 42;
    let mut value: f64 = 1.25;
    let mut negative_value: f64 = (-1.25);
    let mut negative_zero: f64 = (-0.0);
    let mut zero: f64 = 0.0;
    let mut infinity: f64 = (1.0 / zero);
    let mut nan: f64 = (zero / zero);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("decimal=|"); __sn_interpolated.push_str(&{ let __sn_value = positive; let __sn_rendered = format!("{:+}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = negative; let __sn_rendered = format!("{:+}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = zero_int; let __sn_rendered = format!("{:+}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&{ let __sn_value = positive; let __sn_rendered = format!("{:+6}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = negative; let __sn_rendered = format!("{:+6}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero=|"); __sn_interpolated.push_str(&{ let __sn_value = positive; let __sn_rendered = format!("{:+06}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = negative; let __sn_rendered = format!("{:+06}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&{ let __sn_value = positive; let __sn_rendered = format!("{:<+6}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = negative; let __sn_rendered = format!("{:<+6}", __sn_value); if __sn_value < 0 { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("plus-wins=|"); __sn_interpolated.push_str(&format!("{:+6}", positive)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unsigned=|"); __sn_interpolated.push_str(&format!("{:6}", unsigned)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&format!("{:06x}", unsigned)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("fixed=|"); __sn_interpolated.push_str(&{ let __sn_value = value; let __sn_rendered = format!("{:+8.2}", __sn_value); if __sn_value.is_sign_negative() { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = negative_value; let __sn_rendered = format!("{:+8.2}", __sn_value); if __sn_value.is_sign_negative() { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("fixed-zero=|"); __sn_interpolated.push_str(&{ let __sn_value = value; let __sn_rendered = format!("{:+08.2}", __sn_value); if __sn_value.is_sign_negative() { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&{ let __sn_value = negative_zero; let __sn_rendered = format!("{:+08.2}", __sn_value); if __sn_value.is_sign_negative() { __sn_rendered } else { __sn_rendered.replacen('+', " ", 1) } }); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("scientific=|"); __sn_interpolated.push_str(&__sn_format_scientific((value) as f64, 2, false, 12, false, false, true, false)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&__sn_format_scientific((negative_value) as f64, 2, true, 12, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("scientific-zero=|"); __sn_interpolated.push_str(&__sn_format_scientific((value) as f64, 2, false, 12, false, false, true, true)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&__sn_format_scientific((negative_zero) as f64, 2, false, 12, false, false, true, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("special=|"); __sn_interpolated.push_str(&__sn_format_scientific((infinity) as f64, 2, false, 10, false, false, true, false)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&__sn_format_scientific((nan) as f64, 2, true, 10, false, false, true, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
