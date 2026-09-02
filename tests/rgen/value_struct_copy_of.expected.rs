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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
    y: i64,
}
#[derive(Clone, Debug, PartialEq)]
struct Payload {
    name: String,
    values: Vec<i64>,
    point: Point,
}

fn main() {
    let mut point: Point = Point { x: 1, y: 2 };
    let mut point_copy: Point = (point).clone();
    ((point_copy).x = 10);
    ((point).y = 20);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("point="); __sn_interpolated.push_str(&format!("{}", (point).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (point).y)); __sn_interpolated.push_str("; copy="); __sn_interpolated.push_str(&format!("{}", (point_copy).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (point_copy).y)); __sn_interpolated });
    let mut payload: Payload = Payload { name: "source".to_string(), values: vec![1, 2], point: Point { x: 7, y: 8 } };
    let mut payload_copy: Payload = (payload).clone();
    ((payload_copy).name = "copy".to_string());
    { let __sn_array_index = __sn_index(((payload_copy).values).len(), 0); ((payload_copy).values)[__sn_array_index] = 99; };
    (((payload_copy).point).x = 70);
    ((payload).name = "source-updated".to_string());
    { let __sn_array_index = __sn_index(((payload).values).len(), 1); ((payload).values)[__sn_array_index] = 88; };
    (((payload).point).y = 80);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("payload="); __sn_interpolated.push_str(&format!("{}", (payload).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((payload).values)[__sn_index(((payload).values).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((payload).values)[__sn_index(((payload).values).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((payload).point).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((payload).point).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("payload-copy="); __sn_interpolated.push_str(&format!("{}", (payload_copy).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((payload_copy).values)[__sn_index(((payload_copy).values).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((payload_copy).values)[__sn_index(((payload_copy).values).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((payload_copy).point).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((payload_copy).point).y)); __sn_interpolated });
}
