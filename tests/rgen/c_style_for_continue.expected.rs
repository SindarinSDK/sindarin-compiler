#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut odd_total: i64 = 0;
    {
        let mut i: i64 = 0;

        while (i < 6) {
            if ((i).checked_rem(2).expect("checked arithmetic failed") == 0) {
        { { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous }; continue; }
    }
            (odd_total = (odd_total).checked_add(i).expect("checked arithmetic failed"));
            { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", odd_total);
    let mut nested_total: i64 = 0;
    {
        let mut outer: i64 = 0;

        while (outer < 3) {
            let mut inner: i64 = 0;
            while (inner < 3) {
        { let __sn_place = &mut (inner); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        if (inner < 3) {
        continue;
    }
        (nested_total = (nested_total).checked_add(outer).expect("checked arithmetic failed"));
    }
            { let __sn_place = &mut (outer); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", nested_total);
    let mut pair_total: i64 = 0;
    {
        let mut row: i64 = 0;

        while (row < 3) {
            {
        let mut column: i64 = 0;

        while (column < 3) {
            if (row == column) {
        { { let __sn_place = &mut (column); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous }; continue; }
    }
            { let __sn_place = &mut (pair_total); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
            { let __sn_place = &mut (column); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        }
    }
            { let __sn_place = &mut (row); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", pair_total);
}
