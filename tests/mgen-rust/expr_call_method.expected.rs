struct Counter {
    value: i64,
}

fn main() {
    let mut c: Counter = Counter { value: 10_i64 };
    c.increment();
    return c.getValue();
}
