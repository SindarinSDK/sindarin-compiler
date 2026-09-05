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

static __sn_concurrency0_global_gate: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));
static __sn_concurrency0_global_counter: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));

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

fn update(early: bool) {
    { let __sn_concurrency0_lock_guard = __sn_concurrency0_global_gate.guard(); {
        { let __sn_concurrency0_value = __sn_checked_0(({ let value = __sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add(1), "Runtime error: integer overflow in addition"); *__sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value.clone(); __sn_concurrency0_value };
        if early {
        return;
    }
        { let __sn_concurrency0_value = __sn_checked_0(({ let value = __sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add(1), "Runtime error: integer overflow in addition"); *__sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value.clone(); __sn_concurrency0_value };
    } }

}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_gate);
    std::sync::LazyLock::force(&__sn_concurrency0_global_counter);
    update(true);
    { let __sn_concurrency0_lock_guard = __sn_concurrency0_global_gate.guard(); {
        { let __sn_concurrency0_value = __sn_checked_0(({ let value = __sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add(100), "Runtime error: integer overflow in addition"); *__sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value.clone(); __sn_concurrency0_value };
    } }

    update(false);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("counter: "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("twice: "); __sn_interpolated.push_str(&format!("{}", __sn_checked_0(({ let value = __sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add({ let value = __sn_concurrency0_global_counter.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }), "Runtime error: integer overflow in addition"))); __sn_interpolated });
}
