
fn main() {
    let mut sum: i64 = 0_i64;
    {
        let mut i: i64 = 0_i64;
        while (i < 10_i64) {
            sum += i;
            { i += 1; i };
        }
    }
    return sum;
}
