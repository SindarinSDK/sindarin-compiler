#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticBump(value: &mut i64) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
    fn instanceBump(&self, value: &mut i64) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
}

fn forwardStatic(value: &mut i64) -> i64 {
    RefOps::staticBump(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut i64) -> i64 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceBump(&mut *(value));
    return *(value);
}

fn main() {
    let mut staticValue: i64 = 1;
    let mut instanceValue: i64 = 1;
    println!("{}", forwardStatic(&mut (staticValue)));
    println!("{}", staticValue);
    println!("{}", forwardInstance(&mut (instanceValue)));
    println!("{}", instanceValue);
}
