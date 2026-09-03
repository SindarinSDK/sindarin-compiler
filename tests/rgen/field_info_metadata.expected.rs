#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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

fn main() {
    let mut field: FieldInfo = FieldInfo { name: "count".to_string(), typeName: "int".to_string(), typeId: 367623774 };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (field).name)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (field).typeName)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (field).typeId)); __sn_interpolated });
}
