#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

unsafe extern "C" {
    #[cfg(not(windows))]
    #[cfg_attr(target_vendor = "apple", link_name = "__stdoutp")]
    #[cfg_attr(not(target_vendor = "apple"), link_name = "stdout")]
    static mut __sn_c_stdout: *mut std::ffi::c_void;

    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_c_stdout(index: u32) -> *mut std::ffi::c_void;

    #[link_name = "fwrite"]
    fn __sn_c_fwrite(
        data: *const std::ffi::c_void,
        size: usize,
        count: usize,
        stream: *mut std::ffi::c_void,
    ) -> usize;
}

fn __sn_stdout_stream() -> *mut std::ffi::c_void {
    unsafe {
        #[cfg(windows)]
        { __sn_c_stdout(1) }
        #[cfg(not(windows))]
        { __sn_c_stdout }
    }
}

fn __sn_stdout_write(bytes: &[u8]) {
    if bytes.is_empty() { return; }
    let written = unsafe {
        __sn_c_fwrite(
            bytes.as_ptr().cast(), 1, bytes.len(),
            __sn_stdout_stream())
    };
    if written != bytes.len() { panic!("failed to write stdout"); }
}

macro_rules! print {
    ($($arg:tt)*) => {
        let rendered = format!($($arg)*);
        __sn_stdout_write(rendered.as_bytes());
    };
}

macro_rules! println {
    () => { __sn_stdout_write(b"\n") };
    ($($arg:tt)*) => {
        let mut rendered = format!($($arg)*);
        rendered.push('\n');
        __sn_stdout_write(rendered.as_bytes());
    };
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
