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

#[allow(non_snake_case)]
#[derive(Clone, Debug, PartialEq)]
struct FieldInfo {
    name: String,
    typeName: String,
    typeId: i64,
}

#[allow(non_snake_case)]
#[derive(Clone, Debug, PartialEq)]
struct TypeInfo {
    name: String,
    fields: Vec<FieldInfo>,
    fieldCount: i64,
    typeId: i64,
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct Inner {
    code: i32,
}
#[derive(Clone, Debug, PartialEq)]
struct Record {
    name: String,
    count: i64,
    flags: Vec<bool>,
    inner: Inner,
    ratio: f32,
}

fn touch(counter: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return *(counter);
}

fn reflectedRecord() -> TypeInfo {
    let mut value: Record = Record { name: "returned".to_string(), count: 2, flags: vec![true, false], inner: Inner { code: 7 }, ratio: 1.5 };
    let mut info: TypeInfo = TypeInfo { name: "Record".to_string(), fields: vec![FieldInfo { name: "name".to_string(), typeName: "str".to_string(), typeId: 1112265104 }, FieldInfo { name: "count".to_string(), typeName: "int".to_string(), typeId: 367623774 }, FieldInfo { name: "flags".to_string(), typeName: "array".to_string(), typeId: 173583654 }, FieldInfo { name: "inner".to_string(), typeName: "Inner".to_string(), typeId: 2124115655 }, FieldInfo { name: "ratio".to_string(), typeName: "float".to_string(), typeId: 650403205 }], fieldCount: 5, typeId: 524641772 };
    return info;
}

fn main() {
    let mut integer: i64 = 1;
    let mut long_value: i64 = 2;
    let mut int32_value: i32 = 3;
    let mut uint_value: u64 = 4;
    let mut uint32_value: u32 = 5;
    let mut double_value: f64 = 6.0;
    let mut float_value: f32 = 7.0;
    let mut bool_value: bool = true;
    let mut char_value: char = '\u{78}';
    let mut byte_value: u8 = 8;
    let mut string_value: String = "text".to_string();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "int".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 367623774)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "long".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 1122819923)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "int32".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 2078204607)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "uint".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 1268266657)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "uint32".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 848563180)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "double".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 552275720)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "float".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 650403205)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "bool".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 1217697085)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "char".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 676070173)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "byte".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 1683620383)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", "str".to_string())); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 1112265104)); __sn_interpolated });
    let mut numbers: Vec<i64> = vec![1, 2, 3];
    let mut words: Vec<String> = vec!["a".to_string(), "b".to_string()];
    let mut number_info: TypeInfo = TypeInfo { name: "array".to_string(), fields: vec![], fieldCount: 0, typeId: 173583654 };
    let mut word_info: TypeInfo = TypeInfo { name: "array".to_string(), fields: vec![], fieldCount: 0, typeId: 173583654 };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (number_info).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (number_info).fieldCount)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (number_info).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (word_info).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (word_info).fieldCount)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (word_info).typeId)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((number_info).typeId == (word_info).typeId)
)); __sn_interpolated });
    let mut inner: Inner = Inner { code: 9 };
    let mut record: Record = Record { name: "record".to_string(), count: 3, flags: vec![true], inner: inner, ratio: 2.5 };
    let mut info: TypeInfo = TypeInfo { name: "Record".to_string(), fields: vec![FieldInfo { name: "name".to_string(), typeName: "str".to_string(), typeId: 1112265104 }, FieldInfo { name: "count".to_string(), typeName: "int".to_string(), typeId: 367623774 }, FieldInfo { name: "flags".to_string(), typeName: "array".to_string(), typeId: 173583654 }, FieldInfo { name: "inner".to_string(), typeName: "Inner".to_string(), typeId: 2124115655 }, FieldInfo { name: "ratio".to_string(), typeName: "float".to_string(), typeId: 650403205 }], fieldCount: 5, typeId: 524641772 };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (info).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (info).fieldCount)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (info).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 0)]).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 0)]).typeName)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 0)]).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 1)]).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 1)]).typeName)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 1)]).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 2)]).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 2)]).typeName)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 2)]).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 3)]).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 3)]).typeName)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 3)]).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 4)]).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 4)]).typeName)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((info).fields)[__sn_index(((info).fields).len(), 4)]).typeId)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((((info).fields)[__sn_index(((info).fields).len(), 3)]).typeId == 2124115655)
)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (524641772 == (info).typeId)
)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (524641772 != 367623774)
)); __sn_interpolated });
    let mut assigned: TypeInfo = info.clone();
    ((assigned).fields).clear();
    let mut copied: TypeInfo = (info).clone();
    ((copied).fields).clear();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (info).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((info).fields).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (assigned).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((assigned).fields).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((copied).fields).len() as i64)); __sn_interpolated });
    let mut returned: TypeInfo = reflectedRecord();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (returned).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (returned).fieldCount)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (((returned).fields)[__sn_index(((returned).fields).len(), 3)]).typeName)); __sn_interpolated });
    let mut counter: i64 = 0;
    let mut unevaluated: TypeInfo = TypeInfo { name: "int".to_string(), fields: vec![], fieldCount: 0, typeId: 367623774 };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", counter)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (unevaluated).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", 0)); __sn_interpolated });
}

