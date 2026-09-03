#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct HygieneIter {
    current: i64,
    remaining: i64,
}

impl HygieneIter {
    fn hasNext(&self) -> bool {
        return ((self).current < (self).remaining);
    }
    fn next(&mut self) -> i64 {
        let mut value: i64 = (self).current;
        ((self).current = ((self).current).checked_add(1).expect("checked arithmetic failed"));
        return value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct HygieneSource {
    limit: i64,
}

impl HygieneSource {
    fn iter(&self) -> HygieneIter {
        return HygieneIter { current: 0, remaining: (self).limit };
    }
}

fn selectSource(calls: &mut i64) -> HygieneSource {
    (*(calls) = (*(calls)).checked_add(1).expect("checked arithmetic failed"));
    return HygieneSource { limit: 2 };
}

fn main() {
    let mut __sn_iter: i64 = 41;
    let mut __sn_iter_0: i64 = 42;
    let mut evaluations: i64 = 0;
    {
    let mut __sn_iter_2 = (selectSource(&mut (evaluations))).iter();
    while __sn_iter_2.hasNext() {
        let mut outer_value = __sn_iter_2.next();
        {
    let mut __sn_iter_1 = (selectSource(&mut (evaluations))).iter();
    while __sn_iter_1.hasNext() {
        let mut inner_value = __sn_iter_1.next();
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("collision="); __sn_interpolated.push_str(&format!("{}", __sn_iter)); __sn_interpolated.push_str(" candidate="); __sn_interpolated.push_str(&format!("{}", __sn_iter_0)); __sn_interpolated.push_str(" outer="); __sn_interpolated.push_str(&format!("{}", outer_value)); __sn_interpolated.push_str(" inner="); __sn_interpolated.push_str(&format!("{}", inner_value)); __sn_interpolated });
        break;
    }
}
        break;
    }
}
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("final="); __sn_interpolated.push_str(&format!("{}", __sn_iter)); __sn_interpolated.push_str(" candidate-final="); __sn_interpolated.push_str(&format!("{}", __sn_iter_0)); __sn_interpolated.push_str(" evaluations="); __sn_interpolated.push_str(&format!("{}", evaluations)); __sn_interpolated });
}
