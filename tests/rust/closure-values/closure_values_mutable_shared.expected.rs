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
fn makeCounter() -> __SnClosure<dyn Fn() -> i64> {
    let count: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(10));
    return { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let __sn_cell = &count; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous };return count.get();})) }
;
}

fn main() {
    let count: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let mut read: __SnClosure<dyn Fn() -> i64> = { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { count.get().clone()})) }
;
    let mut bump: __SnClosure<dyn Fn() -> i64> = { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let (__sn_rhs, __sn_cell) = (2, &count); let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(__sn_rhs).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_next };return count.get();})) }
;
    { let (__sn_value, __sn_cell) = (5, &count); __sn_cell.set(__sn_value); __sn_value };
    println!("{}", ((read.clone()).0)());
    println!("{}", ((bump.clone()).0)());
    println!("{}", count.get());
    println!("{}", { let __sn_cell = &count; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous });
    println!("{}", ((read.clone()).0)());
    { let (__sn_rhs, __sn_cell) = (3, &count); let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_sub(__sn_rhs).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_next };
    println!("{}", ((read.clone()).0)());
    if true {
        let count: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(20));
        let mut inner: __SnClosure<dyn Fn() -> i64> = { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let __sn_cell = &count; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous };return count.get();})) }
;
        println!("{}", ((inner.clone()).0)());
    }
    println!("{}", ((read.clone()).0)());
    let mut first: __SnClosure<dyn Fn() -> i64> = makeCounter();
    let mut second: __SnClosure<dyn Fn() -> i64> = makeCounter();
    println!("{}", ((first.clone()).0)());
    println!("{}", ((first.clone()).0)());
    println!("{}", ((second.clone()).0)());
}
