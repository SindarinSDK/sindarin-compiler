#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

extern "C" {
    #[link_name = "fwrite"]
    fn __sn_stdio_fwrite(_: *const std::ffi::c_void, _: usize, _: usize, _: *mut std::ffi::c_void) -> usize;
    #[link_name = "fflush"]
    fn __sn_stdio_fflush(_: *mut std::ffi::c_void) -> std::ffi::c_int;
    #[link_name = "exit"]
    fn __sn_stdio_c_exit(_: std::ffi::c_int) -> !;
    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_stdio_iob(_: u32) -> *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stdoutp"]
    static mut __sn_stdio_stdout: *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stderrp"]
    static mut __sn_stdio_stderr: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stdout"]
    static mut __sn_stdio_stdout: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stderr"]
    static mut __sn_stdio_stderr: *mut std::ffi::c_void;
}

fn __sn_stdio_write(bytes: &[u8], stderr: bool) {
    unsafe {
        #[cfg(windows)]
        let stream = __sn_stdio_iob(if stderr { 2 } else { 1 });
        #[cfg(not(windows))]
        let stream = if stderr { __sn_stdio_stderr } else { __sn_stdio_stdout };
        __sn_stdio_fwrite(bytes.as_ptr().cast(), 1, bytes.len(), stream);
    }
}

fn __sn_stdio_exit(status: i32) -> ! {
    unsafe { __sn_stdio_c_exit(status) }
}

struct __SnStdioGuard;
impl Drop for __SnStdioGuard {
    fn drop(&mut self) {
        unsafe { __sn_stdio_fflush(std::ptr::null_mut()); }
    }
}


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

fn __sn_write_stdout_bytes(bytes: &[u8]) {
    __sn_stdio_write(bytes, false);
}

fn __sn_write_stderr_bytes(bytes: &[u8]) {
    __sn_stdio_write(bytes, true);
}

fn __sn_print_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    __sn_write_stdout_bytes(rendered.as_bytes());
}

fn __sn_println_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    rendered.push('\n');
    __sn_write_stdout_bytes(rendered.as_bytes());
}

macro_rules! print {
    ($($arg:tt)*) => { crate::__sn_print_format(format_args!($($arg)*)) };
}

macro_rules! println {
    () => { crate::__sn_println_format(format_args!("")) };
    ($($arg:tt)*) => { crate::__sn_println_format(format_args!($($arg)*)) };
}


fn aliasInt(left: std::rc::Rc<std::cell::Cell<i64>>, right: std::rc::Rc<std::cell::Cell<i64>>) -> bool {
    { let (__sn_value, __sn_cell) = (3, &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = (right.get() == 3);
    { let (__sn_value, __sn_cell) = (4, &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && (left.get() == 4));
}

fn aliasLong(left: std::rc::Rc<std::cell::Cell<i64>>, right: std::rc::Rc<std::cell::Cell<i64>>) -> bool {
    { let (__sn_value, __sn_cell) = (3, &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = (right.get() == 3);
    { let (__sn_value, __sn_cell) = (4, &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && (left.get() == 4));
}

fn aliasInt32(left: std::rc::Rc<std::cell::Cell<i32>>, right: std::rc::Rc<std::cell::Cell<i32>>) -> bool {
    { let (__sn_value, __sn_cell) = (((((3i128) as i64)) as i32), &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = { let (__sn_left, __sn_right): (i64, i64) = ((right.get()) as i64, (3) as i64); __sn_left == __sn_right }
;
    { let (__sn_value, __sn_cell) = (((((4i128) as i64)) as i32), &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && { let (__sn_left, __sn_right): (i64, i64) = ((left.get()) as i64, (4) as i64); __sn_left == __sn_right }
 );
}

fn aliasUint(left: std::rc::Rc<std::cell::Cell<u64>>, right: std::rc::Rc<std::cell::Cell<u64>>) -> bool {
    { let (__sn_value, __sn_cell) = (((((3i128) as i64)) as u64), &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = { let (__sn_left, __sn_right): (u64, u64) = ((right.get()) as u64, (3) as u64); __sn_left == __sn_right }
;
    { let (__sn_value, __sn_cell) = (((((4i128) as i64)) as u64), &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && { let (__sn_left, __sn_right): (u64, u64) = ((left.get()) as u64, (4) as u64); __sn_left == __sn_right }
 );
}

fn aliasUint32(left: std::rc::Rc<std::cell::Cell<u32>>, right: std::rc::Rc<std::cell::Cell<u32>>) -> bool {
    { let (__sn_value, __sn_cell) = (((((3i128) as i64)) as u32), &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = { let (__sn_left, __sn_right): (i64, i64) = ((right.get()) as i64, (3) as i64); __sn_left == __sn_right }
;
    { let (__sn_value, __sn_cell) = (((((4i128) as i64)) as u32), &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && { let (__sn_left, __sn_right): (i64, i64) = ((left.get()) as i64, (4) as i64); __sn_left == __sn_right }
 );
}

fn aliasByte(left: std::rc::Rc<std::cell::Cell<u8>>, right: std::rc::Rc<std::cell::Cell<u8>>) -> bool {
    { let (__sn_value, __sn_cell) = (((((3i128) as i64)) as u8), &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = { let (__sn_left, __sn_right): (i64, i64) = ((right.get()) as i64, (3) as i64); __sn_left == __sn_right }
;
    { let (__sn_value, __sn_cell) = (((((4i128) as i64)) as u8), &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && { let (__sn_left, __sn_right): (i64, i64) = ((left.get()) as i64, (4) as i64); __sn_left == __sn_right }
 );
}

fn aliasFloat(left: std::rc::Rc<std::cell::Cell<f32>>, right: std::rc::Rc<std::cell::Cell<f32>>) -> bool {
    { let (__sn_value, __sn_cell) = (3.0, &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = (right.get() == 3.0);
    { let (__sn_value, __sn_cell) = (4.0, &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && (left.get() == 4.0));
}

fn aliasDouble(left: std::rc::Rc<std::cell::Cell<f64>>, right: std::rc::Rc<std::cell::Cell<f64>>) -> bool {
    { let (__sn_value, __sn_cell) = (3.0, &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = (right.get() == 3.0);
    { let (__sn_value, __sn_cell) = (4.0, &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && (left.get() == 4.0));
}

fn aliasBool(left: std::rc::Rc<std::cell::Cell<bool>>, right: std::rc::Rc<std::cell::Cell<bool>>) -> bool {
    { let (__sn_value, __sn_cell) = (true, &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = (right.get() == true);
    { let (__sn_value, __sn_cell) = (false, &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && (left.get() == false));
}

fn aliasChar(left: std::rc::Rc<std::cell::Cell<char>>, right: std::rc::Rc<std::cell::Cell<char>>) -> bool {
    { let (__sn_value, __sn_cell) = ('\u{58}', &left); __sn_cell.set(__sn_value); __sn_value };
    let mut saw: bool = (right.get() == '\u{58}');
    { let (__sn_value, __sn_cell) = ('\u{59}', &right); __sn_cell.set(__sn_value); __sn_value };
    return (saw && (left.get() == '\u{59}'));
}

fn main() {
    let sameInt: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let leftInt: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let rightInt: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(2));
    println!("{}", aliasInt(sameInt.clone(), sameInt.clone()));
    println!("{}", aliasInt(leftInt.clone(), rightInt.clone()));
    let sameLong: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let leftLong: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(1));
    let rightLong: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(2));
    println!("{}", aliasLong(sameLong.clone(), sameLong.clone()));
    println!("{}", aliasLong(leftLong.clone(), rightLong.clone()));
    let sameInt32: std::rc::Rc<std::cell::Cell<i32>> = std::rc::Rc::new(std::cell::Cell::new(((1) as i32)));
    let leftInt32: std::rc::Rc<std::cell::Cell<i32>> = std::rc::Rc::new(std::cell::Cell::new(((1) as i32)));
    let rightInt32: std::rc::Rc<std::cell::Cell<i32>> = std::rc::Rc::new(std::cell::Cell::new(((2) as i32)));
    println!("{}", aliasInt32(sameInt32.clone(), sameInt32.clone()));
    println!("{}", aliasInt32(leftInt32.clone(), rightInt32.clone()));
    let sameUint: std::rc::Rc<std::cell::Cell<u64>> = std::rc::Rc::new(std::cell::Cell::new(((1) as u64)));
    let leftUint: std::rc::Rc<std::cell::Cell<u64>> = std::rc::Rc::new(std::cell::Cell::new(((1) as u64)));
    let rightUint: std::rc::Rc<std::cell::Cell<u64>> = std::rc::Rc::new(std::cell::Cell::new(((2) as u64)));
    println!("{}", aliasUint(sameUint.clone(), sameUint.clone()));
    println!("{}", aliasUint(leftUint.clone(), rightUint.clone()));
    let sameUint32: std::rc::Rc<std::cell::Cell<u32>> = std::rc::Rc::new(std::cell::Cell::new(((1) as u32)));
    let leftUint32: std::rc::Rc<std::cell::Cell<u32>> = std::rc::Rc::new(std::cell::Cell::new(((1) as u32)));
    let rightUint32: std::rc::Rc<std::cell::Cell<u32>> = std::rc::Rc::new(std::cell::Cell::new(((2) as u32)));
    println!("{}", aliasUint32(sameUint32.clone(), sameUint32.clone()));
    println!("{}", aliasUint32(leftUint32.clone(), rightUint32.clone()));
    let sameByte: std::rc::Rc<std::cell::Cell<u8>> = std::rc::Rc::new(std::cell::Cell::new(((1) as u8)));
    let leftByte: std::rc::Rc<std::cell::Cell<u8>> = std::rc::Rc::new(std::cell::Cell::new(((1) as u8)));
    let rightByte: std::rc::Rc<std::cell::Cell<u8>> = std::rc::Rc::new(std::cell::Cell::new(((2) as u8)));
    println!("{}", aliasByte(sameByte.clone(), sameByte.clone()));
    println!("{}", aliasByte(leftByte.clone(), rightByte.clone()));
    let sameFloat: std::rc::Rc<std::cell::Cell<f32>> = std::rc::Rc::new(std::cell::Cell::new(1.0));
    let leftFloat: std::rc::Rc<std::cell::Cell<f32>> = std::rc::Rc::new(std::cell::Cell::new(1.0));
    let rightFloat: std::rc::Rc<std::cell::Cell<f32>> = std::rc::Rc::new(std::cell::Cell::new(2.0));
    println!("{}", aliasFloat(sameFloat.clone(), sameFloat.clone()));
    println!("{}", aliasFloat(leftFloat.clone(), rightFloat.clone()));
    let sameDouble: std::rc::Rc<std::cell::Cell<f64>> = std::rc::Rc::new(std::cell::Cell::new(1.0));
    let leftDouble: std::rc::Rc<std::cell::Cell<f64>> = std::rc::Rc::new(std::cell::Cell::new(1.0));
    let rightDouble: std::rc::Rc<std::cell::Cell<f64>> = std::rc::Rc::new(std::cell::Cell::new(2.0));
    println!("{}", aliasDouble(sameDouble.clone(), sameDouble.clone()));
    println!("{}", aliasDouble(leftDouble.clone(), rightDouble.clone()));
    let sameBool: std::rc::Rc<std::cell::Cell<bool>> = std::rc::Rc::new(std::cell::Cell::new(false));
    let leftBool: std::rc::Rc<std::cell::Cell<bool>> = std::rc::Rc::new(std::cell::Cell::new(false));
    let rightBool: std::rc::Rc<std::cell::Cell<bool>> = std::rc::Rc::new(std::cell::Cell::new(false));
    println!("{}", aliasBool(sameBool.clone(), sameBool.clone()));
    println!("{}", aliasBool(leftBool.clone(), rightBool.clone()));
    let sameChar: std::rc::Rc<std::cell::Cell<char>> = std::rc::Rc::new(std::cell::Cell::new('\u{41}'));
    let leftChar: std::rc::Rc<std::cell::Cell<char>> = std::rc::Rc::new(std::cell::Cell::new('\u{41}'));
    let rightChar: std::rc::Rc<std::cell::Cell<char>> = std::rc::Rc::new(std::cell::Cell::new('\u{42}'));
    println!("{}", aliasChar(sameChar.clone(), sameChar.clone()));
    println!("{}", aliasChar(leftChar.clone(), rightChar.clone()));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
