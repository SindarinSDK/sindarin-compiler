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

fn main() {
    let mut total: i64 = 0;
    {
        let mut i: i64 = 0;

        while (i < 5) {
            (total = (total).checked_add(i).expect("checked arithmetic failed"));
            { let __sn_previous = i; i += 1; __sn_previous };
        }
    }
    println!("{}", total);
    let mut values: Vec<i64> = vec![2, 4, 6];
    for value in (values).iter().cloned() {
        (total = (total).checked_add(value).expect("checked arithmetic failed"));
    }
    println!("{}", total);
    if (total == 22) {
        println!("{}", "matched".to_string());
    }
    let mut countdown: i64 = 2;
    while (countdown > 0) {
        println!("{}", countdown);
        { let __sn_previous = countdown; countdown -= 1; __sn_previous };
    }
    let mut range_values: Vec<i64> = (3..7).collect::<Vec<i64>>();
    println!("{}", (range_values)[__sn_index((range_values).len(), 0)]);
    println!("{}", (range_values)[__sn_index((range_values).len(), (-1))]);
    let mut range_total: i64 = 0;
    for value in ((1..5).collect::<Vec<i64>>()).iter().cloned() {
        (range_total = (range_total).checked_add(value).expect("checked arithmetic failed"));
    }
    println!("{}", range_total);
    let mut names: Vec<String> = vec!["one".to_string(), "two".to_string()];
    for name in (names).iter().cloned() {
        println!("{}", name);
    }
}
