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
#[derive(Clone, Copy, Debug, PartialEq)]
struct Dispatcher {
}

impl Dispatcher {
    fn reset(callback: &mut __SnClosure<dyn Fn(i64) -> i64>, marker: i64, order: &mut i64) {
        (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
        { *(callback) = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { (value).checked_add(10).expect("checked arithmetic failed")})) }
; (*(callback)).clone() };
    }
}

fn marked(order: &mut i64, marker: i64) -> i64 {
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return marker;
}

fn main() {
    let mut order: i64 = 0;
    let mut callback: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { (value).checked_add(1).expect("checked arithmetic failed")})) }
;
    Dispatcher::reset(&mut (callback), marked(&mut (order), 2), &mut (order));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((callback.clone()).0)(2))); __sn_interpolated });
    { callback = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { (value).checked_add(100).expect("checked arithmetic failed")})) }
; callback.clone() };
    println!("{}", ((callback.clone()).0)(1));
}
