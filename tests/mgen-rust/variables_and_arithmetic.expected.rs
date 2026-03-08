
fn add(a: i64, b: i64) -> i64 {
    return (a + b);
}


fn main() {
    let mut x: i64 = 42_i64;
    let mut y: i64 = add(x, 10_i64);
    return y;
}
