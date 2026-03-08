
fn main() {
    let mut arr: Vec<i64> = vec![1_i64, 2_i64, 3_i64];
    let mut sum: i64 = 0_i64;
    for item in arr.iter() {
        sum += item;
    }
    return sum;
}
