#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
}
