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
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    bags: Vec<Bag>,
}

impl Holder {
    fn replacement(&mut self) -> Vec<i64> {
        println!("{}", "rhs".to_string());
        ((self).bags).push(Bag { values: vec![3] });
        return vec![7, 8];
    }
    fn selectedIndex(&self) -> i64 {
        println!("{}", "index".to_string());
        return (-1);
    }
    fn replaceSelected(&mut self) {
        { let __sn_place_value_1 = (self).replacement(); let __sn_place_raw_index_0 = (self).selectedIndex(); (({ let __sn_place_owner_0 = &mut ((self).bags); let __sn_place_index_0 = __sn_index(__sn_place_owner_0.len(), __sn_place_raw_index_0); &mut __sn_place_owner_0[__sn_place_index_0] }).values = __sn_place_value_1); };
    }
}
#[derive(Clone, Debug, PartialEq)]
struct Shelf {
    rows: Vec<Holder>,
}

impl Shelf {
    fn nestedReplacement(&mut self) -> Vec<i64> {
        println!("{}", "nested rhs".to_string());
        ((self).rows).push(Holder { bags: vec![Bag { values: vec![3] }] });
        return vec![9, 10];
    }
    fn rowIndex(&self) -> i64 {
        println!("{}", "row index".to_string());
        return (-1);
    }
    fn bagIndex(&self) -> i64 {
        println!("{}", "bag index".to_string());
        return (-1);
    }
    fn replaceNested(&mut self) {
        { let __sn_place_value_4 = (self).nestedReplacement(); let __sn_place_raw_index_2 = (self).rowIndex(); let __sn_place_raw_index_3 = (self).bagIndex(); (({ let __sn_place_owner_3 = &mut (({ let __sn_place_owner_2 = &mut ((self).rows); let __sn_place_index_2 = __sn_index(__sn_place_owner_2.len(), __sn_place_raw_index_2); &mut __sn_place_owner_2[__sn_place_index_2] }).bags); let __sn_place_index_3 = __sn_index(__sn_place_owner_3.len(), __sn_place_raw_index_3); &mut __sn_place_owner_3[__sn_place_index_3] }).values = __sn_place_value_4); };
    }
}

fn main() {
    let mut holder: Holder = Holder { bags: vec![Bag { values: vec![1, 2] }] };
    (holder).replaceSelected();
    println!("{}", ((((holder).bags)[__sn_index(((holder).bags).len(), 0)]).values)[__sn_index(((((holder).bags)[__sn_index(((holder).bags).len(), 0)]).values).len(), 0)]);
    println!("{}", ((((holder).bags)[__sn_index(((holder).bags).len(), 0)]).values)[__sn_index(((((holder).bags)[__sn_index(((holder).bags).len(), 0)]).values).len(), 1)]);
    println!("{}", ((((holder).bags)[__sn_index(((holder).bags).len(), 1)]).values)[__sn_index(((((holder).bags)[__sn_index(((holder).bags).len(), 1)]).values).len(), 0)]);
    println!("{}", ((((holder).bags)[__sn_index(((holder).bags).len(), 1)]).values)[__sn_index(((((holder).bags)[__sn_index(((holder).bags).len(), 1)]).values).len(), 1)]);
    let mut shelf: Shelf = Shelf { rows: vec![Holder { bags: vec![Bag { values: vec![4, 5] }] }] };
    (shelf).replaceNested();
    println!("{}", ((((((shelf).rows)[__sn_index(((shelf).rows).len(), 0)]).bags)[__sn_index(((((shelf).rows)[__sn_index(((shelf).rows).len(), 0)]).bags).len(), 0)]).values)[__sn_index(((((((shelf).rows)[__sn_index(((shelf).rows).len(), 0)]).bags)[__sn_index(((((shelf).rows)[__sn_index(((shelf).rows).len(), 0)]).bags).len(), 0)]).values).len(), 0)]);
    println!("{}", ((((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags)[__sn_index(((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags).len(), 0)]).values)[__sn_index(((((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags)[__sn_index(((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags).len(), 0)]).values).len(), 0)]);
    println!("{}", ((((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags)[__sn_index(((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags).len(), 0)]).values)[__sn_index(((((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags)[__sn_index(((((shelf).rows)[__sn_index(((shelf).rows).len(), 1)]).bags).len(), 0)]).values).len(), 1)]);
}
