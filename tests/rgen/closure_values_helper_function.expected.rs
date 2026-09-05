#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

struct __SnClosure_2<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure_2<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure_2<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure_2<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
fn __SnClosure(value: i64) -> i64 {
    return (value).checked_add(1).expect("checked arithmetic failed");
}

fn __SnClosure_0() -> i64 {
    return 10;
}

fn main() {
    let mut __SnClosure_1: i64 = 20;
    let mut action: __SnClosure_2<dyn Fn(i64) -> i64> = { self::__SnClosure_2::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { value.clone()})) }
;
    { action = self::__SnClosure_2::<dyn Fn(i64) -> i64>(std::rc::Rc::new(__SnClosure)); action.clone() };
    let mut wrap: __SnClosure_2<dyn Fn(__SnClosure_2<dyn Fn(i64) -> i64>) -> __SnClosure_2<dyn Fn(i64) -> i64>> = { self::__SnClosure_2::<dyn Fn(__SnClosure_2<dyn Fn(i64) -> i64>) -> __SnClosure_2<dyn Fn(i64) -> i64>>(std::rc::Rc::new(move |callback: __SnClosure_2<dyn Fn(i64) -> i64>| -> __SnClosure_2<dyn Fn(i64) -> i64> { return { let (callback, ) = (callback.clone(), ); self::__SnClosure_2::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { ((callback.clone()).0)(value.clone())})) }
;})) }
;
    let mut wrapped: __SnClosure_2<dyn Fn(i64) -> i64> = ((wrap.clone()).0)(action.clone().clone());
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((wrapped.clone()).0)(2))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", __SnClosure_0())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", __SnClosure_1)); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
