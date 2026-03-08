
fn main() {
    let mut sum: i64 = 0_i64;
    {
        let mut i: i64 = 0_i64;
        while (i < 10_i64) {
            if (i == 5_i64) {
                continue;}
            sum += i;
            { i += 1; i };
        }
    }
    return sum;
}
