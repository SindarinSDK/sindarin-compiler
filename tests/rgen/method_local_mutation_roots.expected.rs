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
struct Pair {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Counter {
    value: i64,
}

impl Counter {
    fn localMutations(&self) -> i64 {
        let mut number: i64 = 1;
        { let __sn_place = &mut (number); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        { let __sn_place = &mut (number); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        let mut pair: Pair = Pair { value: 0 };
        ((pair).value = 4);
        let mut values: Vec<i64> = vec![1, 2];
        { let __sn_array_index = __sn_index((values).len(), 0); (values)[__sn_array_index] = 3; };
        (values).push(4);
        return (((number).checked_add((pair).value).expect("checked arithmetic failed")).checked_add((values)[__sn_index((values).len(), 0)]).expect("checked arithmetic failed")).checked_add((values).len() as i64).expect("checked arithmetic failed");
    }
}

fn r#use(counter: Counter) -> i64 {
    return (counter).localMutations();
}

fn main() {
    let mut counter: Counter = Counter { value: 0 };
    println!("{}", r#use(counter));
}
