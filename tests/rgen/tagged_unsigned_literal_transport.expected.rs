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
struct Box {
    value: u32,
}

fn carry(value: u32) -> u32 {
    return value;
}

fn produce() -> u32 {
    return (-(1 as i64) as u32);
}

fn main() {
    let mut stored: u32 = (-(1 as i64) as u32);
    let mut values: Vec<u32> = vec![(-(1 as i64) as u32)];
    let mut r#box: Box = Box { value: (-(1 as i64) as u32) };
    let mut nested: u32 = (-(!(1 as i64) as i64) as u32);
    (stored = (-(2 as i64) as u32));
    { let __sn_array_index = __sn_index((values).len(), 0); (values)[__sn_array_index] = (-(2 as i64) as u32); };
    ((r#box).value = (-(2 as i64) as u32));
    println!("{}", stored);
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    println!("{}", (r#box).value);
    println!("{}", nested);
    println!("{}", carry((-(1 as i64) as u32)));
    println!("{}", produce());
    println!("{}", ((stored as i64) == (-(1 as i64) as i64)));
    println!("{}", ((carry((-(1 as i64) as u32)) as i64) == (-(1 as i64) as i64)));
    print!("{}", -(1 as i64));
    println!("{}", "".to_string());
}
