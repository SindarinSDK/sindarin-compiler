#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

struct __sn_concurrency0_Cell<T> {
    value: std::sync::Mutex<T>,
    gate: std::sync::Mutex<()>,
}
impl<T> __sn_concurrency0_Cell<T> {
    fn new(value: T) -> Self { Self { value: std::sync::Mutex::new(value), gate: std::sync::Mutex::new(()) } }
    fn lock(&self) -> std::sync::LockResult<std::sync::MutexGuard<'_, T>> { self.value.lock() }
    fn guard(&self) -> std::sync::MutexGuard<'_, ()> { self.gate.lock().unwrap_or_else(|e| e.into_inner()) }
}

static __sn_concurrency0_global_first: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(7));
static __sn_concurrency0_global_second: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(__sn_checked_0(({ let value = __sn_concurrency0_global_first.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add(2), "Runtime error: integer overflow in addition")));

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

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_first);
    std::sync::LazyLock::force(&__sn_concurrency0_global_second);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_first.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_second.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated });
}
