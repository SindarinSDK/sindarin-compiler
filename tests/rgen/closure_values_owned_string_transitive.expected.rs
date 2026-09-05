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
fn make(seed: String) -> __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(String) -> String>> {
    let mut value: String = seed.clone();
    return { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> __SnClosure<dyn Fn(String) -> String>>(std::rc::Rc::new(move || -> __SnClosure<dyn Fn(String) -> String> { let mut value = value.clone(); { let (__sn_string_part, __sn_string_place) = (("-middle".to_string()).clone(), &mut (value)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn(String) -> String>(std::rc::Rc::new(move |suffix: String| -> String { let mut value = value.clone(); { let (__sn_string_part, __sn_string_place) = ((suffix).clone(), &mut (value)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.clone();})) }
 ;})) }
;
}

fn main() {
    let mut factory: __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(String) -> String>> = make("root".to_string());
    let mut first: __SnClosure<dyn Fn(String) -> String> = ((factory.clone()).0)();
    let mut second: __SnClosure<dyn Fn(String) -> String> = ((factory.clone()).0)();
    println!("{}", ((first.clone()).0)("-one".to_string()));
    println!("{}", ((first.clone()).0)("-two".to_string()));
    println!("{}", ((second.clone()).0)("-three".to_string()));
    let mut other: __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(String) -> String>> = make("other".to_string());
    let mut third: __SnClosure<dyn Fn(String) -> String> = ((other.clone()).0)();
    println!("{}", ((third.clone()).0)("-four".to_string()));
}
