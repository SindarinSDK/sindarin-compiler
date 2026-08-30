#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_format_string_width(value: &str, width: usize, left_align: bool) -> String {
    let padding = width.saturating_sub(value.len());
    if left_align {
        format!("{}{}", value, " ".repeat(padding))
    } else {
        format!("{}{}", " ".repeat(padding), value)
    }
}

fn main() {
    let mut x: i64 = 42;
    let mut pi: f64 = 3.1415926500000002;
    let mut name: String = "Alice".to_string();
    let mut unicode: String = "é".to_string();
    let mut unsigned: u64 = 42;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("padded="); __sn_interpolated.push_str(&format!("{:05}", x)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("hex="); __sn_interpolated.push_str(&format!("{:x}", 255)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{:X}", 255)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("octal="); __sn_interpolated.push_str(&format!("{:o}", 64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("fixed="); __sn_interpolated.push_str(&format!("{:.2}", pi)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{:.4}", pi)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("default-fixed="); __sn_interpolated.push_str(&format!("{:.6}", pi)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("string=|"); __sn_interpolated.push_str(&__sn_format_string_width(&(name), 10, false)); __sn_interpolated.push_str("|/|"); __sn_interpolated.push_str(&__sn_format_string_width(&(name), 10, true)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unicode=|"); __sn_interpolated.push_str(&__sn_format_string_width(&(unicode), 3, false)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("width=|"); __sn_interpolated.push_str(&format!("{:8.2}", pi)); __sn_interpolated.push_str("|"); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("expr="); __sn_interpolated.push_str(&format!("{:04}", (x).checked_mul(2).expect("checked arithmetic failed"))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("signed="); __sn_interpolated.push_str(&format!("{:+}", x)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unsigned="); __sn_interpolated.push_str(&format!("{:}", unsigned)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("left=|"); __sn_interpolated.push_str(&format!("{:<5}", x)); __sn_interpolated.push_str("|"); __sn_interpolated });
}
