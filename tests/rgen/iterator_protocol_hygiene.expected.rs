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

#[derive(Clone, Copy, Debug, PartialEq)]
struct HygieneIter {
    current: i64,
    remaining: i64,
}

impl HygieneIter {
    fn hasNext(&self) -> bool {
        return ((self).current < (self).remaining)
;
    }
    fn next(&mut self) -> i64 {
        let mut value: i64 = (self).current;
        ((self).current = __sn_checked(((self).current).checked_add(1), "Runtime error: integer overflow in addition")
);
        return value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct HygieneSource {
    limit: i64,
}

impl HygieneSource {
    fn iter(&self) -> HygieneIter {
        return HygieneIter { current: 0, remaining: (self).limit };
    }
}

fn selectSource(calls: &mut i64) -> HygieneSource {
    (*(calls) = __sn_checked((*(calls)).checked_add(1), "Runtime error: integer overflow in addition")
);
    return HygieneSource { limit: 2 };
}

fn main() {
    let mut __sn_iter: i64 = 41;
    let mut __sn_iter_0: i64 = 42;
    let mut evaluations: i64 = 0;
    {
    let mut __sn_iter_2 = (selectSource(&mut (evaluations))).iter();
    while __sn_iter_2.hasNext() {
        let mut outer_value = __sn_iter_2.next();
        {
    let mut __sn_iter_1 = (selectSource(&mut (evaluations))).iter();
    while __sn_iter_1.hasNext() {
        let mut inner_value = __sn_iter_1.next();
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("collision="); __sn_interpolated.push_str(&format!("{}", __sn_iter)); __sn_interpolated.push_str(" candidate="); __sn_interpolated.push_str(&format!("{}", __sn_iter_0)); __sn_interpolated.push_str(" outer="); __sn_interpolated.push_str(&format!("{}", outer_value)); __sn_interpolated.push_str(" inner="); __sn_interpolated.push_str(&format!("{}", inner_value)); __sn_interpolated });
        break;
    }
}
        break;
    }
}
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("final="); __sn_interpolated.push_str(&format!("{}", __sn_iter)); __sn_interpolated.push_str(" candidate-final="); __sn_interpolated.push_str(&format!("{}", __sn_iter_0)); __sn_interpolated.push_str(" evaluations="); __sn_interpolated.push_str(&format!("{}", evaluations)); __sn_interpolated });
}

