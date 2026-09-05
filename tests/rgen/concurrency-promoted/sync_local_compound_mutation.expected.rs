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
    { let __sn_concurrency0_numeric_rhs = (1).clone(); let mut __sn_concurrency0_value_guard = counter.lock().unwrap_or_else(|e| e.into_inner()); { let (__sn_rhs, __sn_place): (i64, &mut i64) = (__sn_concurrency0_numeric_rhs, &mut ((*__sn_concurrency0_value_guard))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next } };
}
