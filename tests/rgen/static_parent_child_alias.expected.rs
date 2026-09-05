#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Inner {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Outer {
    inner: Inner,
    count: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Ops {
}

impl Ops {
    unsafe fn touch(outer: *mut Outer, inner: *mut Inner) -> i64 {
        ((*(outer)).count = ((*(outer)).count).checked_add(1).expect("checked arithmetic failed"));
        ((*(inner)).value = ((*(inner)).value).checked_add(10).expect("checked arithmetic failed"));
        return ((*(outer)).count).checked_add((*(inner)).value).expect("checked arithmetic failed");
    }
}

fn main() {
    let mut outer: Outer = Outer { inner: Inner { value: 1 }, count: 0 };
    println!("{}", unsafe { Ops::touch(std::ptr::addr_of_mut!(outer), std::ptr::addr_of_mut!((outer).inner)) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (outer).count)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((outer).inner).value)); __sn_interpolated });
}
