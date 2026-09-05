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

fn __sn_runtime_error(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error(message),
    }
}

fn __sn_checked_div<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

#[derive(Clone, Debug, PartialEq)]
struct ResultBox {
    label: String,
    rows: Vec<Vec<String>>,
}

impl ResultBox {
    fn memberResult(&self, calls: &mut i64) -> String {
        { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        return ((self).label).to_ascii_uppercase();
    }
    fn staticResult(calls: &mut i64) -> String {
        { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        return "static".to_string();
    }
}

fn selectSubject(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return 2;
}

fn ownedResult(calls: &mut i64, value: String) -> String {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("<"); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated.push_str(">"); __sn_interpolated };
}

fn chooseForReturn(value: bool, fallback: String) -> String {
    return match (value) {
         true => {
             ("returned".to_string())
         },
         _ => {
             (fallback.clone())
         },
     };
}

fn main() {
    let mut variableResult: String = "variable".to_string();
    let mut fallbackResult: String = "fallback".to_string();
    let mut r#box: ResultBox = ResultBox { label: "member".to_string(), rows: vec![vec!["zero".to_string(), "one".to_string()], vec!["two".to_string(), "three".to_string()]] };
    let mut localRows: Vec<Vec<String>> = vec![vec!["local-zero".to_string()], vec!["local-one".to_string()]];
    let mut escapedBorrowedSource: String = "borrowed\n\tquote:\" slash:\\".to_string();
    let mut escapedRows: Vec<Vec<String>> = vec![vec!["indexed\n\tquote:\" slash:\\".to_string()]];
    let mut subjectCalls: i64 = 0;
    let mut selectedCalls: i64 = 0;
    let mut selected: String = match (selectSubject(&mut (subjectCalls)) as i64) {
        1 => {
            (ownedResult(&mut (selectedCalls), "wrong".to_string()))
        },
        2 => {
            (ownedResult(&mut (selectedCalls), "selected".to_string()))
        },
        2 => {
            (ownedResult(&mut (selectedCalls), "duplicate".to_string()))
        },
        _ => {
            (ownedResult(&mut (selectedCalls), "else".to_string()))
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", subjectCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", selectedCalls)); __sn_interpolated });
    let mut literal: String = match (true) {
        true => {
            ("literal".to_string())
        },
        _ => {
            ("wrong".to_string())
        },
    };
    let mut variable: String = {
    let __sn_match_subject_0: f32 = 1.0;
    if (__sn_match_subject_0 == 1.0) {
        (variableResult.clone())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut member: String = {
    let __sn_match_subject_1: f64 = 1.0;
    if (__sn_match_subject_1 == 1.0) {
        ((r#box).label.clone())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut localIndexed: String = match (7 as i64) {
        7 => {
            (((localRows)[__sn_index((localRows).len(), 1)])[__sn_index(((localRows)[__sn_index((localRows).len(), 1)]).len(), 0)].clone())
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut memberIndexed: String = {
    let __sn_match_subject_2: String = "key".to_string();
    if (__sn_match_subject_2.as_str() == "key") {
        ((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)]).len(), 1)].clone())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut multiIndexed: String = match (false) {
        true => {
            (fallbackResult.clone())
        },
        _ => {
            ((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)]).len(), 1)].clone())
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", literal)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", variable)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", member)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", localIndexed)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", memberIndexed)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", multiIndexed)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", variableResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (r#box).label)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", ((localRows)[__sn_index((localRows).len(), 1)])[__sn_index(((localRows)[__sn_index((localRows).len(), 1)]).len(), 0)])); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (((r#box).rows)[__sn_index(((r#box).rows).len(), 0)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 0)]).len(), 1)])); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (((r#box).rows)[__sn_index(((r#box).rows).len(), 1)])[__sn_index((((r#box).rows)[__sn_index(((r#box).rows).len(), 1)]).len(), 1)])); __sn_interpolated });
    let mut concatenated: String = match (10 as i64) {
        10 => {
            ({ let mut __sn_string = String::new(); __sn_string.push_str(&("con".to_string())); __sn_string.push_str(&(variableResult)); __sn_string }
)
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut interpolated: String = match (10 as i32) {
        10 => {
            ({ let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("inter-"); __sn_interpolated.push_str(&format!("{}", variableResult)); __sn_interpolated })
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut freeCalls: i64 = 0;
    let mut freeCalled: String = match (10 as u64) {
        10 => {
            (ownedResult(&mut (freeCalls), "free".to_string()))
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut staticCalls: i64 = 0;
    let mut staticCalled: String = match (10 as u32) {
        10 => {
            (ResultBox::staticResult(&mut (staticCalls)))
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut memberCalls: i64 = 0;
    let mut memberCalled: String = match (10 as u8) {
        10 => {
            ((r#box).memberResult(&mut (memberCalls)))
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    let mut stringMemberCalled: String = {
    let __sn_match_subject_3: String = "upper".to_string();
    if (__sn_match_subject_3.as_str() == "upper") {
        ((variableResult).to_ascii_uppercase())
    }
    else {
        (fallbackResult.clone())
    }
};
    let mut joined: String = match (3 as i64) {
        3 => {
            ({ let __sn_array = &(((r#box).rows)[__sn_index(((r#box).rows).len(), 0)]); let __sn_separator = &("+".to_string()); __sn_array.join(__sn_separator.as_str()) })
        },
        _ => {
            (fallbackResult.clone())
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", concatenated)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", interpolated)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", freeCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", staticCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", memberCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", stringMemberCalled)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", joined)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", freeCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", staticCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", memberCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", variableResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (r#box).label)); __sn_interpolated });
    let mut nestedCalls: i64 = 0;
    let mut nested: String = {
    let __sn_match_subject_4: String = "outer".to_string();
    if (__sn_match_subject_4.as_str() == "outer") {
        (match (4 as i64) {
        4 => {
            (ownedResult(&mut (nestedCalls), "nested".to_string()))
        },
        _ => {
            ("inner-else".to_string())
        },
    })
    }
    else {
        ("outer-else".to_string())
    }
};
    let mut fallbackCalls: i64 = 0;
    let mut fallback: String = match (99 as i64) {
        1 => {
            (ownedResult(&mut (fallbackCalls), "ordinary".to_string()))
        },
        _ => {
            (ownedResult(&mut (fallbackCalls), "fallback".to_string()))
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", nested)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", nestedCalls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", fallback)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", fallbackCalls)); __sn_interpolated });
    let mut returned: String = chooseForReturn(false, fallbackResult.clone());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", returned)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", fallbackResult)); __sn_interpolated });
    let mut escapedDirect: String = match (true) {
        true => {
            ("direct\n\tquote:\" slash:\\".to_string())
        },
        _ => {
            ("wrong".to_string())
        },
    };
    let mut escapedBorrowed: String = match (1 as i64) {
        1 => {
            (escapedBorrowedSource.clone())
        },
        _ => {
            ("wrong".to_string())
        },
    };
    let mut escapedIndexed: String = match (false) {
        true => {
            ("wrong".to_string())
        },
        _ => {
            (((escapedRows)[__sn_index((escapedRows).len(), 0)])[__sn_index(((escapedRows)[__sn_index((escapedRows).len(), 0)]).len(), 0)].clone())
        },
    };
    let mut escapedNested: String = match (2 as i64) {
        2 => {
            ({
    let __sn_match_subject_5: String = "nested".to_string();
    if (__sn_match_subject_5.as_str() == "nested") {
        ("nested\n\tquote:\" slash:\\".to_string())
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
    print!("{}", "direct[".to_string());
    print!("{}", escapedDirect);
    println!("{}", "]".to_string());
    print!("{}", "borrowed[".to_string());
    print!("{}", escapedBorrowed);
    println!("{}", "]".to_string());
    print!("{}", "indexed[".to_string());
    print!("{}", escapedIndexed);
    println!("{}", "]".to_string());
    print!("{}", "nested[".to_string());
    print!("{}", escapedNested);
    println!("{}", "]".to_string());
}

