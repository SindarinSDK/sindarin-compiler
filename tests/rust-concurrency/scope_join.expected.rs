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

static __sn_concurrency0_global_completed: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));

fn work() -> i64 {
    { let __sn_concurrency0_gate = __sn_concurrency0_global_completed.guard(); let mut __sn_concurrency0_value = __sn_concurrency0_global_completed.lock().unwrap_or_else(|e| e.into_inner()); let __sn_concurrency0_previous = *__sn_concurrency0_value; *__sn_concurrency0_value += 1; drop(__sn_concurrency0_value); drop(__sn_concurrency0_gate); { let value = __sn_concurrency0_global_completed.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } };
    return 7;
}

fn launch() {
    let mut result: i64 = 0; let mut __sn_concurrency0_handle_result: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || work()) }
);
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_completed);
    launch();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("completed: "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_completed.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated });
}
