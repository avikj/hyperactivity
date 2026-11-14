// book.cpp — the hot path of a feed handler: decode a packed wire message, apply it to a
// fixed-depth order book, answer with the top of book. The acceptance input of track C
// (archive/2026-11-11/C-clang.md, sha256 31dca2011367…, “Track C — C++ as data: Clang's AST into the checker (first half of D3)”) and of deliverable D3 “C++ through our compiler” (archive/2026-11-11/PLAN-rev9.md, revision 9, 9 Nov 2026, sha256 4d012c3a10fd…, §6.9 “Deliverables, sizes and order”). Built as
//   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c book.cpp
// No I/O here: the entry is on_message(p, n), called once per message by book_main.c.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace feed {

enum class Side : std::uint8_t { Bid = 0, Ask = 1 };

enum class MsgType : std::uint8_t { Add = 'A', Cancel = 'X', Execute = 'E', Reset = 'R' };

// A price in ticks. Comparison and arithmetic are the integer's.
struct Price {
    std::int64_t ticks;

    constexpr Price() : ticks(0) {}
    constexpr explicit Price(std::int64_t t) : ticks(t) {}

    friend constexpr bool operator==(const Price& a, const Price& b) { return a.ticks == b.ticks; }
    friend constexpr bool operator<(const Price& a, const Price& b) { return a.ticks < b.ticks; }
    friend constexpr bool operator>(const Price& a, const Price& b) { return b < a; }
    constexpr Price operator-(const Price& o) const { return Price(ticks - o.ticks); }
    Price& operator+=(std::int64_t d) {
        ticks += d;
        return *this;
    }
};

// The message as it is on the wire: 22 bytes, little-endian, no padding.
struct __attribute__((packed)) WireMsg {
    std::uint8_t type;
    std::uint8_t side;
    std::uint32_t qty;
    std::int64_t price;
    std::uint64_t order_id;
};

struct Level {
    Price price;
    std::uint64_t qty = 0;
    std::uint32_t orders = 0;
};

inline std::uint64_t saturating_sub(std::uint64_t a, std::uint64_t b) { return a > b ? a - b : 0; }

// One side of the book, N levels, kept sorted best first.
template <std::size_t N>
class Book {
public:
    Book() : bid_count_(0), ask_count_(0) {}

    void add(Side side, Price price, std::uint32_t qty) {
        std::array<Level, N>& levels = side == Side::Bid ? bids_ : asks_;
        std::size_t& count = side == Side::Bid ? bid_count_ : ask_count_;
        // the comparison that orders this side: bids high to low, asks low to high
        auto better = [side](const Price& a, const Price& b) -> bool {
            return side == Side::Bid ? a > b : a < b;
        };
        std::size_t i = 0;
        while (i < count && better(levels[i].price, price)) ++i;
        if (i < count && levels[i].price == price) {
            levels[i].qty += qty;
            levels[i].orders += 1;
            return;
        }
        if (i >= N) return;  // worse than every level kept
        std::size_t last = count < N ? count : N - 1;
        for (std::size_t j = last; j > i; --j) levels[j] = levels[j - 1];
        levels[i].price = price;
        levels[i].qty = qty;
        levels[i].orders = 1;
        if (count < N) count = count + 1;
    }

    // Remove qty at a price; a level whose quantity reaches zero is closed up.
    bool reduce(Side side, Price price, std::uint32_t qty, bool whole_order) {
        std::array<Level, N>& levels = side == Side::Bid ? bids_ : asks_;
        std::size_t& count = side == Side::Bid ? bid_count_ : ask_count_;
        for (std::size_t i = 0; i < count; ++i) {
            Level& l = levels[i];
            if (!(l.price == price)) continue;
            l.qty = saturating_sub(l.qty, static_cast<std::uint64_t>(qty));
            if (whole_order && l.orders > 0) l.orders -= 1;
            if (l.qty == 0) {
                for (std::size_t j = i; j + 1 < count; ++j) levels[j] = levels[j + 1];
                count = count - 1;
                levels[count] = Level{};
            }
            return true;
        }
        return false;
    }

    void reset() {
        for (Level& l : bids_) l = Level{};
        for (Level& l : asks_) l = Level{};
        bid_count_ = 0;
        ask_count_ = 0;
    }

    bool has_bid() const { return bid_count_ != 0; }
    bool has_ask() const { return ask_count_ != 0; }
    const Level& best_bid() const { return bids_[0]; }
    const Level& best_ask() const { return asks_[0]; }

    // Total quantity resting on one side.
    std::uint64_t depth(Side side) const {
        const std::array<Level, N>& levels = side == Side::Bid ? bids_ : asks_;
        std::uint64_t total = 0;
        for (const Level& l : levels) total += l.qty;
        return total;
    }

private:
    std::array<Level, N> bids_;
    std::array<Level, N> asks_;
    std::size_t bid_count_;
    std::size_t ask_count_;
};

// What a decoded message is handed to.
class Handler {
public:
    virtual long on_add(Side side, Price price, std::uint32_t qty) = 0;
    virtual long on_cancel(Side side, Price price, std::uint32_t qty) = 0;
    virtual long on_execute(Side side, Price price, std::uint32_t qty) = 0;
    virtual long on_reset() = 0;

protected:
    ~Handler() = default;
};

class BookHandler final : public Handler {
public:
    long on_add(Side side, Price price, std::uint32_t qty) override {
        book_.add(side, price, qty);
        ++adds_;
        return spread();
    }
    long on_cancel(Side side, Price price, std::uint32_t qty) override {
        if (!book_.reduce(side, price, qty, true)) ++misses_;
        return spread();
    }
    long on_execute(Side side, Price price, std::uint32_t qty) override {
        if (!book_.reduce(side, price, qty, false)) ++misses_;
        volume_ += qty;
        return spread();
    }
    long on_reset() override {
        book_.reset();
        return 0;
    }

    // The spread in ticks, or -1 when a side is empty.
    long spread() const {
        if (!book_.has_bid() || !book_.has_ask()) return -1;
        Price s = book_.best_ask().price - book_.best_bid().price;
        return static_cast<long>(s.ticks);
    }

private:
    Book<8> book_;
    std::uint64_t adds_ = 0;
    std::uint64_t misses_ = 0;
    std::uint64_t volume_ = 0;
};

inline bool decode(const unsigned char* p, long n, WireMsg& out) {
    if (n < static_cast<long>(sizeof(WireMsg))) return false;
    std::memcpy(&out, p, sizeof(WireMsg));
    return true;
}

long dispatch(Handler& h, const WireMsg& m) {
    const Side side = m.side == 0 ? Side::Bid : Side::Ask;
    const Price price(m.price);
    switch (static_cast<MsgType>(m.type)) {
        case MsgType::Add:
            return h.on_add(side, price, m.qty);
        case MsgType::Cancel:
            return h.on_cancel(side, price, m.qty);
        case MsgType::Execute:
            return h.on_execute(side, price, m.qty);
        case MsgType::Reset:
            return h.on_reset();
        default:
            break;
    }
    return -2;
}

static BookHandler g_handler;

}  // namespace feed

// The entry: one wire message at p, n bytes long. The answer is the spread after the message
// (-1 when a side is empty), -2 for an unknown type, -3 for a short message.
extern "C" long on_message(const unsigned char* p, long n) {
    feed::WireMsg m;
    if (!feed::decode(p, n, m)) return -3;
    return feed::dispatch(feed::g_handler, m);
}
