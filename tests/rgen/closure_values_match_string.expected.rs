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
    let mut choose: __SnClosure<dyn Fn(String) -> String> = { self::__SnClosure::<dyn Fn(String) -> String>(std::rc::Rc::new(move |value: String| -> String { value.clone()})) }
;
    let mut result: String = match (1 as i64) {
        1 => {
            (((choose.clone()).0)("one".to_string()))
        },
        _ => {
            ("other".to_string())
        },
    };
    println!("{}", result);
}
