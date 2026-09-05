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
fn same(n: i64) -> i64 {
    return (n).checked_add(1).expect("checked arithmetic failed");
}

fn identicalBody(n: i64) -> i64 {
    return (n).checked_add(1).expect("checked arithmetic failed");
}

fn different(n: i64) -> i64 {
    return (n).checked_add(2).expect("checked arithmetic failed");
}

fn main() {
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", true)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", false)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", false)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", true)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", false)); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut first: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { n.clone()})) }
;
    { first = self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(same)); first.clone() };
    let mut copied: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { n.clone()})) }
;
    { copied = first.clone(); copied.clone() };
    let mut second: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { n.clone()})) }
;
    { second = self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(same)); second.clone() };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (first.clone() == copied.clone()))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (first.clone() != copied.clone()))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (first.clone() == second.clone()))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(same)) == first.clone()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut identity: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { n.clone()})) }
;
    let mut copiedLambda: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |n: i64| -> i64 { n.clone()})) }
;
    { copiedLambda = identity.clone(); copiedLambda.clone() };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (identity.clone() == copiedLambda.clone()))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (identity.clone() == first.clone()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
