#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct ScalarAssignments {
    marker: i64,
}

impl ScalarAssignments {
    fn assignInt32(mut value: i32, untouched: i32) -> i32 {
        { value = (value + 4); value };
        return (value + untouched);
    }
    fn assignByte(mut value: u8, untouched: u8) -> u8 {
        let mut assigned: u8 = { value = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (value, 5); __sn_byte_left.wrapping_add(__sn_byte_right) }; value };
        return { let (__sn_byte_left, __sn_byte_right): (u8, u8) = ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (assigned, value); __sn_byte_left.wrapping_add(__sn_byte_right) }, untouched); __sn_byte_left.wrapping_add(__sn_byte_right) };
    }
    fn assignUint32(mut value: u32, untouched: u32) -> u32 {
        return { value = (value + untouched); value };
    }
    fn assignUint(&self, mut value: u64, untouched: u64) -> u64 {
        { value = (value + 7); value };
        return ((value + untouched) + ((self).marker as u64));
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
    (*(calls) = (*(calls) + 1));
    return value;
}

fn assignBool(mut value: bool, untouched: bool) -> bool {
    { value = (!value); value };
    return (value && untouched);
}

fn assignInt(mut value: i64, calls: &mut i64, untouched: i64) -> i64 {
    { value = observeInt(&mut *(calls), (value + 2)); value };
    return (value + untouched);
}

fn assignLong(mut value: i64, untouched: i64) -> i64 {
    let mut assigned: i64 = { value = (value + 3); value };
    return ((assigned + value) + untouched);
}

fn helperNames(mut __sn_rhs: i64, __sn_place: i64, __sn_next: i64) -> i64 {
    let mut assigned: i64 = { __sn_rhs = (__sn_rhs + __sn_place); __sn_rhs };
    if true {
        let mut __sn_next: i64 = 4;
        (__sn_next = 5);
    }
    return ((assigned + __sn_rhs) + __sn_next);
}

fn statementOrder(mut value: i64, delta: i64) -> i64 {
    if true {
        let mut readBefore: i64 = (value + delta);
        { value = (value + 1); value };
        let mut value: i64 = readBefore;
        (value = (value + 10));
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
