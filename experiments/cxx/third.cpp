// third.cpp — the third unit of track C (archive/2026-11-11/C-clang.md): what real code reaches at once and
// the first two units do not have. A small hierarchy of instruments with virtual destructors,
// built in an arena by placement new and on the heap; a call that names its base's method
// with a qualifier (not virtual); array new and delete; an array initializer with a filler;
// floating point; a static data member and a static local; a function template instantiated
// at two types; a conversion operator; a lambda that captures `this`; and the same name
// (`Quote`) declared in two namespaces, both used; and in `Band` and `from_c` the operators,
// integer types and casts the other units happen not to use, and default arguments.
//   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c third.cpp
#include <cstddef>
#include <cstdint>
#include <new>

namespace venue {
struct Quote {
    int bid;
    int ask;
};
template <class T>
T twice(T x) { return x + x; }
}  // namespace venue

namespace risk {

struct Quote {
    long mid2;  // twice the mid price
};

constexpr int kTick = 5;

class Instrument {
public:
    virtual ~Instrument() {}
    virtual int margin(int qty) const { return qty * base_; }

protected:
    int base_ = 3;
};

class Future : public Instrument {
public:
    int margin(int qty) const override { return qty * steps_[1] + base_; }

protected:
    int steps_[6] = {1, 2};
};

class Spread final : public Future {
public:
    ~Spread() override { live -= 1; }
    Spread() { live += 1; }
    // the margin of the legs, and the future's own by a qualified (non-virtual) call
    int margin(int qty) const override { return Future::margin(qty) + legs_[0].ask - legs_[0].bid; }

    static int live;

private:
    venue::Quote legs_[4] = {{1, 2}};
};

int Spread::live = 0;

// A running total, fed through a lambda that captures `this`.
struct Total {
    long sum = 0;
    void add(const int* xs, std::size_t n) {
        auto each = [this](int x) { sum += x; };
        for (std::size_t i = 0; i < n; ++i) each(xs[i]);
    }
    explicit operator bool() const { return sum != 0; }
};

inline long mid2(const venue::Quote& q, Quote r) { return q.bid + q.ask + r.mid2; }

alignas(Spread) static unsigned char arena[sizeof(Spread)];
alignas(64) static std::int64_t counters[2];

// A band of ticks: the remaining operators, integer types and casts in one place.
struct Band {
    short lo;
    signed char step;
    long long hi;

    Band() : Band(0, 1, 0) {}  // a delegating constructor
    Band(short l, signed char s, long long h = 8) : lo(l), step(s), hi(h) {}

    long long width() const& { return hi - lo; }
    long long width() && { return (hi - lo) / 2; }
    bool holds(long long x) const { return lo <= x && x <= hi; }
};

inline Band make_band(int k, signed char step = 2) { return Band(static_cast<short>(-k), step, k); }

// What a C interface hands over: an opaque pointer and a selector.
inline long from_c(void* opaque, int which, long Total::* field) {
    Total* t = (Total*)opaque;
    const unsigned char* raw = reinterpret_cast<const unsigned char*>(t);
    (void)raw;
    long Total::* none = nullptr;
    bool present = t;
    bool chosen = which;
    switch (which) {
        case -1:
            return present ? ~(t->*field) : 0;
        case 0:
            return field == none ? +1L : t->*field;
        default:
            break;
    }
    unsigned __int128 wide = static_cast<unsigned long long>(t->sum);
    wide = wide * wide;
    int left = which;
    while (left-- > 0) wide = wide / 3, counters[1] += 1;
    return static_cast<long>(wide) + chosen;
}

}  // namespace risk
// The entry: n prices at p. The answer mixes every construct above; it has no other meaning.
extern "C" long risk_step(const int* p, long n) {
    using namespace risk;
    static int calls = 0;
    ++calls;

    Spread* s = new (arena) Spread;
    Instrument* i = s;
    long r = i->margin(2);
    const Instrument& ri = *s;
    r += ri.margin(1);
    s->~Spread();

    Instrument* h = new Future();
    r += h->margin(calls);
    delete h;

    long* scratch = new long[3]{};
    long pad[3] = {n};
    scratch[0] = pad[0] + pad[2];
    r += scratch[0];
    delete[] scratch;

    double scaled = static_cast<double>(r) * 1.5 + 0.25;
    r = static_cast<long>(scaled);

    Total total;
    total.add(p, static_cast<std::size_t>(n));
    if (total) r += total.sum;

    const Band& kept = make_band(4);          // a temporary whose lifetime is the reference's
    static const Band& fixed = Band(-8, 1);    // and one of static storage (a default argument)
    Band unit;
    r += kept.width() + make_band(6, 3).width() + fixed.holds(r) + unit.step;
    r += from_c(&total, -1, &Total::sum) + from_c(&total, 0, nullptr) + from_c(&total, 2, &Total::sum);
    counters[0] += r;

    venue::Quote q{venue::twice(kTick), venue::twice(kTick) + 1};
    r += mid2(q, Quote{venue::twice(4L)});
    return r + Spread::live;
}
