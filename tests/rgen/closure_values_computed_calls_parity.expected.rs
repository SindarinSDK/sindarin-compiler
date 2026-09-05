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
fn factory(offset: i64) -> __SnClosure<dyn Fn(i64) -> i64> {
    print!("{}", "factory\n".to_string());
    return { let (offset, ) = (offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { (offset.clone()).checked_add(n).expect("checked arithmetic failed")})) }
;
}

fn argument() -> i64 {
    print!("{}", "argument\n".to_string());
    return 2;
}

fn main() {
    let mut produced: __SnClosure<dyn Fn(i64) -> i64> = factory(10);
    println!("{}", ((produced.clone()).0)(argument()));
    let mut immediate: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { (n).checked_add(1).expect("checked arithmetic failed")})) }
;
    println!("{}", ((immediate.clone()).0)(argument()));
}
