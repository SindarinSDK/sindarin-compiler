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
    action: __SnClosure<dyn Fn(i64) -> i64>,
}
#[derive(Clone, Debug, PartialEq)]
struct Payload {
    text: String,
    values: Vec<i64>,
}

fn plusOne(x: i64) -> i64 {
    return (x).checked_add(1).expect("checked arithmetic failed");
}

fn apply(action: __SnClosure<dyn Fn(i64) -> i64>, x: i64) -> i64 {
    return ((action.clone()).0)(x.clone());
}

fn identity(action: __SnClosure<dyn Fn(i64) -> i64>) -> __SnClosure<dyn Fn(i64) -> i64> {
    return { let (action, ) = (action.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { ((action.clone()).0)(x.clone())})) }
;
}

fn factory(offset: i64) -> __SnClosure<dyn Fn(i64) -> i64> {
    return { let (offset, ) = (offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { (x).checked_add(offset.clone()).expect("checked arithmetic failed")})) }
;
}

fn owned() -> __SnClosure<dyn Fn() -> String> {
    let mut text: String = "snapshot".to_string();
    let mut values: Vec<i64> = vec![7, 8];
    let mut payload: Payload = Payload { text: "owned".to_string(), values: values.clone() };
    let mut callback: __SnClosure<dyn Fn(i64) -> i64> = factory(30);
    return { let (text, values, payload, callback, ) = (text.clone(), values.clone(), payload.clone(), callback.clone(), ); self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", text.clone())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values.clone())[__sn_index((values.clone()).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (payload.clone()).text)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((callback.clone()).0)(2))); __sn_interpolated }})) }
;
}

fn main() {
    let mut add: __SnClosure<dyn Fn(i64, i64) -> i64> = { self::__SnClosure::<dyn Fn(i64, i64) -> i64>(std::rc::Rc::new(move |a: i64, b: i64| -> i64 { (a).checked_add(b).expect("checked arithmetic failed")})) }
;
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((add.clone()).0)(2, 3))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut named: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { x.clone()})) }
;
    { named = self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(plusOne)); named.clone() };
    let mut alias: __SnClosure<dyn Fn(i64) -> i64> = identity(named.clone());
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", apply(alias.clone(), 4))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((named.clone()).0)(5))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    { named = factory(100); named.clone() };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((alias.clone()).0)(5))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((named.clone()).0)(5))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut outer: i64 = 10;
    let mut first: __SnClosure<dyn Fn(i64) -> i64> = { let (outer, ) = (outer.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { (outer.clone()).checked_add(x).expect("checked arithmetic failed")})) }
;
    let mut sibling: __SnClosure<dyn Fn(i64) -> i64> = { let (outer, ) = (outer.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { (outer.clone()).checked_sub(x).expect("checked arithmetic failed")})) }
;
    (outer = 50);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((first.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((sibling.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", outer)); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut h: Holder = Holder { action: factory(10) };
    let mut copied: Holder = h.clone();
    ((h).action = factory(100));
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((copied).action.clone()).0)(2))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((h).action.clone()).0)(2))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut callbacks: Vec<__SnClosure<dyn Fn(i64) -> i64>> = vec![self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(plusOne)), (first.clone()).clone(), factory(20)];
    let mut arrays: Vec<__SnClosure<dyn Fn(i64) -> i64>> = callbacks.clone();
    (callbacks = vec![factory(100)]);
    for mut action in (arrays).iter().cloned() {
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((action.clone()).0)(3))); __sn_interpolated.push_str("\n"); __sn_interpolated });
        { action = self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(plusOne)); action.clone() };
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((action.clone()).0)(4))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    }
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (({ let (__sn_functions, __sn_function_index) = (&(callbacks), 0); __sn_functions[__sn_index(__sn_functions.len(), __sn_function_index)].clone() }).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (({ let (__sn_functions, __sn_function_index) = (&(arrays), 0); __sn_functions[__sn_index(__sn_functions.len(), __sn_function_index)].clone() }).0)(1))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut build: __SnClosure<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> i64>>> = { let (outer, ) = (outer.clone(), ); self::__SnClosure::<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> i64>>>(std::rc::Rc::new(move |a: i64| -> __SnClosure<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> i64>> { return { let (outer, a, ) = (outer.clone(), a.clone(), ); self::__SnClosure::<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> i64>>(std::rc::Rc::new(move |b: i64| -> __SnClosure<dyn Fn(i64) -> i64> { return { let (outer, a, b, ) = (outer.clone(), a.clone(), b.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |c: i64| -> i64 { (((outer.clone()).checked_add(a.clone()).expect("checked arithmetic failed")).checked_add(b.clone()).expect("checked arithmetic failed")).checked_add(c).expect("checked arithmetic failed")})) }
 ;})) }
;})) }
;
    (outer = 99);
    let mut middle: __SnClosure<dyn Fn(i64) -> __SnClosure<dyn Fn(i64) -> i64>> = ((build.clone()).0)(1);
    let mut inner: __SnClosure<dyn Fn(i64) -> i64> = ((middle.clone()).0)(2);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((inner.clone()).0)(3))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut other: __SnClosure<dyn Fn(i64) -> i64> = factory(200);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((other.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((first.clone()).0)(1))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut producer: __SnClosure<dyn Fn() -> String> = owned();
    let mut producerCopy: __SnClosure<dyn Fn() -> String> = { self::__SnClosure::<dyn Fn() -> String>(std::rc::Rc::new(move || -> String { "unused".to_string()})) }
;
    { producerCopy = producer.clone(); producerCopy.clone() };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((producer.clone()).0)())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((producerCopy.clone()).0)())); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
