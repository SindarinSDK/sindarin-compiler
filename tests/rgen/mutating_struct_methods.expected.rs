#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error(message),
    }
}

fn __sn_checked_div<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

#[derive(Clone, Debug, PartialEq)]
struct Counter {
    label: String,
    value: i64,
}

impl Counter {
    fn increment(&mut self) {
        ((self).value = __sn_checked(((self).value).checked_add(1), "Runtime error: integer overflow in addition")
);
    }
    fn rename(&mut self, label: String) {
        ((self).label = label.clone());
    }
    fn incrementTwice(&mut self) {
        (self).increment();
        (self).increment();
    }
    fn describe(&self) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (self).label.clone())); __sn_interpolated.push_str("="); __sn_interpolated.push_str(&format!("{}", (self).value)); __sn_interpolated };
    }
}

fn main() {
    let mut counter: Counter = Counter { label: "old".to_string(), value: 5 };
    let mut label: String = "new".to_string();
    (counter).incrementTwice();
    (counter).rename(label.clone());
    println!("{}", (counter).describe());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("source="); __sn_interpolated.push_str(&format!("{}", label)); __sn_interpolated });
}

