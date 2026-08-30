#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_string_substring(value: &str, start: i64, end: i64) -> String {
    let length = value.len() as i64;
    let start = start.max(0);
    let end = end.min(length);
    if start >= end {
        return String::new();
    }
    value.get(start as usize..end as usize)
        .expect("substring index is not a UTF-8 boundary")
        .to_string()
}

fn __sn_string_replace(value: &str, old: &str, new: &str) -> String {
    if old.is_empty() {
        value.to_string()
    } else {
        value.replace(old, new)
    }
}

fn __sn_string_char_at(value: &str, index: i64) -> char {
    if index < 0 {
        return '\0';
    }
    value.as_bytes().get(index as usize).copied().map(char::from).unwrap_or('\0')
}

fn __sn_string_index_of(value: &str, needle: &str) -> i64 {
    value.find(needle).map(|index| index as i64).unwrap_or(-1)
}

fn decorate(value: String) -> String {
    return { let mut __sn_string = String::new(); __sn_string.push_str(&(value)); __sn_string.push_str(&("!".to_string())); __sn_string };
}

fn main() {
    let mut source: String = "  Hello World  ".to_string();
    let mut assigned: String = source.clone();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("assignedCopy="); __sn_interpolated.push_str(&format!("{}", assigned)); __sn_interpolated });
    (assigned = "changed".to_string());
    let mut explicit_copy: String = (source).clone();
    { let __sn_string_part = (" copy".to_string()).clone(); explicit_copy.push_str(&__sn_string_part); };
    let mut decorated: String = decorate(source.clone());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("source="); __sn_interpolated.push_str(&format!("{}", source)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("assigned="); __sn_interpolated.push_str(&format!("{}", assigned)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("copy="); __sn_interpolated.push_str(&format!("{}", explicit_copy)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("decorated="); __sn_interpolated.push_str(&format!("{}", decorated)); __sn_interpolated });
    let mut hello: String = (source).trim_matches(|value: char| value.is_ascii_whitespace()).to_string();
    let mut joined: String = { let mut __sn_string = String::new(); __sn_string.push_str(&(hello)); __sn_string.push_str(&(" from".to_string())); __sn_string.push_str(&(" Rust".to_string())); __sn_string };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("joined="); __sn_interpolated.push_str(&format!("{}", joined)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("length="); __sn_interpolated.push_str(&format!("{}", (hello).len() as i64)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", (hello).len() as i64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("equal="); __sn_interpolated.push_str(&format!("{}", (hello == "Hello World".to_string()))); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", (hello != source))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("contains="); __sn_interpolated.push_str(&format!("{}", (hello).contains(("lo Wo".to_string()).as_str()))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("starts="); __sn_interpolated.push_str(&format!("{}", (hello).starts_with(("Hell".to_string()).as_str()))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("ends="); __sn_interpolated.push_str(&format!("{}", (hello).ends_with(("World".to_string()).as_str()))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("upper="); __sn_interpolated.push_str(&format!("{}", (hello).to_ascii_uppercase())); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("lower="); __sn_interpolated.push_str(&format!("{}", (hello).to_ascii_lowercase())); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("substring="); __sn_interpolated.push_str(&format!("{}", __sn_string_substring(&(hello), 6, 11))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("replace="); __sn_interpolated.push_str(&format!("{}", __sn_string_replace(&(hello), &("World".to_string()), &("Rust".to_string())))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("emptyReplace="); __sn_interpolated.push_str(&format!("{}", __sn_string_replace(&(hello), &("".to_string()), &("ignored".to_string())))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("char="); __sn_interpolated.push_str(&format!("{}", __sn_string_char_at(&(hello), 1))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("index="); __sn_interpolated.push_str(&format!("{}", __sn_string_index_of(&(hello), &("World".to_string())))); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", __sn_string_index_of(&(hello), &("missing".to_string())))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("chain="); __sn_interpolated.push_str(&format!("{}", __sn_string_replace(&(((source).trim_matches(|value: char| value.is_ascii_whitespace()).to_string()).to_ascii_lowercase()), &("world".to_string()), &("rust".to_string())))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("sourceAgain="); __sn_interpolated.push_str(&format!("{}", source)); __sn_interpolated });
}
