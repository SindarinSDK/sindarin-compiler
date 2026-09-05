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
    let mut value: f32 = 1.0;
    let mut action: __SnClosure<dyn Fn() -> f32> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> f32>(std::rc::Rc::new(move || -> f32 { let mut value = value; (value = 2.0);return value.clone();})) }
;
    println!("{}", ((action.clone()).0)());
}
