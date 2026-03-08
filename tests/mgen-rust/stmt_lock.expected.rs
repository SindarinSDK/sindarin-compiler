
fn main() {
    let mut x: i64 = 0_i64;
    let mut y: i64 = 0_i64;
    {
        let _lock_x = x.lock().unwrap();
        x = (x + 1_i64);
        y = x;
    }
    return y;
}
