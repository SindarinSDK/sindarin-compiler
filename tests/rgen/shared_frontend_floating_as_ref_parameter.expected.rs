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


#[derive(Clone, Copy, Debug, PartialEq)]
struct Values {
    single: f32,
    precise: f64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticWriteFloat(value: &mut f32) -> f32 {
        let mut before: f32 = *(value);
        (*(value) = 6.5);
        return before;
    }
    fn staticWriteDouble(value: &mut f64) -> f64 {
        let mut before: f64 = *(value);
        (*(value) = 8.5);
        return before;
    }
    fn instanceWriteFloat(&self, value: &mut f32) -> f32 {
        let mut before: f32 = *(value);
        (*(value) = 10.5);
        return before;
    }
    fn instanceWriteDouble(&self, value: &mut f64) -> f64 {
        let mut before: f64 = *(value);
        (*(value) = 12.5);
        return before;
    }
}

fn writeFloat(value: &mut f32) -> f32 {
    let mut before: f32 = *(value);
    (*(value) = 2.5);
    return before;
}

fn writeDouble(value: &mut f64) -> f64 {
    let mut before: f64 = *(value);
    (*(value) = 4.5);
    return before;
}

fn forwardFloat(value: &mut f32) -> f32 {
    RefOps::staticWriteFloat(&mut *(value));
    return *(value);
}

fn forwardDouble(value: &mut f64) -> f64 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceWriteDouble(&mut *(value));
    return *(value);
}

fn main() {
    let mut freeSingle: f32 = 1.5;
    let mut freeValues: Values = Values { single: 0.0, precise: 2.25 };
    let mut staticValues: Values = Values { single: 4.0, precise: 0.0 };
    let mut staticDouble: f64 = 5.25;
    let mut instanceSingle: f32 = 9.0;
    let mut instanceValues: Values = Values { single: 0.0, precise: 11.25 };
    let mut forwardedSingle: f32 = 16.0;
    let mut forwardedDouble: f64 = 32.0;
    let mut ops: RefOps = RefOps {  };
    println!("{}", ((writeFloat(&mut (freeSingle)) == 1.5) && (freeSingle == 2.5)));
    println!("{}", ((writeDouble(&mut ((freeValues).precise)) == 2.25) && ((freeValues).precise == 4.5)));
    println!("{}", ((RefOps::staticWriteFloat(&mut ((staticValues).single)) == 4.0) && ((staticValues).single == 6.5)));
    println!("{}", ((RefOps::staticWriteDouble(&mut (staticDouble)) == 5.25) && (staticDouble == 8.5)));
    println!("{}", (((ops).instanceWriteFloat(&mut (instanceSingle)) == 9.0) && (instanceSingle == 10.5)));
    println!("{}", (((ops).instanceWriteDouble(&mut ((instanceValues).precise)) == 11.25) && ((instanceValues).precise == 12.5)));
    println!("{}", ((forwardFloat(&mut (forwardedSingle)) == 6.5) && (forwardedSingle == 6.5)));
    println!("{}", ((forwardDouble(&mut (forwardedDouble)) == 12.5) && (forwardedDouble == 12.5)));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
