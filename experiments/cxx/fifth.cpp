// fifth.cpp — the fifth unit of track C (tracks/C-clang.md, item 4): inheritance beyond one
// base. Two interfaces implemented by one class (a second vtable pointer, a this-adjusting
// thunk); a diamond with a virtual base (virtual base offsets, vcall offsets, construction
// vtables and the VTT); a covariant return through a non-primary base (a return-adjusting
// thunk); and a class whose vtable is emitted by another unit (its key function is not
// defined here).
//   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c fifth.cpp
#include <cstdint>

namespace gate {

struct Reader {
    virtual long read(long at) const = 0;
    virtual ~Reader() = default;
    long reads = 0;
};

struct Writer {
    virtual long write(long at, long v) = 0;
    virtual Writer* self() { return this; }
    virtual ~Writer() = default;
    long writes = 0;
};

// Both interfaces over one array: Writer is at a non-zero offset, so its slots are thunks.
struct Store final : Reader, Writer {
    long read(long at) const override { return cells[at & 3]; }
    long write(long at, long v) override {
        cells[at & 3] = v;
        return ++writes;
    }
    Store* self() override { return this; }  // covariant: Writer* through a thunk
    long cells[4] = {};
};

// A diamond: one Clock shared by the two sides.
struct Clock {
    explicit Clock(long start) : now(start) {}
    virtual long tick() { return ++now; }
    virtual ~Clock() = default;
    long now;
};
struct Fast : virtual Clock {
    Fast() : Clock(0) {}
    long tick() override { return now += 2; }
    long fast = 1;
};
struct Slow : virtual Clock {
    Slow() : Clock(0) {}
    virtual long lag() const { return slow; }
    long slow = 7;
};
struct Both final : Fast, Slow {
    Both() : Clock(100) {}
    long tick() override { return Fast::tick() + lag(); }
};

// Declared here, its vtable emitted where `settle` is defined (another unit).
struct Remote {
    virtual long settle(long v);  // the key function: not defined in this unit
    virtual long peek() const { return held; }
    virtual ~Remote() = default;
    long held = 0;
};

long through(Reader& r, Writer& w, Clock& c, Remote* far) {
    long a = w.write(1, 40) + r.read(1) + w.self()->writes;
    a += c.tick();
    if (far) a += far->settle(a) + far->peek();
    return a;
}

}  // namespace gate

extern "C" long gate_step(long seed, void* remote) {
    using namespace gate;
    Store s;
    Both b;
    Fast& f = b;
    Slow* sl = &b;
    Clock* base = sl;
    Writer* w = &s;
    Store* back = static_cast<Store*>(w);
    return through(s, *w, *base, static_cast<Remote*>(remote)) + f.tick() + sl->lag() + back->reads + seed;
}
