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
    let value: std::rc::Rc<std::cell::RefCell<String>> = std::rc::Rc::new(std::cell::RefCell::new("seed".to_string()));
    println!("{}", value.borrow().clone());
    let mut read: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { value.borrow().clone().clone()})) }
;
    let mut first: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let (__sn_value, __sn_cell) = ({ let mut __sn_string = String::new(); __sn_string.push_str(&(value.borrow().clone())); __sn_string.push_str(&("-set".to_string())); __sn_string }, &value); __sn_cell.replace(__sn_value.clone()); __sn_value };{ let (__sn_string_part, __sn_cell) = (("-append".to_string()).clone(), &value); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return (value.borrow().clone()).to_ascii_uppercase();})) }
;
    let mut second: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let (__sn_string_part, __sn_cell) = (("-sibling".to_string()).clone(), &value); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.borrow().clone();})) }
;
    { let (__sn_value, __sn_cell) = ("outer".to_string(), &value); __sn_cell.replace(__sn_value.clone()); __sn_value };
    println!("{}", ((read.clone()).0)());
    println!("{}", ((first.clone()).0)());
    println!("{}", ((first.clone()).0)());
    println!("{}", ((second.clone()).0)());
    println!("{}", ((second.clone()).0)());
    println!("{}", value.borrow().clone());
    if true {
        let value: std::rc::Rc<std::cell::RefCell<String>> = std::rc::Rc::new(std::cell::RefCell::new("shadow".to_string()));
        let mut shadowed: __SnClosure<dyn Fn() -> String> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let (__sn_string_part, __sn_cell) = (("-local".to_string()).clone(), &value); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };return value.borrow().clone();})) }
;
        println!("{}", ((shadowed.clone()).0)());
        println!("{}", ((shadowed.clone()).0)());
    }
    println!("{}", ((read.clone()).0)());
}
