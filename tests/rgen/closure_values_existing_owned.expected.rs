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
fn callTwice(action: __SnClosure<dyn Fn() -> String>) -> String {
    return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((action.clone()).0)())); __sn_interpolated.push_str(" | "); __sn_interpolated.push_str(&format!("{}", ((action.clone()).0)())); __sn_interpolated };
}

fn main() {
    std::process::exit((|| -> i64 {
        let mut greeting: String = "hello world".to_string();
        let mut producer: __SnClosure<dyn Fn() -> String> = { let (greeting, ) = (greeting.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { greeting.clone().clone()})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", callTwice(producer.clone()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((producer.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
        return 0;
        return 0;
    })() as i32);
}
