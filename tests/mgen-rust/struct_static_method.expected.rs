struct Counter {
    value: i64,
}

impl Counter {
    fn zero() -> i64 {
        return 0_i64;
    }

}


fn main() {
    let mut z: i64 = Counter::zero();
    return z;
}
