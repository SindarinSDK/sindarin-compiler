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

fn mutate(first: &mut Vec<i64>, second: &mut Vec<i64>) -> i64 {
    (first).push(2);
    (second).push((first).len() as i64);
    return (first).len() as i64;
}

fn forward(left: &mut Vec<i64>, right: &mut Vec<i64>) -> i64 {
    return mutate(&mut *(left), &mut *(right));
}

fn main() {
    let mut bag: Bag = Bag { values: vec![1] };
    println!("{}", __sn_array_alias_call_0(&mut ((bag).values)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((bag).values).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((bag).values)[__sn_index(((bag).values).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((bag).values)[__sn_index(((bag).values).len(), 2)])); __sn_interpolated });
}

fn __sn_array_alias_call_0(left: &mut Vec<i64>) -> i64 {
    return __sn_array_alias_call_1(&mut *(left));
}

fn __sn_array_alias_call_1(first: &mut Vec<i64>) -> i64 {
    (first).push(2);
    (first).push((first).len() as i64);
    return (first).len() as i64;
}
