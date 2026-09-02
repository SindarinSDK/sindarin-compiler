#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn longOps(value: &mut i64) -> i64 {
        let mut add: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut sub: i64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut mul: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut div: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut rem: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        return (((((((add).checked_add(sub).expect("checked arithmetic failed")).checked_add(mul).expect("checked arithmetic failed")).checked_add(div).expect("checked arithmetic failed")).checked_add(rem).expect("checked arithmetic failed")).checked_add(old_inc).expect("checked arithmetic failed")).checked_add(old_dec).expect("checked arithmetic failed")).checked_add(*(value)).expect("checked arithmetic failed");
    }
    fn intPostfix(&self, value: &mut i64) -> i64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
    fn longPostfix(&self, value: &mut i64) -> i64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
    fn intCompound(&self, value: &mut i64) -> i64 {
        return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
    fn longCompound(&self, value: &mut i64) -> i64 {
        return { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut i64) -> i64 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn intOps(value: &mut i64) -> i64 {
    let mut add: i64 = { let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut sub: i64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut mul: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut div: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut rem: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return (((((((add).checked_add(sub).expect("checked arithmetic failed")).checked_add(mul).expect("checked arithmetic failed")).checked_add(div).expect("checked arithmetic failed")).checked_add(rem).expect("checked arithmetic failed")).checked_add(old_inc).expect("checked arithmetic failed")).checked_add(old_dec).expect("checked arithmetic failed")).checked_add(*(value)).expect("checked arithmetic failed");
}

fn main() {
    let mut integer: i64 = 2;
    let mut long_value: i64 = 10;
    let mut ops: RefOps = RefOps {  };
    println!("{}", intOps(&mut (integer)));
    println!("{}", integer);
    println!("{}", RefOps::longOps(&mut (long_value)));
    println!("{}", long_value);
    println!("{}", (ops).intPostfix(&mut (integer)));
    println!("{}", integer);
    println!("{}", (ops).longPostfix(&mut (long_value)));
    println!("{}", long_value);
    println!("{}", (ops).intCompound(&mut (integer)));
    println!("{}", integer);
    println!("{}", (ops).longCompound(&mut (long_value)));
    println!("{}", long_value);
}
