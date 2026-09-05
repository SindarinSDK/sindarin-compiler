#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Debug, PartialEq)]
struct Metric {
    name: String,
    value: i64,
}

impl Metric {
    fn getValue(&self) -> i64 {
        return (self).value;
    }
    fn doubled(&self) -> i64 {
        return ((self).getValue()).checked_mul(2).expect("checked arithmetic failed");
    }
    fn getName(&self) -> String {
        return (self).name.clone();
    }
    fn describe(&self, suffix: String) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (self).getName())); __sn_interpolated.push_str("="); __sn_interpolated.push_str(&format!("{}", (self).doubled())); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated };
    }
}

fn main() {
    let mut metric: Metric = Metric { name: "count".to_string(), value: 21 };
    let mut suffix: String = "!".to_string();
    println!("{}", (metric).describe(suffix.clone()));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("unchanged="); __sn_interpolated.push_str(&format!("{}", (metric).name)); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated });
}
