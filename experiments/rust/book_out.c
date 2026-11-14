/* book_out.c — the reference driver of the pair book.rs / book.cpp: reads 22-byte wire
 * messages from stdin, calls on_message(p, n) on each (n is argv[1], default 22) and prints
 * every answer, one per line. Linked once against each implementation, the two binaries'
 * outputs on the same input are compared (archive/2026-11-11/R-rust.md, Acceptance):
 *   rustc --edition=2021 --crate-type=staticlib -O -Cpanic=abort book.rs -o libbook.a
 *   clang -O2 book_out.c libbook.a -o book_out_rs
 *   clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c ../cxx/book.cpp -o book.o
 *   clang -O2 book_out.c book.o -o book_out_cxx
 */
#include <stdio.h>
#include <stdlib.h>

long on_message(const unsigned char *p, long n);

enum { MSG = 22 };

int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : MSG;
    unsigned char m[MSG];
    while (fread(m, 1, MSG, stdin) == MSG) printf("%ld\n", on_message(m, n));
    return 0;
}
