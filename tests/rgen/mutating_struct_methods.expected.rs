#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Debug, PartialEq)]
struct Counter {
    label: String,
    value: i64,
}

impl Counter {
    fn increment(&mut self) {
        ((self).value = ((self).value).checked_add(1).expect("checked arithmetic failed"));
    }
    fn rename(&mut self, label: String) {
        ((self).label = label.clone());
    }
    fn incrementTwice(&mut self) {
        (self).increment();
        (self).increment();
    }
    fn describe(&self) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (self).label.clone())); __sn_interpolated.push_str("="); __sn_interpolated.push_str(&format!("{}", (self).value)); __sn_interpolated };
    }
}

fn main() {
    let mut counter: Counter = Counter { label: "old".to_string(), value: 5 };
    let mut label: String = "new".to_string();
    (counter).incrementTwice();
    (counter).rename(label.clone());
    println!("{}", (counter).describe());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("source="); __sn_interpolated.push_str(&format!("{}", label)); __sn_interpolated });
}
