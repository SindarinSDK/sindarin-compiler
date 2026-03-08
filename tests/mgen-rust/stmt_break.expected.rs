
fn main() {
    let mut x: i64 = 0_i64;
    while true {
        { x += 1; x };
        if (x == 5_i64) {
            break;}
    }
    return x;
}
