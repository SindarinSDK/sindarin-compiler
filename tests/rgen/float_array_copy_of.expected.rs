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
    let mut zero: f32 = 0.0;
    let mut negative_zero: f32 = (-0.0);
    let mut nan: f32 = (zero / zero);
    let mut infinity: f32 = (1.0 / zero);
    let mut negative_infinity: f32 = ((-1.0) / zero);
    let mut source: Vec<f32> = vec![1.25, negative_zero, nan, infinity, negative_infinity];
    let mut copied: Vec<f32> = (source).clone();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (source).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied).len() as i64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{:.2}", (copied)[__sn_index((copied).len(), 0)])); __sn_interpolated });
    println!("{}", ((1.0 / (copied)[__sn_index((copied).len(), 1)]) == negative_infinity));
    println!("{}", ((copied)[__sn_index((copied).len(), 2)] != (copied)[__sn_index((copied).len(), 2)]));
    println!("{}", ((copied)[__sn_index((copied).len(), 3)] == infinity));
    println!("{}", ((copied)[__sn_index((copied).len(), 4)] == negative_infinity));
    { let __sn_array_index = __sn_index((source).len(), 0); (source)[__sn_array_index] = 2.5; };
    { let __sn_array_index = __sn_index((copied).len(), 4); (copied)[__sn_array_index] = (-3.75); };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{:.2}", (source)[__sn_index((source).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{:.2}", (source)[__sn_index((source).len(), 4)])); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{:.2}", (copied)[__sn_index((copied).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{:.2}", (copied)[__sn_index((copied).len(), 4)])); __sn_interpolated });
}
