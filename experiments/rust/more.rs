// more.rs — the second input of track R (archive/2026-11-11/R-rust.md): what MIR has that book.rs does not
// reach. A tuple returned and taken apart, a call through a function pointer, a generic
// function at two types, a type with a destructor, an enum with fields and explicit
// discriminants, an array filled by repetition, division (with its checks), shifts, negation,
// and 16- and 32-bit integers. The entry is more_entry(x, y).

#[repr(u8)]
pub enum Shape {
    Dot = 2,
    Line(u16) = 5,
    Box { w: u32, h: u32 } = 9,
}

pub struct Tally {
    total: u64,
    log: *mut u64,
}

impl Drop for Tally {
    fn drop(&mut self) {
        unsafe { *self.log = (*self.log).wrapping_add(self.total) };
    }
}

static mut LOG: u64 = 0;

#[inline(never)]
fn min_max(a: i64, b: i64) -> (i64, i64) {
    if a < b { (a, b) } else { (b, a) }
}

#[inline(never)]
fn pick<T: Copy>(a: T, b: T, first: bool) -> T {
    if first { a } else { b }
}

fn double(x: u32) -> u32 {
    x.wrapping_mul(2)
}

fn halve(x: u32) -> u32 {
    x >> 1
}

#[inline(never)]
fn apply(f: fn(u32) -> u32, x: u32) -> u32 {
    f(x)
}

#[inline(never)]
fn area(s: &Shape) -> u64 {
    match s {
        Shape::Dot => 0,
        Shape::Line(n) => *n as u64,
        Shape::Box { w, h } => (*w as u64).wrapping_mul(*h as u64),
    }
}

#[inline(never)]
fn make(x: u32) -> Shape {
    match x % 3 {
        0 => Shape::Dot,
        1 => Shape::Line(x as u16),
        _ => Shape::Box { w: x, h: x ^ 0xff },
    }
}

#[inline(never)]
fn fill(seed: u8, i: usize) -> u8 {
    let mut a = [seed; 16];
    a[i & 15] = !seed;
    a[(i >> 2) & 15].wrapping_add(a[0])
}

#[inline(never)]
fn tally(x: u64) {
    let t = Tally { total: x, log: unsafe { core::ptr::addr_of_mut!(LOG) } };
    if x == 7 {
        return;
    }
    let u = Tally { total: t.total / 3, log: t.log };
    drop(u);
}

#[no_mangle]
pub extern "C" fn more_entry(x: i64, y: i64) -> i64 {
    let (lo, hi) = min_max(x, y);
    let d = if lo != 0 { hi / lo } else { -hi };
    let p = pick::<i32>(lo as i32, hi as i32, d > 0);
    let q = pick::<u16>(lo as u16, hi as u16, d < 0);
    let f: fn(u32) -> u32 = if x & 1 == 0 { double } else { halve };
    let r = apply(f, p as u32);
    let s = make(r);
    let b = fill(q as u8, r as usize);
    tally(x as u64);
    (area(&s) as i64)
        .wrapping_add(b as i64)
        .wrapping_add((q as i64) << 3)
        .wrapping_add(unsafe { LOG } as i64)
}
