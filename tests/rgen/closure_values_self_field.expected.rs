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
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    action: __SnClosure<dyn Fn(i64) -> i64>,
}

impl Holder {
    fn apply(&self, value: i64) -> i64 {
        return (((self).action.clone()).0)(value.clone());
    }
}

fn main() {
    let mut offset: i64 = 10;
    let mut holder: Holder = Holder { action: { let (offset, ) = (offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { (value).checked_add(offset.clone()).expect("checked arithmetic failed")})) }
 };
    let mut copied: Holder = holder.clone();
    ((holder).action = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { (value).checked_add(100).expect("checked arithmetic failed")})) }
);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (holder).apply(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied).apply(2))); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
