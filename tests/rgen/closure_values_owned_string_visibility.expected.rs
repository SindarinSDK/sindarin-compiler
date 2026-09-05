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
    let mut value: String = "seed".to_string();
    let mut read: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { value.clone().clone()})) }
;
    let mut first: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { let mut value = value.clone(); (value = { let mut __sn_string = String::new(); __sn_string.push_str(&(value.clone())); __sn_string.push_str(&("-set".to_string())); __sn_string });{ let (__sn_string_part, __sn_string_place) = (("-append".to_string()).clone(), &mut (value)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return (value.clone()).to_ascii_uppercase();})) }
;
    let mut second: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { let mut value = value.clone(); { let (__sn_string_part, __sn_string_place) = (("-sibling".to_string()).clone(), &mut (value)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.clone();})) }
;
    (value = "outer".to_string());
    println!("{}", ((read.clone()).0)());
    println!("{}", ((first.clone()).0)());
    println!("{}", ((first.clone()).0)());
    println!("{}", ((second.clone()).0)());
    println!("{}", ((second.clone()).0)());
    println!("{}", value);
    if true {
        let mut value: String = "shadow".to_string();
        let mut shadowed: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { let mut value = value.clone(); { let (__sn_string_part, __sn_string_place) = (("-local".to_string()).clone(), &mut (value)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.clone();})) }
;
        println!("{}", ((shadowed.clone()).0)());
        println!("{}", ((shadowed.clone()).0)());
    }
    println!("{}", ((read.clone()).0)());
}
