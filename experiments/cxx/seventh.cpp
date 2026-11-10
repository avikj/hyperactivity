// seventh.cpp — the seventh unit of track C (tracks/C-clang.md, item 5): coroutines, as Clang
// desugars them. A generator of longs (co_yield, co_return;), a task that awaits an awaiter
// with a result (co_await, co_return v), and the code that drives both by resume().
//   clang++ -std=c++20 -O2 -c seventh.cpp
#include <coroutine>
#include <exception>

namespace flow {

// A lazily produced sequence of longs.
struct Stream {
    struct promise_type {
        long current = 0;
        Stream get_return_object() { return Stream{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(long v) noexcept {
            current = v;
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    explicit Stream(std::coroutine_handle<promise_type> h) : handle(h) {}
    Stream(Stream&& o) noexcept : handle(o.handle) { o.handle = nullptr; }
    Stream(const Stream&) = delete;
    ~Stream() {
        if (handle) handle.destroy();
    }

    bool next() {
        handle.resume();
        return !handle.done();
    }
    long value() const { return handle.promise().current; }

    std::coroutine_handle<promise_type> handle;
};

// An awaiter that never suspends and hands back twice what it was given.
struct Doubled {
    long v;
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    long await_resume() const noexcept { return v * 2; }
};

// A computation started eagerly, whose result is read when it is done.
struct Task {
    struct promise_type {
        long result = 0;
        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(long v) noexcept { result = v; }
        void unhandled_exception() { std::terminate(); }
    };
    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    Task(const Task&) = delete;
    ~Task() { handle.destroy(); }
    long get() const { return handle.promise().result; }
    std::coroutine_handle<promise_type> handle;
};

Stream count_to(long n) {
    for (long i = 1; i <= n; ++i) co_yield i;
    co_return;
}

Task total_doubled(long n) {
    long sum = 0;
    for (long i = 0; i < n; ++i) sum += co_await Doubled{i};
    co_return sum;
}

}  // namespace flow

extern "C" long flow_step(long n) {
    long r = 0;
    flow::Stream s = flow::count_to(n);
    while (s.next()) r += s.value();
    flow::Task t = flow::total_doubled(n);
    return r + t.get();
}
