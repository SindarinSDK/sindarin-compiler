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
fn main() {
    let count: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let mut factory: __SnClosure<dyn Fn() -> __SnClosure<dyn Fn() -> i64>> = { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn() -> __SnClosure<dyn Fn() -> i64>>(std::rc::Rc::new(move || -> __SnClosure<dyn Fn() -> i64> { return { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let __sn_cell = &count; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous };return count.get();})) }
;})) }
;
    let mut first: __SnClosure<dyn Fn() -> i64> = ((factory.clone()).0)();
    let mut second: __SnClosure<dyn Fn() -> i64> = ((factory.clone()).0)();
    println!("{}", ((first.clone()).0)());
    println!("{}", ((second.clone()).0)());
    { let (__sn_value, __sn_cell) = (9, &count); __sn_cell.set(__sn_value); __sn_value };
    println!("{}", ((first.clone()).0)());
    println!("{}", count.get());
}
