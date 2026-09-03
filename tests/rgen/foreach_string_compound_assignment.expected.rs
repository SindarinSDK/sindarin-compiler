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

fn suffix(calls: &mut i64) -> String {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("!"); __sn_interpolated.push_str(&format!("{}", *(calls))); __sn_interpolated };
}

fn main() {
    let mut names: Vec<String> = vec!["alpha".to_string(), "beta".to_string(), "gamma".to_string()];
    let mut calls: i64 = 0;
    let mut index: i64 = 0;
    for mut __sn_string_place in (names).iter().cloned() {
        let mut appended: String = { let (__sn_string_part, __sn_string_place) = ((suffix(&mut (calls))).clone(), &mut (__sn_string_place)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", appended)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", __sn_string_place)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (names)[__sn_index((names).len(), index)])); __sn_interpolated });
        { let __sn_rhs = 1; let __sn_place = &mut (index); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    }
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("source="); __sn_interpolated.push_str(&format!("{}", (names)[__sn_index((names).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (names)[__sn_index((names).len(), 1)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (names)[__sn_index((names).len(), 2)])); __sn_interpolated.push_str(" calls="); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated });
}
