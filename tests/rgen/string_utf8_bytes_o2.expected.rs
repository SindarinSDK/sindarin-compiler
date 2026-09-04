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

fn main() {
    let mut direct: String = "X\u{1f}AY".to_string();
    let mut longGreedy: String = "X\u{1f}Ab09Y".to_string();
    let mut lower: String = "x\u{1f}ay".to_string();
    let mut expectedFour: String = "X\u{1f}AY".to_string();
    let mut expectedSeven: String = "X\u{1f}Ab09Y".to_string();
    let mut expectedLower: String = "x\u{1f}ay".to_string();
    let mut borrowedSource: String = "X\u{1f}Ab09Y".to_string();
    let mut rows: Vec<String> = vec!["X\u{1f}Ab09Y".to_string()];
    let mut borrowed: String = match (1 as i64) {
        1 => {
            (borrowedSource.clone())
        },
        _ => {
            ("wrong".to_string())
        },
    };
    let mut indexed: String = match (2 as i64) {
        2 => {
            ((rows)[__sn_index((rows).len(), 0)].clone())
        },
        _ => {
            ("wrong".to_string())
        },
    };
    let mut nested: String = match (true) {
        true => {
            ({
    let __sn_match_subject_0: String = "X\u{1f}AY".to_string();
    if (__sn_match_subject_0.as_str() == "X\u{1f}AY") {
        ("X\u{1f}Ab09Y".to_string())
    }
    else {
        ("wrong-inner".to_string())
    }
})
        },
        _ => {
            ("wrong-outer".to_string())
        },
    };
    let mut result: String = match (false) {
        true => {
            ("wrong".to_string())
        },
        _ => {
            ("X\u{1f}AY".to_string())
        },
    };
    let mut concatenated: String = "X\u{1f}Ab09Y".to_string();
    let mut interpolated: String = { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("X\u{1f}Ab09Y"); __sn_interpolated };
    let mut boundaries: String = "߿ࠀ퟿￿𐀀􏿿".to_string();
    let mut controls: String = "\n\t\r\"\\".to_string();
    let mut unicode: String = "é世界🙂".to_string();
    println!("{}", (direct == expectedFour));
    println!("{}", (longGreedy == expectedSeven));
    println!("{}", (lower == expectedLower));
    println!("{}", (((borrowed == expectedSeven) && (indexed == expectedSeven)) && (nested == expectedSeven)));
    println!("{}", (result == expectedFour));
    println!("{}", ((concatenated == expectedSeven) && (interpolated == expectedSeven)));
    println!("{}", (((boundaries).len() as i64 == 24) && ((controls).len() as i64 == 5)));
    println!("{}", ((unicode == "é世界🙂".to_string()) && ((unicode).len() as i64 == 12)));
    println!("{}", direct);
    println!("{}", longGreedy);
    println!("{}", lower);
    println!("{}", unicode);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (boundaries).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (controls).len() as i64)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (unicode).len() as i64)); __sn_interpolated });
}
