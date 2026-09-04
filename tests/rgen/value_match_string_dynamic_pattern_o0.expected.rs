#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut pattern: String = "one".to_string();
    let mut result: i64 = {
    let __sn_match_subject_0: String = "one".to_string();
    if (__sn_match_subject_0.as_str() == (pattern).as_str()) {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", result);
}
