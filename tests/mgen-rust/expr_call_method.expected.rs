struct Counter {
    value: i64,
}

impl Counter {
    fn getValue(&mut self) -> i64 {
        return self.value;
    }

    fn increment(&mut self) {
        self.value += 1_i64;
    }

}


fn main() {
    let mut c: Counter = Counter { value: 10_i64 };
    c.increment();
    return c.getValue();
}
