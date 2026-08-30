#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_format_string_width(value: &str, width: usize, left_align: bool) -> String {
    let padding = width.saturating_sub(value.len());
    if left_align {
        format!("{}{}", value, " ".repeat(padding))
    } else {
        format!("{}{}", " ".repeat(padding), value)
    }
}

fn __sn_format_scientific(value: f64, precision: usize, uppercase: bool,
                          width: usize, left_align: bool, force_sign: bool,
                          zero_pad: bool) -> String {
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
    let mut x: i64 = 42;
    let mut pi: f64 = 3.1415926500000002;
    let mut name: String = "Alice".to_string();
    let mut unicode: String = "é".to_string();
    let mut unsigned: u64 = 42;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("padded="); __sn_interpolated.push_str(&format!("{:05}", x)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("hex="); __sn_interpolated.push_str(&format!("{:x}", 255)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{:X}", 255)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("octal="); __sn_interpolated.push_str(&format!("{:o}", 64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("fixed="); __sn_interpolated.push_str(&format!("{:.2}", pi)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{:.4}", pi)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("default-fixed="); __sn_interpolated.push_str(&format!("{:.6}", pi)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("string=|"); __sn_interpolated.push_str(&__sn_format_string_width(&(name), 10, false)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&__sn_format_string_width(&(name), 10, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unicode=|"); __sn_interpolated.push_str(&__sn_format_string_width(&(unicode), 3, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&format!("{:8.2}", pi)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("expr="); __sn_interpolated.push_str(&format!("{:04}", (x).checked_mul(2).expect("checked arithmetic failed"))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("signed="); __sn_interpolated.push_str(&format!("{:+}", x)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unsigned="); __sn_interpolated.push_str(&format!("{:}", unsigned)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&format!("{:<5}", x)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
