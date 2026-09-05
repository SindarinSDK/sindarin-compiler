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

#[derive(Clone, Debug, PartialEq)]
struct Metric {
    name: String,
    value: i64,
}

impl Metric {
    fn getValue(&self) -> i64 {
        return (self).value;
    }
    fn doubled(&self) -> i64 {
        return __sn_checked_0(((self).getValue()).checked_mul(2), "Runtime error: integer overflow in multiplication");
    }
    fn getName(&self) -> String {
        return (self).name.clone();
    }
    fn describe(&self, suffix: String) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (self).getName())); __sn_interpolated.push_str("="); __sn_interpolated.push_str(&format!("{}", (self).doubled())); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated };
    }
}

fn main() {
    let mut metric: Metric = Metric { name: "count".to_string(), value: 21 };
    let mut suffix: String = "!".to_string();
    println!("{}", (metric).describe(suffix.clone()));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unchanged="); __sn_interpolated.push_str(&format!("{}", (metric).name)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated });
}
