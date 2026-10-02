#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Box {
    value: u64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Wrapper {
    r#box: Box,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticOps(value: &mut u64) -> u64 {
        let mut add: u64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut sub: u64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut mul: u64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut div: u64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut rem: u64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: u64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: u64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        return (((((((add).checked_add(sub).expect("checked arithmetic failed")).checked_add(mul).expect("checked arithmetic failed")).checked_add(div).expect("checked arithmetic failed")).checked_add(rem).expect("checked arithmetic failed")).checked_add(old_inc).expect("checked arithmetic failed")).checked_add(old_dec).expect("checked arithmetic failed")).checked_add(*(value)).expect("checked arithmetic failed");
    }
    fn staticBump(value: &mut u64) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
    fn instancePostfix(&self, value: &mut u64) -> u64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
    fn instanceBump(&self, value: &mut u64) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut u64) -> u64 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn freeOps(value: &mut u64) -> u64 {
    let mut add: u64 = { let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut sub: u64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut mul: u64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut div: u64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut rem: u64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: u64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: u64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return (((((((add).checked_add(sub).expect("checked arithmetic failed")).checked_add(mul).expect("checked arithmetic failed")).checked_add(div).expect("checked arithmetic failed")).checked_add(rem).expect("checked arithmetic failed")).checked_add(old_inc).expect("checked arithmetic failed")).checked_add(old_dec).expect("checked arithmetic failed")).checked_add(*(value)).expect("checked arithmetic failed");
}

fn forwardStatic(value: &mut u64) -> u64 {
    RefOps::staticBump(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut u64) -> u64 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceBump(&mut *(value));
    return *(value);
}

fn maxBoundary(value: &mut u64) -> u64 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn minBoundary(value: &mut u64) -> u64 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn assignAndRead(value: &mut u64, assigned: u64) -> u64 {
    (*(value) = assigned);
    return *(value);
}

fn read(value: &mut u64) -> u64 {
    return *(value);
}

fn freeBump(value: &mut u64) {
    { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut free_value: u64 = 2;
    let mut static_value: u64 = 10;
    let mut postfix_value: u64 = 5;
    let mut forwarded_static: u64 = 1;
    let mut forwarded_instance: u64 = 1;
    let mut half: u64 = 9223372036854775807;
    let mut high: u64 = (half).checked_add(1).expect("checked arithmetic failed");
    let mut max_minus_one: u64 = (half).checked_mul(2).expect("checked arithmetic failed");
    let mut max: u64 = (max_minus_one).checked_add(1).expect("checked arithmetic failed");
    let mut maximum: u64 = max_minus_one;
    let mut minimum: u64 = 1;
    let mut assigned: u64 = 42;
    let mut direct: Box = Box { value: 1 };
    let mut nested: Wrapper = Wrapper { r#box: Box { value: 10 } };
    let mut ops: RefOps = RefOps {  };
    freeBump(&mut ((direct).value));
    freeBump(&mut (((nested).r#box).value));
    RefOps::staticBump(&mut ((direct).value));
    RefOps::staticBump(&mut (((nested).r#box).value));
    (ops).instanceBump(&mut ((direct).value));
    (ops).instanceBump(&mut (((nested).r#box).value));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", freeOps(&mut (free_value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", free_value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", RefOps::staticOps(&mut (static_value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", static_value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (ops).instancePostfix(&mut (postfix_value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix_value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwardStatic(&mut (forwarded_static)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwarded_static)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwardInstance(&mut (forwarded_instance)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwarded_instance)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (maxBoundary(&mut (maximum)) == max))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (maximum == max))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", minBoundary(&mut (minimum)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (assignAndRead(&mut (assigned), high) == high))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (assigned == high))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", read(&mut ((direct).value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (direct).value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((nested).r#box).value)); __sn_interpolated });
}
