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
#[derive(Clone, Copy, Debug, PartialEq)]
struct Dispatcher {
    bias: i64,
}

impl Dispatcher {
    fn apply(callback: __SnClosure<dyn Fn(i64) -> i64>, value: i64) -> i64 {
        return ((callback.clone()).0)(value.clone());
    }
    fn same(left: __SnClosure<dyn Fn(i64) -> i64>, right: __SnClosure<dyn Fn(i64) -> i64>) -> bool {
        return (left.clone() == right.clone());
    }
    fn make(offset: i64) -> __SnClosure<dyn Fn(i64) -> i64> {
        return { let (offset, ) = (offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((offset.clone()).checked_add(value), "Runtime error: integer overflow in addition")})) }
;
    }
    fn composeStatic(callback: __SnClosure<dyn Fn(i64) -> i64>, offset: i64) -> __SnClosure<dyn Fn(i64) -> i64> {
        return { let (callback, offset, ) = (callback.clone(), offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((((callback.clone()).0)(value.clone())).checked_add(offset.clone()), "Runtime error: integer overflow in addition")})) }
;
    }
    fn run(&self, callback: __SnClosure<dyn Fn(i64) -> i64>, value: i64) -> i64 {
        return __sn_checked_0((((callback.clone()).0)(value.clone())).checked_add((self).bias), "Runtime error: integer overflow in addition");
    }
    fn compose(&self, callback: __SnClosure<dyn Fn(i64) -> i64>) -> __SnClosure<dyn Fn(i64) -> i64> {
        let mut bias: i64 = (self).bias;
        return { let (callback, bias, ) = (callback.clone(), bias.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((((callback.clone()).0)(value.clone())).checked_add(bias.clone()), "Runtime error: integer overflow in addition")})) }
;
    }
}

fn markedValue(order: &mut i64, marker: i64, value: i64) -> i64 {
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn makeDispatcher(order: &mut i64, marker: i64, callback: __SnClosure<dyn Fn(i64) -> i64>) -> Dispatcher {
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    ((callback.clone()).0)(marker.clone());
    return Dispatcher { bias: marker };
}

fn returnComposed(dispatcher: Dispatcher, callback: __SnClosure<dyn Fn(i64) -> i64>) -> __SnClosure<dyn Fn(i64) -> i64> {
    return (dispatcher).compose(callback.clone());
}

fn main() {
    let mut order: i64 = 0;
    let mut source: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition")})) }
;
    let mut dispatcher: Dispatcher = Dispatcher { bias: 10 };
    let mut staticApplied: i64 = Dispatcher::apply(source.clone(), 2);
    let mut instanceApplied: i64 = (dispatcher).run(source.clone(), 3);
    let mut same: bool = Dispatcher::same(source.clone(), source.clone());
    let mut made: __SnClosure<dyn Fn(i64) -> i64> = Dispatcher::make(20);
    let mut staticComposed: __SnClosure<dyn Fn(i64) -> i64> = Dispatcher::composeStatic(source.clone(), 30);
    let mut instanceComposed: __SnClosure<dyn Fn(i64) -> i64> = (dispatcher).compose(source.clone());
    let mut returned: __SnClosure<dyn Fn(i64) -> i64> = returnComposed(dispatcher, source.clone());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", staticApplied)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", instanceApplied)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", same)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((made.clone()).0)(4))); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((staticComposed.clone()).0)(5))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((instanceComposed.clone()).0)(6))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(7))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((source.clone()).0)(8))); __sn_interpolated });
    { source = Dispatcher::make(100); source.clone() };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((staticComposed.clone()).0)(9))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((instanceComposed.clone()).0)(10))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(11))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((source.clone()).0)(1))); __sn_interpolated });
    let mut ordered: i64 = (makeDispatcher(&mut (order), 1, source.clone())).run(source.clone(), markedValue(&mut (order), 2, 5));
    let mut temporary: __SnClosure<dyn Fn(i64) -> i64> = Dispatcher::make(40);
    let mut temporaryApplied: i64 = Dispatcher::apply(temporary.clone(), 2);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ordered)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", temporaryApplied)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((source.clone()).0)(12))); __sn_interpolated });
}
