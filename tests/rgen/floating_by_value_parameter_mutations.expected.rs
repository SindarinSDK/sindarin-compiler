#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Effects {
}

impl Effects {
    fn floatRhs(calls: &mut i64, value: f32) -> f32 {
        { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
        return value;
    }
    fn doubleRhs(calls: &mut i64, value: f64) -> f64 {
        { let __sn_previous = *(calls); *(calls) += 1; __sn_previous };
        return value;
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct FloatingOps {
    marker: i64,
}

impl FloatingOps {
    fn staticFloat(mut value: f32, calls: &mut i64, untouched: f32) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut added: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 2.0)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut subtracted: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut multiplied: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value - 7.0)), &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut divided: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut beforeIncrement: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        let mut beforeDecrement: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        return (((((((((added == 12.0) && (subtracted == 10.0)) && (multiplied == 30.0)) && (divided == 6.0)) && (beforeIncrement == 6.0)) && (beforeDecrement == 7.0)) && (value == 6.0)) && (*(calls) == (beforeCalls + 4))) && (untouched == 99.0));
    }
    fn staticDouble(mut value: f64, calls: &mut i64, untouched: f64) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut added: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 2.0)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut subtracted: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut multiplied: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 10.0)), &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut divided: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 8.0)), &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut beforeIncrement: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        let mut beforeDecrement: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        return (((((((((added == 24.0) && (subtracted == 20.0)) && (multiplied == 40.0)) && (divided == 8.0)) && (beforeIncrement == 8.0)) && (beforeDecrement == 9.0)) && (value == 8.0)) && (*(calls) == (beforeCalls + 4))) && (untouched == 99.0));
    }
    fn instanceFloat(&self, mut value: f32, calls: &mut i64, untouched: f32) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut added: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 2.0)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut subtracted: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut multiplied: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value - 7.0)), &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut divided: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut beforeIncrement: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        let mut beforeDecrement: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        return (((((((((((self).marker == 1) && (added == 12.0)) && (subtracted == 10.0)) && (multiplied == 30.0)) && (divided == 6.0)) && (beforeIncrement == 6.0)) && (beforeDecrement == 7.0)) && (value == 6.0)) && (*(calls) == (beforeCalls + 4))) && (untouched == 99.0));
    }
    fn instanceDouble(&self, mut value: f64, calls: &mut i64, untouched: f64) -> bool {
        let mut beforeCalls: i64 = *(calls);
        let mut added: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 2.0)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut subtracted: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut multiplied: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 10.0)), &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut divided: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 8.0)), &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut beforeIncrement: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
        let mut beforeDecrement: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
        return (((((((((((self).marker == 1) && (added == 24.0)) && (subtracted == 20.0)) && (multiplied == 40.0)) && (divided == 8.0)) && (beforeIncrement == 8.0)) && (beforeDecrement == 9.0)) && (value == 8.0)) && (*(calls) == (beforeCalls + 4))) && (untouched == 99.0));
    }
}

fn freeFloat(mut value: f32, calls: &mut i64, untouched: f32) -> bool {
    let mut beforeCalls: i64 = *(calls);
    let mut added: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 2.0)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut subtracted: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut multiplied: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value - 7.0)), &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut divided: f32 = { let (__sn_rhs, __sn_place) = (Effects::floatRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut beforeIncrement: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut beforeDecrement: f32 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    return (((((((((added == 12.0) && (subtracted == 10.0)) && (multiplied == 30.0)) && (divided == 6.0)) && (beforeIncrement == 6.0)) && (beforeDecrement == 7.0)) && (value == 6.0)) && (*(calls) == (beforeCalls + 4))) && (untouched == 99.0));
}

fn freeDouble(mut value: f64, calls: &mut i64, untouched: f64) -> bool {
    let mut beforeCalls: i64 = *(calls);
    let mut added: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 2.0)), &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut subtracted: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 6.0)), &mut (value)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut multiplied: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 10.0)), &mut (value)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut divided: f64 = { let (__sn_rhs, __sn_place) = (Effects::doubleRhs(&mut *(calls), (value / 8.0)), &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut beforeIncrement: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut beforeDecrement: f64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    return (((((((((added == 24.0) && (subtracted == 20.0)) && (multiplied == 40.0)) && (divided == 8.0)) && (beforeIncrement == 8.0)) && (beforeDecrement == 9.0)) && (value == 8.0)) && (*(calls) == (beforeCalls + 4))) && (untouched == 99.0));
}

fn floatSpecial(mut positive: f32, mut zero: f32, mut negativeZero: f32) -> bool {
    let mut infinity: f32 = { let (__sn_rhs, __sn_place) = (0.0, &mut (positive)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut nan: f32 = { let (__sn_rhs, __sn_place) = (zero, &mut (zero)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut signed: f32 = { let (__sn_rhs, __sn_place) = (1.0, &mut (negativeZero)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    return ((((infinity > 0.0) && (nan != nan)) && (signed == 0.0)) && ((1.0 / signed) < 0.0));
}

fn doubleSpecial(mut positive: f64, mut zero: f64, mut negativeZero: f64) -> bool {
    let mut infinity: f64 = { let (__sn_rhs, __sn_place) = (0.0, &mut (positive)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut nan: f64 = { let (__sn_rhs, __sn_place) = (zero, &mut (zero)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut signed: f64 = { let (__sn_rhs, __sn_place) = (1.0, &mut (negativeZero)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    return ((((infinity > 0.0) && (nan != nan)) && (signed == 0.0)) && ((1.0 / signed) < 0.0));
}

fn helperNames(mut __sn_rhs: f32, mut __sn_place: f32, mut __sn_next: f32, mut __sn_previous: f32, untouched: f32) -> bool {
    let mut added: f32 = { let (__sn_rhs, __sn_place) = (__sn_rhs, &mut (__sn_rhs)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut incremented: f32 = { let __sn_place = &mut (__sn_place); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    let mut multiplied: f32 = { let (__sn_rhs, __sn_place) = (2.0, &mut (__sn_next)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut decremented: f32 = { let __sn_place = &mut (__sn_previous); let __sn_previous = *__sn_place; let __sn_next = __sn_previous - 1.0; *__sn_place = __sn_next; __sn_previous };
    return (((((((((added == 4.0) && (__sn_rhs == 4.0)) && (incremented == 3.0)) && (__sn_place == 4.0)) && (multiplied == 8.0)) && (__sn_next == 8.0)) && (decremented == 5.0)) && (__sn_previous == 4.0)) && (untouched == 6.0));
}

fn shadowOrder(mut value: f64) -> f64 {
    if true {
        { let (__sn_rhs, __sn_place) = (1.0, &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        let mut beforeShadow: f64 = value;
        let mut value: f64 = beforeShadow;
        { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous + 1.0; *__sn_place = __sn_next; __sn_previous };
    }
    return value;
}

fn main() {
    let mut calls: i64 = 0;
    let mut freeFloatCaller: f32 = 8.0;
    let mut freeDoubleCaller: f64 = 16.0;
    let mut staticFloatCaller: f32 = 8.0;
    let mut staticDoubleCaller: f64 = 16.0;
    let mut instanceFloatCaller: f32 = 8.0;
    let mut instanceDoubleCaller: f64 = 16.0;
    let mut specialFloatOne: f32 = 1.0;
    let mut specialFloatZero: f32 = 0.0;
    let mut specialFloatNegativeZero: f32 = (-0.0);
    let mut specialDoubleOne: f64 = 1.0;
    let mut specialDoubleZero: f64 = 0.0;
    let mut specialDoubleNegativeZero: f64 = (-0.0);
    let mut orderCaller: f64 = 4.0;
    let mut ops: FloatingOps = FloatingOps { marker: 1 };
    println!("{}", freeFloat(freeFloatCaller, &mut (calls), 99.0));
    println!("{}", freeDouble(freeDoubleCaller, &mut (calls), 99.0));
    println!("{}", FloatingOps::staticFloat(staticFloatCaller, &mut (calls), 99.0));
    println!("{}", FloatingOps::staticDouble(staticDoubleCaller, &mut (calls), 99.0));
    println!("{}", (ops).instanceFloat(instanceFloatCaller, &mut (calls), 99.0));
    println!("{}", (ops).instanceDouble(instanceDoubleCaller, &mut (calls), 99.0));
    println!("{}", (calls == 24));
    println!("{}", ((((((freeFloatCaller == 8.0) && (freeDoubleCaller == 16.0)) && (staticFloatCaller == 8.0)) && (staticDoubleCaller == 16.0)) && (instanceFloatCaller == 8.0)) && (instanceDoubleCaller == 16.0)));
    println!("{}", floatSpecial(specialFloatOne, specialFloatZero, specialFloatNegativeZero));
    println!("{}", doubleSpecial(specialDoubleOne, specialDoubleZero, specialDoubleNegativeZero));
    println!("{}", (((specialFloatOne == 1.0) && (specialFloatZero == 0.0)) && ((1.0 / specialFloatNegativeZero) < 0.0)));
    println!("{}", (((specialDoubleOne == 1.0) && (specialDoubleZero == 0.0)) && ((1.0 / specialDoubleNegativeZero) < 0.0)));
    println!("{}", helperNames(2.0, 3.0, 4.0, 5.0, 6.0));
    println!("{}", ((shadowOrder(orderCaller) == 5.0) && (orderCaller == 4.0)));
}
