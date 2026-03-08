
fn increment(x: i64) {
    x = (x + 1_i64);
}


fn main() {
    let mut a: i64 = 10_i64;
    increment(a);
    return a;
}
