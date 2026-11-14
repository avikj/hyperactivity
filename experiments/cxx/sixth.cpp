// sixth.cpp — the sixth unit of track C (archive/2026-11-11/C-clang.md, sha256 31dca2011367…, “Track C — C++ as data: Clang's AST into the checker (first half of D3)”, “What is built”, item 5 “What `-fno-exceptions -fno-rtti` and scope kept out”): compiled WITH exceptions
// and RTTI (the other units have neither). try/catch/throw with objects that have
// destructors live across the throw, a function-try-block, noexcept (the specifier and the
// operator), dynamic_cast and typeid; std::atomic operations and a volatile object; goto and
// labels; a union with non-trivial members managed by hand; bit-fields beside ordinary fields;
// a pointer to data member; and statics whose initialization runs at start-up, in order.
//   clang++ -std=c++20 -O2 -c sixth.cpp
#include <atomic>
#include <cstdint>
#include <exception>
#include <new>
#include <typeinfo>

namespace desk {

struct Rejected : std::exception {
    explicit Rejected(int c) : code(c) {}
    const char* what() const noexcept override { return "rejected"; }
    int code;
};
struct Throttled final : Rejected {
    Throttled() : Rejected(429) {}
};

// Counts the scopes alive: its destructor must run when a throw unwinds through one.
struct Scope {
    explicit Scope(long& depth) : depth_(depth) { ++depth_; }
    ~Scope() { --depth_; }
    Scope(const Scope&) = delete;
    long& depth_;
};

struct Shape {
    virtual ~Shape() = default;
    virtual long area() const = 0;
};
struct Square : Shape {
    explicit Square(long s) : side(s) {}
    long area() const override { return side * side; }
    long side;
};
struct Strip : Shape {
    long area() const override { return 3; }
};

// A tag and one of two members with constructors and destructors: the union's own special
// members are deleted, the owner runs them.
struct Text {
    explicit Text(char c) : first(c) {}
    ~Text() {}
    char first;
};
struct Slot {
    bool is_text;
    union Payload {
        Payload() {}
        ~Payload() {}
        Square square;
        Text text;
    } u;
    explicit Slot(long side) : is_text(false) { new (&u.square) Square(side); }
    explicit Slot(char c) : is_text(true) { new (&u.text) Text(c); }
    ~Slot() {
        if (is_text)
            u.text.~Text();
        else
            u.square.~Square();
    }
};

struct Packet {
    std::uint16_t kind : 3;
    std::uint16_t urgent : 1;
    std::uint16_t : 0;  // the next field starts a new unit
    std::uint16_t length : 12;
    std::int32_t delta : 20;
    std::uint8_t tail;
    long payload;
};

inline long must_be_small(long v) {
    if (v > 1000) throw Throttled();
    if (v < 0) throw Rejected(static_cast<int>(-v));
    return v;
}

inline long quiet(long v) noexcept { return v + 1; }

long checked(long v, long& depth) {
    Scope outer(depth);
    try {
        Scope inner(depth);
        return must_be_small(v) + (noexcept(quiet(v)) ? 1 : 0);
    } catch (const Throttled& t) {
        return -t.code;
    } catch (const Rejected& r) {
        return -r.code - depth;
    } catch (...) {
        throw;
    }
}

long guarded(long v) try {
    return must_be_small(v);
} catch (const std::exception& e) {
    return e.what()[0];
}

std::atomic<long> g_seen{0};
volatile long g_port = 0;

long next_base() { return g_seen.load(std::memory_order_relaxed) + 10; }
// initialized at start-up, in this order
long g_base = next_base();
long g_scaled = g_base * 2;
static Square g_unit(g_scaled > 0 ? 1 : 2);

long classify(const Shape& s) {
    if (const Square* sq = dynamic_cast<const Square*>(&s)) return sq->side;
    return typeid(s) == typeid(Strip) ? -1 : -2;
}

long retry(long v) {
    int attempts = 0;
again:
    long expected = g_seen.load();
    if (!g_seen.compare_exchange_weak(expected, expected + v)) {
        if (++attempts < 3) goto again;
        goto done;
    }
    g_seen.fetch_add(1, std::memory_order_acq_rel);
done:
    g_port = g_port + attempts;
    return g_seen.exchange(v);
}

}  // namespace desk

extern "C" long desk_step(long v) {
    using namespace desk;
    long depth = 0;
    long r = checked(v, depth) + guarded(v) + depth;
    Square sq(4);
    Strip st;
    r += classify(sq) + classify(st) + classify(g_unit);
    Slot a(5L), b('t');
    long Square::*field = &Square::side;
    r += a.u.square.*field + b.u.text.first;
    Packet p{};
    p.kind = 5;
    p.length = 100;
    p.delta = -7;
    r += p.kind + p.length + p.delta + static_cast<long>(sizeof(Packet));
    return r + retry(v) + g_base;
}
