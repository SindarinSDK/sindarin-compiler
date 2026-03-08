
fn main() {
    let mut x: i64 = 99_i64;
    let mut result: i64 = match x {
        1_i64 => { 10_i64 },
        _ => { 0_i64 },
    };
    return result;
}
