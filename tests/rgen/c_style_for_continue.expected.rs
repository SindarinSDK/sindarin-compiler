#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut odd_total: i64 = 0;
    {
        let mut i: i64 = 0;

        while (i < 6) {
            if ((i).checked_rem(2).expect("checked arithmetic failed") == 0) {
        { { i += 1; i }; continue; }
    }
            (odd_total = (odd_total).checked_add(i).expect("checked arithmetic failed"));
            { i += 1; i };
        }
    }
    println!("{}", odd_total);
    let mut nested_total: i64 = 0;
    {
        let mut outer: i64 = 0;

        while (outer < 3) {
            let mut inner: i64 = 0;
            while (inner < 3) {
        { inner += 1; inner };
        if (inner < 3) {
        continue;
    }
        (nested_total = (nested_total).checked_add(outer).expect("checked arithmetic failed"));
    }
            { outer += 1; outer };
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
        { { column += 1; column }; continue; }
    }
            { pair_total += 1; pair_total };
            { column += 1; column };
        }
    }
            { row += 1; row };
        }
    }
    println!("{}", pair_total);
}
