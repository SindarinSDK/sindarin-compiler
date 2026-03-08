struct Counter {
    value: i64,
}

impl Counter {
    fn increment(&mut self) {
        self.value += 1_i64;
    }

    fn getValue(&mut self) -> i64 {
        return self.value;
    }

}


fn main() {
    let mut c: Counter = Counter { value: 0_i64 };
    c.increment();
    return c.getValue();
}
