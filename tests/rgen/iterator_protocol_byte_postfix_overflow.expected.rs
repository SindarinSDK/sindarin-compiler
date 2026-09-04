#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct ByteIterator {
    value: u8,
    remaining: i64,
}

impl ByteIterator {
    fn iter(&self) -> ByteIterator {
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

fn main() {
    let mut source: ByteIterator = ByteIterator { value: 255, remaining: 1 };
    {
    let mut __sn_iter_0 = (source).iter();
    while __sn_iter_0.hasNext() {
        let mut value = __sn_iter_0.next();
        let mut previous: u8 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        { let __sn_place = &mut (previous); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
}
}
