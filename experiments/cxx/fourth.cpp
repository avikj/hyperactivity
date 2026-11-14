// fourth.cpp — the fourth unit of track C (archive/2026-11-11/C-clang.md, sha256 31dca2011367…, “Track C — C++ as data: Clang's AST into the checker (first half of D3)”, “What is built”, item 4 “Every construct the importer stopped on by design, closed”): the constructs the
// importer used to stop on, one use of each. std::span (and the C++20 rewritten `!=` its
// iterators bring); enumerations without a fixed underlying type; anonymous struct and union
// members; an init-capture; alias templates; decltype(expr) and noexcept(expr) in types;
// float, long double, wide and UTF literals; thread_local; if constexpr; std::initializer_list.
//   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c fourth.cpp
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>

namespace quote {

// Unscoped, no fixed underlying type: the implementation chooses it from the values.
enum Venue { kNone, kLit = 7, kDark = 300 };
enum Wide : std::uint8_t { kNarrow = 1 };
enum Big { kHuge = 0x1'0000'0000LL };
enum Signed { kMinus = -1, kPlus = 1 };

// A value that is an integer or a real, with the tag beside an anonymous union, and an
// anonymous struct inside that.
struct Value {
    bool real;
    union {
        std::int64_t i;
        double d;
        struct {
            std::uint32_t lo;
            std::uint32_t hi;
        };
    };
};

template <class T>
using Ref = const T&;
template <class T, std::size_t N>
using Block = T[N];
template <bool B>
using Count = std::conditional_t<B, std::uint64_t, std::uint32_t>;

// The mean of the two, in the type their sum has.
template <class A, class B>
auto mean(A a, B b) noexcept(noexcept(a + b)) -> decltype(a + b) {
    if constexpr (std::is_floating_point_v<decltype(a + b)>) {
        return (a + b) / 2;
    } else {
        return (a + b) >> 1;
    }
}

inline long sum(std::span<const int> xs) {
    long total = 0;
    for (int x : xs) total += x;
    return total;
}

inline long sum_list(std::initializer_list<long> xs) {
    long total = 0;
    for (const long* p = xs.begin(); p != xs.end(); ++p) total += *p;
    return total;
}

thread_local std::uint32_t t_calls = 0;

inline long weigh(Ref<Value> v, Venue where) {
    Block<float, 2> scale = {0.5f, 1.25f};
    long double fine = 1.0L / 3;
    float f = v.real ? static_cast<float>(v.d) : static_cast<float>(v.i);
    f *= scale[where == kDark];
    return static_cast<long>(f + fine) + (v.lo & 1);
}

}  // namespace quote

// The entry: n prices at p.
extern "C" long quote_step(const int* p, long n) {
    using namespace quote;
    ++t_calls;
    std::span<const int> xs(p, static_cast<std::size_t>(n));
    long r = sum(xs) + sum_list({1, 2, n});
    auto scaled = [k = n * 2, &r](long x) { return x * k + r; };
    r = scaled(3);

    Value v{};
    v.real = false;
    v.i = r;
    Venue where = n > 4 ? kDark : kLit;
    r += weigh(v, where) + where + kNarrow + (kHuge > 0) + kMinus;

    Count<true> wide = mean(std::uint64_t{6}, std::uint64_t{8});
    double m = mean(1.5, 2.5f);
    const wchar_t* ws = L"wide";
    const char8_t* u8s = u8"utf8";
    const char16_t* u16s = u"utf16";
    const char32_t* u32s = U"utf32";
    wchar_t wc = L'w';
    char16_t c16 = u'x';
    return r + static_cast<long>(wide) + static_cast<long>(m) + ws[0] + u8s[0] + u16s[0] + static_cast<long>(u32s[0]) +
           wc + c16 + static_cast<long>(t_calls);
}
