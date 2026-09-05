#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn observe(counter: &mut i64) -> char {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return '\u{78}';
}

fn main() {
    let mut type_int: i64 = 8i64;
    let mut type_long: i64 = 8i64;
    let mut type_int32: i64 = 4i64;
    let mut type_uint: i64 = 8i64;
    let mut type_uint32: i64 = 4i64;
    let mut type_byte: i64 = 1i64;
    let mut type_bool: i64 = 1i64;
    let mut type_char: i64 = 1i64;
    let mut type_float: i64 = 4i64;
    let mut type_double: i64 = 8i64;
    let mut integer: i64 = 9;
    let mut single: f32 = 1.5;
    let mut counter: i64 = 0;
    let mut expression_sizes: i64 = ((8i64).checked_add(4i64).expect("checked arithmetic failed")).checked_add(1i64).expect("checked arithmetic failed");
    let mut arithmetic: i64 = (8i64).checked_add((4i64).checked_mul(1i64).expect("checked arithmetic failed")).expect("checked arithmetic failed");
    let mut comparison: bool = (1i64 < 8i64);
    let mut types_ok: bool = ((((((((((type_int == 8) && (type_long == 8)) && (type_int32 == 4)) && (type_uint == 8)) && (type_uint32 == 4)) && (type_byte == 1)) && (type_bool == 1)) && (type_char == 1)) && (type_float == 4)) && (type_double == 8));
    let mut expressions_ok: bool = (expression_sizes == 13);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", types_ok)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", expressions_ok)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((arithmetic == 12) && comparison))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (counter == 0))); __sn_interpolated });
}
