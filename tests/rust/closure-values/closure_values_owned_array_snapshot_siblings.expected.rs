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
fn main() {
    let mut values: Vec<i64> = vec![10];
    let mut left: __SnClosure<dyn Fn(i64) -> i64> = { let (values, ) = (std::rc::Rc::new(std::cell::RefCell::new(values.clone())), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { { let __sn_array_value = value; values.borrow_mut().push(__sn_array_value); };return (values.borrow().clone()).len() as i64;})) }
;
    let mut right: __SnClosure<dyn Fn(i64) -> i64> = { let (values, ) = (std::rc::Rc::new(std::cell::RefCell::new(values.clone())), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { { let __sn_array_value = value; values.borrow_mut().push(__sn_array_value); };return (values.borrow().clone()).len() as i64;})) }
;
    println!("{}", ((left.clone()).0)(20));
    println!("{}", ((right.clone()).0)(30));
    println!("{}", ((left.clone()).0)(40));
    println!("{}", ((right.clone()).0)(50));
    println!("{}", (values).len() as i64);
}
