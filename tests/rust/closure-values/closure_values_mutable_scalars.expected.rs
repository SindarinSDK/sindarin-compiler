#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_0<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_0(message),
    }
}

fn __sn_checked_div_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
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
    let i: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let l: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(2));
    let b: std::rc::Rc<std::cell::Cell<u8>> = std::rc::Rc::new(std::cell::Cell::new(3));
    let d: std::rc::Rc<std::cell::Cell<f64>> = std::rc::Rc::new(std::cell::Cell::new(4.0));
    let yes: std::rc::Rc<std::cell::Cell<bool>> = std::rc::Rc::new(std::cell::Cell::new(false));
    let letter: std::rc::Rc<std::cell::Cell<char>> = std::rc::Rc::new(std::cell::Cell::new('\u{61}'));
    let mut update: __SnClosure<dyn Fn() -> ()> = { let (i, l, b, d, yes, letter, ) = (i.clone(), l.clone(), b.clone(), d.clone(), yes.clone(), letter.clone(), ); self::__SnClosure::<dyn Fn() -> ()>(std::rc::Rc::new(move || -> () { { let (__sn_value, __sn_cell) = (11, &i); __sn_cell.set(__sn_value); __sn_value };{ let (__sn_value, __sn_cell) = (12, &l); __sn_cell.set(__sn_value); __sn_value };{ let (__sn_value, __sn_cell) = (13, &b); __sn_cell.set(__sn_value); __sn_value };{ let (__sn_value, __sn_cell) = (14.0, &d); __sn_cell.set(__sn_value); __sn_value };{ let (__sn_value, __sn_cell) = (true, &yes); __sn_cell.set(__sn_value); __sn_value };{ let (__sn_value, __sn_cell) = ('\u{7a}', &letter); __sn_cell.set(__sn_value); __sn_value };})) }
;
    ((update.clone()).0)();
    println!("{}", i.get());
    println!("{}", l.get());
    println!("{}", (b.get() == 13));
    println!("{}", (d.get() == 14.0));
    println!("{}", yes.get());
    println!("{}", letter.get());
    let mut i32: i32 = 1;
    let mut u32: u32 = 2;
    let mut u: u64 = 3;
    let mut f: f32 = 4.0;
    let mut snapshot: __SnClosure<dyn Fn() -> ()> = { let (i32, u32, u, f, ) = (i32.clone(), u32.clone(), u.clone(), f.clone(), ); self::__SnClosure::<dyn Fn() -> ()>(std::rc::Rc::new(move || -> () { let mut i32 = i32; let mut u32 = u32; let mut u = u; let mut f = f; { let __sn_rhs = 1; let __sn_place = &mut (i32); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };{ let __sn_rhs = 1; let __sn_place = &mut (u32); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };{ let __sn_rhs = 1; let __sn_place = &mut (u); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };{ let (__sn_rhs, __sn_place) = (1.0, &mut (f)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };println!("{}", i32.clone());println!("{}", u32.clone());println!("{}", u.clone());println!("{}", (f.clone() == 5.0));})) }
;
    ((snapshot.clone()).0)();
    ((snapshot.clone()).0)();
    println!("{}", i32);
    println!("{}", u32);
    println!("{}", u);
    println!("{}", (f == 4.0));
}
