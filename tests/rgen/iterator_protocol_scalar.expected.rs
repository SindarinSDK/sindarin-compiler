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
struct TraceIter {
    current: i64,
    remaining: i64,
    has_next_calls: i64,
    next_calls: i64,
}

impl TraceIter {
    fn hasNext(&mut self) -> bool {
        ((self).has_next_calls = ((self).has_next_calls).checked_add(1).expect("checked arithmetic failed"));
        return ((self).next_calls < (self).remaining);
    }
    fn next(&mut self) -> i64 {
        let mut value: i64 = ((((self).has_next_calls).checked_mul(100).expect("checked arithmetic failed")).checked_add(((self).next_calls).checked_mul(10).expect("checked arithmetic failed")).expect("checked arithmetic failed")).checked_add((self).current).expect("checked arithmetic failed");
        ((self).current = ((self).current).checked_add(1).expect("checked arithmetic failed"));
        ((self).next_calls = ((self).next_calls).checked_add(1).expect("checked arithmetic failed"));
        return value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct TraceSource {
    start: i64,
    count: i64,
}

impl TraceSource {
    fn iter(&self) -> TraceIter {
        return TraceIter { current: (self).start, remaining: (self).count, has_next_calls: 0, next_calls: 0 };
    }
}

fn selectSource(calls: &mut i64) -> i64 {
    (*(calls) = (*(calls)).checked_add(1).expect("checked arithmetic failed"));
    return 0;
}

fn main() {
    let mut sources: Vec<TraceSource> = vec![TraceSource { start: 7, count: 4 }];
    let mut evaluations: i64 = 0;
    let mut sum: i64 = 0;
    {
    let mut __sn_iter_0 = ((sources)[__sn_index((sources).len(), selectSource(&mut (evaluations)))]).iter();
    while __sn_iter_0.hasNext() {
        let mut value = __sn_iter_0.next();
        let mut produced: i64 = value;
        (value = (-1));
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("value="); __sn_interpolated.push_str(&format!("{}", produced)); __sn_interpolated.push_str(" binding="); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
        if (produced == 218) {
        continue;
    }
        (sum = (sum).checked_add(produced).expect("checked arithmetic failed"));
    }
}
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("natural evaluations="); __sn_interpolated.push_str(&format!("{}", evaluations)); __sn_interpolated.push_str(" sum="); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated });
    {
    let mut __sn_iter_1 = ((sources)[__sn_index((sources).len(), selectSource(&mut (evaluations)))]).iter();
    while __sn_iter_1.hasNext() {
        let mut value = __sn_iter_1.next();
        if (value == 329) {
        break;
    }
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("before-break="); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
    }
}
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("total evaluations="); __sn_interpolated.push_str(&format!("{}", evaluations)); __sn_interpolated.push_str(" source="); __sn_interpolated.push_str(&format!("{}", ((sources)[__sn_index((sources).len(), 0)]).start)); __sn_interpolated });
    let mut outer: i64 = 0;
    {
        let mut r#loop: i64 = 0;

        while (outer < 1) {
            {
    let mut __sn_iter_2 = ((sources)[__sn_index((sources).len(), selectSource(&mut (evaluations)))]).iter();
    while __sn_iter_2.hasNext() {
        let mut value = __sn_iter_2.next();
        if (value == 107) {
        continue;
    }
    }
}
            { let __sn_place = &mut (outer); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("nested outer="); __sn_interpolated.push_str(&format!("{}", outer)); __sn_interpolated.push_str(" evaluations="); __sn_interpolated.push_str(&format!("{}", evaluations)); __sn_interpolated });
}
