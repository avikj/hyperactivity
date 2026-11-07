/* tests/native/main.c — a C program calls checked definitions through libcubical (B18): plusw on
 * words, and the derived equality on Nat (a Bool read as 1/0) */
#include <stdio.h>
#include <stdlib.h>
#include "../../src/libcubical.h"
typedef struct V { uint64_t tag; uint64_t n; struct V *f[]; } V;
static V *mkv(uint64_t tag, uint64_t n) { V *v = calloc(1, sizeof(V) + n * sizeof(V *)); v->tag = tag; v->n = n; return v; }
int main(void) {
    const char *libs[] = { "lib/prelude.cub", "lib/code.cub", "lib/check.cub", "lib/std.cub" };
    if (cubical_init(4, libs)) { fprintf(stderr, "a library failed to check\n"); return 1; }
    uint64_t a[2] = { 40, 2 }, r;
    if (cubical_call_w("plusw", a, 2, &r) || r != 42) { fprintf(stderr, "plusw 40 2 = %llu (%s)\n", (unsigned long long)r, cubical_last_text()); return 1; }
    uint64_t b[2] = { 7, 7 };
    if (cubical_call_w("eqw", b, 2, &r) || r != 1) { fprintf(stderr, "eqw 7 7: %s\n", cubical_last_text()); return 1; }
    /* the boxed form the emitted C uses: plusw by its signature index, found through the name table */
    V x = { 0, 40 }, y = { 0, 2 }; void *bx[2] = { &x, &y };
    long k = cubical_index("plusw"); if (k < 0) { fprintf(stderr, "plusw is not checked\n"); return 1; }
    V *bv = cubical_call((uint64_t)k, 2, bx);
    if (!bv || bv->n != 42) { fprintf(stderr, "the boxed form did not answer 42\n"); return 1; }
    /* a data value both ways through the general boxed call: prelude's length of the list
     * cons 7 (cons 8 nil) (a constructor is tag c+1 with its fields), the Nat result unfolded */
    V *nil = mkv(1, 0), *w8 = mkv(0, 8), *w7 = mkv(0, 7);
    V *c8 = mkv(2, 2); c8->f[0] = w8; c8->f[1] = nil;
    V *c7 = mkv(2, 2); c7->f[0] = w7; c7->f[1] = c8;
    const char *tys[1] = { "List" }; void *la[1] = { c7 };
    V *len = cubical_call_data("length", 1, la, tys);
    if (!len) { fprintf(stderr, "length through the boxed data call failed\n"); return 1; }
    uint64_t n = 0; for (V *q = len; q && q->tag == 2; q = q->f[0]) n++;
    if (n != 2) { fprintf(stderr, "length [7,8] read as %llu\n", (unsigned long long)n); return 1; }
    printf("plusw 40 2 = %llu; eqw 7 7 = %llu; boxed: definition %llu answers %llu; length [7,8] = %llu\n", (unsigned long long)42, (unsigned long long)r, (unsigned long long)k, (unsigned long long)bv->n, (unsigned long long)n);
    return 0;
}
