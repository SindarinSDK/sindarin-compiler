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
struct Matcher {
    text: String,
}

impl Matcher {
    fn instanceValue(&self) -> i64 {
        let mut result: i64 = {
    let __sn_match_subject_1: String = (self).text.clone();
    if (__sn_match_subject_1.as_str() == "north") {
        (10 as i64)
    }
    else if (__sn_match_subject_1.as_str() == "north") {
        (20 as i64)
    }
    else {
        (0 as i64)
    }
};
        return result;
    }
    fn staticValue(value: String) -> bool {
        return {
     let __sn_match_subject_2: String = value.clone();
     if (__sn_match_subject_2.as_str() == "") {
         (false)
     }
     else if (__sn_match_subject_2.as_str() == "héllo" || __sn_match_subject_2.as_str() == "hello") {
         (true)
     }
     else {
         (false)
     }
 };
    }
}

fn makeSubject(calls: &mut i64) -> String {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return { let mut __sn_string = String::new(); __sn_string.push_str(&("to".to_string())); __sn_string.push_str(&("ken".to_string())); __sn_string }
;
}

fn makeLabels(calls: &mut i64) -> Vec<String> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec!["indexed".to_string()];
}

fn makeRows(calls: &mut i64) -> Vec<Vec<String>> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![vec!["nested".to_string()]];
}

fn recordIndex(trace: &mut i64, marker: i64) -> i64 {
    (*(trace) = __sn_checked((__sn_checked((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(marker), "Runtime error: integer overflow in addition")
);
    return (-1);
}

fn makeCube(trace: &mut i64) -> Vec<Vec<Vec<String>>> {
    (*(trace) = __sn_checked((__sn_checked((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(1), "Runtime error: integer overflow in addition")
);
    return vec![vec![vec!["deep".to_string()]]];
}

fn selectedValue(calls: &mut i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn parameterValue(value: String) -> i64 {
    let mut result: i64 = {
    let __sn_match_subject_3: String = value.clone();
    if (__sn_match_subject_3.as_str() == "token") {
        (7 as i64)
    }
    else {
        (0 as i64)
    }
};
    return result;
}

fn main() {
    let mut subject: String = "token".to_string();
    let mut statementCalls: i64 = 0;
    {
    let __sn_match_subject_4: String = subject.clone();
    if (__sn_match_subject_4.as_str() == "miss") {
        (statementCalls = 100);
    }
    else if (__sn_match_subject_4.as_str() == "token" || __sn_match_subject_4.as_str() == "token") {
        { let __sn_place = &mut (statementCalls); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
    else {
        (statementCalls = 200);
    }
};
    println!("{}", ((statementCalls == 1)
 && (subject == "token".to_string())
)
);
    let mut subjectCalls: i64 = 0;
    let mut armCalls: i64 = 0;
    let mut chosen: i64 = {
    let __sn_match_subject_5: String = makeSubject(&mut (subjectCalls));
    if (__sn_match_subject_5.as_str() == "miss") {
        (selectedValue(&mut (armCalls), 1) as i64)
    }
    else if (__sn_match_subject_5.as_str() == "token") {
        (selectedValue(&mut (armCalls), 7) as i64)
    }
    else if (__sn_match_subject_5.as_str() == "token") {
        (selectedValue(&mut (armCalls), 9) as i64)
    }
    else {
        (selectedValue(&mut (armCalls), 0) as i64)
    }
};
    println!("{}", (((chosen == 7)
 && (subjectCalls == 1)
)
 && (armCalls == 1)
)
);
    let mut noElseCalls: i64 = 0;
    {
    let __sn_match_subject_6: String = "absent".to_string();
    if (__sn_match_subject_6.as_str() == "present") {
        { let __sn_place = &mut (noElseCalls); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (noElseCalls == 0)
);
    let mut literalResult: bool = {
    let __sn_match_subject_7: String = "".to_string();
    if (__sn_match_subject_7.as_str() == "") {
        (true)
    }
    else {
        (false)
    }
};
    println!("{}", literalResult);
    let mut nested: bool = {
    let __sn_match_subject_9: String = "outer".to_string();
    if (__sn_match_subject_9.as_str() == "outer") {
        ({
    let __sn_match_subject_8: String = subject.clone();
    if (__sn_match_subject_8.as_str() == "token") {
        (true)
    }
    else {
        (false)
    }
})
    }
    else {
        (false)
    }
};
    println!("{}", nested);
    let mut __sn_match_subject: String = "source-subject".to_string();
    let mut __sn_match_array: i64 = 0;
    let mut __sn_match_index: i64 = 41;
    let mut __sn_match_subject_0: String = "candidate-subject".to_string();
    let mut __sn_match_array_0: i64 = 7;
    let mut __sn_match_index_0: i64 = 8;
    let mut hygieneCalls: i64 = 0;
    let mut hygieneRows: Vec<Vec<String>> = vec![vec!["nested".to_string()]];
    {
    let __sn_match_array_12 = &(makeRows(&mut (hygieneCalls)));
    let __sn_match_index_12 = __sn_index(__sn_match_array_12.len(), 0);
    let __sn_match_array_12 = &(__sn_match_array_12[__sn_match_index_12]);
    let __sn_match_index_12 = __sn_index(__sn_match_array_12.len(), 0);
    let __sn_match_subject_12: String = __sn_match_array_12[__sn_match_index_12].clone();
    if (__sn_match_subject_12.as_str() == "nested") {
        (__sn_match_subject = __sn_match_subject.clone());
        (__sn_match_array = __sn_checked((__sn_match_index).checked_add(1), "Runtime error: integer overflow in addition")
);
        (__sn_match_subject_0 = __sn_match_subject_0.clone());
        (__sn_match_array_0 = __sn_checked((__sn_match_index_0).checked_add(1), "Runtime error: integer overflow in addition")
);
        {
    let __sn_match_array_10 = &(hygieneRows);
    let __sn_match_index_10 = __sn_index(__sn_match_array_10.len(), 0);
    let __sn_match_array_10 = &(__sn_match_array_10[__sn_match_index_10]);
    let __sn_match_index_10 = __sn_index(__sn_match_array_10.len(), 0);
    let __sn_match_subject_10: String = __sn_match_array_10[__sn_match_index_10].clone();
    if (__sn_match_subject_10.as_str() == "nested") {
        (__sn_match_index = __sn_checked((__sn_match_array).checked_sub(1), "Runtime error: integer overflow in subtraction")
);
        (__sn_match_index_0 = __sn_checked((__sn_match_array_0).checked_sub(1), "Runtime error: integer overflow in subtraction")
);
    }
};
        {
    let __sn_match_subject_11: f32 = 1.0;
    if (__sn_match_subject_11 == 1.0) {
        (__sn_match_subject = __sn_match_subject.clone());
    }
};
    }
};
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_subject)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_array)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_index)); __sn_interpolated });
    println!("{}", ((((hygieneCalls == 1)
 && (__sn_match_subject_0 == "candidate-subject".to_string())
)
 && (__sn_match_array_0 == 9)
)
 && (__sn_match_index_0 == 8)
)
);
    let mut labels: Vec<String> = vec!["indexed".to_string()];
    let mut indexed: bool = {
    let __sn_match_array_13 = &(labels);
    let __sn_match_index_13 = __sn_index(__sn_match_array_13.len(), 0);
    let __sn_match_subject_13: String = __sn_match_array_13[__sn_match_index_13].clone();
    if (__sn_match_subject_13.as_str() == "indexed") {
        (true)
    }
    else {
        (false)
    }
};
    println!("{}", (((indexed && ((labels)[__sn_index((labels).len(), 0)] == "indexed".to_string())
)
 && (__sn_match_array == 42)
)
 && (__sn_match_index == 41)
)
);
    let mut indexedReceiverCalls: i64 = 0;
    let mut indexedReceiverResult: i64 = {
    let __sn_match_array_14 = &(makeLabels(&mut (indexedReceiverCalls)));
    let __sn_match_index_14 = __sn_index(__sn_match_array_14.len(), 0);
    let __sn_match_subject_14: String = __sn_match_array_14[__sn_match_index_14].clone();
    if (__sn_match_subject_14.as_str() == "indexed") {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", indexedReceiverResult);
    println!("{}", indexedReceiverCalls);
    let mut nestedReceiverCalls: i64 = 0;
    let mut nestedReceiverResult: i64 = {
    let __sn_match_array_15 = &(makeRows(&mut (nestedReceiverCalls)));
    let __sn_match_index_15 = __sn_index(__sn_match_array_15.len(), 0);
    let __sn_match_array_15 = &(__sn_match_array_15[__sn_match_index_15]);
    let __sn_match_index_15 = __sn_index(__sn_match_array_15.len(), 0);
    let __sn_match_subject_15: String = __sn_match_array_15[__sn_match_index_15].clone();
    if (__sn_match_subject_15.as_str() == "nested") {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", nestedReceiverResult);
    println!("{}", nestedReceiverCalls);
    let mut nestedOrder: i64 = 0;
    let mut deepIndexedResult: i64 = {
    let __sn_match_array_16 = &(makeCube(&mut (nestedOrder)));
    let __sn_match_index_16 = __sn_index(__sn_match_array_16.len(), recordIndex(&mut (nestedOrder), 2));
    let __sn_match_array_16 = &(__sn_match_array_16[__sn_match_index_16]);
    let __sn_match_index_16 = __sn_index(__sn_match_array_16.len(), recordIndex(&mut (nestedOrder), 3));
    let __sn_match_array_16 = &(__sn_match_array_16[__sn_match_index_16]);
    let __sn_match_index_16 = __sn_index(__sn_match_array_16.len(), recordIndex(&mut (nestedOrder), 4));
    let __sn_match_subject_16: String = __sn_match_array_16[__sn_match_index_16].clone();
    if (__sn_match_subject_16.as_str() == "deep") {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", ((((deepIndexedResult == 1)
 && (nestedOrder == 1234)
)
 && (__sn_match_array == 42)
)
 && (__sn_match_index == 41)
)
);
    let mut concatenated: i64 = {
    let __sn_match_subject_17: String = { let mut __sn_string = String::new(); __sn_string.push_str(&("con".to_string())); __sn_string.push_str(&("tent".to_string())); __sn_string }
;
    if (__sn_match_subject_17.as_str() == "content") {
        (1 as i64)
    }
    else {
        (0 as i64)
    }
};
    println!("{}", (concatenated == 1)
);
    let mut matcher: Matcher = Matcher { text: "north".to_string() };
    println!("{}", (((matcher).instanceValue() == 10)
 && ((matcher).text == "north".to_string())
)
);
    let mut greeting: String = "héllo".to_string();
    println!("{}", (Matcher::staticValue(greeting.clone()) && (greeting == "héllo".to_string())
)
);
    println!("{}", ((parameterValue(subject.clone()) == 7)
 && (subject == "token".to_string())
)
);
}

