#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Box_int {
    value: i64,
}

fn main() {
    let mut number: i64 = identity_int(42);
    let mut original_word: String = "sindarin".to_string();
    let mut word: String = identity_str(original_word.clone());
    let mut boxed: Box_int = Box_int { value: number };
    let mut boxed_copy: Box_int = boxed;
    ((boxed_copy).value = 99);
    println!("{}", number);
    println!("{}", word);
    println!("{}", original_word);
    println!("{}", (boxed).value);
    println!("{}", (boxed_copy).value);
}

fn identity_int(value: i64) -> i64 {
    return value;
}

fn identity_str(value: String) -> String {
    return value;
}
