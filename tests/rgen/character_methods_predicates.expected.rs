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


mod __sn_ctype {
    use std::ffi::c_int;
    unsafe extern "C" {
        #[link_name = "toupper"] fn crt_upper(value: c_int) -> c_int;
        #[link_name = "tolower"] fn crt_lower(value: c_int) -> c_int;
        #[link_name = "isdigit"] fn crt_digit(value: c_int) -> c_int;
        #[link_name = "isalpha"] fn crt_alpha(value: c_int) -> c_int;
        #[link_name = "isspace"] fn crt_space(value: c_int) -> c_int;
        #[link_name = "isalnum"] fn crt_alnum(value: c_int) -> c_int;
    }
    fn byte(value: char) -> u8 { value as u32 as u8 }
    fn upper_byte(value: u8) -> u8 { unsafe { crt_upper(value as c_int) as u8 } }
    fn lower_byte(value: u8) -> u8 { unsafe { crt_lower(value as c_int) as u8 } }
    fn space_byte(value: u8) -> bool { unsafe { crt_space(value as c_int) != 0 } }
    pub(super) fn upper(value: char) -> char { upper_byte(byte(value)) as char }
    pub(super) fn lower(value: char) -> char { lower_byte(byte(value)) as char }
    pub(super) fn integer(value: char) -> i64 { byte(value) as std::ffi::c_char as i64 }
    pub(super) fn digit(value: char) -> bool { unsafe { crt_digit(byte(value) as c_int) != 0 } }
    pub(super) fn alpha(value: char) -> bool { unsafe { crt_alpha(byte(value) as c_int) != 0 } }
    pub(super) fn space(value: char) -> bool { space_byte(byte(value)) }
    pub(super) fn alnum(value: char) -> bool { unsafe { crt_alnum(byte(value) as c_int) != 0 } }
}


fn main() {
    __sn_stdio_exit((|| -> i64 {
        if ((!__sn_ctype::digit('\u{39}')) || __sn_ctype::digit('\u{61}')) {
        return 1;
    }
        if ((!__sn_ctype::alpha('\u{61}')) || __sn_ctype::alpha('\u{39}')) {
        return 2;
    }
        if ((!__sn_ctype::space('\u{20}')) || __sn_ctype::space('\u{61}')) {
        return 3;
    }
        if (((!__sn_ctype::alnum('\u{61}')) || (!__sn_ctype::alnum('\u{39}'))) || __sn_ctype::alnum('\u{40}')) {
        return 4;
    }
        if ((__sn_ctype::upper('\u{61}') != '\u{41}') || (__sn_ctype::lower('\u{5a}') != '\u{7a}')) {
        return 5;
    }
        if (__sn_ctype::integer('\u{41}') != 65) {
        return 6;
    }
        return 0;
        return 0;
    })() as i32);
}
