/* tests/native/rec.c — records by value written by hand, entered by #native … from (B18, item 18):
 * a struct named after its data type (struct D), laid out as the machine lays D out: one constructor
 * its fields; several a uint64_t tag and a union of one struct per constructor (any member names);
 * a nested record inline; Nat (represented by Word) uint64_t, UInt32 uint32_t, Float32 float,
 * Float64 double, Int64 int64_t. tests/native.cub declares the data types, and probes (definitions
 * answering otherwise) these entries replace. */
#include <stdint.h>
struct Pt { uint64_t a; uint64_t b; };
struct Shape { uint64_t tag; union { struct { uint64_t r; } circ; struct { uint64_t w, h; } rect; } u; };
struct Seg { struct Pt p; struct Pt q; uint64_t k; };
struct Px { uint32_t a; float f; double d; int64_t i; };
/* area: circ r is r + r, rect w h is w + h */
uint64_t shape_area(struct Shape s) { return s.tag == 0 ? s.u.circ.r + s.u.circ.r : s.u.rect.w + s.u.rect.h; }
/* grow: circ r becomes rect r 1, rect w h becomes circ (w + h) */
struct Shape shape_grow(struct Shape s) {
    struct Shape r;
    if (s.tag == 0) { r.tag = 1; r.u.rect.w = s.u.circ.r; r.u.rect.h = 1; }
    else { r.tag = 0; r.u.circ.r = s.u.rect.w + s.u.rect.h; }
    return r;
}
/* the ends swapped, k one more */
struct Seg seg_swap(struct Seg s) { struct Seg r = { s.q, s.p, s.k + 1 }; return r; }
/* a one more, f as it is, d and i doubled */
struct Px px_mix(struct Px p) { struct Px r = { p.a + 1, p.f, p.d + p.d, p.i + p.i }; return r; }
/* f's bits as a word, a nested record's sum: the scalars as C reads them */
uint64_t px_fbits(struct Px p, uint64_t k) { union { float f; uint32_t u; } b = { p.f }; return (uint64_t)b.u + k; }
