#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error(message),
    }
}

fn __sn_checked_div<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

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
    let mut i: i64 = 10;
    let mut l: i64 = 11;
    let mut i32: i32 = 12;
    let mut b: u8 = 13;
    let mut u32: u32 = 14;
    let mut u: u64 = 15;
    let mut f: f32 = 1.5;
    let mut d: f64 = 2.5;
    let mut yes: bool = true;
    let mut letter: char = '\u{51}';
    let mut fi: __SnClosure<dyn Fn(i64) -> i64> = { let (i, ) = (i.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((i.clone()).checked_add(x), "Runtime error: integer overflow in addition")
})) }
;
    let mut fl: __SnClosure<dyn Fn(i64) -> i64> = { let (l, ) = (l.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((l.clone()).checked_add(x), "Runtime error: integer overflow in addition")
})) }
;
    let mut fi32: __SnClosure<dyn Fn(i32) -> i32> = { let (i32, ) = (i32.clone(), ); self::__SnClosure::<dyn Fn(i32) -> i32>(std::rc::Rc::new(move |x: i32| -> i32 { __sn_checked((i32.clone()).checked_add(x), "Runtime error: integer overflow in addition")
})) }
;
    let mut fb: __SnClosure<dyn Fn(u8) -> u8> = { let (b, ) = (b.clone(), ); self::__SnClosure::<dyn Fn(u8) -> u8>(std::rc::Rc::new(move |x: u8| -> u8 { __sn_checked((b.clone()).checked_add(x), "Runtime error: integer overflow in addition")
})) }
;
    let mut fu32: __SnClosure<dyn Fn(u32) -> u32> = { let (u32, ) = (u32.clone(), ); self::__SnClosure::<dyn Fn(u32) -> u32>(std::rc::Rc::new(move |x: u32| -> u32 { __sn_checked((u32.clone()).checked_add(x), "Runtime error: integer overflow in addition")
})) }
;
    let mut fu: __SnClosure<dyn Fn(u64) -> u64> = { let (u, ) = (u.clone(), ); self::__SnClosure::<dyn Fn(u64) -> u64>(std::rc::Rc::new(move |x: u64| -> u64 { __sn_checked((u.clone()).checked_add(x), "Runtime error: integer overflow in addition")
})) }
;
    let mut ff: __SnClosure<dyn Fn(f32) -> f32> = { let (f, ) = (f.clone(), ); self::__SnClosure::<dyn Fn(f32) -> f32>(std::rc::Rc::new(move |x: f32| -> f32 { (f.clone() + x)
})) }
;
    let mut fd: __SnClosure<dyn Fn(f64) -> f64> = { let (d, ) = (d.clone(), ); self::__SnClosure::<dyn Fn(f64) -> f64>(std::rc::Rc::new(move |x: f64| -> f64 { (d.clone() + x)
})) }
;
    let mut fy: __SnClosure<dyn Fn(bool) -> bool> = { let (yes, ) = (yes.clone(), ); self::__SnClosure::<dyn Fn(bool) -> bool>(std::rc::Rc::new(move |x: bool| -> bool { (yes.clone() && x)
})) }
;
    let mut fc: __SnClosure<dyn Fn(char) -> char> = { let (letter, ) = (letter.clone(), ); self::__SnClosure::<dyn Fn(char) -> char>(std::rc::Rc::new(move |x: char| -> char { letter.clone().clone()})) }
;
    (i = 100);
    (l = 100);
    (i32 = 100);
    (b = 100);
    (u32 = 100);
    (u = 100);
    (f = 100.0);
    (d = 100.0);
    (yes = false);
    (letter = '\u{5a}');
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((fi.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fl.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fi32.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fb.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fu32.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fu.clone()).0)(1))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{:.5}", ((ff.clone()).0)(0.5))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{:.5}", ((fd.clone()).0)(0.5))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fy.clone()).0)(true))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((fc.clone()).0)('\u{58}'))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut many: __SnClosure<dyn Fn(i64, i64, i32, u8, u32, u64, f32, f64, bool, char) -> bool> = { self::__SnClosure::<dyn Fn(i64, i64, i32, u8, u32, u64, f32, f64, bool, char) -> bool>(std::rc::Rc::new(move |a: i64, z: i64, c: i32, e: u8, g: u32, h: u64, j: f32, k: f64, m: bool, n: char| -> bool { ((((((((((a == 1)
 && (z == 2)
)
 && (c == 3)
)
 && (e == 4)
)
 && (g == 5)
)
 && (h == 6)
)
 && (j == 7.0)
)
 && (k == 8.0)
)
 && m)
 && (n == '\u{4e}')
)
})) }
;
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((many.clone()).0)(1, 2, 3, 4, 5, 6, 7.0, 8.0, true, '\u{4e}'))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut empty: __SnClosure<dyn Fn() -> ()> = { self::__SnClosure::<dyn Fn() -> ()>(std::rc::Rc::new(move || -> () { print!("{}", "void\n".to_string());})) }
;
    ((empty.clone()).0)();
}

