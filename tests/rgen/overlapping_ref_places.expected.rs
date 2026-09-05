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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Leaf {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct InnerNode {
    leaf: Leaf,
    score: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct OuterNode {
    inner: InnerNode,
    total: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct AliasOps {
}

impl AliasOps {
    unsafe fn touchNested(outer: *mut OuterNode, inner: *mut InnerNode, leaf: *mut Leaf) -> i64 {
        ((*(outer)).total = __sn_checked_0(((*(outer)).total).checked_add(1), "Runtime error: integer overflow in addition"));
        ((*(inner)).score = __sn_checked_0(((*(inner)).score).checked_add(10), "Runtime error: integer overflow in addition"));
        ((*(leaf)).value = __sn_checked_0(((*(leaf)).value).checked_add(100), "Runtime error: integer overflow in addition"));
        return __sn_checked_0((__sn_checked_0(((*(outer)).total).checked_add(((*(outer)).inner).score), "Runtime error: integer overflow in addition")).checked_add((((*(outer)).inner).leaf).value), "Runtime error: integer overflow in addition");
    }
    unsafe fn touchSame(value: *mut i64, alias: *mut i64) -> i64 {
        (*(value) = __sn_checked_0((*(value)).checked_add(1), "Runtime error: integer overflow in addition"));
        (*(alias) = __sn_checked_0((*(alias)).checked_add(10), "Runtime error: integer overflow in addition"));
        return *(value);
    }
    unsafe fn forwardedSink(outer: *mut OuterNode, inner: *mut InnerNode) -> i64 {
        ((*(outer)).total = __sn_checked_0(((*(outer)).total).checked_add(2), "Runtime error: integer overflow in addition"));
        ((*(inner)).score = __sn_checked_0(((*(inner)).score).checked_add(20), "Runtime error: integer overflow in addition"));
        return __sn_checked_0(((*(outer)).total).checked_add(((*(outer)).inner).score), "Runtime error: integer overflow in addition");
    }
    unsafe fn forwardedWrapper(outer: *mut OuterNode, inner: *mut InnerNode) -> i64 {
        return unsafe { AliasOps::forwardedSink(std::ptr::addr_of_mut!(*(outer)), std::ptr::addr_of_mut!(*(inner))) };
    }
    unsafe fn recursiveTouch(outer: *mut OuterNode, inner: *mut InnerNode, remaining: i64) -> i64 {
        if (remaining == 0) {
        return __sn_checked_0(((*(outer)).total).checked_add(((*(outer)).inner).score), "Runtime error: integer overflow in addition");
    }
        ((*(outer)).total = __sn_checked_0(((*(outer)).total).checked_add(1), "Runtime error: integer overflow in addition"));
        ((*(inner)).score = __sn_checked_0(((*(inner)).score).checked_add(2), "Runtime error: integer overflow in addition"));
        return unsafe { AliasOps::recursiveTouch(std::ptr::addr_of_mut!(*(outer)), std::ptr::addr_of_mut!(*(inner)), __sn_checked_0((remaining).checked_sub(1), "Runtime error: integer overflow in subtraction")) };
    }
}

unsafe fn touchSameFree(value: *mut i64, alias: *mut i64) -> i64 {
    (*(value) = __sn_checked_0((*(value)).checked_add(2), "Runtime error: integer overflow in addition"));
    (*(alias) = __sn_checked_0((*(alias)).checked_add(20), "Runtime error: integer overflow in addition"));
    return *(value);
}

unsafe fn forwardedSameSink(value: *mut i64, alias: *mut i64) -> i64 {
    (*(value) = __sn_checked_0((*(value)).checked_add(3), "Runtime error: integer overflow in addition"));
    (*(alias) = __sn_checked_0((*(alias)).checked_add(30), "Runtime error: integer overflow in addition"));
    return *(value);
}

unsafe fn forwardedSameWrapper(value: *mut i64, alias: *mut i64) -> i64 {
    return unsafe { forwardedSameSink(std::ptr::addr_of_mut!(*(value)), std::ptr::addr_of_mut!(*(alias))) };
}

fn scopedMutation() -> i64 {
    let mut outer: OuterNode = OuterNode { inner: InnerNode { leaf: Leaf { value: 3 }, score: 2 }, total: 1 };
    println!("{}", unsafe { AliasOps::touchNested(std::ptr::addr_of_mut!(outer), std::ptr::addr_of_mut!((outer).inner), std::ptr::addr_of_mut!(((outer).inner).leaf)) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (outer).total)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((outer).inner).score)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((outer).inner).leaf).value)); __sn_interpolated });
    return (((outer).inner).leaf).value;
}

fn main() {
    let mut first: i64 = 1;
    println!("{}", unsafe { AliasOps::touchSame(std::ptr::addr_of_mut!(first), std::ptr::addr_of_mut!(first)) });
    println!("{}", first);
    let mut second: i64 = 5;
    println!("{}", unsafe { touchSameFree(std::ptr::addr_of_mut!(second), std::ptr::addr_of_mut!(second)) });
    println!("{}", second);
    let mut third: i64 = 7;
    println!("{}", unsafe { forwardedSameWrapper(std::ptr::addr_of_mut!(third), std::ptr::addr_of_mut!(third)) });
    println!("{}", third);
    println!("{}", scopedMutation());
    let mut forwarded: OuterNode = OuterNode { inner: InnerNode { leaf: Leaf { value: 0 }, score: 1 }, total: 2 };
    println!("{}", unsafe { AliasOps::forwardedWrapper(std::ptr::addr_of_mut!(forwarded), std::ptr::addr_of_mut!((forwarded).inner)) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (forwarded).total)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((forwarded).inner).score)); __sn_interpolated });
    let mut recursive: OuterNode = OuterNode { inner: InnerNode { leaf: Leaf { value: 0 }, score: 2 }, total: 1 };
    println!("{}", unsafe { AliasOps::recursiveTouch(std::ptr::addr_of_mut!(recursive), std::ptr::addr_of_mut!((recursive).inner), 3) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (recursive).total)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((recursive).inner).score)); __sn_interpolated });
}
