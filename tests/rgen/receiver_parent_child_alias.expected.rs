#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct ReceiverInner {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct ReceiverOuter {
    inner: ReceiverInner,
    count: i64,
}

impl ReceiverOuter {
    unsafe fn touch(__sn_raw_self_1: *mut Self, value: *mut i64) -> i64 {
        ((*(__sn_raw_self_1)).count = ((*(__sn_raw_self_1)).count).checked_add(1).expect("checked arithmetic failed"));
        (*(value) = (*(value)).checked_add(10).expect("checked arithmetic failed"));
        return ((*(__sn_raw_self_1)).count).checked_add(*(value)).expect("checked arithmetic failed");
    }
}

fn main() {
    let mut __sn_raw_self_0: i64 = 5;
    println!("{}", __sn_raw_self_0);
    let mut outer: ReceiverOuter = ReceiverOuter { inner: ReceiverInner { value: 1 }, count: 0 };
    println!("{}", unsafe { ReceiverOuter::touch(std::ptr::addr_of_mut!(outer), std::ptr::addr_of_mut!(((outer).inner).value)) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (outer).count)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((outer).inner).value)); __sn_interpolated });
}
