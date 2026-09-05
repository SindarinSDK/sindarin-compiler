#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Counter {
    value: i64,
}

impl Counter {
    fn addOne(&mut self) -> i64 {
        return { let __sn_rhs = 1; let __sn_place = &mut ((self).value); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
}

fn main() {
    let mut counter: Counter = Counter { value: 4 };
    println!("{}", (counter).addOne());
}
