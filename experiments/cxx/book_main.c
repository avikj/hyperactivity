/* book_main.c — the reference driver for book.cpp: reads 22-byte wire messages from stdin,
 * calls on_message on each, and prints the count, the sum of the answers and the nanoseconds
 * per message. Built with clang it is the reference binary for the latency comparison:
 *   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c book.cpp -o book.o
 *   clang -O2 book_main.c book.o -o book_ref
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

long on_message(const unsigned char *p, long n);

enum { MSG = 22 };

int main(void) {
    size_t cap = 1 << 20, len = 0;
    unsigned char *buf = malloc(cap);
    if (!buf) return 1;
    for (;;) {
        size_t r = fread(buf + len, 1, cap - len, stdin);
        len += r;
        if (r == 0) break;
        if (len == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) return 1;
        }
    }
    size_t count = len / MSG;
    struct timespec t0, t1;
    long sum = 0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (size_t i = 0; i < count; i++) sum += on_message(buf + i * MSG, MSG);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 + (double)(t1.tv_nsec - t0.tv_nsec);
    printf("messages %zu\nsum %ld\nns/message %.1f\n", count, sum, count ? ns / (double)count : 0.0);
    free(buf);
    return 0;
}
