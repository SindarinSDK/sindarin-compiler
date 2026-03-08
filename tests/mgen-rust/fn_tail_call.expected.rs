
fn factorial(n: i64, acc: i64) -> i64 {
    if (n <= 1_i64) {
        return acc;
    }
    return factorial((n - 1_i64), (acc * n));
}


fn main() {
    return factorial(5_i64, 1_i64);
}
