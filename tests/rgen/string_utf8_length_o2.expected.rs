#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut ascii: String = "ASCII".to_string();
    let mut accent: String = "é".to_string();
    let mut world: String = "世界".to_string();
    let mut emoji: String = "🙂".to_string();
    let mut decomposed: String = "é".to_string();
    let mut controls: String = "\n\t\r\u{1f}".to_string();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (ascii).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (accent).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (world).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (emoji).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (decomposed).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (controls).len() as i64)); __sn_interpolated });
}
