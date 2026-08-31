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
    let mut ints: Vec<i64> = vec![1, 2, 3];
    let mut ints_copy: Vec<i64> = (ints).clone();
    { let __sn_array_index = __sn_index((ints).len(), 0); (ints)[__sn_array_index] = 10; };
    { let __sn_array_index = __sn_index((ints_copy).len(), 2); (ints_copy)[__sn_array_index] = 30; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (ints).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (ints)[__sn_index((ints).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (ints)[__sn_index((ints).len(), 2)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (ints_copy)[__sn_index((ints_copy).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (ints_copy)[__sn_index((ints_copy).len(), 2)])); __sn_interpolated });
    let mut longs: Vec<i64> = vec![4, 5];
    let mut longs_copy: Vec<i64> = (longs).clone();
    { let __sn_array_index = __sn_index((longs).len(), 0); (longs)[__sn_array_index] = 40; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (longs)[__sn_index((longs).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (longs_copy)[__sn_index((longs_copy).len(), 0)])); __sn_interpolated });
    let mut int32s: Vec<i32> = vec![6, 7];
    let mut int32s_copy: Vec<i32> = (int32s).clone();
    { let __sn_array_index = __sn_index((int32s).len(), 0); (int32s)[__sn_array_index] = 60; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (int32s)[__sn_index((int32s).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (int32s_copy)[__sn_index((int32s_copy).len(), 0)])); __sn_interpolated });
    let mut uints: Vec<u64> = vec![8, 9];
    let mut uints_copy: Vec<u64> = (uints).clone();
    { let __sn_array_index = __sn_index((uints).len(), 0); (uints)[__sn_array_index] = 80; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (uints)[__sn_index((uints).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (uints_copy)[__sn_index((uints_copy).len(), 0)])); __sn_interpolated });
    let mut uint32s: Vec<u32> = vec![10, 11];
    let mut uint32s_copy: Vec<u32> = (uint32s).clone();
    { let __sn_array_index = __sn_index((uint32s).len(), 0); (uint32s)[__sn_array_index] = 100; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (uint32s)[__sn_index((uint32s).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (uint32s_copy)[__sn_index((uint32s_copy).len(), 0)])); __sn_interpolated });
    let mut bytes: Vec<u8> = vec![12, 13];
    let mut bytes_copy: Vec<u8> = (bytes).clone();
    { let __sn_array_index = __sn_index((bytes).len(), 0); (bytes)[__sn_array_index] = 120; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (bytes)[__sn_index((bytes).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (bytes_copy)[__sn_index((bytes_copy).len(), 0)])); __sn_interpolated });
}
