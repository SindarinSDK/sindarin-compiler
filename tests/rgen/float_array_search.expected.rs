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

fn observeReceiver(calls: &mut i64) -> Vec<f32> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return vec![0.0, 1.5, 1.5];
}

fn observeNeedle(calls: &mut i64, value: f32) -> f32 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn main() {
    let mut positive_zero: f32 = 0.0;
    let mut negative_zero: f32 = (-0.0);
    let mut nan: f32 = (positive_zero / positive_zero);
    let mut copied_nan: f32 = nan;
    let mut values: Vec<f32> = vec![positive_zero, 1.5, 1.5, nan];
    let mut empty: Vec<f32> = vec![];
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (1.5 as f32).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (9.5 as f32).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (1.5 as f32).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (9.5 as f32).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(empty); let __sn_array_search = (1.5 as f32).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(empty); let __sn_array_search = (1.5 as f32).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (negative_zero as f32).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (negative_zero as f32).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (copied_nan as f32).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (copied_nan as f32).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut receiver_calls: i64 = 0;
    let mut needle_calls: i64 = 0;
    println!("{}", { let __sn_array = &(observeReceiver(&mut (receiver_calls))); let __sn_array_search = (observeNeedle(&mut (needle_calls), 1.5) as f32).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", receiver_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", needle_calls)); __sn_interpolated });
    println!("{}", { let __sn_array = &(observeReceiver(&mut (receiver_calls))); let __sn_array_search = (observeNeedle(&mut (needle_calls), 1.5) as f32).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", receiver_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", needle_calls)); __sn_interpolated });
}
