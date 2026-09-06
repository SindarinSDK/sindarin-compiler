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

fn mutate(first: &mut Vec<i64>, second: &mut Vec<i64>) -> i64 {
    if true {
        let mut second: Vec<i64> = vec![9];
        (second).push(10);
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("inner:"); __sn_interpolated.push_str(&format!("{}", (second).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 1)])); __sn_interpolated });
    }
    (first).push(2);
    return (first).len() as i64;
}

fn main() {
    let mut values: Vec<i64> = vec![1];
    println!("{}", __sn_array_alias_call_0(&mut (values)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("outer:"); __sn_interpolated.push_str(&format!("{}", (values).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 1)])); __sn_interpolated });
}

fn __sn_array_alias_call_0(first: &mut Vec<i64>) -> i64 {
    if true {
        let mut second: Vec<i64> = vec![9];
        (second).push(10);
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("inner:"); __sn_interpolated.push_str(&format!("{}", (second).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 1)])); __sn_interpolated });
    }
    (first).push(2);
    return (first).len() as i64;
}
