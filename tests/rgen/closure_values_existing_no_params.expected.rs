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
    let mut get42: __SnClosure<dyn Fn() -> i64> = { self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { 42})) }
;
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((get42.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
