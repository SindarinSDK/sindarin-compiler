#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    value: i64,
}

impl Point {
    fn op_eq(&self, other: Point) -> bool {
        return ((self).value == (other).value);
    }
    fn op_lt(&self, other: Point) -> bool {
        return ((self).value < (other).value);
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Explicit {
    value: i64,
}

impl Explicit {
    fn op_ne(&self, other: Explicit) -> bool {
        return ((self).value != (other).value);
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Holder {
    point: Point,
}
#[derive(Clone, Debug, PartialEq)]
struct OwnedPoint {
    label: String,
    values: Vec<i64>,
}

impl OwnedPoint {
    fn op_eq(&self, other: &mut OwnedPoint) -> bool {
        return (((self).label.clone() == (other).label.clone()) && (((self).values.clone()).len() as i64 == ((other).values.clone()).len() as i64));
    }
    fn op_lt(&self, other: &mut OwnedPoint) -> bool {
        return (((self).values.clone())[__sn_index(((self).values.clone()).len(), 0)] < ((other).values.clone())[__sn_index(((other).values.clone()).len(), 0)]);
    }
}
#[derive(Clone, Debug, PartialEq)]
struct CallValues {
    label: String,
}

impl CallValues {
    fn labelMatches(value: String) -> bool {
        return (value == "source".to_string());
    }
    fn makeLabel() -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("source"); __sn_interpolated };
    }
    fn countNumbers(values: Vec<i64>) -> i64 {
        return (values).len() as i64;
    }
    fn makeNumbers() -> Vec<i64> {
        return vec![1, 2];
    }
    fn joinLabel(&self, value: String) -> String {
        return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (self).label.clone())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated };
    }
    fn countNumbersAgain(&self, values: Vec<i64>) -> i64 {
        return (values).len() as i64;
    }
    fn makeNumbersAgain(&self) -> Vec<i64> {
        return vec![3, 4];
    }
}

fn markedPoint(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> Point {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return Point { value: value };
}

fn markedOwnedPoint(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> OwnedPoint {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return OwnedPoint { label: { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("value-"); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated }, values: vec![value] };
}

fn acceptBool(value: bool) -> bool {
    return value;
}

fn returnedComparison(left: Point, right: Point) -> bool {
    return (left).op_eq(right.clone());
}

fn main() {
    let mut left: Point = Point { value: 7 };
    let mut same: Point = Point { value: 7 };
    let mut greater: Point = Point { value: 9 };
    let mut holder: Holder = Holder { point: Point { value: 7 } };
    let mut points: Vec<Point> = vec![];
    (points).push(left);
    (points).push(greater);
    (left).op_eq(same.clone());
    let mut initialized: bool = (left).op_eq(same.clone());
    let mut argument: bool = acceptBool((!(left).op_eq(greater.clone())));
    let mut returned: bool = returnedComparison(left, same);
    let mut memberReceiver: bool = ((holder).point).op_eq(same.clone());
    let mut indexedReceiver: bool = ((points)[__sn_index((points).len(), 0)]).op_lt((points)[__sn_index((points).len(), 1)].clone());
    let mut explicitNe: bool = (Explicit { value: 1 }).op_ne(Explicit { value: 2 });
    let mut derivedGe: bool = (!(greater).op_lt(left.clone()));
    let mut derivedLe: bool = (!{ let __sn_resolved_arg_1 = left.clone(); let __sn_resolved_receiver_1 = &mut (greater); (__sn_resolved_receiver_1).op_lt(__sn_resolved_arg_1) });
    let mut matched: bool = match (true) {
        true => {
            (!(left).op_eq(greater.clone()));
            ((left).op_eq(same.clone()))
        },
        _ => {
            (false)
        },
    };
    let mut calls: i64 = 0;
    let mut order: i64 = 0;
    let mut __sn_resolved_arg_0: i64 = 40;
    let mut __sn_resolved_receiver_0: i64 = 2;
    let mut directOrder: bool = (markedPoint(&mut (calls), &mut (order), 1, 1)).op_lt(markedPoint(&mut (calls), &mut (order), 2, 2));
    let mut swappedOrder: bool = { let __sn_resolved_arg_2 = markedPoint(&mut (calls), &mut (order), 3, 3); let mut __sn_resolved_receiver_2 = markedPoint(&mut (calls), &mut (order), 4, 4); (__sn_resolved_receiver_2).op_lt(__sn_resolved_arg_2) };
    let mut ownedLeft: OwnedPoint = OwnedPoint { label: "owned".to_string(), values: vec![5, 6] };
    let mut ownedSame: OwnedPoint = ownedLeft.clone();
    let mut ownedEqual: bool = (ownedLeft).op_eq(&mut (ownedSame));
    let mut ownedSwapped: bool = { let mut __sn_resolved_arg_3 = markedOwnedPoint(&mut (calls), &mut (order), 5, 5); let mut __sn_resolved_receiver_3 = markedOwnedPoint(&mut (calls), &mut (order), 6, 6); (__sn_resolved_receiver_3).op_lt(&mut (__sn_resolved_arg_3)) };
    ((ownedSame).values).push(7);
    let mut sourceLabel: String = "source".to_string();
    let mut sourceNumbers: Vec<i64> = vec![1, 2];
    let mut callValues: CallValues = CallValues { label: "prefix".to_string() };
    let mut staticMatch: bool = CallValues::labelMatches(sourceLabel.clone());
    let mut staticLabel: String = CallValues::makeLabel();
    let mut instanceLabel: String = (callValues).joinLabel(sourceLabel.clone());
    let mut staticCount: i64 = CallValues::countNumbers(sourceNumbers.clone());
    let mut instanceCount: i64 = (callValues).countNumbersAgain(sourceNumbers.clone());
    let mut staticNumbers: Vec<i64> = CallValues::makeNumbers();
    let mut instanceNumbers: Vec<i64> = (callValues).makeNumbersAgain();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", initialized)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", argument)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", returned)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", memberReceiver)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", indexedReceiver)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", explicitNe)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", derivedGe)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", derivedLe)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", matched)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", directOrder)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", swappedOrder)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", ownedEqual)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", ownedSwapped)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (__sn_resolved_arg_0).checked_add(__sn_resolved_receiver_0).expect("checked arithmetic failed"))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (ownedLeft).label)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", ((ownedLeft).values).len() as i64)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", ((ownedSame).values).len() as i64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", staticMatch)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", staticLabel)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", instanceLabel)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", sourceLabel)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (sourceNumbers).len() as i64)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", staticCount)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", instanceCount)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (staticNumbers).len() as i64)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (instanceNumbers).len() as i64)); __sn_interpolated });
}
