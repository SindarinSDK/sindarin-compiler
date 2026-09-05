#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut pattern: String = "one".to_string();
    {
    let __sn_match_subject_0: String = "one".to_string();
    if (__sn_match_subject_0.as_str() == (pattern).as_str()) {
        println!("{}", "matched".to_string());
    }
    else {
        println!("{}", "other".to_string());
    }
};
}
