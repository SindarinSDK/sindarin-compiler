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

#[derive(Clone, Debug, PartialEq)]
struct Bag {
    values: Vec<i64>,
}

impl Bag {
    fn observe(&mut self, values: &mut Vec<i64>) -> i64 {
        ((self).values).push(9);
        return (values).len() as i64;
    }
    fn __sn_receiver_array_alias_0(&mut self) -> i64 {
        ((self).values).push(9);
        return ((self).values).len() as i64;
    }
}

fn main() {
    let mut bag: Bag = Bag { values: vec![1] };
    println!("{}", (bag).__sn_receiver_array_alias_0());
    println!("{}", ((bag).values).len() as i64);
}
