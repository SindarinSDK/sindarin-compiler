#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut directStatement: i64 = 0;
    {
    let __sn_match_subject_0: String = "alphabet".to_string();
    if (__sn_match_subject_0.as_str() == "alphabet") {
        (directStatement = 1);
    }
};
    let mut nestedStatement: i64 = 0;
    match (1 as i64) {
        1 => {
            {
    let __sn_match_subject_1: String = "nested".to_string();
    if (__sn_match_subject_1.as_str() == "nested") {
        (nestedStatement = 2);
    }
};
        },
        _ => {},
    };
    let mut directValue: i64 = {
    let __sn_match_subject_2: String = "value".to_string();
    if (__sn_match_subject_2.as_str() == "value") {
        (3 as i64)
    }
    else {
        (0 as i64)
    }
};
    let mut nestedValue: i64 = match (1 as i64) {
        1 => {
            ({
    let __sn_match_subject_3: String = "inside".to_string();
    if (__sn_match_subject_3.as_str() == "inside") {
        (4 as i64)
    }
    else {
        (0 as i64)
    }
} as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    println!("{}", ((((directStatement == 1) && (nestedStatement == 2)) && (directValue == 3)) && (nestedValue == 4)));
}
