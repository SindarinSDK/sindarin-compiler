#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Debug, PartialEq)]
struct Label {
    text: String,
}
#[derive(Clone, Debug, PartialEq)]
struct Item {
    name: String,
}

impl Item {
    fn describe(&self, suffix: String) -> String {
        return decorate((self).name.clone(), suffix.clone());
    }
    fn copyLabel(&self) -> Label {
        return makeLabel((self).name.clone());
    }
}

fn decorate(value: String, suffix: String) -> String {
    return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("<"); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated.push_str(">"); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated };
}

fn makeLabel(value: String) -> Label {
    return Label { text: value.clone() };
}

fn main() {
    let mut item: Item = Item { name: "source".to_string() };
    let mut suffix: String = "!".to_string();
    let mut label: Label = (item).copyLabel();
    println!("{}", (item).describe(suffix.clone()));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("copy="); __sn_interpolated.push_str(&format!("{}", (label).text)); __sn_interpolated.push_str("; original="); __sn_interpolated.push_str(&format!("{}", (item).name)); __sn_interpolated.push_str("; suffix="); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated });
}
