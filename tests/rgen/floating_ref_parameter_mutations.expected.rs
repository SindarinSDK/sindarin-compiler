#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct FloatBox {
    value: f32,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct DoubleBox {
    value: f64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Values {
    single: f32,
    precise: f64,
    singleBox: FloatBox,
    preciseBox: DoubleBox,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct SideEffects {
}

impl SideEffects {
    fn floatRhs(calls: &mut i64, result: f32) -> f32 {
        { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        return result;
    }
    fn doubleRhs(calls: &mut i64, result: f64) -> f64 {
        { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        return result;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct MutationOps {
}

impl MutationOps {
    fn staticFloat(__sn_next: &mut f32, calls: &mut i64) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut ok: bool = true;
        let mut afterAdd: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 4.0), &mut (*(__sn_next))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterAdd == 12.0) && (*(__sn_next) == 12.0)) && (*(calls) == (beforeCalls).checked_add(1).expect("checked arithmetic failed"))) && ok));
        let mut afterSubtract: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 2.0), &mut (*(__sn_next))); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterSubtract == 10.0) && (*(__sn_next) == 10.0)) && (*(calls) == (beforeCalls).checked_add(2).expect("checked arithmetic failed"))) && ok));
        let mut afterMultiply: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 3.0), &mut (*(__sn_next))); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterMultiply == 30.0) && (*(__sn_next) == 30.0)) && (*(calls) == (beforeCalls).checked_add(3).expect("checked arithmetic failed"))) && ok));
        let mut afterDivide: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 5.0), &mut (*(__sn_next))); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterDivide == 6.0) && (*(__sn_next) == 6.0)) && (*(calls) == (beforeCalls).checked_add(4).expect("checked arithmetic failed"))) && ok));
        let mut beforeIncrement: f32 = { let __sn_place = &mut (*(__sn_next)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeIncrement == 6.0) && (*(__sn_next) == 7.0)) && ok));
        let mut beforeDecrement: f32 = { let __sn_place = &mut (*(__sn_next)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeDecrement == 7.0) && (*(__sn_next) == 6.0)) && ok));
        return ok;
    }
    fn staticDouble(__sn_previous: &mut f64, calls: &mut i64) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut ok: bool = true;
        let mut afterAdd: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 8.0), &mut (*(__sn_previous))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterAdd == 24.0) && (*(__sn_previous) == 24.0)) && (*(calls) == (beforeCalls).checked_add(1).expect("checked arithmetic failed"))) && ok));
        let mut afterSubtract: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 4.0), &mut (*(__sn_previous))); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterSubtract == 20.0) && (*(__sn_previous) == 20.0)) && (*(calls) == (beforeCalls).checked_add(2).expect("checked arithmetic failed"))) && ok));
        let mut afterMultiply: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 2.0), &mut (*(__sn_previous))); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterMultiply == 40.0) && (*(__sn_previous) == 40.0)) && (*(calls) == (beforeCalls).checked_add(3).expect("checked arithmetic failed"))) && ok));
        let mut afterDivide: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 5.0), &mut (*(__sn_previous))); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterDivide == 8.0) && (*(__sn_previous) == 8.0)) && (*(calls) == (beforeCalls).checked_add(4).expect("checked arithmetic failed"))) && ok));
        let mut beforeIncrement: f64 = { let __sn_place = &mut (*(__sn_previous)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeIncrement == 8.0) && (*(__sn_previous) == 9.0)) && ok));
        let mut beforeDecrement: f64 = { let __sn_place = &mut (*(__sn_previous)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeDecrement == 9.0) && (*(__sn_previous) == 8.0)) && ok));
        return ok;
    }
    fn instanceFloat(&self, value: &mut f32, calls: &mut i64) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut ok: bool = true;
        let mut afterAdd: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 4.0), &mut (*(value))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterAdd == 12.0) && (*(value) == 12.0)) && (*(calls) == (beforeCalls).checked_add(1).expect("checked arithmetic failed"))) && ok));
        let mut afterSubtract: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 2.0), &mut (*(value))); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterSubtract == 10.0) && (*(value) == 10.0)) && (*(calls) == (beforeCalls).checked_add(2).expect("checked arithmetic failed"))) && ok));
        let mut afterMultiply: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 3.0), &mut (*(value))); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterMultiply == 30.0) && (*(value) == 30.0)) && (*(calls) == (beforeCalls).checked_add(3).expect("checked arithmetic failed"))) && ok));
        let mut afterDivide: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 5.0), &mut (*(value))); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterDivide == 6.0) && (*(value) == 6.0)) && (*(calls) == (beforeCalls).checked_add(4).expect("checked arithmetic failed"))) && ok));
        let mut beforeIncrement: f32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeIncrement == 6.0) && (*(value) == 7.0)) && ok));
        let mut beforeDecrement: f32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeDecrement == 7.0) && (*(value) == 6.0)) && ok));
        return ok;
    }
    fn instanceDouble(&self, value: &mut f64, calls: &mut i64) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut ok: bool = true;
        let mut afterAdd: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 8.0), &mut (*(value))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterAdd == 24.0) && (*(value) == 24.0)) && (*(calls) == (beforeCalls).checked_add(1).expect("checked arithmetic failed"))) && ok));
        let mut afterSubtract: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 4.0), &mut (*(value))); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterSubtract == 20.0) && (*(value) == 20.0)) && (*(calls) == (beforeCalls).checked_add(2).expect("checked arithmetic failed"))) && ok));
        let mut afterMultiply: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 2.0), &mut (*(value))); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterMultiply == 40.0) && (*(value) == 40.0)) && (*(calls) == (beforeCalls).checked_add(3).expect("checked arithmetic failed"))) && ok));
        let mut afterDivide: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 5.0), &mut (*(value))); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        (ok = ((((afterDivide == 8.0) && (*(value) == 8.0)) && (*(calls) == (beforeCalls).checked_add(4).expect("checked arithmetic failed"))) && ok));
        let mut beforeIncrement: f64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeIncrement == 8.0) && (*(value) == 9.0)) && ok));
        let mut beforeDecrement: f64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        (ok = (((beforeDecrement == 9.0) && (*(value) == 8.0)) && ok));
        return ok;
    }
    fn forwardStaticFloat(value: &mut f32, calls: &mut i64) -> bool {
        return MutationOps::staticFloat(&mut *(value), &mut *(calls));
    }
    fn forwardStaticDouble(value: &mut f64, calls: &mut i64) -> bool {
        return MutationOps::staticDouble(&mut *(value), &mut *(calls));
    }
    fn forwardInstanceFloat(&self, value: &mut f32, calls: &mut i64) -> bool {
        return (self).instanceFloat(&mut *(value), &mut *(calls));
    }
    fn forwardInstanceDouble(&self, value: &mut f64, calls: &mut i64) -> bool {
        return (self).instanceDouble(&mut *(value), &mut *(calls));
    }
}

fn freeFloat(__sn_place: &mut f32, calls: &mut i64) -> bool {
    let mut beforeCalls: i64 = *(calls);
    let mut ok: bool = true;
    let mut afterAdd: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 4.0), &mut (*(__sn_place))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterAdd == 12.0) && (*(__sn_place) == 12.0)) && (*(calls) == (beforeCalls).checked_add(1).expect("checked arithmetic failed"))) && ok));
    let mut afterSubtract: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 2.0), &mut (*(__sn_place))); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterSubtract == 10.0) && (*(__sn_place) == 10.0)) && (*(calls) == (beforeCalls).checked_add(2).expect("checked arithmetic failed"))) && ok));
    let mut afterMultiply: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 3.0), &mut (*(__sn_place))); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterMultiply == 30.0) && (*(__sn_place) == 30.0)) && (*(calls) == (beforeCalls).checked_add(3).expect("checked arithmetic failed"))) && ok));
    let mut afterDivide: f32 = { let (__sn_rhs, __sn_place) = (SideEffects::floatRhs(&mut *(calls), 5.0), &mut (*(__sn_place))); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterDivide == 6.0) && (*(__sn_place) == 6.0)) && (*(calls) == (beforeCalls).checked_add(4).expect("checked arithmetic failed"))) && ok));
    let mut beforeIncrement: f32 = { let __sn_place = &mut (*(__sn_place)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    (ok = (((beforeIncrement == 6.0) && (*(__sn_place) == 7.0)) && ok));
    let mut beforeDecrement: f32 = { let __sn_place = &mut (*(__sn_place)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    (ok = (((beforeDecrement == 7.0) && (*(__sn_place) == 6.0)) && ok));
    return ok;
}

fn freeDouble(__sn_rhs: &mut f64, calls: &mut i64) -> bool {
    let mut beforeCalls: i64 = *(calls);
    let mut ok: bool = true;
    let mut afterAdd: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 8.0), &mut (*(__sn_rhs))); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterAdd == 24.0) && (*(__sn_rhs) == 24.0)) && (*(calls) == (beforeCalls).checked_add(1).expect("checked arithmetic failed"))) && ok));
    let mut afterSubtract: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 4.0), &mut (*(__sn_rhs))); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterSubtract == 20.0) && (*(__sn_rhs) == 20.0)) && (*(calls) == (beforeCalls).checked_add(2).expect("checked arithmetic failed"))) && ok));
    let mut afterMultiply: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 2.0), &mut (*(__sn_rhs))); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterMultiply == 40.0) && (*(__sn_rhs) == 40.0)) && (*(calls) == (beforeCalls).checked_add(3).expect("checked arithmetic failed"))) && ok));
    let mut afterDivide: f64 = { let (__sn_rhs, __sn_place) = (SideEffects::doubleRhs(&mut *(calls), 5.0), &mut (*(__sn_rhs))); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    (ok = ((((afterDivide == 8.0) && (*(__sn_rhs) == 8.0)) && (*(calls) == (beforeCalls).checked_add(4).expect("checked arithmetic failed"))) && ok));
    let mut beforeIncrement: f64 = { let __sn_place = &mut (*(__sn_rhs)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    (ok = (((beforeIncrement == 8.0) && (*(__sn_rhs) == 9.0)) && ok));
    let mut beforeDecrement: f64 = { let __sn_place = &mut (*(__sn_rhs)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    (ok = (((beforeDecrement == 9.0) && (*(__sn_rhs) == 8.0)) && ok));
    return ok;
}

fn forwardFreeFloat(value: &mut f32, calls: &mut i64) -> bool {
    return freeFloat(&mut *(value), &mut *(calls));
}

fn forwardFreeDouble(value: &mut f64, calls: &mut i64) -> bool {
    return freeDouble(&mut *(value), &mut *(calls));
}

fn freshValues() -> Values {
    return Values { single: 8.0, precise: 16.0, singleBox: FloatBox { value: 8.0 }, preciseBox: DoubleBox { value: 16.0 } };
}

fn main() {
    let mut calls: i64 = 0;
    let mut freeSingle: f32 = 8.0;
    let mut direct: Values = freshValues();
    let mut staticPrecise: f64 = 16.0;
    let mut instanceSingle: f32 = 8.0;
    let mut ops: MutationOps = MutationOps {  };
    let mut freeFloatOk: bool = freeFloat(&mut (freeSingle), &mut (calls));
    println!("{}", ((freeFloatOk && (freeSingle == 6.0)) && (calls == 4)));
    let mut freeDoubleOk: bool = freeDouble(&mut ((direct).precise), &mut (calls));
    println!("{}", ((freeDoubleOk && ((direct).precise == 8.0)) && (calls == 8)));
    let mut staticFloatOk: bool = MutationOps::staticFloat(&mut (((direct).singleBox).value), &mut (calls));
    println!("{}", ((staticFloatOk && (((direct).singleBox).value == 6.0)) && (calls == 12)));
    let mut staticDoubleOk: bool = MutationOps::staticDouble(&mut (staticPrecise), &mut (calls));
    println!("{}", ((staticDoubleOk && (staticPrecise == 8.0)) && (calls == 16)));
    let mut instanceFloatOk: bool = (ops).instanceFloat(&mut (instanceSingle), &mut (calls));
    println!("{}", ((instanceFloatOk && (instanceSingle == 6.0)) && (calls == 20)));
    let mut instanceDoubleOk: bool = (ops).instanceDouble(&mut (((direct).preciseBox).value), &mut (calls));
    println!("{}", ((instanceDoubleOk && (((direct).preciseBox).value == 8.0)) && (calls == 24)));
    let mut forwarded: Values = freshValues();
    let mut forwardedFloat: f32 = 8.0;
    let mut forwardedDouble: f64 = 16.0;
    let mut forwardFreeFloatOk: bool = forwardFreeFloat(&mut ((forwarded).single), &mut (calls));
    println!("{}", ((forwardFreeFloatOk && ((forwarded).single == 6.0)) && (calls == 28)));
    let mut forwardFreeDoubleOk: bool = forwardFreeDouble(&mut (forwardedDouble), &mut (calls));
    println!("{}", ((forwardFreeDoubleOk && (forwardedDouble == 8.0)) && (calls == 32)));
    let mut forwardStaticFloatOk: bool = MutationOps::forwardStaticFloat(&mut (forwardedFloat), &mut (calls));
    println!("{}", ((forwardStaticFloatOk && (forwardedFloat == 6.0)) && (calls == 36)));
    let mut forwardStaticDoubleOk: bool = MutationOps::forwardStaticDouble(&mut (((forwarded).preciseBox).value), &mut (calls));
    println!("{}", ((forwardStaticDoubleOk && (((forwarded).preciseBox).value == 8.0)) && (calls == 40)));
    let mut forwardInstanceFloatOk: bool = (ops).forwardInstanceFloat(&mut (((forwarded).singleBox).value), &mut (calls));
    println!("{}", ((forwardInstanceFloatOk && (((forwarded).singleBox).value == 6.0)) && (calls == 44)));
    let mut forwardInstanceDoubleOk: bool = (ops).forwardInstanceDouble(&mut ((forwarded).precise), &mut (calls));
    println!("{}", ((forwardInstanceDoubleOk && ((forwarded).precise == 8.0)) && (calls == 48)));
}
