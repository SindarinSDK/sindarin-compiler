#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct __SnClosure {
    value: i64,
}

fn main() {
    let mut item: __SnClosure = __SnClosure { value: 7 };
    let mut copy: __SnClosure = item;
    ((copy).value = 9);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (item).value)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copy).value)); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
