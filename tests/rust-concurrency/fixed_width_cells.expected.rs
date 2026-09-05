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

static __sn_concurrency0_global_atomic_value: std::sync::LazyLock<__sn_concurrency0_Cell<u8>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(255));
static __sn_concurrency0_global_byte_value: std::sync::LazyLock<__sn_concurrency0_Cell<u8>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(255));
static __sn_concurrency0_global_wide_value: std::sync::LazyLock<__sn_concurrency0_Cell<u32>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(4294967295));

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
    { let __sn_concurrency0_numeric_rhs = (1).clone(); let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_byte_value.lock().unwrap_or_else(|e| e.into_inner()); { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (__sn_concurrency0_numeric_rhs, &mut ((*__sn_concurrency0_value_guard))); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next } };
    { let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_byte_value.lock().unwrap_or_else(|e| e.into_inner()); { let __sn_byte_place = &mut ((*__sn_concurrency0_value_guard)); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous } };
    { let __sn_concurrency0_numeric_rhs = (2).clone(); let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_byte_value.lock().unwrap_or_else(|e| e.into_inner()); { let (__sn_rhs, __sn_place) = (__sn_concurrency0_numeric_rhs, &mut ((*__sn_concurrency0_value_guard))); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition"); let __sn_next = __sn_promoted as u8; *__sn_place = __sn_next; __sn_next } };
    { let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_wide_value.lock().unwrap_or_else(|e| e.into_inner()); { let __sn_byte_place = &mut ((*__sn_concurrency0_value_guard)); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } };
    return 1;
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_atomic_value);
    std::sync::LazyLock::force(&__sn_concurrency0_global_byte_value);
    std::sync::LazyLock::force(&__sn_concurrency0_global_wide_value);
    let mut handle: i64 = 0; let mut __sn_concurrency0_handle_handle: Option<__sn_concurrency0_Join<i64>> = Some({ __sn_concurrency0_Join::spawn(move || worker()) }
);
    { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_handle.take() { handle = __sn_concurrency0_handle.join(); } handle.clone() }
;
    println!("0x{:02X}", ({ let value = __sn_concurrency0_global_byte_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } as u32));
    println!("{}", { let value = __sn_concurrency0_global_wide_value.lock().unwrap_or_else(|e| e.into_inner()).clone(); value });
    let mut next: u8 = { let __sn_concurrency0_gate = __sn_concurrency0_global_atomic_value.guard(); let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_atomic_value.lock().unwrap_or_else(|e| e.into_inner()); { let __sn_byte_place = &mut ((*__sn_concurrency0_value_guard)); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous }; let __sn_concurrency0_value = (*__sn_concurrency0_value_guard).clone(); drop(__sn_concurrency0_value_guard); drop(__sn_concurrency0_gate); __sn_concurrency0_value };
    println!("0x{:02X}", (next as u32));
    let local: __sn_concurrency0_Cell<u8> = __sn_concurrency0_Cell::new(255);
    let mut previous: u8 = { let __sn_concurrency0_gate = local.guard(); let mut __sn_concurrency0_value_guard = local.lock().unwrap_or_else(|e| e.into_inner()); { let __sn_byte_place = &mut ((*__sn_concurrency0_value_guard)); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } };
    println!("0x{:02X}", (previous as u32));
    println!("0x{:02X}", ({ let value = local.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } as u32));
    { let __sn_concurrency0_numeric_rhs = (2).clone(); let mut __sn_concurrency0_value_guard = local.lock().unwrap_or_else(|e| e.into_inner()); { let (__sn_rhs, __sn_place) = (__sn_concurrency0_numeric_rhs, &mut ((*__sn_concurrency0_value_guard))); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_left.wrapping_sub(__sn_right); let __sn_next = __sn_promoted as u8; *__sn_place = __sn_next; __sn_next } };
    println!("0x{:02X}", ({ let value = local.lock().unwrap_or_else(|e| e.into_inner()).clone(); value } as u32));
}
