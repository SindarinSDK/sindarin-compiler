#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    value: i64,
}

impl Point {
    fn op_lt(&self, other: Point) -> bool {
        return ((self).value < (other).value);
    }
}

fn marked(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> Point {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return Point { value: value };
}

fn main() {
    let mut calls: i64 = 0;
    let mut order: i64 = 0;
    println!("{}", (marked(&mut (calls), &mut (order), 1, 1)).op_lt(marked(&mut (calls), &mut (order), 2, 2)));
    println!("{}", (marked(&mut (calls), &mut (order), 4, 4)).op_lt(marked(&mut (calls), &mut (order), 3, 3)));
    println!("{}", calls);
    println!("{}", order);
}
