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

#[derive(Clone, Debug, PartialEq)]
struct Playlist {
    names: Vec<String>,
    replacement: String,
}

impl Playlist {
    fn replace(&mut self, index: i64, value: String) {
        { let __sn_array_index = __sn_index(((self).names).len(), index); ((self).names)[__sn_array_index] = value.clone(); };
    }
    fn replaceWithStored(&mut self, index: i64) {
        { let __sn_array_index = __sn_index(((self).names).len(), index); ((self).names)[__sn_array_index] = (self).replacement.clone(); };
    }
    fn replaceLast(&mut self, value: String) {
        (self).replace((-1), value.clone());
    }
}

fn main() {
    let mut playlist: Playlist = Playlist { names: vec!["one".to_string(), "two".to_string(), "three".to_string()], replacement: "stored".to_string() };
    let mut first: String = "first".to_string();
    (playlist).replace(0, first.clone());
    (playlist).replaceWithStored(1);
    (playlist).replaceLast("last".to_string());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("names="); __sn_interpolated.push_str(&format!("{:?}", (playlist).names)); __sn_interpolated.push_str("; replacement="); __sn_interpolated.push_str(&format!("{}", (playlist).replacement)); __sn_interpolated.push_str("; source="); __sn_interpolated.push_str(&format!("{}", first)); __sn_interpolated });
}
