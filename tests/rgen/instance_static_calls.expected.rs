#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Debug, PartialEq)]
struct Formatter {
    prefix: String,
}

impl Formatter {
    fn bracket(value: String) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("["); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated.push_str("]"); __sn_interpolated };
    }
    fn describe(&self, suffix: String) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", Formatter::bracket((self).prefix.clone()))); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated };
    }
}
#[derive(Clone, Debug, PartialEq)]
struct Label {
    text: String,
}

impl Label {
    fn from(value: String) -> Label {
        return Label { text: value.clone() };
    }
    fn copy(&self) -> Label {
        return Label::from((self).text.clone());
    }
}

fn main() {
    let mut formatter: Formatter = Formatter { prefix: "item".to_string() };
    let mut suffix: String = "!".to_string();
    let mut label: Label = Label { text: "source".to_string() };
    let mut copied: Label = (label).copy();
    println!("{}", (formatter).describe(suffix.clone()));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("copy="); __sn_interpolated.push_str(&format!("{}", (copied).text)); __sn_interpolated.push_str("; original="); __sn_interpolated.push_str(&format!("{}", (label).text)); __sn_interpolated.push_str("; suffix="); __sn_interpolated.push_str(&format!("{}", suffix)); __sn_interpolated });
}
