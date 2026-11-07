/* tests/native/main.c — a C program calls checked definitions through libcubical (B18): plusw on
 * words, the derived equality on Nat (a Bool read as 1/0), and values of a represented type (Nat
 * as Word, lib/prim.cub) crossing as the representation */
#include <stdio.h>
#include <stdlib.h>
#include "../../src/libcubical.h"
typedef struct V { uint64_t tag; uint64_t n; struct V *f[]; } V;
static V *mkv(uint64_t tag, uint64_t n) { V *v = calloc(1, sizeof(V) + n * sizeof(V *)); v->tag = tag; v->n = n; return v; }
int main(void) {
    const char *libs[] = { "lib/prelude.cub", "lib/code.cub", "lib/check.cub", "lib/std.cub", "lib/prim.cub", "tests/native/rec.cub" };
    if (cubical_init(6, libs)) { fprintf(stderr, "a library failed to check\n"); return 1; }
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
    uint64_t n = 0; for (V *q = len; q && q->tag == 2; q = q->f[0]) n++;   /* prelude's length is unchecked: its Nat is the tree */
    if (n != 2) { fprintf(stderr, "length [7,8] read as %llu\n", (unsigned long long)n); return 1; }
    /* a checked definition over Nat (std's plus), Nat represented by Word (lib/prim.cub): the arguments
     * and the result cross as words, read in by from and out by to */
    const char *tn[2] = { "Nat", "Nat" }; void *na[2] = { &x, &y };
    V *pn = cubical_call_data("plus", 2, na, tn);
    if (!pn || pn->tag != 0 || pn->n != 42) { fprintf(stderr, "plus 40 2 on Nat as Word failed\n"); return 1; }
    /* a record by value (tests/native/rec.cub's Pt, two Nat fields as words): the C struct in, the C
     * struct out, and a struct argument beside a represented one */
    struct Pt { uint64_t a, b; } p = { 3, 40 };
    const char *tp[1] = { "struct Pt" }; void *pa[1] = { &p };
    struct Pt *sw = cubical_call_data_r("swapPt", 1, pa, tp, "struct Pt");
    if (!sw || sw->a != 41 || sw->b != 3) { fprintf(stderr, "swapPt (pt 3 40) as a struct failed\n"); return 1; }
    const char *tps[2] = { "struct Pt", "Nat" }; V two = { 0, 2 }; void *psa[2] = { sw, &two };
    V *sm = cubical_call_data("sumPt", 2, psa, tps);
    if (!sm || sm->tag != 0 || sm->n != 46) { fprintf(stderr, "sumPt (pt 41 3) 2 failed\n"); return 1; }
    /* item 18, the rest: a sum of records as a tag and a union, both ways */
    struct Shape { uint64_t tag; union { struct { uint64_t r; } circ; struct { uint64_t w, h; } rect; } u; };
    struct Shape sh = { 1, { .rect = { 3, 4 } } };
    const char *tsh[1] = { "struct Shape" }; void *ash[1] = { &sh };
    V *ar = cubical_call_data("area", 1, ash, tsh);
    if (!ar || ar->tag != 0 || ar->n != 7) { fprintf(stderr, "area (rect 3 4) from C failed\n"); return 1; }
    struct Shape *g1 = cubical_call_data_r("grow", 1, ash, tsh, "struct Shape");
    if (!g1 || g1->tag != 0 || g1->u.circ.r != 7) { fprintf(stderr, "grow (rect 3 4) from C failed\n"); return 1; }
    void *ag1[1] = { g1 };
    struct Shape *g2 = cubical_call_data_r("grow", 1, ag1, tsh, "struct Shape");
    if (!g2 || g2->tag != 1 || g2->u.rect.w != 7 || g2->u.rect.h != 1) { fprintf(stderr, "grow (circ 7) from C failed\n"); return 1; }
    struct Shape bad = { 5, { .circ = { 1 } } }; void *abad[1] = { &bad };
    if (cubical_call_data("area", 1, abad, tsh)) { fprintf(stderr, "a tag naming no constructor was taken\n"); return 1; }
    /* a record holding records inline */
    struct Seg { struct Pt p, q; uint64_t k; } sg = { { 1, 2 }, { 3, 4 }, 5 };
    const char *tsg[1] = { "struct Seg" }; void *asg[1] = { &sg };
    struct Seg *ss = cubical_call_data_r("segswap", 1, asg, tsg, "struct Seg");
    if (!ss || ss->p.a != 3 || ss->p.b != 4 || ss->q.a != 1 || ss->q.b != 2 || ss->k != 6) { fprintf(stderr, "segswap from C failed\n"); return 1; }
    /* UInt32, Float32, Float64, Int64 at their C widths (a at 0, f at 4, d at 8, i at 16) */
    struct Px { uint32_t a; float f; double d; int64_t i; } px = { 7, 1.5f, 2.25, -3 };
    const char *tpx[1] = { "struct Px" }; void *apx[1] = { &px };
    struct Px *pm = cubical_call_data_r("pxmix", 1, apx, tpx, "struct Px");
    if (!pm || pm->a != 8 || pm->f != 1.5f || pm->d != 4.5 || pm->i != -6) { fprintf(stderr, "pxmix from C failed\n"); return 1; }
    printf("area {rect 3 4} = %llu; grow twice = {rect %llu %llu}; segswap = {{%llu,%llu},{%llu,%llu},%llu}; pxmix {7,1.5,2.25,-3} = {%u,%g,%g,%lld}\n",
        (unsigned long long)ar->n, (unsigned long long)g2->u.rect.w, (unsigned long long)g2->u.rect.h, (unsigned long long)ss->p.a, (unsigned long long)ss->p.b,
        (unsigned long long)ss->q.a, (unsigned long long)ss->q.b, (unsigned long long)ss->k, pm->a, (double)pm->f, pm->d, (long long)pm->i);
    printf("swapPt {3,40} = {%llu,%llu}; sumPt {41,3} 2 = %llu\n", (unsigned long long)sw->a, (unsigned long long)sw->b, (unsigned long long)sm->n);
    printf("plusw 40 2 = %llu; eqw 7 7 = %llu; boxed: definition %llu answers %llu; length [7,8] = %llu; plus 40 2 on Nat as Word = %llu\n", (unsigned long long)42, (unsigned long long)r, (unsigned long long)k, (unsigned long long)bv->n, (unsigned long long)n, (unsigned long long)pn->n);
    return 0;
}
