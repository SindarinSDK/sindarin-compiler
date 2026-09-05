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

fn main() {
    let value: __sn_concurrency0_Cell<f64> = __sn_concurrency0_Cell::new(1.0);
    { let mut __sn_concurrency0_value_guard = { (*value.lock().unwrap_or_else(|e| e.into_inner())).clone() }; let __sn_concurrency0_numeric_rhs = (1.0).clone(); let __sn_concurrency0_value = { { let (__sn_rhs, __sn_place) = (__sn_concurrency0_numeric_rhs, &mut (__sn_concurrency0_value_guard)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next } }; *value.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value_guard; __sn_concurrency0_value };
}
