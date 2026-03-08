
fn main() {
    let mut x: i64 = 0_i64;
    while (x < 10_i64) {
        { x += 1; x };
    }
    return x;
}
