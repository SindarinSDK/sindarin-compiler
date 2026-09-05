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
fn factory(value: i64) -> __SnClosure<dyn Fn() -> i64> {
    let mut result: __SnClosure<dyn Fn() -> i64> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { value.clone().clone()})) }
;
    if true {
        let mut value: i64 = 70;
        let mut inner: __SnClosure<dyn Fn() -> i64> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { value.clone().clone()})) }
;
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((inner.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
    }
    return result.clone();
}

fn main() {
    let mut value: i64 = 3;
    let mut before: __SnClosure<dyn Fn() -> i64> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { value.clone().clone()})) }
;
    if true {
        let mut value: i64 = 9;
        let mut inside: __SnClosure<dyn Fn() -> i64> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { value.clone().clone()})) }
;
        (value = 90);
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((inside.clone()).0)())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((before.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
    }
    let mut after: __SnClosure<dyn Fn() -> i64> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { value.clone().clone()})) }
;
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((after.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut made: __SnClosure<dyn Fn() -> i64> = factory(11);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((made.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
    for mut value in (vec![1, 2, 3]).iter().cloned() {
        let mut r#loop: __SnClosure<dyn Fn() -> i64> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { value.clone().clone()})) }
;
        (value = 99);
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((r#loop.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
    }
    let mut __sn_functions: i64 = 8;
    let mut __sn_function_index: i64 = 0;
    let mut callbacks: Vec<__SnClosure<dyn Fn() -> i64>> = vec![{ let (__sn_functions, ) = (__sn_functions.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { __sn_functions.clone().clone()})) }
];
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (({ let (__sn_functions, __sn_function_index) = (&(callbacks), __sn_function_index); __sn_functions[__sn_index(__sn_functions.len(), __sn_function_index)].clone() }).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
