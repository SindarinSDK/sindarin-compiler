#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Numbers {
    i: i64,
    l: i64,
    i32: i32,
    u: u64,
    u32: u32,
    b: u8,
}

fn main() {
    let mut i: i64 = 10;
    let mut one: i64 = 1;
    let mut two: i64 = 2;
    let mut three: i64 = 3;
    { let __sn_rhs = two; let __sn_place = &mut (i); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = one; let __sn_place = &mut (i); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = two; let __sn_place = &mut (i); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = two; let __sn_place = &mut (i); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = three; let __sn_place = &mut (i); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut i_before: i64 = { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut fields: Numbers = Numbers { i: 10, l: 10, i32: 10, u: 10, u32: 10, b: 10 };
    { let __sn_rhs = two; let __sn_place = &mut ((fields).i); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = one; let __sn_place = &mut ((fields).i); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = two; let __sn_place = &mut ((fields).i); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = two; let __sn_place = &mut ((fields).i); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = three; let __sn_place = &mut ((fields).i); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut field_i_before: i64 = { let __sn_place = &mut ((fields).i); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).i); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut l: i64 = 1;
    { let __sn_rhs = 1; let __sn_place = &mut (l); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut l_before: i64 = { let __sn_place = &mut (l); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (l); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_rhs = 1; let __sn_place = &mut ((fields).l); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut field_l_before: i64 = { let __sn_place = &mut ((fields).l); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).l); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut i32: i32 = 1;
    { let __sn_rhs = 1; let __sn_place = &mut (i32); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut i32_before: i32 = { let __sn_place = &mut (i32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (i32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_rhs = 1; let __sn_place = &mut ((fields).i32); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut field_i32_before: i32 = { let __sn_place = &mut ((fields).i32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).i32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut u: u64 = 1;
    { let __sn_rhs = 1; let __sn_place = &mut (u); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut u_before: u64 = { let __sn_place = &mut (u); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (u); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_rhs = 1; let __sn_place = &mut ((fields).u); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut field_u_before: u64 = { let __sn_place = &mut ((fields).u); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).u); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut u32: u32 = 1;
    { let __sn_rhs = 1; let __sn_place = &mut (u32); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut u32_before: u32 = { let __sn_place = &mut (u32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (u32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_rhs = 1; let __sn_place = &mut ((fields).u32); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut field_u32_before: u32 = { let __sn_place = &mut ((fields).u32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).u32); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    let mut b: u8 = 1;
    { let __sn_rhs = 1; let __sn_place = &mut (b); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut b_before: u8 = { let __sn_place = &mut (b); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (b); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_rhs = 1; let __sn_place = &mut ((fields).b); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    let mut field_b_before: u8 = { let __sn_place = &mut ((fields).b); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).b); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("i="); __sn_interpolated.push_str(&format!("{}", i_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", i)); __sn_interpolated.push_str(" field="); __sn_interpolated.push_str(&format!("{}", field_i_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).i)); __sn_interpolated.push_str("; other="); __sn_interpolated.push_str(&format!("{}", l_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", l)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_l_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).l)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", i32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", i32)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_i32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).i32)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", u_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", u)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_u_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).u)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", u32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", u32)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_u32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).u32)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", b_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", b)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_b_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).b)); __sn_interpolated });
}
