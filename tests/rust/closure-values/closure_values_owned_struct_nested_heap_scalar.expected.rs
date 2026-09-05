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

struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
#[derive(Clone, Debug, PartialEq)]
struct Inner {
    label: String,
    value: i64,
}
#[derive(Clone, Debug, PartialEq)]
struct Outer {
    inner: Inner,
    values: Vec<i64>,
}

fn main() {
    let mut state: Outer = Outer { inner: Inner { label: "seed".to_string(), value: 10 }, values: vec![1, 2] };
    let mut mutate: __SnClosure<dyn Fn(i64) -> i64> = { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |delta: i64| -> i64 { let mut state = state.clone(); { let __sn_rhs = delta; let __sn_place = &mut (((state).inner).value); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };return ((state.clone()).inner).value;})) }
;
    println!("{}", ((mutate.clone()).0)(1));
    println!("{}", ((mutate.clone()).0)(2));
    println!("{}", ((state).inner).value);
}
