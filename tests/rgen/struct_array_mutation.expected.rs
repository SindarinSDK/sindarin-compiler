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
    fn add(&mut self, value: i64) {
        ((self).values).push(value);
    }
    fn addPair(&mut self, first: i64, second: i64) {
        (self).add(first);
        (self).add(second);
    }
    fn reverse(&mut self) {
        ((self).values).reverse();
    }
    fn removeMiddle(&mut self) {
        { let __sn_array_index = __sn_index(((self).values).len(), 1); ((self).values).remove(__sn_array_index) };
    }
    fn size(&self) -> i64 {
        return ((self).values.clone()).len() as i64;
    }
}

fn main() {
    let mut bag: Bag = Bag { values: vec![] };
    (bag).add(1);
    (bag).addPair(2, 3);
    (bag).reverse();
    (bag).removeMiddle();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("values="); __sn_interpolated.push_str(&format!("{:?}", (bag).values)); __sn_interpolated.push_str("; size="); __sn_interpolated.push_str(&format!("{}", (bag).size())); __sn_interpolated });
}
