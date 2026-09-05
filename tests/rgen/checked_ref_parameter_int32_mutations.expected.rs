#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Box {
    value: i32,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Wrapper {
    r#box: Box,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticOps(value: &mut i32) -> i32 {
        let mut add: i32 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut sub: i32 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut mul: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut div: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut rem: i32 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        return (((((((add).checked_add(sub).expect("checked arithmetic failed")).checked_add(mul).expect("checked arithmetic failed")).checked_add(div).expect("checked arithmetic failed")).checked_add(rem).expect("checked arithmetic failed")).checked_add(old_inc).expect("checked arithmetic failed")).checked_add(old_dec).expect("checked arithmetic failed")).checked_add(*(value)).expect("checked arithmetic failed");
    }
    fn staticBump(value: &mut i32) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
    fn instancePostfix(&self, value: &mut i32) -> i32 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
    fn instanceBump(&self, value: &mut i32) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut i32) -> i32 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn freeOps(value: &mut i32) -> i32 {
    let mut add: i32 = { let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut sub: i32 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut mul: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut div: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut rem: i32 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return (((((((add).checked_add(sub).expect("checked arithmetic failed")).checked_add(mul).expect("checked arithmetic failed")).checked_add(div).expect("checked arithmetic failed")).checked_add(rem).expect("checked arithmetic failed")).checked_add(old_inc).expect("checked arithmetic failed")).checked_add(old_dec).expect("checked arithmetic failed")).checked_add(*(value)).expect("checked arithmetic failed");
}

fn forwardStatic(value: &mut i32) -> i32 {
    RefOps::staticBump(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut i32) -> i32 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceBump(&mut *(value));
    return *(value);
}

fn maxBoundary(value: &mut i32) -> i32 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn minBoundary(value: &mut i32) -> i32 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn read(value: &mut i32) -> i32 {
    return *(value);
}

fn freeBump(value: &mut i32) {
    { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut free_value: i32 = 2;
    let mut static_value: i32 = 10;
    let mut postfix_value: i32 = 5;
    let mut forwarded_static: i32 = 1;
    let mut forwarded_instance: i32 = 1;
    let mut maximum: i32 = 2147483646;
    let mut minimum: i32 = (-2147483647);
    let mut direct: Box = Box { value: 1 };
    let mut nested: Wrapper = Wrapper { r#box: Box { value: 10 } };
    let mut ops: RefOps = RefOps {  };
    println!("{}", freeOps(&mut (free_value)));
    println!("{}", free_value);
    println!("{}", RefOps::staticOps(&mut (static_value)));
    println!("{}", static_value);
    println!("{}", (ops).instancePostfix(&mut (postfix_value)));
    println!("{}", postfix_value);
    println!("{}", forwardStatic(&mut (forwarded_static)));
    println!("{}", forwarded_static);
    println!("{}", forwardInstance(&mut (forwarded_instance)));
    println!("{}", forwarded_instance);
    println!("{}", maxBoundary(&mut (maximum)));
    println!("{}", minBoundary(&mut (minimum)));
    freeBump(&mut ((direct).value));
    freeBump(&mut (((nested).r#box).value));
    RefOps::staticBump(&mut ((direct).value));
    RefOps::staticBump(&mut (((nested).r#box).value));
    (ops).instanceBump(&mut ((direct).value));
    (ops).instanceBump(&mut (((nested).r#box).value));
    println!("{}", read(&mut ((direct).value)));
    println!("{}", (direct).value);
    println!("{}", ((nested).r#box).value);
}
