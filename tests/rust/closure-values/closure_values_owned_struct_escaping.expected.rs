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
struct State {
    label: String,
    values: Vec<i64>,
    count: i64,
}

fn makeReader() -> __SnClosure<dyn Fn() -> String> {
    let mut state: State = State { label: "seed".to_string(), values: vec![1, 2], count: 7 };
    return { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (state.clone()).label)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (state.clone()).count)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((state.clone()).values).len() as i64)); __sn_interpolated }})) }
;
}

fn main() {
    let mut read: __SnClosure<dyn Fn() -> String> = makeReader();
    println!("{}", ((read.clone()).0)());
    println!("{}", ((read.clone()).0)());
}
