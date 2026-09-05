#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
    (ops).instanceToggle(&mut *(value))
;
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
    println!("{}", (readBool(&mut (readValue))
 && readValue))
;
    println!("{}", (toggleBool(&mut (freeValue))
 && (!freeValue)))
;
    println!("{}", ((!RefOps::staticRead(&mut (((holder).state).enabled))) && (!((holder).state).enabled)))
;
    println!("{}", ((!RefOps::staticToggle(&mut (((holder).state).enabled))) && ((holder).state).enabled))
;
    println!("{}", ((!(ops).instanceToggle(&mut (instanceValue))
) && instanceValue))
;
    println!("{}", ((!forwardStatic(&mut (forwardedStatic))
) && (!forwardedStatic)))
;
    println!("{}", (forwardInstance(&mut (forwardedInstance))
 && forwardedInstance))
;
}
