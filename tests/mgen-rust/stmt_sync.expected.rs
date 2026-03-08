
fn main() {
    let mut counter: i64 = 0_i64;
    {
        let _lock_counter = counter.lock().unwrap();
        counter = (counter + 1_i64);
    }
    return counter;
}
