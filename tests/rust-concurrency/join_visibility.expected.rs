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

static __sn_concurrency0_global_gate: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));
static __sn_concurrency0_global_count: std::sync::LazyLock<__sn_concurrency0_Cell<i64>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(0));

fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

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

fn worker() -> i64 {
    for mut i in ((0..100).collect::<Vec<i64>>()).iter().cloned() {
        { let __sn_concurrency0_lock_guard = __sn_concurrency0_global_gate.guard(); {
        { let __sn_concurrency0_value = __sn_checked_0(({ let value = __sn_concurrency0_global_count.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).checked_add(1), "Runtime error: integer overflow in addition"); *__sn_concurrency0_global_count.lock().unwrap_or_else(|e| e.into_inner()) = __sn_concurrency0_value.clone(); __sn_concurrency0_value };
    } }

    }
    return 100;
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_gate);
    std::sync::LazyLock::force(&__sn_concurrency0_global_count);
    let mut a: i64 = 0; let mut __sn_concurrency0_handle_a: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || worker()) }
);
    let mut b: i64 = 0; let mut __sn_concurrency0_handle_b: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || worker()) }
);
    { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_a.take() { a = __sn_concurrency0_handle.join(); } if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_b.take() { b = __sn_concurrency0_handle.join(); }  }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("count: "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_count.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated.push_str(", returns: "); __sn_interpolated.push_str(&format!("{}", __sn_checked_0((a).checked_add(b), "Runtime error: integer overflow in addition"))); __sn_interpolated });
}
