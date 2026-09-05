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
    let counter: __sn_concurrency0_Cell<i64> = __sn_concurrency0_Cell::new(1);
    { let __sn_concurrency0_rhs = 1; let mut __sn_concurrency0_value = counter.lock().unwrap_or_else(|e| e.into_inner()); *__sn_concurrency0_value = *__sn_concurrency0_value + __sn_concurrency0_rhs; __sn_concurrency0_value.clone() };
}
