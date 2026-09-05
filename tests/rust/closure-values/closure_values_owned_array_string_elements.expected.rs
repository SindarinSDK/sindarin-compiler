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
fn makeCollector() -> __SnClosure<dyn Fn(String) -> String> {
    let mut values: Vec<String> = vec!["first".to_string()];
    return { let (values, ) = (std::rc::Rc::new(std::cell::RefCell::new(values.clone())), ); self::__SnClosure::<dyn Fn(String) -> String>(std::rc::Rc::new(move |value: String| -> String { { let __sn_array_value = value.clone(); values.borrow_mut().push(__sn_array_value); };return (values.borrow().clone())[__sn_index((values.borrow().clone()).len(), ((values.borrow().clone()).len() as i64).checked_sub(1).expect("checked arithmetic failed"))].clone();})) }
;
}

fn main() {
    let mut collect: __SnClosure<dyn Fn(String) -> String> = makeCollector();
    println!("{}", ((collect.clone()).0)("second".to_string()));
    println!("{}", ((collect.clone()).0)("third".to_string()));
}
