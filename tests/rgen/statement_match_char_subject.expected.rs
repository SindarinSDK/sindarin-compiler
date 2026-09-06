#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    match ('\u{61}' as char) {
        '\u{61}' => {
            println!("{}", "a".to_string());
        },
        _ => {},
    };
}
