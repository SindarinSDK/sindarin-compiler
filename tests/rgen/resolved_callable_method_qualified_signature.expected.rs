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
    fn apply(callback: &mut __SnClosure<dyn Fn(i64) -> i64>, value: i64) -> i64 {
        return (((*(callback)).clone()).0)(value.clone());
    }
}

fn main() {
    let mut callback: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { value.clone()})) }
;
    let mut value: i64 = 1;
    println!("{}", { let __sn_resolved_arg_0 = value; Dispatcher::apply(&mut (callback), __sn_resolved_arg_0) });
}
