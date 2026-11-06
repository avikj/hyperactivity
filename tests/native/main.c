/* tests/native/main.c — a C program calls checked definitions through libcubical (B18): plusw on
 * words, and the derived equality on Nat (a Bool read as 1/0) */
#include <stdio.h>
#include <stdlib.h>
#include "../../src/libcubical.h"
int main(void) {
    const char *libs[] = { "lib/prelude.cub", "lib/code.cub", "lib/check.cub", "lib/std.cub" };
    if (cubical_init(4, libs)) { fprintf(stderr, "a library failed to check\n"); return 1; }
    uint64_t a[2] = { 40, 2 }, r;
    if (cubical_call("plusw", a, 2, &r) || r != 42) { fprintf(stderr, "plusw 40 2 = %llu (%s)\n", (unsigned long long)r, cubical_last_text()); return 1; }
    uint64_t b[2] = { 7, 7 };
    if (cubical_call("eqw", b, 2, &r) || r != 1) { fprintf(stderr, "eqw 7 7: %s\n", cubical_last_text()); return 1; }
    printf("plusw 40 2 = %llu; eqw 7 7 = %llu\n", (unsigned long long)42, (unsigned long long)r);
    return 0;
}
