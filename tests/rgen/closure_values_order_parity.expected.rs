#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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

struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    action: __SnClosure<dyn Fn(i64, i64) -> i64>,
}

fn combine(a: i64, b: i64) -> i64 {
    return ((a).checked_mul(10).expect("checked arithmetic failed")).checked_add(b).expect("checked arithmetic failed");
}

fn argument(value: i64) -> i64 {
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("arg"); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated.push_str("\n"); __sn_interpolated });
    return value;
}

fn arraySource() -> Vec<__SnClosure<dyn Fn(i64, i64) -> i64>> {
    print!("{}", "array\n".to_string());
    let mut callbacks: Vec<__SnClosure<dyn Fn(i64, i64) -> i64>> = vec![self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(combine))];
    return callbacks;
}

fn fieldSource() -> Holder {
    print!("{}", "field\n".to_string());
    let mut holder: Holder = Holder { action: { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { combine(a, b)})) }
 };
    return holder;
}

fn index() -> i64 {
    print!("{}", "index\n".to_string());
    return 0;
}

fn recursive(n: i64) -> i64 {
    if (n < 2) {
        return 1;
    }
    return (n).checked_mul(recursive((n).checked_sub(1).expect("checked arithmetic failed"))).expect("checked arithmetic failed");
}

fn main() {
    let mut selected: i64 = index();
    let mut callbacks: Vec<__SnClosure<dyn Fn(i64, i64) -> i64>> = arraySource();
    let mut first: i64 = argument(1);
    let mut second: i64 = argument(2);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (({ let (__sn_functions, __sn_function_index) = (&(callbacks), selected); __sn_functions[__sn_index(__sn_functions.len(), __sn_function_index)].clone() }).0)(first.clone(), second.clone()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut holder: Holder = fieldSource();
    let mut third: i64 = argument(3);
    let mut fourth: i64 = argument(4);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((holder).action.clone()).0)(third.clone(), fourth.clone()))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut factorial: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { x.clone()})) }
;
    { factorial = self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(recursive)); factorial.clone() };
    let mut copy: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { x.clone()})) }
;
    { copy = factorial.clone(); copy.clone() };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((copy.clone()).0)(5))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((factorial.clone()).0)(4))); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
