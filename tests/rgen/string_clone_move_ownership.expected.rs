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
struct Label {
    text: String,
}

fn copyForCall(value: String) -> String {
    return value;
}

fn main() {
    let mut source: String = "source".to_string();
    let mut assigned: String = source.clone();
    (assigned = "assigned".to_string());
    let mut returned: String = copyForCall(source.clone());
    (returned = "returned".to_string());
    let mut label: Label = Label { text: source.clone() };
    ((label).text = "label".to_string());
    let mut extractedField: String = (label).text.clone();
    (extractedField = "field".to_string());
    let mut values: Vec<String> = vec![];
    (values).push(source.clone());
    { let __sn_array_index = __sn_insert_index((values).len(), 0); (values).insert(__sn_array_index, source.clone()); };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("afterInsert="); __sn_interpolated.push_str(&format!("{}", source)); __sn_interpolated });
    let mut extractedElement: String = (values)[__sn_index((values).len(), 0)].clone();
    (extractedElement = "element".to_string());
    { let __sn_array_index = __sn_index((values).len(), 0); (values)[__sn_array_index] = "array".to_string(); };
    (source = "source-updated".to_string());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("source="); __sn_interpolated.push_str(&format!("{}", source)); __sn_interpolated.push_str("; assigned="); __sn_interpolated.push_str(&format!("{}", assigned)); __sn_interpolated.push_str("; returned="); __sn_interpolated.push_str(&format!("{}", returned)); __sn_interpolated.push_str("; label="); __sn_interpolated.push_str(&format!("{}", (label).text)); __sn_interpolated.push_str("; field="); __sn_interpolated.push_str(&format!("{}", extractedField)); __sn_interpolated.push_str("; array0="); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 0)])); __sn_interpolated.push_str("; array1="); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 1)])); __sn_interpolated.push_str("; element="); __sn_interpolated.push_str(&format!("{}", extractedElement)); __sn_interpolated });
}
