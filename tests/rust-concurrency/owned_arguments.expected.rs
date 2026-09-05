#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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

fn combine(a: String, b: Vec<i64>) -> String {
    return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", a)); __sn_interpolated.push_str(": "); __sn_interpolated.push_str(&format!("{}", (b)[__sn_index((b).len(), 0)])); __sn_interpolated };
}

fn launch() -> String {
    let mut name: String = "kept".to_string();
    let mut data: Vec<i64> = vec![42];
    let mut result: String = String::new(); let mut __sn_concurrency0_handle_result: Option<__sn_concurrency0_Join<String>> = Some({ let __sn_concurrency0_arg0 = (name.clone()).clone(); let __sn_concurrency0_arg1 = (data).clone(); __sn_concurrency0_Join::spawn(move || combine(__sn_concurrency0_arg0.clone(), __sn_concurrency0_arg1)) }
);
    { if let Some(__sn_concurrency0_handle) = __sn_concurrency0_handle_result.take() { result = __sn_concurrency0_handle.join(); } result.clone() }
;
    (name = "changed".to_string());
    (data = vec![99]);
    return result;
}

fn main() {
    println!("{}", launch());
}
