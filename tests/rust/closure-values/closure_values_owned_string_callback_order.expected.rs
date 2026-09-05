#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
fn invoke(callback: __SnClosure<dyn Fn() -> String>) -> String {
    return ((callback.clone()).0)();
}

fn main() {
    let calls: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut part: __SnClosure<dyn Fn() -> String> = { let (calls, ) = (calls.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let __sn_cell = &calls; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous };return "-part".to_string();})) }
;
    let value: std::rc::Rc<std::cell::RefCell<String>> = std::rc::Rc::new(std::cell::RefCell::new("base".to_string()));
    let mut update: __SnClosure<dyn Fn() -> String> = { let (value, part, ) = (value.clone(), part.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let (__sn_string_part, __sn_cell) = ((invoke(part.clone())).clone(), &value); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.borrow().clone();})) }
;
    println!("{}", ((update.clone()).0)());
    println!("{}", calls.get());
    println!("{}", ((update.clone()).0)());
    println!("{}", calls.get());
    println!("{}", value.borrow().clone());
}
