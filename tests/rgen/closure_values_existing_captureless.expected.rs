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
    std::process::exit((|| -> i64 {
        print!("{}", "Test pure lambdas (no capture):\n\n".to_string());
        print!("{}", "Test 1 - Arithmetic lambda:\n".to_string());
        let mut add: __SnClosure<dyn Fn(i64, i64) -> i64> = { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { (a).checked_add(b).expect("checked arithmetic failed")})) }
;
        let mut multiply: __SnClosure<dyn Fn(i64, i64) -> i64> = { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { (a).checked_mul(b).expect("checked arithmetic failed")})) }
;
        let mut subtract: __SnClosure<dyn Fn(i64, i64) -> i64> = { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { (a).checked_sub(b).expect("checked arithmetic failed")})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  add(10, 5) = "); __sn_interpolated.push_str(&format!("{}", ((add.clone()).0)(10, 5))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  multiply(10, 5) = "); __sn_interpolated.push_str(&format!("{}", ((multiply.clone()).0)(10, 5))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  subtract(10, 5) = "); __sn_interpolated.push_str(&format!("{}", ((subtract.clone()).0)(10, 5))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", "\nTest 2 - Conditional lambda:\n".to_string());
        let mut max: __SnClosure<dyn Fn(i64, i64) -> i64> = { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { if (a > b) {
        return a;
    } else {
        return b;
    }})) }
;
        let mut min: __SnClosure<dyn Fn(i64, i64) -> i64> = { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { if (a < b) {
        return a;
    } else {
        return b;
    }})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  max(10, 20) = "); __sn_interpolated.push_str(&format!("{}", ((max.clone()).0)(10, 20))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  min(10, 20) = "); __sn_interpolated.push_str(&format!("{}", ((min.clone()).0)(10, 20))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", "\nTest 3 - Boolean lambda:\n".to_string());
        let mut is_even: __SnClosure<dyn Fn(i64) -> bool> = { self::__SnClosure::<dyn Fn(i64) -> bool>(std::rc::Rc::new(move |n: i64| -> bool { ((n).checked_rem(2).expect("checked arithmetic failed") == 0)})) }
;
        let mut is_positive: __SnClosure<dyn Fn(i64) -> bool> = { self::__SnClosure::<dyn Fn(i64) -> bool>(std::rc::Rc::new(move |n: i64| -> bool { (n > 0)})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  is_even(4) = "); __sn_interpolated.push_str(&format!("{}", ((is_even.clone()).0)(4))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  is_even(7) = "); __sn_interpolated.push_str(&format!("{}", ((is_even.clone()).0)(7))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  is_positive(5) = "); __sn_interpolated.push_str(&format!("{}", ((is_positive.clone()).0)(5))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  is_positive(-3) = "); __sn_interpolated.push_str(&format!("{}", ((is_positive.clone()).0)((-3)))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", "\nTest 4 - String lambda:\n".to_string());
        let mut greet: __SnClosure<dyn Fn(String) -> String> = { self::__SnClosure::<dyn Fn(String) -> String>(std::rc::Rc::new(move |name: String| -> String { { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("Hello, "); __sn_interpolated.push_str(&format!("{}", name)); __sn_interpolated.push_str("!"); __sn_interpolated }})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  "); __sn_interpolated.push_str(&format!("{}", ((greet.clone()).0)("World".to_string()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  "); __sn_interpolated.push_str(&format!("{}", ((greet.clone()).0)("Sn".to_string()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", "\nTest 5 - Double lambda:\n".to_string());
        let mut half: __SnClosure<dyn Fn(f64) -> f64> = { self::__SnClosure::<dyn Fn(f64) -> f64>(std::rc::Rc::new(move |x: f64| -> f64 { (x / 2.0)})) }
;
        let mut square: __SnClosure<dyn Fn(f64) -> f64> = { self::__SnClosure::<dyn Fn(f64) -> f64>(std::rc::Rc::new(move |x: f64| -> f64 { (x * x)})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  half(10.0) = "); __sn_interpolated.push_str(&format!("{:.5}", ((half.clone()).0)(10.0))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  square(5.0) = "); __sn_interpolated.push_str(&format!("{:.5}", ((square.clone()).0)(5.0))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", "\nTest 6 - Lambda composition:\n".to_string());
        let mut double_val: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { (x).checked_mul(2).expect("checked arithmetic failed")})) }
;
        let mut increment: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { (x).checked_add(1).expect("checked arithmetic failed")})) }
;
        let mut composed: i64 = ((double_val.clone()).0)(((increment.clone()).0)(5));
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("  double(increment(5)) = "); __sn_interpolated.push_str(&format!("{}", composed)); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", "\nAll pure lambda tests passed!\n".to_string());
        return 0;
        return 0;
    })() as i32);
}
