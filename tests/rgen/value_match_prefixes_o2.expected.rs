#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Prefixes {
    marker: i64,
}

impl Prefixes {
    fn staticMark(calls: &mut i64, order: &mut i64, marker: i64) -> i64 {
        return markInt(&mut *(calls), &mut *(order), marker, 0);
    }
    fn instanceMark(&self, calls: &mut i64, order: &mut i64, marker: i64) -> i64 {
        return markInt(&mut *(calls), &mut *(order), marker, (self).marker);
    }
    fn chooseBool(value: bool, calls: &mut i64, order: &mut i64) -> bool {
        return match (value) {
         true => {
             { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
             (*(order) = ((*(order) * 10) + 1));
             markInt(&mut *(calls), &mut *(order), 2, 0);
             (markBool(&mut *(calls), &mut *(order), 3, true))
         },
         false => {
             markInt(&mut *(calls), &mut *(order), 4, 0);
             (false)
         },
         _ => {
             markInt(&mut *(calls), &mut *(order), 5, 0);
             (false)
         },
     };
    }
    fn chooseDouble(&self, value: f64, calls: &mut i64, order: &mut i64) -> f64 {
        return {
     let __sn_match_subject_0: f64 = value;
     if (__sn_match_subject_0 == 1.5) {
         markInt(&mut *(calls), &mut *(order), 4, 0);
         (markDouble(&mut *(calls), &mut *(order), 5, 2.5) as f64)
     }
     else {
         markInt(&mut *(calls), &mut *(order), 6, 0);
         (3.5 as f64)
     }
 };
    }
}

fn markInt(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> i64 {
    { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
    (*(order) = ((*(order) * 10) + marker));
    return value;
}

fn markBool(calls: &mut i64, order: &mut i64, marker: i64, value: bool) -> bool {
    { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
    (*(order) = ((*(order) * 10) + marker));
    return value;
}

fn markDouble(calls: &mut i64, order: &mut i64, marker: i64, value: f64) -> f64 {
    { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
    (*(order) = ((*(order) * 10) + marker));
    return value;
}

fn markString(calls: &mut i64, order: &mut i64, marker: i64, value: String) -> String {
    { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
    (*(order) = ((*(order) * 10) + marker));
    return value;
}

fn acceptInt(value: i64) -> i64 {
    return value;
}

fn chooseInt(value: i64, calls: &mut i64, order: &mut i64) -> i64 {
    let mut prefixes: Prefixes = Prefixes { marker: 0 };
    return match (markInt(&mut *(calls), &mut *(order), 1, value) as i64) {
         1 => {
             markInt(&mut *(calls), &mut *(order), 7, 0);
             (10 as i64)
         },
         2 => {
             markInt(&mut *(calls), &mut *(order), 2, 0);
             Prefixes::staticMark(&mut *(calls), &mut *(order), 3);
             (prefixes).instanceMark(&mut *(calls), &mut *(order), 4);
             match (true) {
         true => {
             markInt(&mut *(calls), &mut *(order), 5, 0);
         },
         _ => {
             markInt(&mut *(calls), &mut *(order), 9, 0);
         },
     };
             (markInt(&mut *(calls), &mut *(order), 6, 20) as i64)
         },
         2 => {
             markInt(&mut *(calls), &mut *(order), 7, 0);
             (markInt(&mut *(calls), &mut *(order), 8, 30) as i64)
         },
         _ => {
             markInt(&mut *(calls), &mut *(order), 9, 0);
             (40 as i64)
         },
     };
}

fn scalarFamilies(calls: &mut i64) -> bool {
    let mut order: i64 = 0;
    let mut longResult: i64 = match (1 as u8) {
        1 => {
            markInt(&mut *(calls), &mut (order), 1, 0);
            (2 as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut int32Result: i32 = match (2 as u64) {
        2 => {
            markInt(&mut *(calls), &mut (order), 2, 0);
            (3 as i32)
        },
        _ => {
            (0 as i32)
        },
    };
    let mut uint32Result: u32 = match (3 as i32) {
        3 => {
            markInt(&mut *(calls), &mut (order), 3, 0);
            (4 as u32)
        },
        _ => {
            (0 as u32)
        },
    };
    let mut uintResult: u64 = match (4 as u32) {
        4 => {
            markInt(&mut *(calls), &mut (order), 4, 0);
            (5 as u64)
        },
        _ => {
            (0 as u64)
        },
    };
    let mut byteResult: u8 = match (5 as i64) {
        5 => {
            markInt(&mut *(calls), &mut (order), 5, 0);
            (6 as u8)
        },
        _ => {
            (0 as u8)
        },
    };
    let mut floatResult: f32 = {
    let __sn_match_subject_1: f32 = 6.0;
    if (__sn_match_subject_1 == 6.0) {
        markInt(&mut *(calls), &mut (order), 6, 0);
        (7.0 as f32)
    }
    else {
        (0.0 as f32)
    }
};
    return (((((((longResult == 2) && (int32Result == 3)) && (uint32Result == 4)) && (uintResult == 5)) && (byteResult == 6)) && (floatResult == 7.0)) && (order == 123456));
}

fn main() {
    let mut calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = chooseInt(2, &mut (calls), &mut (order));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated });
    (calls = 0);
    (order = 0);
    let mut boolResult: bool = Prefixes::chooseBool(true, &mut (calls), &mut (order));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", boolResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated });
    (calls = 0);
    (order = 0);
    let mut prefixes: Prefixes = Prefixes { marker: 0 };
    let mut doubleResult: f64 = (prefixes).chooseDouble(1.5, &mut (calls), &mut (order));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{:.5}", doubleResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated });
    (calls = 0);
    (order = 0);
    let mut borrowed: String = "borrowed".to_string();
    let mut borrowedResult: String = {
    let __sn_match_subject_2: String = markString(&mut (calls), &mut (order), 1, "key".to_string());
    if (__sn_match_subject_2.as_str() == "key") {
        markString(&mut (calls), &mut (order), 2, "discard-owned".to_string());
        match (1 as i64) {
        1 => {
            markInt(&mut (calls), &mut (order), 3, 0);
        },
        _ => {
            markInt(&mut (calls), &mut (order), 9, 0);
        },
    };
        (borrowed.clone())
    }
    else if (__sn_match_subject_2.as_str() == "key") {
        markString(&mut (calls), &mut (order), 8, "duplicate".to_string());
        ("wrong".to_string())
    }
    else {
        markString(&mut (calls), &mut (order), 9, "else".to_string());
        ("wrong".to_string())
    }
};
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", borrowedResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", borrowed)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated });
    (calls = 0);
    (order = 0);
    let mut ownedResult: String = {
    let __sn_match_subject_3: String = "owned".to_string();
    if (__sn_match_subject_3.as_str() == "owned") {
        markString(&mut (calls), &mut (order), 5, "discard-owned".to_string());
        (markString(&mut (calls), &mut (order), 6, "owned-final".to_string()))
    }
    else {
        (borrowed.clone())
    }
};
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ownedResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", borrowed)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated });
    (calls = 0);
    (order = 0);
    let mut argumentResult: i64 = acceptInt(70);
    let mut nestedResult: i64 = match (false) {
        true => {
            (0 as i64)
        },
        _ => {
            markInt(&mut (calls), &mut (order), 8, 0);
            (match (1 as i64) {
        1 => {
            markInt(&mut (calls), &mut (order), 9, 0);
            (90 as i64)
        },
        _ => {
            (0 as i64)
        },
    } as i64)
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", argumentResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", nestedResult)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated });
    (calls = 0);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", scalarFamilies(&mut (calls)))); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated });
}
