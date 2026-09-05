#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Counter {
    value: i64,
}

impl Counter {
    fn localMath(&self) -> i64 {
        let mut total: i64 = 4;
        { let __sn_rhs = 3; let __sn_place = &mut (total); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return total;
    }
}

fn r#use(counter: Counter) -> i64 {
    return (counter).localMath();
}

fn main() {
    let mut counter: Counter = Counter { value: 1 };
    println!("{}", r#use(counter));
}
