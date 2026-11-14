/* more_out.c — the reference driver of more.rs (archive/2026-11-11/R-rust.md, "Diagnostics"): reads records
 * of two little-endian 64-bit integers from stdin, calls more_entry(x, y) on each and prints
 * every answer, one per line.
 *   rustc --edition=2021 --crate-type=staticlib -O -Cpanic=abort more.rs -o libmore.a
 *   clang -O2 more_out.c libmore.a -o more_out_rs
 */
#include <stdio.h>
#include <string.h>

long more_entry(long x, long y);

int main(void) {
    unsigned char r[16];
    while (fread(r, 1, 16, stdin) == 16) {
        long x, y;
        memcpy(&x, r, 8);
        memcpy(&y, r + 8, 8);
        printf("%ld\n", more_entry(x, y));
    }
    return 0;
}
