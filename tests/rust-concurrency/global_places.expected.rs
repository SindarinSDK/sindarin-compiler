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

static __sn_concurrency0_global_numbers: std::sync::LazyLock<__sn_concurrency0_Cell<Vec<i64>>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(vec![1, 2]));
static __sn_concurrency0_global_pair: std::sync::LazyLock<__sn_concurrency0_Cell<Pair>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new(Pair { number: 3 }));
static __sn_concurrency0_global_text: std::sync::LazyLock<__sn_concurrency0_Cell<String>> = std::sync::LazyLock::new(|| __sn_concurrency0_Cell::new("a".to_string()));

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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Pair {
    number: i64,
}

fn main() {
    std::sync::LazyLock::force(&__sn_concurrency0_global_numbers);
    std::sync::LazyLock::force(&__sn_concurrency0_global_pair);
    std::sync::LazyLock::force(&__sn_concurrency0_global_text);
    { let __sn_concurrency0_operand_index = (1).clone(); let __sn_concurrency0_operand_value = (9).clone(); let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_numbers.lock().unwrap_or_else(|e| e.into_inner()); { let __sn_array_index = __sn_index(((*__sn_concurrency0_value_guard)).len(), __sn_concurrency0_operand_index); ((*__sn_concurrency0_value_guard))[__sn_array_index] = __sn_concurrency0_operand_value; } };
    { let __sn_concurrency0_operand_value = (8).clone(); let mut __sn_concurrency0_value_guard = __sn_concurrency0_global_pair.lock().unwrap_or_else(|e| e.into_inner()); (((*__sn_concurrency0_value_guard)).number = __sn_concurrency0_operand_value) };
    { let __sn_concurrency0_rhs = "b".to_string(); let mut __sn_concurrency0_value = __sn_concurrency0_global_text.lock().unwrap_or_else(|e| e.into_inner()); __sn_concurrency0_value.push_str(&__sn_concurrency0_rhs); __sn_concurrency0_value.clone() };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ({ let value = __sn_concurrency0_global_numbers.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })[__sn_index(({ let value = __sn_concurrency0_global_numbers.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).len(), 1)])); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{}", ({ let value = __sn_concurrency0_global_pair.lock().unwrap_or_else(|e| e.into_inner()).clone(); value }).number)); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{}", { let value = __sn_concurrency0_global_text.lock().unwrap_or_else(|e| e.into_inner()).clone(); value })); __sn_interpolated });
}
