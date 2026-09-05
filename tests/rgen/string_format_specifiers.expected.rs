#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("string=|"); __sn_interpolated.push_str(&__sn_format_string(&(name), 10, false, false, 0)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&__sn_format_string(&(name), 10, true, false, 0)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unicode=|"); __sn_interpolated.push_str(&__sn_format_string(&(unicode), 3, false, false, 0)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&format!("{:8.2}", pi)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("expr="); __sn_interpolated.push_str(&format!("{:04}", __sn_checked_0((x).checked_mul(2), "Runtime error: integer overflow in multiplication"))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("signed="); __sn_interpolated.push_str(&format!("{:+}", x)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unsigned="); __sn_interpolated.push_str(&format!("{:}", unsigned)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&format!("{:<5}", x)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
