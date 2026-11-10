// book.rs — the Rust twin of experiments/cxx/book.cpp: decode a packed wire message, apply it
// to a fixed-depth order book, answer with the spread. The acceptance input of track R
// (tracks/R-rust.md) and, with book.cpp, of the cross-language equality (PLAN Phase 6).
// Reference build (the same driver as the C++ one, experiments/cxx/book_main.c):
//   rustc --edition=2021 --crate-type=staticlib -O -Cpanic=abort book.rs -o libbook.a
//   clang -O2 ../cxx/book_main.c libbook.a -o book_ref_rs
// No I/O here: the entry is on_message(p, n), called once per message.

#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Side {
    Bid = 0,
    Ask = 1,
}

// A price in ticks. Comparison and arithmetic are the integer's.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
pub struct Price {
    ticks: i64,
}

impl core::ops::Sub for Price {
    type Output = Price;
    fn sub(self, o: Price) -> Price {
        Price { ticks: self.ticks.wrapping_sub(o.ticks) }
    }
}

// The message as it is on the wire: 22 bytes, little-endian, no padding.
#[repr(C, packed)]
pub struct WireMsg {
    typ: u8,
    side: u8,
    qty: u32,
    price: i64,
    order_id: u64,
}

const WIRE_LEN: usize = 22;

#[derive(Clone, Copy, Default)]
pub struct Level {
    price: Price,
    qty: u64,
    orders: u32,
}

fn saturating_sub(a: u64, b: u64) -> u64 {
    match a.checked_sub(b) {
        Some(d) => d,
        None => 0,
    }
}

// Both sides of the book, N levels each, kept sorted best first.
pub struct Book<const N: usize> {
    bids: [Level; N],
    asks: [Level; N],
    bid_count: usize,
    ask_count: usize,
}

impl<const N: usize> Book<N> {
    pub const fn new() -> Self {
        Book {
            bids: [Level { price: Price { ticks: 0 }, qty: 0, orders: 0 }; N],
            asks: [Level { price: Price { ticks: 0 }, qty: 0, orders: 0 }; N],
            bid_count: 0,
            ask_count: 0,
        }
    }

    fn side_mut(&mut self, side: Side) -> (&mut [Level; N], &mut usize) {
        match side {
            Side::Bid => (&mut self.bids, &mut self.bid_count),
            Side::Ask => (&mut self.asks, &mut self.ask_count),
        }
    }

    pub fn add(&mut self, side: Side, price: Price, qty: u32) {
        let (levels, count) = self.side_mut(side);
        // the comparison that orders this side: bids high to low, asks low to high
        let better = |a: &Price, b: &Price| -> bool {
            if side == Side::Bid { a > b } else { a < b }
        };
        let mut i: usize = 0;
        while i < *count && better(&levels[i].price, &price) {
            i += 1;
        }
        if i < *count && levels[i].price == price {
            levels[i].qty = levels[i].qty.wrapping_add(qty as u64);
            levels[i].orders = levels[i].orders.wrapping_add(1);
            return;
        }
        if i >= N {
            return; // worse than every level kept
        }
        let last = if *count < N { *count } else { N - 1 };
        let mut j = last;
        while j > i {
            levels[j] = levels[j - 1];
            j -= 1;
        }
        levels[i].price = price;
        levels[i].qty = qty as u64;
        levels[i].orders = 1;
        if *count < N {
            *count += 1;
        }
    }

    // Remove qty at a price; a level whose quantity reaches zero is closed up.
    pub fn reduce(&mut self, side: Side, price: Price, qty: u32, whole_order: bool) -> bool {
        let (levels, count) = self.side_mut(side);
        let mut i: usize = 0;
        while i < *count {
            if levels[i].price != price {
                i += 1;
                continue;
            }
            let l = &mut levels[i];
            l.qty = saturating_sub(l.qty, qty as u64);
            if whole_order && l.orders > 0 {
                l.orders -= 1;
            }
            if l.qty == 0 {
                let mut j = i;
                while j + 1 < *count {
                    levels[j] = levels[j + 1];
                    j += 1;
                }
                *count -= 1;
                levels[*count] = Level::default();
            }
            return true;
        }
        false
    }

    pub fn reset(&mut self) {
        for l in self.bids.iter_mut() {
            *l = Level::default();
        }
        for l in self.asks.iter_mut() {
            *l = Level::default();
        }
        self.bid_count = 0;
        self.ask_count = 0;
    }

    pub fn has_bid(&self) -> bool {
        self.bid_count != 0
    }
    pub fn has_ask(&self) -> bool {
        self.ask_count != 0
    }
    pub fn best_bid(&self) -> &Level {
        &self.bids[0]
    }
    pub fn best_ask(&self) -> &Level {
        &self.asks[0]
    }
}

// What a decoded message is handed to.
pub trait Handler {
    fn on_add(&mut self, side: Side, price: Price, qty: u32) -> i64;
    fn on_cancel(&mut self, side: Side, price: Price, qty: u32) -> i64;
    fn on_execute(&mut self, side: Side, price: Price, qty: u32) -> i64;
    fn on_reset(&mut self) -> i64;
}

pub struct BookHandler {
    book: Book<8>,
    adds: u64,
    misses: u64,
    volume: u64,
}

impl BookHandler {
    pub const fn new() -> Self {
        BookHandler { book: Book::new(), adds: 0, misses: 0, volume: 0 }
    }

    // The spread in ticks, or -1 when a side is empty.
    fn spread(&self) -> i64 {
        if !self.book.has_bid() || !self.book.has_ask() {
            return -1;
        }
        let s = self.book.best_ask().price - self.book.best_bid().price;
        s.ticks
    }
}

impl Handler for BookHandler {
    fn on_add(&mut self, side: Side, price: Price, qty: u32) -> i64 {
        self.book.add(side, price, qty);
        self.adds = self.adds.wrapping_add(1);
        self.spread()
    }
    fn on_cancel(&mut self, side: Side, price: Price, qty: u32) -> i64 {
        if !self.book.reduce(side, price, qty, true) {
            self.misses = self.misses.wrapping_add(1);
        }
        self.spread()
    }
    fn on_execute(&mut self, side: Side, price: Price, qty: u32) -> i64 {
        if !self.book.reduce(side, price, qty, false) {
            self.misses = self.misses.wrapping_add(1);
        }
        self.volume = self.volume.wrapping_add(qty as u64);
        self.spread()
    }
    fn on_reset(&mut self) -> i64 {
        self.book.reset();
        0
    }
}

fn decode(b: &[u8]) -> Option<WireMsg> {
    if b.len() < WIRE_LEN {
        return None;
    }
    Some(WireMsg {
        typ: b[0],
        side: b[1],
        qty: u32::from_le_bytes([b[2], b[3], b[4], b[5]]),
        price: i64::from_le_bytes([b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13]]),
        order_id: u64::from_le_bytes([b[14], b[15], b[16], b[17], b[18], b[19], b[20], b[21]]),
    })
}

#[inline(never)]
pub fn dispatch(h: &mut dyn Handler, m: &WireMsg) -> i64 {
    let side = if m.side == 0 { Side::Bid } else { Side::Ask };
    let price = Price { ticks: m.price };
    match m.typ {
        b'A' => h.on_add(side, price, m.qty),
        b'X' => h.on_cancel(side, price, m.qty),
        b'E' => h.on_execute(side, price, m.qty),
        b'R' => h.on_reset(),
        _ => -2,
    }
}

static mut G_HANDLER: BookHandler = BookHandler::new();

// The entry: one wire message at p, n bytes long. The answer is the spread after the message
// (-1 when a side is empty), -2 for an unknown type, -3 for a short message.
#[no_mangle]
pub extern "C" fn on_message(p: *const u8, n: usize) -> i64 {
    let b: &[u8] = unsafe { core::slice::from_raw_parts(p, n) };
    let m = match decode(b) {
        Some(m) => m,
        None => return -3,
    };
    let h: &mut dyn Handler = unsafe { &mut *core::ptr::addr_of_mut!(G_HANDLER) };
    dispatch(h, &m)
}
