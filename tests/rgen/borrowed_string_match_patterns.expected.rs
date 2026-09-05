#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_0<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_0(message),
    }
}

fn __sn_checked_div_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

#[derive(Clone, Debug, PartialEq)]
struct Leaf {
    text: String,
}
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    direct: String,
    leaf: Leaf,
}

impl Holder {
    fn selfMatches(&self, subject: String) -> bool {
        return {
     let __sn_match_subject_1: String = subject.clone();
     if (__sn_match_subject_1.as_str() == (((self).leaf).text).as_str()) {
         (true)
     }
     else {
         (false)
     }
 };
    }
}

fn parameterMatch(subject: String, pattern: String) -> String {
    return {
     let __sn_match_subject_2: String = subject.clone();
     if (__sn_match_subject_2.as_str() == "miss" || __sn_match_subject_2.as_str() == (pattern).as_str() || __sn_match_subject_2.as_str() == (pattern).as_str()) {
         ("matched".to_string())
     }
     else {
         ("other".to_string())
     }
 };
}

fn main() {
    let mut empty: String = "".to_string();
    let mut utf8: String = "héllo-世界-🙂".to_string();
    let mut escaped: String = "quote:\" slash:\\ line:\n tab:\t".to_string();
    let mut leaf: Leaf = Leaf { text: "nested".to_string() };
    let mut holder: Holder = Holder { direct: "direct".to_string(), leaf: leaf.clone() };
    let mut statementHits: i64 = 0;
    {
    let __sn_match_subject_3: String = "nested".to_string();
    if (__sn_match_subject_3.as_str() == "miss" || __sn_match_subject_3.as_str() == ((holder).direct).as_str()) {
        (statementHits = 10);
    }
    else if (__sn_match_subject_3.as_str() == (((holder).leaf).text).as_str() || __sn_match_subject_3.as_str() == (((holder).leaf).text).as_str()) {
        { let __sn_place = &mut (statementHits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (statementHits == 1));
    let mut scalar: i64 = {
    let __sn_match_subject_4: String = "héllo-世界-🙂".to_string();
    if (__sn_match_subject_4.as_str() == (empty).as_str()) {
        (0 as i64)
    }
    else if (__sn_match_subject_4.as_str() == (utf8).as_str() || __sn_match_subject_4.as_str() == (utf8).as_str()) {
        (7 as i64)
    }
    else {
        ((-1) as i64)
    }
};
    println!("{}", (scalar == 7));
    let mut prefix: i64 = 0;
    let mut stringResult: String = {
    let __sn_match_subject_5: String = escaped.clone();
    if (__sn_match_subject_5.as_str() == (escaped).as_str()) {
        { let __sn_place = &mut (prefix); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        ("ok".to_string())
    }
    else {
        ("bad".to_string())
    }
};
    println!("{}", ((stringResult == "ok".to_string()) && (prefix == 1)));
    (utf8 = "changed".to_string());
    let mut later: bool = {
    let __sn_match_subject_6: String = "changed".to_string();
    if (__sn_match_subject_6.as_str() == (utf8).as_str()) {
        (true)
    }
    else {
        (false)
    }
};
    { let (__sn_string_part, __sn_string_place) = (("!".to_string()).clone(), &mut (utf8)); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
    println!("{}", (later && (utf8 == "changed!".to_string())));
    let mut nestedResult: bool = {
    let __sn_match_subject_8: String = "outer".to_string();
    if (__sn_match_subject_8.as_str() == "outer") {
        ({
    let __sn_match_subject_7: String = "direct".to_string();
    if (__sn_match_subject_7.as_str() == ((holder).direct).as_str()) {
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
    println!("{}", nestedResult);
    let mut __sn_match_subject_0: String = "helper".to_string();
    let mut __sn_match_array_0: String = "helper".to_string();
    let mut __sn_match_index_0: String = "helper".to_string();
    {
    let __sn_match_subject_9: String = "helper".to_string();
    if (__sn_match_subject_9.as_str() == (__sn_match_subject_0).as_str() || __sn_match_subject_9.as_str() == (__sn_match_array_0).as_str() || __sn_match_subject_9.as_str() == (__sn_match_index_0).as_str()) {
        { let __sn_place = &mut (prefix); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
};
    println!("{}", (prefix == 2));
    println!("{}", (parameterMatch("".to_string(), empty.clone()) == "matched".to_string()));
    println!("{}", (holder).selfMatches("nested".to_string()));
    (((holder).leaf).text = "after".to_string());
    println!("{}", ((((holder).leaf).text == "after".to_string()) && (empty == "".to_string())));
}
