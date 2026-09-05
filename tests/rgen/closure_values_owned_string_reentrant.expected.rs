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
fn invoke(callback: __SnClosure<dyn Fn(i64) -> String>, depth: i64) -> String {
    return ((callback.clone()).0)(depth.clone());
}

fn main() {
    let mut trace: String = "r".to_string();
    let calls: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut action: __SnClosure<dyn Fn(i64) -> String> = { let (trace, calls, ) = (trace.clone(), calls.clone(), ); self::__SnClosure::<dyn Fn(i64) -> String>::recursive(move |action| std::rc::Rc::new(move |depth: i64| -> String { let mut trace = trace.clone(); { let (__sn_string_part, __sn_string_place) = (("a".to_string()).clone(), &mut (trace)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };{ let __sn_cell = &calls; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); __sn_cell.set(__sn_next); __sn_previous };if (depth > 0) {
        { let (__sn_string_part, __sn_string_place) = ((invoke(self::__SnClosure(action.get().expect("recursive identity initialized").upgrade().expect("recursive callable alive")), (depth).checked_sub(1).expect("checked arithmetic failed"))).clone(), &mut (trace)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
    }return trace.clone();})) }
;
    println!("{}", ((action.clone()).0)(2));
    println!("{}", calls.get());
    println!("{}", ((action.clone()).0)(1));
    println!("{}", calls.get());
    println!("{}", trace);
}
