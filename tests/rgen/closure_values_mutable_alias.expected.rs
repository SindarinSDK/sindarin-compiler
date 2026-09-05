#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> __SnClosure<F> {
    fn recursive(build: impl FnOnce(std::rc::Rc<std::cell::OnceCell<std::rc::Weak<F>>>) -> std::rc::Rc<F>) -> Self {
        let slot = std::rc::Rc::new(std::cell::OnceCell::new());
        let callable = build(slot.clone());
        let _ = slot.set(std::rc::Rc::downgrade(&callable));
        Self(callable)
    }
}
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
fn invoke(action: __SnClosure<dyn Fn(i64) -> i64>, n: i64) -> i64 {
    return ((action.clone()).0)(n.clone());
}

fn main() {
    let count: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut action: __SnClosure<dyn Fn(i64) -> i64> = { let (count, ) = (count.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>::recursive(move |action| std::rc::Rc::new(move |n: i64| -> i64 { { let __sn_cell = &count; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous };if (n > 0) {
        { let (__sn_rhs, __sn_cell) = (invoke(self::__SnClosure(action.get().expect("recursive identity initialized").upgrade().expect("recursive callable alive")), (n).checked_sub(1).expect("checked arithmetic failed")), &count); let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(__sn_rhs).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_next };
    }return count.get();})) }
;
    let mut alias: __SnClosure<dyn Fn(i64) -> i64> = action.clone();
    { action = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { 99})) }
; action.clone() };
    println!("{}", ((alias.clone()).0)(2));
    println!("{}", count.get());
    println!("{}", ((action.clone()).0)(0));
}
