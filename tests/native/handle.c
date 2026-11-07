/* tests/native/handle.c — a C library written by hand, entered by #native … from (B18): its
 * entries make and read a handle the net keeps opaque (a `void *` of type Native), and its
 * cubical_release finalizes a handle when the net erases its last copy. The entries treat a
 * handle as read-only: a new value is a new handle (the net may share a handle among readers). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct { uint64_t n; uint64_t *xs; } Buf;
static uint64_t live, made;
/* an empty buffer; the word is ignored (an entry takes at least one argument) */
void *buf_new(uint64_t unused) { (void)unused; Buf *b = calloc(1, sizeof(Buf)); live++; made++; return b; }
/* a new buffer: b with x appended */
void *buf_push(void *bp, uint64_t x) {
    const Buf *b = bp; Buf *r = calloc(1, sizeof(Buf)); r->n = b->n + 1; r->xs = malloc(r->n * sizeof(uint64_t));
    if (b->n) memcpy(r->xs, b->xs, b->n * sizeof(uint64_t));
    r->xs[b->n] = x; live++; made++; return r;
}
uint64_t buf_sum(void *bp) { const Buf *b = bp; uint64_t s = 0; for (uint64_t i = 0; i < b->n; i++) s += b->xs[i]; return s; }
uint64_t buf_len(void *bp) { return ((const Buf *)bp)->n; }
/* how many buffers are alive, and how many were made (the word is ignored) */
uint64_t buf_live(uint64_t unused) { (void)unused; return live; }
uint64_t buf_made(uint64_t unused) { (void)unused; return made; }
void cubical_release(void *bp) { Buf *b = bp; free(b->xs); free(b); live--; }
