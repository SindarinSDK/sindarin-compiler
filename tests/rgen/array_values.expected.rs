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
struct Bucket {
    items: Vec<i64>,
}

fn main() {
    let mut values: Vec<i64> = vec![1, 2, 3];
    println!("{}", (values).len() as i64);
    println!("{}", (values).len() as i64);
    println!("{}", (values)[__sn_index((values).len(), (-1))]);
    { let __sn_array_index = __sn_index((values).len(), 0); (values)[__sn_array_index] = 10; };
    (values).push(4);
    { let __sn_array_index = __sn_insert_index((values).len(), 1); (values).insert(__sn_array_index, 20); };
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    println!("{}", (values)[__sn_index((values).len(), 1)]);
    let mut removed: i64 = { let __sn_array_index = __sn_index((values).len(), 2); (values).remove(__sn_array_index) };
    let mut last: i64 = (values).pop().expect("pop from empty array");
    println!("{}", removed);
    println!("{}", last);
    (values).reverse();
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    let mut copied: Vec<i64> = values.clone();
    (copied).push(99);
    println!("{}", (values).len() as i64);
    println!("{}", (copied).len() as i64);
    (copied).clear();
    println!("{}", (copied).len() as i64);
    let mut sized: Vec<i64> = vec![0; __sn_array_size(3)];
    { let __sn_array_index = __sn_index((sized).len(), 1); (sized)[__sn_array_index] = 7; };
    println!("{}", (sized)[__sn_index((sized).len(), 0)]);
    println!("{}", (sized)[__sn_index((sized).len(), 1)]);
    let mut names: Vec<String> = vec!["alpha".to_string(), "beta".to_string()];
    let mut picked: String = (names)[__sn_index((names).len(), 0)].clone();
    { let __sn_array_index = __sn_index((names).len(), 0); (names)[__sn_array_index] = "changed".to_string(); };
    (names).push(picked.clone());
    println!("{}", picked);
    println!("{}", (names)[__sn_index((names).len(), 0)]);
    println!("{}", (names)[__sn_index((names).len(), (-1))]);
    let mut bucket: Bucket = Bucket { items: vec![5, 6] };
    let mut bucket_copy: Bucket = bucket.clone();
    ((bucket_copy).items).push(7);
    println!("{}", ((bucket).items).len() as i64);
    println!("{}", ((bucket_copy).items)[__sn_index(((bucket_copy).items).len(), (-1))]);
}
