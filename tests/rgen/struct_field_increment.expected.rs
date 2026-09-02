#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Counter {
    value: i64,
}

impl Counter {
    fn next(&mut self) -> i64 {
        return { let __sn_place = &mut ((self).value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
    fn previous(&mut self) -> i64 {
        return { let __sn_place = &mut ((self).value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
    fn advance(&mut self) -> i64 {
        return (self).next();
    }
}

fn main() {
    let mut counter: Counter = Counter { value: 5 };
    let mut beforeIncrement: i64 = (counter).advance();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("increment="); __sn_interpolated.push_str(&format!("{}", beforeIncrement)); __sn_interpolated.push_str("->"); __sn_interpolated.push_str(&format!("{}", (counter).value)); __sn_interpolated });
    let mut beforeDecrement: i64 = (counter).previous();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("decrement="); __sn_interpolated.push_str(&format!("{}", beforeDecrement)); __sn_interpolated.push_str("->"); __sn_interpolated.push_str(&format!("{}", (counter).value)); __sn_interpolated });
}
