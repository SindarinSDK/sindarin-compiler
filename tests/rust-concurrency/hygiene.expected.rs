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

struct __sn_concurrency0_Join<T>(Option<std::thread::JoinHandle<T>>);
impl<T: Send + 'static> __sn_concurrency0_Join<T> {
    fn spawn<F: FnOnce() -> T + Send + 'static>(call: F) -> Self {
        Self(Some(std::thread::spawn(call)))
    }
}
impl<T> __sn_concurrency0_Join<T> {
    fn join(mut self) -> T {
        match self.0.take().expect("thread already joined").join() {
            Ok(value) => value,
            Err(error) => std::panic::resume_unwind(error),
        }
    }
    fn detach(mut self) { self.0.take(); }
}
impl<T> Drop for __sn_concurrency0_Join<T> {
    fn drop(&mut self) {
        if let Some(handle) = self.0.take() { let _ = handle.join(); }
    }
}

static __sn_concurrency0_global_rhs: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(1));
static __sn_concurrency0_global_value: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(2));

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

fn calculate() -> i64 {
    return 9;
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_rhs);
    std::sync::LazyLock::force(&__sn_concurrency0_global_value);
    { let __sn_concurrency0_rhs = 3; let mut __sn_concurrency0_value = __sn_concurrency0_global_rhs.lock().unwrap_or_else(|e| e.into_inner()); *__sn_concurrency0_value = *__sn_concurrency0_value + __sn_concurrency0_rhs; __sn_concurrency0_value.clone() };
    { let __sn_concurrency0_value = __sn_checked_0(({ let value = __sn_concurrency0_global_rhs.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add({ let value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }), "Runtime error: integer overflow in addition"); *__sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value.clone(); __sn_concurrency0_value };
    let mut handle: i64 = 0; let mut __sn_concurrency0_handle_handle: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || calculate()) }
);
    { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_handle.take() { handle = __sn_concurrency0_handle.join(); } handle.clone() }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_rhs.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{}", handle)); __sn_interpolated });
}
