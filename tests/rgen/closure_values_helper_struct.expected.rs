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
#[derive(Clone, Copy, Debug, PartialEq)]
struct __SnClosure {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct __SnClosure_0 {
    value: i64,
}

fn main() {
    let mut __SnClosure_1: i64 = 20;
    let mut item: __SnClosure = __SnClosure { value: 7 };
    let mut factory: __SnClosure_2<dyn Fn() -> __SnClosure> = { let (item, ) = (item.clone(), ); self::__SnClosure_2::<dyn Fn() -> __SnClosure>(std::rc::Rc::new(move || -> __SnClosure { item.clone().clone()})) }
;
    let mut result: __SnClosure = ((factory.clone()).0)();
    ((item).value = 9);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (result).value)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (item).value)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", __SnClosure_1)); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
