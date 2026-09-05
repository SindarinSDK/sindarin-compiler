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

#[derive(Clone, Copy, Debug, PartialEq)]
struct ScalarAssignments {
    marker: i64,
}

impl ScalarAssignments {
    fn assignInt32(mut value: i32, untouched: i32) -> i32 {
        { value = __sn_checked_0((value).checked_add(4), "Runtime error: integer overflow in addition"); value };
        return __sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition");
    }
    fn assignByte(mut value: u8, untouched: u8) -> u8 {
        let mut assigned: u8 = { value = __sn_checked_0((value).checked_add(5), "Runtime error: integer overflow in addition"); value };
        return __sn_checked_0((__sn_checked_0((assigned).checked_add(value), "Runtime error: integer overflow in addition")).checked_add(untouched), "Runtime error: integer overflow in addition");
    }
    fn assignUint32(mut value: u32, untouched: u32) -> u32 {
        return { value = __sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition"); value };
    }
    fn assignUint(&self, mut value: u64, untouched: u64) -> u64 {
        { value = __sn_checked_0((value).checked_add(7), "Runtime error: integer overflow in addition"); value };
        return __sn_checked_0((__sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition")).checked_add(((self).marker as u64)), "Runtime error: integer overflow in addition");
    }
    fn assignFloat(&self, mut value: f32, untouched: f32) -> f32 {
        let mut assigned: f32 = { value = (value + 1.5); value };
        return ((assigned + value) + untouched);
    }
    fn assignDouble(&self, mut value: f64, untouched: f64) -> f64 {
        return { value = (value + untouched); value };
    }
}

fn observeInt(calls: &mut i64, value: i64) -> i64 {
    (*(calls) = __sn_checked_0((*(calls)).checked_add(1), "Runtime error: integer overflow in addition"));
    return value;
}

fn assignBool(mut value: bool, untouched: bool) -> bool {
    { value = (!value); value };
    return (value && untouched);
}

fn assignInt(mut value: i64, calls: &mut i64, untouched: i64) -> i64 {
    { value = observeInt(&mut *(calls), __sn_checked_0((value).checked_add(2), "Runtime error: integer overflow in addition")); value };
    return __sn_checked_0((value).checked_add(untouched), "Runtime error: integer overflow in addition");
}

fn assignLong(mut value: i64, untouched: i64) -> i64 {
    let mut assigned: i64 = { value = __sn_checked_0((value).checked_add(3), "Runtime error: integer overflow in addition"); value };
    return __sn_checked_0((__sn_checked_0((assigned).checked_add(value), "Runtime error: integer overflow in addition")).checked_add(untouched), "Runtime error: integer overflow in addition");
}

fn helperNames(mut __sn_rhs: i64, __sn_place: i64, __sn_next: i64) -> i64 {
    let mut assigned: i64 = { __sn_rhs = __sn_checked_0((__sn_rhs).checked_add(__sn_place), "Runtime error: integer overflow in addition"); __sn_rhs };
    if true {
        let mut __sn_next: i64 = 4;
        (__sn_next = 5);
    }
    return __sn_checked_0((__sn_checked_0((assigned).checked_add(__sn_rhs), "Runtime error: integer overflow in addition")).checked_add(__sn_next), "Runtime error: integer overflow in addition");
}

fn statementOrder(mut value: i64, delta: i64) -> i64 {
    if true {
        let mut readBefore: i64 = __sn_checked_0((value).checked_add(delta), "Runtime error: integer overflow in addition");
        { value = __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition"); value };
        let mut value: i64 = readBefore;
        (value = __sn_checked_0((value).checked_add(10), "Runtime error: integer overflow in addition"));
    }
    return value;
}

fn main() {
    let mut boolCaller: bool = false;
    let mut intCaller: i64 = 10;
    let mut longCaller: i64 = 20;
    let mut int32Caller: i32 = 30;
    let mut byteCaller: u8 = 40;
    let mut uint32Caller: u32 = 50;
    let mut uintCaller: u64 = 60;
    let mut floatCaller: f32 = 2.0;
    let mut doubleCaller: f64 = 3.0;
    let mut calls: i64 = 0;
    let mut ops: ScalarAssignments = ScalarAssignments { marker: 1 };
    let mut boolResult: bool = assignBool(boolCaller, true);
    let mut intResult: i64 = assignInt(intCaller, &mut (calls), 1);
    let mut longResult: i64 = assignLong(longCaller, 1);
    let mut int32Result: i32 = ScalarAssignments::assignInt32(int32Caller, 1);
    let mut byteResult: u8 = ScalarAssignments::assignByte(byteCaller, 1);
    let mut uint32Result: u32 = ScalarAssignments::assignUint32(uint32Caller, 2);
    let mut uintResult: u64 = (ops).assignUint(uintCaller, 2);
    let mut floatResult: f32 = (ops).assignFloat(floatCaller, 0.5);
    let mut doubleResult: f64 = (ops).assignDouble(doubleCaller, 0.25);
    let mut helperResult: i64 = helperNames(1, 2, 3);
    let mut orderCaller: i64 = 4;
    let mut orderResult: i64 = statementOrder(orderCaller, 2);
    println!("{}", (boolResult && (!boolCaller)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", intResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", intCaller)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", longResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", longCaller)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", int32Result)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", int32Caller)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", byteResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", byteCaller)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", uint32Result)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", uint32Caller)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", uintResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", uintCaller)); __sn_interpolated });
    println!("{}", ((floatResult == 7.5) && (floatCaller == 2.0)));
    println!("{}", ((doubleResult == 3.25) && (doubleCaller == 3.0)));
    println!("{}", helperResult);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", orderResult)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", orderCaller)); __sn_interpolated });
}
