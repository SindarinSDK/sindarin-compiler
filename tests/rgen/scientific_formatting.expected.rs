#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_format_string_width(value: &str, width: usize, left_align: bool) -> String {
    let padding = width.saturating_sub(value.len());
    if left_align {
        format!("{}{}", value, " ".repeat(padding))
    } else {
        format!("{}{}", " ".repeat(padding), value)
    }
}

fn __sn_format_scientific(value: f64, precision: usize, uppercase: bool) -> String {
    if value.is_nan() {
        return if uppercase { "NAN" } else { "nan" }.to_string();
    }
    if value.is_infinite() {
        let magnitude = if uppercase { "INF" } else { "inf" };
        return if value.is_sign_negative() {
            format!("-{}", magnitude)
        } else {
            magnitude.to_string()
        };
    }

    let rendered = if uppercase {
        format!("{:.*E}", precision, value)
    } else {
        format!("{:.*e}", precision, value)
    };
    let marker = if uppercase { 'E' } else { 'e' };
    let (mantissa, exponent) = rendered.rsplit_once(marker)
        .expect("scientific formatting must contain an exponent");
    let exponent: i32 = exponent.parse().expect("scientific exponent must be numeric");
    format!("{}{}{:+03}", mantissa, marker, exponent)
}

fn main() {
    let mut large: f64 = 1234.5;
    let mut small: f64 = 0.00125;
    let mut negative: f64 = (-42.0);
    let mut zero: f64 = 0.0;
    let mut rounded: f64 = 9.9990000000000006;
    let mut single: f32 = 12.5;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("default="); __sn_interpolated.push_str(&__sn_format_scientific((large) as f64, 6, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("precision="); __sn_interpolated.push_str(&__sn_format_scientific((small) as f64, 2, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("uppercase="); __sn_interpolated.push_str(&__sn_format_scientific((negative) as f64, 1, true)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("zero="); __sn_interpolated.push_str(&__sn_format_scientific((zero) as f64, 3, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("rounded="); __sn_interpolated.push_str(&__sn_format_scientific((rounded) as f64, 2, false)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("float="); __sn_interpolated.push_str(&__sn_format_scientific((single) as f64, 3, true)); __sn_interpolated });
}
