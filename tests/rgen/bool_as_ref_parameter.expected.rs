#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[cfg(windows)]
fn __sn_write_windows_text<W: std::io::Write>(writer: &mut W, bytes: &[u8]) {
    let mut start = 0usize;
    for (index, byte) in bytes.iter().enumerate() {
        if *byte == b'\n' {
            writer.write_all(&bytes[start..index]).expect("failed to write output");
            writer.write_all(b"\r\n").expect("failed to write output newline");
            start = index + 1;
        }
    }
    writer.write_all(&bytes[start..]).expect("failed to write output");
    writer.flush().expect("failed to flush output");
}

#[cfg(windows)]
fn __sn_write_stdout_bytes(bytes: &[u8]) {
    __sn_write_windows_text(&mut std::io::stdout().lock(), bytes);
}

#[cfg(windows)]
fn __sn_write_stderr_bytes(bytes: &[u8]) {
    __sn_write_windows_text(&mut std::io::stderr().lock(), bytes);
}

#[cfg(windows)]
fn __sn_print_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    __sn_write_stdout_bytes(rendered.as_bytes());
}

#[cfg(windows)]
fn __sn_println_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    rendered.push('\n');
    __sn_write_stdout_bytes(rendered.as_bytes());
}

#[cfg(windows)]
macro_rules! print {
    ($($arg:tt)*) => { crate::__sn_print_format(format_args!($($arg)*)) };
}

#[cfg(windows)]
macro_rules! println {
    () => { crate::__sn_println_format(format_args!("")) };
    ($($arg:tt)*) => { crate::__sn_println_format(format_args!($($arg)*)) };
}


#[derive(Clone, Copy, Debug, PartialEq)]
struct State {
    enabled: bool,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Holder {
    state: State,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticRead(value: &mut bool) -> bool {
        return *(value);
    }
    fn staticToggle(value: &mut bool) -> bool {
        let mut before: bool = *(value);
        (*(value) = (!*(value)));
        return before;
    }
    fn instanceToggle(&self, value: &mut bool) -> bool {
        let mut before: bool = *(value);
        (*(value) = (!*(value)));
        return before;
    }
}

fn readBool(value: &mut bool) -> bool {
    return *(value);
}

fn toggleBool(value: &mut bool) -> bool {
    let mut before: bool = *(value);
    (*(value) = (!*(value)));
    return before;
}

fn forwardStatic(value: &mut bool) -> bool {
    RefOps::staticToggle(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut bool) -> bool {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceToggle(&mut *(value));
    return *(value);
}

fn main() {
    let mut readValue: bool = true;
    let mut freeValue: bool = true;
    let mut holder: Holder = Holder { state: State { enabled: false } };
    let mut instanceValue: bool = false;
    let mut forwardedStatic: bool = true;
    let mut forwardedInstance: bool = false;
    let mut ops: RefOps = RefOps {  };
    println!("{}", (readBool(&mut (readValue)) && readValue));
    println!("{}", (toggleBool(&mut (freeValue)) && (!freeValue)));
    println!("{}", ((!RefOps::staticRead(&mut (((holder).state).enabled))) && (!((holder).state).enabled)));
    println!("{}", ((!RefOps::staticToggle(&mut (((holder).state).enabled))) && ((holder).state).enabled));
    println!("{}", ((!(ops).instanceToggle(&mut (instanceValue))) && instanceValue));
    println!("{}", ((!forwardStatic(&mut (forwardedStatic))) && (!forwardedStatic)));
    println!("{}", (forwardInstance(&mut (forwardedInstance)) && forwardedInstance));
}
