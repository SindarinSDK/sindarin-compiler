struct Config {
    width: i64,
    height: i64,
    scale: f64,
}


fn main() {
    let mut c: Config = Config { width: 800_i64, height: 600_i64, scale: 1.0_f64 };
    return c.width;
}
