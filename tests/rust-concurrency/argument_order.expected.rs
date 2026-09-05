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

// Arc owns capture identity. Reads clone under the lock and release it before
// evaluating the next source operand; mutable operations borrow the same cell.
struct __sn_concurrency0_Capture<T>(std::sync::Mutex<T>, std::sync::Mutex<()>);
impl<T> __sn_concurrency0_Capture<T> {
    fn new(value: T) -> Self { Self(std::sync::Mutex::new(value), std::sync::Mutex::new(())) }
    fn lock(&self) -> std::sync::LockResult<std::sync::MutexGuard<'_, T>> { self.0.lock() }
    fn guard(&self) -> std::sync::MutexGuard<'_, ()> { self.1.lock().unwrap_or_else(|e| e.into_inner()) }
    fn borrow_mut(&self) -> std::sync::MutexGuard<'_, T> { self.0.lock().unwrap_or_else(|e| e.into_inner()) }
    fn set(&self, value: T) { *self.borrow_mut() = value; }
    fn replace(&self, value: T) -> T { std::mem::replace(&mut *self.borrow_mut(), value) }
}
impl<T: Clone> __sn_concurrency0_Capture<T> {
    fn borrow(&self) -> T { self.borrow_mut().clone() }
    fn get(&self) -> T { self.borrow() }
}

static __sn_concurrency0_global_sequence: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));

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

fn next() -> i64 {
    { let __sn_concurrency0_value = __sn_checked_0((({ let value = __sn_concurrency0_global_sequence.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } as i64)).checked_add(1), "Runtime error: integer overflow in addition"); *__sn_concurrency0_global_sequence.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value.clone(); __sn_concurrency0_value };
    return { let value = __sn_concurrency0_global_sequence.lock().unwrap_or_else(|e| e.into_inner()).clone(); value };
}

fn combine(a: i64, b: i64) -> i64 {
    return __sn_checked_0(((__sn_checked_0(((a as i64)).checked_mul(10), "Runtime error: integer overflow in multiplication") as i64)).checked_add(b), "Runtime error: integer overflow in addition");
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_sequence);
    let mut result: i64 = 0; let mut __sn_concurrency0_handle_result: Option<__sn_concurrency0_Join<i64>> = Some({ let __sn_concurrency0_arg0 = (next()).clone(); let __sn_concurrency0_arg1 = (next()).clone(); __sn_concurrency0_Join::spawn(move || combine(__sn_concurrency0_arg0, __sn_concurrency0_arg1)) }
);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("before join: "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_sequence.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated });
    { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_result.take() { result = __sn_concurrency0_handle.join(); } result.clone() }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("result: "); __sn_interpolated.push_str(&format!("{}", result)); __sn_interpolated });
    { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_result.take() { result = __sn_concurrency0_handle.join(); } result.clone() }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("repeat join: "); __sn_interpolated.push_str(&format!("{}", result)); __sn_interpolated });
}
