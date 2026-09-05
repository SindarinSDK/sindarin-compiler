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
struct RouteCell {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RouteHolder {
    cell: RouteCell,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RouteOps {
}

impl RouteOps {
    fn bumpStatic(value: &mut i64) {
        (*(value) = __sn_checked_0((*(value)).checked_add(1), "Runtime error: integer overflow in addition"));
    }
    fn bumpInstance(&self, value: &mut i64) {
        (*(value) = __sn_checked_0((*(value)).checked_add(1), "Runtime error: integer overflow in addition"));
    }
    unsafe fn forwardStatic(value: *mut i64, alias: *mut i64) -> i64 {
        RouteOps::bumpStatic(&mut (*(value)));
        (*(alias) = __sn_checked_0((*(alias)).checked_add(10), "Runtime error: integer overflow in addition"));
        return *(value);
    }
    unsafe fn forwardInstance(&self, value: *mut i64, alias: *mut i64) -> i64 {
        (self).bumpInstance(&mut (*(value)));
        (*(alias) = __sn_checked_0((*(alias)).checked_add(10), "Runtime error: integer overflow in addition"));
        return *(value);
    }
    fn bumpCell(cell: &mut RouteCell) {
        ((cell).value = __sn_checked_0(((cell).value).checked_add(1), "Runtime error: integer overflow in addition"));
    }
    unsafe fn forwardHolder(holder: *mut RouteHolder, cell: *mut RouteCell) -> i64 {
        RouteOps::bumpCell(&mut ((*(holder)).cell));
        ((*(cell)).value = __sn_checked_0(((*(cell)).value).checked_add(10), "Runtime error: integer overflow in addition"));
        return ((*(holder)).cell).value;
    }
}

fn main() {
    let mut staticValue: i64 = 1;
    println!("{}", unsafe { RouteOps::forwardStatic(std::ptr::addr_of_mut!(staticValue), std::ptr::addr_of_mut!(staticValue)) });
    println!("{}", staticValue);
    let mut ops: RouteOps = RouteOps {  };
    let mut instanceValue: i64 = 2;
    println!("{}", unsafe { (ops).forwardInstance(std::ptr::addr_of_mut!(instanceValue), std::ptr::addr_of_mut!(instanceValue)) });
    println!("{}", instanceValue);
    let mut holder: RouteHolder = RouteHolder { cell: RouteCell { value: 1 } };
    println!("{}", unsafe { RouteOps::forwardHolder(std::ptr::addr_of_mut!(holder), std::ptr::addr_of_mut!((holder).cell)) });
    println!("{}", ((holder).cell).value);
}
