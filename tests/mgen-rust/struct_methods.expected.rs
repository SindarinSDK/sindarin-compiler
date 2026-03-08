struct Counter {
    value: i64,
}

fn main() {
    let mut c: Counter = Counter { value: 0_i64 };
    c.increment();
    return c.getValue();
}
