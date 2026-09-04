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
struct IntSequence {
    value: i64,
    remaining: i64,
    has_next_calls: i64,
    next_calls: i64,
}

impl IntSequence {
    fn iter(&self) -> IntSequence {
        return self.clone();
    }
    fn hasNext(&mut self) -> bool {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).has_next_calls); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return ((self).next_calls < (self).remaining);
    }
    fn next(&mut self) -> i64 {
        let mut result: i64 = ((((self).has_next_calls).checked_mul(100).expect("checked arithmetic failed")).checked_add(((self).next_calls).checked_mul(10).expect("checked arithmetic failed")).expect("checked arithmetic failed")).checked_add((self).value).expect("checked arithmetic failed");
        { let __sn_rhs = 1; let __sn_place = &mut ((self).next_calls); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return result;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct LongSequence {
    value: i64,
    remaining: i64,
}

impl LongSequence {
    fn iter(&self) -> LongSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> i64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Int32Sequence {
    value: i32,
    remaining: i64,
}

impl Int32Sequence {
    fn iter(&self) -> Int32Sequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> i32 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct ByteSequence {
    value: u8,
    remaining: i64,
}

impl ByteSequence {
    fn iter(&self) -> ByteSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u8 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Uint32Sequence {
    value: u32,
    remaining: i64,
}

impl Uint32Sequence {
    fn iter(&self) -> Uint32Sequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u32 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct UintSequence {
    value: u64,
    remaining: i64,
}

impl UintSequence {
    fn iter(&self) -> UintSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct FloatSequence {
    value: f32,
    remaining: i64,
}

impl FloatSequence {
    fn iter(&self) -> FloatSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> f32 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct DoubleSequence {
    value: f64,
    remaining: i64,
}

impl DoubleSequence {
    fn iter(&self) -> DoubleSequence {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> f64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}

fn rhsInt(calls: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return 2;
}

fn rhsFloat(calls: &mut i64) -> f32 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return 2.0;
}

fn selectInt(calls: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return 0;
}

fn main() {
    let mut __sn_rhs: i64 = 41;
    let mut __sn_place: i64 = 42;
    let mut __sn_next: i64 = 43;
    let mut __sn_previous: i64 = 44;
    let mut __sn_iter_0: i64 = 45;
    let mut iterable_calls: i64 = 0;
    let mut int_rhs_calls: i64 = 0;
    let mut float_rhs_calls: i64 = 0;
    let mut int_sources: Vec<IntSequence> = vec![IntSequence { value: 8, remaining: 2, has_next_calls: 0, next_calls: 0 }];
    {
    let mut __sn_iter_1 = ((int_sources)[__sn_index((int_sources).len(), selectInt(&mut (iterable_calls)))]).iter();
    while __sn_iter_1.hasNext() {
        let mut value = __sn_iter_1.next();
        let mut original: i64 = value;
        let mut compound: i64 = { let __sn_rhs = rhsInt(&mut (int_rhs_calls)); let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("int "); __sn_interpolated.push_str(&format!("{}", original)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
        if (original == 108) {
        continue;
    }
    }
}
    let mut longs: LongSequence = LongSequence { value: 20, remaining: 2 };
    {
    let mut __sn_iter_2 = (longs).iter();
    while __sn_iter_2.hasNext() {
        let mut value = __sn_iter_2.next();
        let mut compound: i64 = { let __sn_rhs = 3; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("long "); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
    }
}
    let mut int32s: Int32Sequence = Int32Sequence { value: 6, remaining: 2 };
    {
    let mut __sn_iter_3 = (int32s).iter();
    while __sn_iter_3.hasNext() {
        let mut value = __sn_iter_3.next();
        let mut compound: i32 = { let __sn_rhs = 2; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: i32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("int32 "); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
    }
}
    let mut bytes: ByteSequence = ByteSequence { value: 20, remaining: 2 };
    {
    let mut __sn_iter_4 = (bytes).iter();
    while __sn_iter_4.hasNext() {
        let mut value = __sn_iter_4.next();
        let mut compound: u8 = { let __sn_rhs = 2; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: u8 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("byte "); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
    }
}
    let mut uint32s: Uint32Sequence = Uint32Sequence { value: 10, remaining: 2 };
    {
    let mut __sn_iter_5 = (uint32s).iter();
    while __sn_iter_5.hasNext() {
        let mut value = __sn_iter_5.next();
        let mut compound: u32 = { let __sn_rhs = 6; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: u32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("uint32 "); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
    }
}
    let mut uints: UintSequence = UintSequence { value: 10, remaining: 2 };
    {
    let mut __sn_iter_6 = (uints).iter();
    while __sn_iter_6.hasNext() {
        let mut value = __sn_iter_6.next();
        let mut compound: u64 = { let __sn_rhs = 3; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        let mut postfix: u64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("uint "); __sn_interpolated.push_str(&format!("{}", compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
    }
}
    let mut floats: FloatSequence = FloatSequence { value: 4.0, remaining: 2 };
    {
    let mut __sn_iter_7 = (floats).iter();
    while __sn_iter_7.hasNext() {
        let mut value = __sn_iter_7.next();
        let mut added: f32 = { let (__sn_rhs, __sn_place) = (rhsFloat(&mut (float_rhs_calls)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut subtracted: f32 = { let (__sn_rhs, __sn_place) = (1.0, &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut postfix: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("float "); __sn_interpolated.push_str(&format!("{}", (added == 6.0))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (subtracted == 5.0))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (postfix == 5.0))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (value == 6.0))); __sn_interpolated });
    }
}
    let mut doubles: DoubleSequence = DoubleSequence { value: 8.0, remaining: 2 };
    {
    let mut __sn_iter_8 = (doubles).iter();
    while __sn_iter_8.hasNext() {
        let mut value = __sn_iter_8.next();
        let mut multiplied: f64 = { let (__sn_rhs, __sn_place) = (0.5, &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut divided: f64 = { let (__sn_rhs, __sn_place) = (2.0, &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut postfix: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("double "); __sn_interpolated.push_str(&format!("{}", (multiplied == 4.0))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (divided == 2.0))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (postfix == 2.0))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (value == 1.0))); __sn_interpolated });
    }
}
    let mut nested_outer: IntSequence = IntSequence { value: 1, remaining: 2, has_next_calls: 0, next_calls: 0 };
    let mut nested_inner: ByteSequence = ByteSequence { value: 5, remaining: 2 };
    {
    let mut __sn_iter_10 = (nested_outer).iter();
    while __sn_iter_10.hasNext() {
        let mut value = __sn_iter_10.next();
        let mut outer_compound: i64 = { let __sn_rhs = 1; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        {
    let mut __sn_iter_9 = (nested_inner).iter();
    while __sn_iter_9.hasNext() {
        let mut value = __sn_iter_9.next();
        let mut inner_postfix: u8 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("nested inner "); __sn_interpolated.push_str(&format!("{}", inner_postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
        break;
    }
}
        let mut outer_postfix: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("nested outer "); __sn_interpolated.push_str(&format!("{}", outer_compound)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", outer_postfix)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated });
        continue;
    }
}
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("state "); __sn_interpolated.push_str(&format!("{}", iterable_calls)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", int_rhs_calls)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", float_rhs_calls)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((int_sources)[__sn_index((int_sources).len(), 0)]).value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((int_sources)[__sn_index((int_sources).len(), 0)]).has_next_calls)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((int_sources)[__sn_index((int_sources).len(), 0)]).next_calls)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (longs).value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (bytes).value)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("helpers "); __sn_interpolated.push_str(&format!("{}", __sn_rhs)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", __sn_place)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", __sn_next)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", __sn_previous)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", __sn_iter_0)); __sn_interpolated });
}
