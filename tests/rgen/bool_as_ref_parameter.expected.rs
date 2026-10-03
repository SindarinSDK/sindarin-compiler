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
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
