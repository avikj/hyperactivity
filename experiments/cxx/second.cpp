// second.cpp — the second acceptance unit of track C (archive/2026-11-11/C-clang.md): a small order-entry
// session, written to use what book.cpp does not: a class template with a type and a non-type
// parameter instantiated twice, constructors with member initializer lists, a destructor,
// new/delete, a call through a pointer to member function, a union, bit-fields, do/while,
// break/continue, sizeof/alignof, compound assignment and ?:, a C array of structs, a string
// literal, pointer arithmetic, and a 128-bit multiply (__int128 and __builtin_mul_overflow).
//   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c second.cpp
#include <cstddef>
#include <cstdint>

namespace session {

// A fixed-capacity ring of T, N slots.
template <typename T, std::size_t N>
class Ring {
public:
    Ring() : head_(0), size_(0) {}

    bool push(const T& v) {
        if (size_ == N) return false;
        slots_[(head_ + size_) % N] = v;
        size_ += 1;
        return true;
    }

    bool pop(T& out) {
        if (size_ == 0) return false;
        out = slots_[head_];
        head_ = (head_ + 1) % N;
        size_ -= 1;
        return true;
    }

    std::size_t size() const { return size_; }
    static constexpr std::size_t capacity() { return N; }

private:
    T slots_[N];
    std::size_t head_;
    std::size_t size_;
};

// Flags packed into one 32-bit word.
struct Flags {
    std::uint32_t side : 1;
    std::uint32_t ioc : 1;
    std::uint32_t hidden : 1;
    std::uint32_t venue : 5;
    std::uint32_t reserved : 24;
};

// The first eight bytes of a request, read either as a whole or by parts.
union Header {
    std::uint64_t raw;
    struct {
        std::uint8_t kind;
        std::uint8_t length;
        std::uint16_t session;
        std::uint32_t sequence;
    } parts;
    unsigned char bytes[8];
};

struct Order {
    std::uint64_t id;
    std::int64_t price;   // fixed point, 1e-4
    std::uint32_t qty;
    Flags flags;
};

// price * qty in fixed point, saturating: the product is taken in 128 bits.
inline std::int64_t notional(std::int64_t price, std::uint32_t qty) {
    __int128 wide = static_cast<__int128>(price) * qty;
    const __int128 max = INT64_MAX;
    const __int128 min = INT64_MIN;
    return static_cast<std::int64_t>(wide > max ? max : wide < min ? min : wide);
}

// a * b + c with the overflow reported, by the builtin.
inline bool mul_add(std::uint64_t a, std::uint64_t b, std::uint64_t c, std::uint64_t* out) {
    std::uint64_t prod;
    if (__builtin_mul_overflow(a, b, &prod)) return false;
    return !__builtin_add_overflow(prod, c, out);
}

// A rotating checksum of the bytes in [p, end), by pointer arithmetic.
inline std::uint32_t checksum(const unsigned char* p, const unsigned char* end) {
    std::uint32_t sum = sizeof(Header) * alignof(Header);  // the seed: 64
    do {
        if (p == end) break;
        sum = (sum << 1 | sum >> 31) ^ *p++;
    } while (true);
    return sum;
}

class Session {
public:
    explicit Session(std::uint16_t id, const char* name)
        : id_(id), name_(name), next_sequence_(1), pending_(new Ring<Order, 16>()), exposure_(0) {}

    ~Session() { delete pending_; }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    long on_new(const Header& h, const unsigned char* body) {
        Order o{};
        o.id = static_cast<std::uint64_t>(h.parts.session) << 32 | h.parts.sequence;
        std::uint64_t price = 0;
        for (int i = 0; i < 8; ++i) price |= static_cast<std::uint64_t>(body[i]) << (8 * i);
        o.price = static_cast<std::int64_t>(price);
        o.qty = static_cast<std::uint32_t>(body[8]) | static_cast<std::uint32_t>(body[9]) << 8;
        o.flags.side = body[10] & 1;
        o.flags.ioc = (body[10] >> 1) & 1;
        o.flags.hidden = 0;
        o.flags.venue = body[11] & 0x1f;
        o.flags.reserved = 0;
        if (!pending_->push(o)) return -1;
        exposure_ += notional(o.price, o.qty);
        acks_.push(h.parts.sequence);
        return static_cast<long>(pending_->size());
    }

    long on_cancel(const Header& h, const unsigned char*) {
        Order o;
        if (!pending_->pop(o)) return -1;
        exposure_ -= notional(o.price, o.qty);
        acks_.push(h.parts.sequence);
        return static_cast<long>(pending_->size());
    }

    long on_heartbeat(const Header& h, const unsigned char* body) {
        std::uint32_t acked = 0, seq = 0;
        while (acks_.pop(seq)) {
            if (seq < h.parts.sequence) continue;
            acked += 1;
        }
        std::uint64_t mixed = 0;
        if (!mul_add(exposure_ < 0 ? 0 : static_cast<std::uint64_t>(exposure_), 31, acked, &mixed)) return -4;
        return static_cast<long>(mixed % 1000003) + (body != nullptr ? name_[0] == 'S' : 0);
    }

    bool expect(std::uint32_t sequence) {
        if (sequence != next_sequence_) return false;
        next_sequence_ += 1;
        return true;
    }

    std::uint16_t id() const { return id_; }

private:
    std::uint16_t id_;
    const char* name_;
    std::uint32_t next_sequence_;
    Ring<Order, 16>* pending_;
    Ring<std::uint32_t, 4> acks_;
    std::int64_t exposure_;
};

// What each request kind is handled by: a table of pointers to member functions.
struct Route {
    char kind;
    long (Session::*handle)(const Header&, const unsigned char*);
};

static const Route routes[] = {
    {'N', &Session::on_new},
    {'C', &Session::on_cancel},
    {'H', &Session::on_heartbeat},
};

static Session g_session(7, "SESSION-7");

}  // namespace session

// The entry: one request at p, n bytes: an 8-byte header, a 12-byte body, a 4-byte checksum
// of the first 20 bytes. The answer is the handler's, or -2 for an unknown kind, -3 for a
// malformed request, -5 for a sequence gap.
extern "C" long session_step(const unsigned char* p, long n) {
    using namespace session;
    constexpr long kRequest = sizeof(Header) + 12 + sizeof(std::uint32_t);
    static_assert(alignof(Header) == 8 && sizeof(Order) == 24);
    if (n != kRequest) return -3;
    Header h;
    for (std::size_t i = 0; i < sizeof h.bytes; ++i) h.bytes[i] = p[i];
    std::uint32_t want = 0;
    for (int i = 3; i >= 0; --i) want = want << 8 | p[20 + i];
    if (checksum(p, p + 20) != want || h.parts.length != 12 || h.parts.session != g_session.id()) return -3;
    if (!g_session.expect(h.parts.sequence)) return -5;
    for (const Route& r : routes) {
        if (r.kind != static_cast<char>(h.parts.kind)) continue;
        return (g_session.*r.handle)(h, p + sizeof(Header));
    }
    return -2;
}
