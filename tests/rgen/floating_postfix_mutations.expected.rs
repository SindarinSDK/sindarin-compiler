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
struct FloatingPostfixValues {
    single: f32,
    precise: f64,
}

impl FloatingPostfixValues {
    fn mutateSelf(&mut self) -> bool {
        let mut before_increment: f64 = { let __sn_place = &mut ((self).precise); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        let mut before_decrement: f64 = { let __sn_place = &mut ((self).precise); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        return (((before_increment == 6.0) && (before_decrement == 7.0)) && ((self).precise == 6.0));
    }
}

fn main() {
    let mut single: f32 = 1.5;
    let mut single_before_increment: f32 = { let __sn_place = &mut (single); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut single_before_decrement: f32 = { let __sn_place = &mut (single); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    println!("{}", (((single_before_increment == 1.5) && (single_before_decrement == 2.5)) && (single == 1.5)));
    let mut precise: f64 = 3.25;
    let mut precise_before_increment: f64 = { let __sn_place = &mut (precise); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut precise_before_decrement: f64 = { let __sn_place = &mut (precise); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    println!("{}", (((precise_before_increment == 3.25) && (precise_before_decrement == 4.25)) && (precise == 3.25)));
    let mut fields: FloatingPostfixValues = FloatingPostfixValues { single: 2.0, precise: 6.0 };
    let mut field_single_before: f32 = { let __sn_place = &mut ((fields).single); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut field_precise_before: f64 = { let __sn_place = &mut ((fields).precise); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    println!("{}", ((((field_single_before == 2.0) && ((fields).single == 3.0)) && (field_precise_before == 6.0)) && ((fields).precise == 5.0)));
    { let __sn_place = &mut ((fields).precise); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    println!("{}", (fields).mutateSelf());
    let mut singles: Vec<f32> = vec![4.0];
    for mut value in (singles).iter().cloned() {
        let mut before: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        println!("{}", (((before == 4.0) && (value == 5.0)) && ((singles)[__sn_index((singles).len(), 0)] == 4.0)));
    }
    let mut doubles: Vec<f64> = vec![9.0];
    for mut value in (doubles).iter().cloned() {
        let mut before: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        println!("{}", (((before == 9.0) && (value == 8.0)) && ((doubles)[__sn_index((doubles).len(), 0)] == 9.0)));
    }
    let mut __sn_place: f32 = 10.0;
    let mut place_before: f32 = { let __sn_place = &mut (__sn_place); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut __sn_previous: f64 = 12.0;
    let mut previous_before: f64 = { let __sn_place = &mut (__sn_previous); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut __sn_next: f32 = 14.0;
    let mut next_before: f32 = { let __sn_place = &mut (__sn_next); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    println!("{}", ((((((place_before == 10.0) && (__sn_place == 11.0)) && (previous_before == 12.0)) && (__sn_previous == 11.0)) && (next_before == 14.0)) && (__sn_next == 15.0)));
}
