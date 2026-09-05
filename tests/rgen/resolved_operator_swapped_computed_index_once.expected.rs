#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct Item {
    value: i64,
}

impl Item {
    fn op_lt(&self, other: &mut Item) -> bool {
        ((other).value = ((other).value).checked_add(1).expect("checked arithmetic failed"));
        return ((self).value < (other).value);
    }
}

fn makeFrom(item: Item) -> Item {
    return Item { value: ((item).value).checked_add(1).expect("checked arithmetic failed") };
}

fn index(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return 0;
}

fn main() {
    let mut items: Vec<Item> = vec![Item { value: 1 }];
    let mut calls: i64 = 0;
    println!("{}", (makeFrom((items)[__sn_index((items).len(), 0)])).op_lt({ let __sn_resolved_index_0 = __sn_index((items).len(), index(&mut (calls))); &mut (items)[__sn_resolved_index_0] }));
    println!("{}", calls);
    println!("{}", ((items)[__sn_index((items).len(), 0)]).value);
}
