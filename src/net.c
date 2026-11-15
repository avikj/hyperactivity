/* Layer: core: the machine implementing the primitive language (the four layers: see the language-implementation-remainder node of task-graph/hyperactivity.cub). */
/* net.c — cells, wiring, the interaction rules, and building a net from
 * compiled syntax. */
#include "core.h"
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#ifndef __APPLE__
extern int pthread_getattr_np(pthread_t th, pthread_attr_t *attr);   /* glibc; the thread's stack, for compiled code's stack limit */
#endif
#include <time.h>
#include <sched.h>

long stat_steps, stat_kind[K_END], stat_cells;
#define ADD(x, v) cnt_add(&(x), (v))
bool net_solo = true;

/* ---------------- cells and variables: per-thread slabs ----------------
 * Cells and variables are carved from 64 KB slabs, one size class per slab, each slab counting
 * its cells in use. A freed cell goes to the freeing thread's free list once the rule is done
 * (flush_frees), and no memory is unmapped while rules run: a reader following a stale handle (a
 * variable freed and reused, net_peek) must land on a cell. Between observations, with every
 * worker parked, slabs with nothing in use are returned (arenas_release): the free lists are
 * rebuilt without their cells first. Cells too large for a slab are malloc'd and returned there. */
#define MAXFREE 4096
#define SLAB ((size_t)1 << 16)
typedef struct Slab { uint32_t n; uint32_t used; long live; struct Slab *next; } Slab;   /* n: ports per cell (or ~0u for variables); used: carved bytes; live: in use */
#define SLAB_HDR 32
_Static_assert(sizeof(Slab) <= SLAB_HDR && sizeof(Cell) + (size_t)MAXFREE * sizeof(Port) + SLAB_HDR <= SLAB, "a cell of any pooled size fits a slab");
static inline Slab *slab_of(const void *c) { return (Slab *)((uintptr_t)c & ~(SLAB - 1)); }
static inline size_t cell_bytes(int n) { return (sizeof(Cell) + (size_t)n * sizeof(Port) + 15) & ~(size_t)15; }
typedef struct { Cell *op, *v; } Redex;
typedef struct Local {
    Cell *free[MAXFREE + 1]; Slab *cur[MAXFREE + 1];   /* free list and the slab being carved, per size */
    Slab *slabs; Cell *big; Cell *defer;                /* every slab of this thread; malloc'd cells freed (returned at release); cells freed by the current rule */
    Var *vfree; Slab *vcur;
    long freed;                                         /* cells and variables freed since the last release */
    Redex *made; size_t nmade, capmade; struct Box **dead; size_t ndead, capdead; struct Wire *wires; size_t nwires, capwires;
    long steps, kind[K_END], cells, hits, keys; long pushed, done;   /* pushed, done: pairs this thread queued, and pairs it finished (its own or stolen); written by it alone */
    struct Local *next;
} Local;
static __thread Local *L;
static Local *ALL; static pthread_mutex_t allmu = PTHREAD_MUTEX_INITIALIZER;
static Local *local_make(void) { L = calloc(1, sizeof(Local)); pthread_mutex_lock(&allmu); L->next = ALL; ALL = L; pthread_mutex_unlock(&allmu); return L; }
static inline Local *local(void) { return L ? L : local_make(); }
static Slab *slab_new(Local *l, uint32_t n) {
    Slab *s = aligned_alloc(SLAB, SLAB);
    if (!s) { fputs("out of memory\n", stderr); exit(2); }
    s->n = n; s->used = SLAB_HDR; s->live = 0; s->next = l->slabs; l->slabs = s;
    return s;
}
/* in use counts cross threads: a cell is freed by whichever thread consumes it */
static inline void slab_take(Slab *s) { if (net_solo) s->live++; else __atomic_fetch_add(&s->live, 1, __ATOMIC_RELAXED); }
static inline void slab_drop(Slab *s) { if (net_solo) s->live--; else __atomic_fetch_sub(&s->live, 1, __ATOMIC_RELAXED); }
/* the counts over every thread, into the globals a reading reports (threads are parked when read) */
void stat_gather(void) {
    long st = 0, cells = 0, hits = 0, keys = 0;
    pthread_mutex_lock(&allmu);
    for (Local *l = ALL; l; l = l->next) { st += l->steps; cells += l->cells; hits += l->hits; keys += l->keys; }
    pthread_mutex_unlock(&allmu);
    stat_steps = st; stat_cells = cells; stat_hits = hits; stat_keys = keys;
}
/* the steps by kind (CUBICAL_PROFILE): gathered only when printed */
void stat_gather_kinds(void) {
    memset(stat_kind, 0, sizeof stat_kind);
    pthread_mutex_lock(&allmu);
    for (Local *l = ALL; l; l = l->next) for (int k = 0; k < K_END; k++) stat_kind[k] += l->kind[k];
    pthread_mutex_unlock(&allmu);
}

/* the cells too large for a slab, while their memory is held (net_scan_payloads walks them) */
static Cell **BIGS; static size_t nbigs, capbigs; static pthread_mutex_t bigmu = PTHREAD_MUTEX_INITIALIZER;
static void big_note(Cell *c) { pthread_mutex_lock(&bigmu); if (nbigs == capbigs) { capbigs = capbigs ? capbigs * 2 : 64; BIGS = realloc(BIGS, capbigs * sizeof(Cell *)); } BIGS[nbigs++] = c; pthread_mutex_unlock(&bigmu); }
static void big_forget(Cell *c) { pthread_mutex_lock(&bigmu); for (size_t i = 0; i < nbigs; i++) if (BIGS[i] == c) { BIGS[i] = BIGS[--nbigs]; break; } pthread_mutex_unlock(&bigmu); }
Cell *cell(int kind, int n) {
    Local *l = local();
    Cell *c;
    if (n > MAXFREE) {
        c = malloc(cell_bytes(n));
        if (!c) { fputs("out of memory\n", stderr); exit(2); }
        big_note(c);
    } else if (l->free[n]) { c = l->free[n]; l->free[n] = (Cell *)(uintptr_t)c->a; slab_take(slab_of(c)); }
    else {
        size_t sz = cell_bytes(n);
        Slab *sl = l->cur[n];
        if (!sl || sl->used + sz > SLAB) sl = l->cur[n] = slab_new(l, (uint32_t)n);
        c = (Cell *)((char *)sl + sl->used); sl->used += (uint32_t)sz; slab_take(sl);
    }
    c->n = (uint16_t)n;   /* never changes: a freed cell is reused only for a cell of the same size */
    memset(c->port, 0, (size_t)n * sizeof(Port));
    c->kind = (uint16_t)kind; c->x = 0; c->ty = 0; c->a = 0; c->p = NULL; c->key = NULL; c->e = NULL;
    l->cells++;
    return c;
}
/* the payload a cell owns */
static inline bool owns_dnf(int k) { switch (k) { case O_PEND: case V_IV: case V_FACE: case V_SYS: case V_NPAPP: case O_PAPPP: case O_BIN2: case O_SYSCONS2: case V_NTRANS: case O_TRANSD2: case V_TRANSH: case O_TRANSH: case O_IEQB: case O_FEQLB: case V_PART: return true; default: return false; } }
static bool owns_subst(int k) { return k == O_SUBST; }
static void box_release(Box *b);
static void payload_retain(Cell *c) {
    if (owns_dnf(c->kind)) dnf_retain(c->p); else if (owns_subst(c->kind)) subst_retain(c->p);
    else if (c->kind == V_REF) { rcl_inc(&((Box *)c->p)->rc); if (c->a) subst_retain((Subst *)(uintptr_t)c->a); }
    else if (c->kind == V_WORD || c->kind == V_WPLUS) big_retain(c->p);
    else if (c->kind == V_BYTES && c->p) rcl_inc(&((Bytes *)c->p)->rc);
    else if (c->kind == V_THUNK && c->a) subst_retain((Subst *)(uintptr_t)c->a);
    else if (c->kind == V_DEFER && c->p && owns_dnf((int)c->ty)) dnf_retain(c->p);
}
static void payload_release(Cell *c) {
    if (owns_dnf(c->kind)) dnf_release(c->p); else if (owns_subst(c->kind)) subst_release(c->p);
    else if (c->kind == V_REF && c->p) { box_release(c->p); if (c->a) subst_release((Subst *)(uintptr_t)c->a); c->a = 0; }
    else if (c->kind == V_WORD || c->kind == V_WPLUS) big_release(c->p);
    else if (c->kind == V_BYTES && c->p) { Bytes *b = c->p; if (rcl_dec(&b->rc) == 0) free(b); }
    else if (c->kind == V_THUNK && c->a) { subst_release((Subst *)(uintptr_t)c->a); c->a = 0; }
    else if (c->kind == V_DEFER && c->p && owns_dnf((int)c->ty)) dnf_release(c->p);
    c->p = NULL;
}
static void memo_leave(Cell *c);
void cell_free(Cell *c) { if (c->e) memo_leave(c); if (c->key) { key_release(c->key); c->key = NULL; } payload_release(c); Local *l = local(); c->a = (uint64_t)(uintptr_t)l->defer; l->defer = c; l->cells--; }
static void flush_frees(void) {
    Local *l = local();
    while (l->defer) {
        Cell *c = l->defer; l->defer = (Cell *)(uintptr_t)c->a;
        l->freed++;
        if (c->n <= MAXFREE) { c->a = (uint64_t)(uintptr_t)l->free[c->n]; l->free[c->n] = c; slab_drop(slab_of(c)); }
        else { c->a = (uint64_t)(uintptr_t)l->big; l->big = c; }
    }
}
/* Between observations (every worker parked): the free lists rebuilt without the cells and
 * variables of slabs with nothing in use, and those slabs returned; malloc'd large cells returned.
 * Run when enough has been freed since the last time to be worth the walk */
#define RELEASE_AT (1L << 20)
static void arenas_release(void) {
    long freed = 0;
    pthread_mutex_lock(&allmu);
    for (Local *l = ALL; l; l = l->next) freed += l->freed;
    if (freed < RELEASE_AT) { pthread_mutex_unlock(&allmu); return; }
    for (Local *l = ALL; l; l = l->next) {
        l->freed = 0;
        for (int n = 0; n <= MAXFREE; n++) {
            if (l->cur[n] && l->cur[n]->live == 0) l->cur[n] = NULL;
            Cell **pp = &l->free[n];
            while (*pp) { Cell *c = *pp; if (slab_of(c)->live == 0) *pp = (Cell *)(uintptr_t)c->a; else pp = (Cell **)&c->a; }
        }
        if (l->vcur && l->vcur->live == 0) l->vcur = NULL;
        Var **vp = &l->vfree;
        while (*vp) { Var *v = *vp; if (slab_of(v)->live == 0) *vp = v->next; else vp = &v->next; }
        while (l->big) { Cell *c = l->big; l->big = (Cell *)(uintptr_t)c->a; big_forget(c); free(c); }
    }
    for (Local *l = ALL; l; l = l->next) {
        Slab **sp = &l->slabs;
        while (*sp) { Slab *sl = *sp; if (sl->live == 0) { *sp = sl->next; free(sl); } else sp = &sl->next; }
    }
    pthread_mutex_unlock(&allmu);
}
static Cell *clone(const Cell *v) { Cell *c = cell(v->kind, v->n); c->x = v->x; c->ty = v->ty; c->a = v->a; c->p = v->p; c->key = v->key ? key_retain(v->key) : NULL; payload_retain(c); return c; }

/* ---------------- variables: the wires ----------------
 * (core.h Var.) A slot is written by exchange: the first side leaves what it has, the second takes
 * it. Reads of a slot (net_peek) are acquire loads, so a cell reached through one is initialized. */
static Var *var_new(void) {
    Local *l = local();
    Var *v = l->vfree;
    if (v) { l->vfree = v->next; slab_take(slab_of(v)); }
    else {
        Slab *sl = l->vcur;
        if (!sl || sl->used + sizeof(Var) > SLAB) sl = l->vcur = slab_new(l, ~0u);
        v = (Var *)((char *)sl + sl->used); sl->used += sizeof(Var); slab_take(sl);
    }
    v->p = 0; v->key = NULL;
    return v;
}
/* emptied, and kept mapped until the next release: a reader following a stale handle finds 0 or a port */
static void var_free(Var *v) {
    if (v->key) { key_release(v->key); v->key = NULL; }
    if (net_solo) v->p = 0; else __atomic_store_n(&v->p, 0, __ATOMIC_RELAXED);
    Local *l = local(); v->next = l->vfree; l->vfree = v; l->freed++; slab_drop(slab_of(v));
}
static inline Port vload(const Var *v) { return net_solo ? v->p : __atomic_load_n(&v->p, __ATOMIC_ACQUIRE); }
static inline Port vxchg(Var *v, Port x) { if (net_solo) { Port g = v->p; v->p = x; return g; } return __atomic_exchange_n(&v->p, x, __ATOMIC_ACQ_REL); }
static inline Port sload(const Port *s) { return net_solo ? *s : __atomic_load_n(s, __ATOMIC_ACQUIRE); }
static inline void sstore(Port *s, Port x) { if (net_solo) *s = x; else __atomic_store_n(s, x, __ATOMIC_RELEASE); }
/* a slot of a held value, changed by the one reader that sees it unchanged */
static inline bool claim(Port *s, Port old, Port x) {
    if (net_solo) { if (*s != old) return false; *s = x; return true; }
    return __atomic_compare_exchange_n(s, &old, x, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}
Port net_peek(Port p) { while (p && is_var(p)) p = vload(PV(p)); return p; }
#define peek net_peek
static inline Cell *cell_at(Port p) { p = peek(p); return p ? PC(p) : NULL; }
/* a slot's value taken out of a wire whose producer has delivered it (the slots, both sides done, freed) */
static Port take(Port p) {
    while (is_var(p)) { Var *v = PV(p); Port got = vxchg(v, 0); var_free(v); p = got; }
    return p;
}

/* ---------------- active pairs: per-worker deques ----------------
 * Each worker owns a deque (Chase and Lev): it pushes and pops at the bottom, so it continues with
 * the pairs its own rewrites just made (their cells are in its cache, and they are each other's
 * neighbours); other workers steal at the top, the oldest pairs, far from where the owner works.
 * Pairs made by the same rewrite are neighbours, so two workers taking consecutive pairs of one
 * queue pull the same cache lines back and forth: that was most of the time with 2 workers when
 * pairs were taken in arrival order. A full ring is replaced whole (a thief holding the old one
 * reads the same pair at the same index). No lock. */
typedef struct { size_t cap; Redex it[]; } Ring;
typedef struct { Ring *ring; char pad0[56]; size_t top; char pad1[56]; size_t bottom; char pad2[56]; } Queue;
static Queue *Q; static int NQ;
static __thread int myq;
static long pending;                       /* one thread: pairs queued. Threaded: each thread counts the pairs it pushed and
                                            * the pairs it finished (Local.pushed, Local.done), each monotone and written by
                                            * its owner alone, so no line is shared per step; nothing is in flight when the
                                            * sums are equal (quiescent(), which reads them only when there is nothing to pop) */
/* Σdone read before Σpushed: d = D ≤ P ≤ P = p, so d == p means P == D, every
 * pushed pair finished at t1, and no push between t1 and t2; a pair comes only from a rewrite of a
 * pair, so none comes after either */
static bool quiescent(void) {
    long d = 0, p = 0;
    pthread_mutex_lock(&allmu);
    for (Local *l = ALL; l; l = l->next) d += __atomic_load_n(&l->done, __ATOMIC_SEQ_CST);
    for (Local *l = ALL; l; l = l->next) p += __atomic_load_n(&l->pushed, __ATOMIC_SEQ_CST);
    pthread_mutex_unlock(&allmu);
    return d == p;
}
static Ring *ring_new(size_t cap) { Ring *r = malloc(sizeof(Ring) + cap * sizeof(Redex)); r->cap = cap; return r; }
static void queues_init(int n) {
    NQ = n; Q = calloc((size_t)n, sizeof(Queue));
    for (int i = 0; i < n; i++) Q[i].ring = ring_new(4096);
}
/* one thread: a ring of pairs (power-of-two capacity), no lock, no compaction */
static Redex *R; static size_t Rhead, Rlen, Rcap;
static inline void ring_push(Cell *op, Cell *v) {
    if (Rlen == Rcap) {
        size_t nc = Rcap ? Rcap * 2 : 4096; Redex *nr = malloc(nc * sizeof(Redex));
        for (size_t i = 0; i < Rlen; i++) nr[i] = R[(Rhead + i) & (Rcap - 1)];
        free(R); R = nr; Rcap = nc; Rhead = 0;
    }
    R[(Rhead + Rlen++) & (Rcap - 1)] = (Redex){ op, v };
    pending++;
}
static inline bool ring_pop(Redex *r) {
    if (!Rlen) return false;
    *r = R[Rhead]; Rhead = (Rhead + 1) & (Rcap - 1); Rlen--;
    return true;
}
void net_push(Cell *op, Cell *v) {
    if (net_solo) { ring_push(op, v); return; }
    if (!Q) queues_init(1);
    Queue *q = &Q[myq];
    size_t b = q->bottom, t = __atomic_load_n(&q->top, __ATOMIC_ACQUIRE);
    Ring *r = q->ring;
    if (b - t == r->cap) {
        Ring *n = ring_new(r->cap * 2);
        for (size_t i = t; i < b; i++) { Redex *x = &r->it[i & (r->cap - 1)], *y = &n->it[i & (n->cap - 1)]; __atomic_store_n(&y->op, __atomic_load_n(&x->op, __ATOMIC_RELAXED), __ATOMIC_RELAXED); __atomic_store_n(&y->v, __atomic_load_n(&x->v, __ATOMIC_RELAXED), __ATOMIC_RELAXED); }
        __atomic_store_n(&q->ring, n, __ATOMIC_RELEASE);   /* the old ring is kept: a thief may hold it */
        r = n;
    }
    /* the slot's two words are written and read atomically (relaxed): a thief may read a slot it
     * then fails to claim while the owner, having wrapped, writes it (the classic deque race) */
    { Redex *x = &r->it[b & (r->cap - 1)]; __atomic_store_n(&x->op, op, __ATOMIC_RELAXED); __atomic_store_n(&x->v, v, __ATOMIC_RELAXED); }
    { Local *l = local(); __atomic_store_n(&l->pushed, l->pushed + 1, __ATOMIC_RELEASE); }
    __atomic_store_n(&q->bottom, b + 1, __ATOMIC_RELEASE);
}
/* the owner takes the newest pair */
static bool pop_own(Queue *q, Redex *out) {
    size_t b = q->bottom;
    if (b == 0) return false;
    b--;
    __atomic_store_n(&q->bottom, b, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    size_t t = __atomic_load_n(&q->top, __ATOMIC_RELAXED);
    if (t > b) { __atomic_store_n(&q->bottom, b + 1, __ATOMIC_RELAXED); return false; }
    Ring *r = q->ring;
    Redex x, *xp = &r->it[b & (r->cap - 1)];
    x.op = __atomic_load_n(&xp->op, __ATOMIC_RELAXED); x.v = __atomic_load_n(&xp->v, __ATOMIC_RELAXED);
    if (t == b) {
        /* the last pair: a thief may be taking it */
        bool won = __atomic_compare_exchange_n(&q->top, &t, t + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED);
        __atomic_store_n(&q->bottom, b + 1, __ATOMIC_RELAXED);
        if (!won) return false;
    }
    *out = x;
    return true;
}
/* a thief takes the oldest, and only from a queue holding STEAL_AT pairs or more, unless it has
 * been idle a while. The pairs of one rewrite are pushed together and are neighbours: from a short
 * queue the thief takes a neighbour of what the owner is doing (both cores then miss on the same
 * cells), and on a sequential stretch, where the queue holds one pair at a time, it moves every
 * step to the other core. From a long queue the oldest pair is several rewrites away. A thief idle
 * for LONE_AT iterations (by then it is yielding) takes anything, so no pair waits on a busy owner
 * for long */
#define STEAL_AT 32
#define LONE_AT 1024
static bool steal(Queue *q, Redex *out, bool lone) {
    size_t t = __atomic_load_n(&q->top, __ATOMIC_ACQUIRE);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    size_t b = __atomic_load_n(&q->bottom, __ATOMIC_ACQUIRE);
    if (t >= b || (!lone && b - t < STEAL_AT)) return false;
    Ring *r = __atomic_load_n(&q->ring, __ATOMIC_ACQUIRE);
    Redex x, *xp = &r->it[t & (r->cap - 1)];
    x.op = __atomic_load_n(&xp->op, __ATOMIC_RELAXED); x.v = __atomic_load_n(&xp->v, __ATOMIC_RELAXED);
    if (!__atomic_compare_exchange_n(&q->top, &t, t + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)) return false;
    *out = x;
    return true;
}
static bool pop(Redex *r, int idle) {
    if (net_solo) return ring_pop(r);
    if (!Q) queues_init(1);
    if (pop_own(&Q[myq], r)) return true;
    bool lone = idle >= LONE_AT;
    for (int k = 1; k < NQ; k++) if (steal(&Q[(myq + k) % NQ], r, lone)) return true;
    return false;
}
/* two principal ports meet: an active pair, queued once the rule making it is complete */
static void redex(Port a, Port b) {
    Cell *x = PC(a), *y = PC(b);
    Cell *op = NULL, *val = NULL;
    if (is_value(x->kind) && !is_value(y->kind)) { op = y; val = x; }
    else if (is_value(y->kind) && !is_value(x->kind)) { op = x; val = y; }
    if (!op) return;   /* two values, or two operations: a wire nothing reads */
    /* one thread: nothing pops before the replacement is complete, so the pair is queued now
     * (in the order it would have been published) */
    if (net_solo) { ring_push(op, val); return; }
    /* held until the replacement creating it is complete */
    Local *l = local();
    if (l->nmade == l->capmade) { l->capmade = l->capmade ? l->capmade * 2 : 64; l->made = realloc(l->made, l->capmade * sizeof(Redex)); }
    l->made[l->nmade++] = (Redex){ op, val };
}
static Key *key_of_aux(Port p);
/* an auxiliary port named here is a slot of a cell being wired (or an observation's root) */
static inline bool is_aux(Port p) { return PI(p) >= 1 || PC(p)->kind == O_ROOT; }
static inline void slot_set(Port a, Port x) { sstore(&PC(a)->port[PI(a)], x); }
typedef struct Wire { Port a, b; } Wire;
/* a and b are each a principal port or a variable: a variable is exchanged with the other side, and
 * what the exchange finds (the far side, arrived first) is linked on; two principal ports meet */
static void link_now(Port a, Port b) {
    for (;;) {
        if (is_var(a)) { Var *v = PV(a); Port got = vxchg(v, b); if (!got) return; var_free(v); a = got; continue; }
        if (is_var(b)) { Var *v = PV(b); Port got = vxchg(v, a); if (!got) return; var_free(v); b = got; continue; }
        redex(a, b);
        return;
    }
}
/* wire a to b. Two auxiliary ports get a fresh variable; an auxiliary port takes the other side into
 * its slot; otherwise each side is a principal port or a variable: a variable is exchanged with the
 * other side, and what the exchange finds (the far side, arrived first) is linked on; two principal
 * ports are an active pair */
void link(Port a, Port b) {
    if (!a || !b) {
        if (a && !is_var(a) && is_aux(a)) slot_set(a, 0);
        if (b && !is_var(b) && is_aux(b)) slot_set(b, 0);
        return;
    }
    bool aa = !is_var(a) && is_aux(a), ab = !is_var(b) && is_aux(b);
    if (aa && ab) {
        Var *v = var_new();
        v->key = key_of_aux(a); if (!v->key) v->key = key_of_aux(b);
        slot_set(a, VARP(v)); slot_set(b, VARP(v));
        return;
    }
    if (aa) { slot_set(a, b); return; }
    if (ab) { slot_set(b, a); return; }
    if (net_solo) { link_now(a, b); return; }
    /* Deferred to the end of the rule (publish): a cell's principal port left in a variable is
     * found there by the other side, which may form the pair and publish it on another thread
     * while this rule is still writing the cell's slots. At the rule's end every cell it made is
     * complete */
    Local *l = local();
    if (l->nwires == l->capwires) { l->capwires = l->capwires ? l->capwires * 2 : 64; l->wires = realloc(l->wires, l->capwires * sizeof(Wire)); }
    l->wires[l->nwires++] = (Wire){ a, b };
}
/* publish the active pairs this thread has made */
static void publish(void) {
    Local *l = local();
    for (size_t i = 0; i < l->nwires; i++) link_now(l->wires[i].a, l->wires[i].b);   /* may make pairs */
    l->nwires = 0;
    for (size_t i = 0; i < l->nmade; i++) net_push(l->made[i].op, l->made[i].v);
    l->nmade = 0;
}
/* move whatever is wired to c's port k onto port `to` */
static void mv(Cell *c, int k, Port to) { link(c->port[k], to); }
static Port out_of(Cell *o) { return o->port[o->n - 1]; }


/* ---------------- keys ----------------
 * Interned terms (core.h Key). One table and one mutex serve interning and the memo below; the
 * mutex is taken inside rules and never while a cell lock is taken, so it is innermost. */
static void era(Port prov);
long stat_hits, stat_keys;
/* The tables are sharded by the high bits of the key's hash (the slot within a table is its low
 * bits), one mutex per shard: workers interning or registering unrelated keys do not meet. A key's
 * children may live in other shards, so a freed key is unlinked from its table under the shard's
 * lock and its children released after the unlock (a cascade holding one shard's lock while taking
 * another's could meet the opposite cascade). */
typedef struct Entry Entry;
#define KSH 64
typedef struct { pthread_mutex_t mu; Key **KT; size_t KTcap, KTn; Entry **MT; size_t MTcap, MTn; } Shard;
static Shard SH[KSH] __attribute__((aligned(64)));
_Static_assert(KSH == 64, "shard_of takes the top 6 bits");
static inline Shard *shard_of(uint64_t h) { return &SH[h >> 58]; }
static inline void klock(Shard *s) { if (!net_solo) pthread_mutex_lock(&s->mu); }
static inline void kunlock(Shard *s) { if (!net_solo) pthread_mutex_unlock(&s->mu); }
static void shards_init(void) { for (int i = 0; i < KSH; i++) pthread_mutex_init(&SH[i].mu, NULL); }
static void key_print_d(FILE *o, const Key *k, int d);
static void key_print(FILE *o, const Key *k) { key_print_d(o, k, 0); }
static void key_print_d(FILE *o, const Key *k, int d) {
    static const char *T[] = { "?", "W", "I", "F", "V", "G", "CON", "DATA", "LAM", "PAIR", "PI", "SIGMA", "PATHP", "U", "APP", "FST", "SND", "PAPP", "CASE", "FORCE", "SUB", "SUBST", "DIM", "TERM", "WTY", "UNFOLD" };
    if (d > 12) { fprintf(o, "…"); return; }
    fprintf(o, "(%s", T[k->tag]);
    if (k->tag == K_GLOBAL) fprintf(o, " %s", name_of((long)k->a));
    else if (k->tag == K_CON) fprintf(o, " %s", CON[k->a].name);
    else if (k->tag == K_WORD || k->tag == K_VAR || k->tag == K_DIM) fprintf(o, " %llu", (unsigned long long)k->a);
    else if (k->tag == K_CASE || k->tag == K_LAM) fprintf(o, " @%p", k->p);
    for (uint32_t i = 0; i < k->n; i++) { fputc(' ', o); key_print_d(o, k->c[i], d + 1); }
    fputc(')', o);
}
static bool key_trace;   /* CUBICAL_KEYS: the keys registered, fired and hit, on stderr */
static int a8_off = -1;
static inline bool getenv_a8_off(void) { if (a8_off < 0) a8_off = getenv("CUBICAL_NOA8") != NULL; return a8_off; }
static bool no_memo;     /* CUBICAL_NOMEMO: no producer is pending on its key (every value computed where it is read) */
static __thread Key *scrut_key;   /* set by a case rule for the branch it builds; taken by the branch's frame */
static uint64_t mix(uint64_t h, uint64_t x) { h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); return h * 0x100000001b3ull; }
static uint64_t dnf_hash(const Dnf *d) {
    uint64_t h = 7;
    if (!d) return h;
    for (uint32_t i = 0; i < d->n; i++) { h = mix(h, d->c[i]->n); for (uint32_t k = 0; k < d->c[i]->n; k++) h = mix(h, d->c[i]->l[k]); }
    return h;
}
static uint64_t key_hash(int tag, uint64_t a, const void *p, int n, Key *const *c) {
    uint64_t h = mix(mix(17, (uint64_t)tag), a);
    if (tag == K_IV || tag == K_FACE) h = mix(h, dnf_hash(p));
    else if (tag == K_WORD) { const Big *b = p; if (b) for (uint32_t i = 0; i < b->n; i++) h = mix(h, b->l[i]); }
    else h = mix(h, (uint64_t)(uintptr_t)p);
    for (int i = 0; i < n; i++) h = mix(h, (uint64_t)(uintptr_t)c[i]);
    return h;
}
static bool key_same(const Key *k, int tag, uint64_t a, const void *p, int n, Key *const *c) {
    if (k->tag != tag || k->a != a || k->n != (uint32_t)n) return false;
    for (int i = 0; i < n; i++) if (k->c[i] != c[i]) return false;
    if (tag == K_IV || tag == K_FACE) return dnf_equal(k->p, p);
    if (tag == K_WORD) { WN x = { a, k->p }, y = { a, (Big *)p }; return wn_cmp(&x, &y) == 0; }
    return k->p == p;
}
/* A key of a constructor or a global (no children) is immortal: it is the most shared of all (every
 * numeral's digits name one), so its count would be a line both cores write on most steps; it is
 * never freed, and its holders neither count nor uncount it */
#define KIMMORTAL 0x40000000u
static inline bool key_imm(const Key *k) { return (net_solo ? k->rc : __atomic_load_n(&k->rc, __ATOMIC_RELAXED)) & KIMMORTAL; }
Key *key_retain(Key *k) { if (!key_imm(k)) rc_inc(&k->rc); return k; }
/* unlink a key whose count reached zero from its shard's table (under the shard's lock) */
static void key_unlink(Shard *s, Key *k) {
    Key **KT = s->KT; size_t KTcap = s->KTcap;
    size_t i = k->h & (KTcap - 1);
    while (KT[i] != k) i = (i + 1) & (KTcap - 1);
    /* open addressing: re-insert the run after the hole */
    KT[i] = NULL; s->KTn--;
    for (size_t j = (i + 1) & (KTcap - 1); KT[j]; j = (j + 1) & (KTcap - 1)) {
        Key *m = KT[j]; KT[j] = NULL;
        size_t q = m->h & (KTcap - 1);
        while (KT[q]) q = (q + 1) & (KTcap - 1);
        KT[q] = m;
    }
}
/* the rest of freeing an unlinked key (no lock held: the children's shards are taken one by one).
 * A key is as deep as the term it names (a unary numeral's, one child per digit), so the children
 * are freed from an explicit stack, not by recursion */
static void key_destroy(Key *k) {
    Key *buf[64], **stk = buf; size_t n = 0, cap = 64;
    stk[n++] = k;
    while (n) {
        k = stk[--n];
        if (k->tag == K_IV || k->tag == K_FACE) dnf_release(k->p);
        else if (k->tag == K_WORD) big_release(k->p);
        for (uint32_t i = 0; i < k->n; i++) {
            Key *c = k->c[i];
            if (key_imm(c) || rc_dec(&c->rc) != 0) continue;
            Shard *s = shard_of(c->h);
            klock(s); key_unlink(s, c); kunlock(s);
            if (n == cap) { cap *= 2; Key **m = malloc(cap * sizeof *m); memcpy(m, stk, n * sizeof *m); if (stk != buf) free(stk); stk = m; }
            stk[n++] = c;
        }
        free(k);
    }
    if (stk != buf) free(stk);
}
/* A count that reaches zero is never raised again: interning takes a reference only to a key whose
 * count is not zero (key() below), and every other retain is by a holder of a reference. So the
 * last releaser frees the key; until it does, interning passes over it */
void key_release(Key *k) {
    if (key_imm(k)) return;
    if (rc_dec(&k->rc) == 0) {
        Shard *s = shard_of(k->h);
        klock(s); key_unlink(s, k); kunlock(s);
        key_destroy(k);
    }
}
/* a release under the key's own shard's lock: the key to destroy after the unlock, or NULL */
static Key *key_release_locked(Shard *s, Key *k) {
    if (key_imm(k) || rc_dec(&k->rc) != 0) return NULL;
    key_unlink(s, k);
    return k;
}
/* the interned key (a reference the caller owns); children are borrowed (retained here); a Dnf or
 * Big payload is retained, others held by pointer */
static Key *key(int tag, uint64_t a, void *p, int n, Key *const *c) {
    for (int i = 0; i < n; i++) if (!c[i]) return NULL;
    uint64_t h = key_hash(tag, a, p, n, c);
    Shard *s = shard_of(h);
    klock(s);
    if (s->KTn * 2 >= s->KTcap) {
        size_t nc = s->KTcap ? s->KTcap * 2 : 1024;
        Key **nt = calloc(nc, sizeof(Key *));
        for (size_t i = 0; i < s->KTcap; i++) if (s->KT[i]) { size_t q = s->KT[i]->h & (nc - 1); while (nt[q]) q = (q + 1) & (nc - 1); nt[q] = s->KT[i]; }
        free(s->KT); s->KT = nt; s->KTcap = nc;
    }
    Key **KT = s->KT; size_t KTcap = s->KTcap;
    size_t i = h & (KTcap - 1);
    for (; KT[i]; i = (i + 1) & (KTcap - 1))
        if (KT[i]->h == h && key_same(KT[i], tag, a, p, n, c)) {
            /* a reference to it, unless its last reference is gone (its releaser frees it) */
            Key *k = KT[i];
            uint32_t r = __atomic_load_n(&k->rc, __ATOMIC_ACQUIRE);
            if (r & KIMMORTAL) { kunlock(s); return k; }
            if (net_solo) { if (r) k->rc = r + 1; }
            else while (r && !__atomic_compare_exchange_n(&k->rc, &r, r + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) { }
            if (r) { kunlock(s); return k; }
        }
    Key *k = malloc(sizeof(Key) + (size_t)n * sizeof(Key *));
    k->rc = n == 0 && (tag == K_CON || tag == K_GLOBAL) ? KIMMORTAL : 1; k->n = (uint32_t)n; k->h = h; k->tag = (uint16_t)tag; k->a = a;
    k->p = (tag == K_IV || tag == K_FACE) ? dnf_retain(p) : tag == K_WORD ? big_retain(p) : p;
    for (int j = 0; j < n; j++) k->c[j] = key_retain(c[j]);
    KT[i] = k; s->KTn++;
    kunlock(s);
    return k;
}
static Key *key0(int tag, uint64_t a, void *p) { return key(tag, a, p, 0, NULL); }
static Key *key1(int tag, uint64_t a, void *p, Key *x) { return key(tag, a, p, 1, &x); }
static Key *key2(int tag, uint64_t a, void *p, Key *x, Key *y) { Key *c[2] = { x, y }; return key(tag, a, p, 2, c); }
Key *key_global(long g) { return key0(K_GLOBAL, (uint64_t)g, NULL); }
Key *key_con0(int c) { return key0(K_CON, (uint64_t)c, NULL); }
/* a substitution as a key: its (dimension, interval) pairs, in order */
static Key *subst_key(const Subst *s) {
    int n = (int)(2 * s->n);
    Key **c = calloc((size_t)n + 1, sizeof(Key *)); int m = 0;
    for (uint32_t i = 0; i < s->n; i++) { c[m++] = key0(K_DIM, s->d[i], NULL); c[m++] = key0(K_IV, 0, s->v[i]); }
    Key *r = key(K_SUBST, 0, NULL, m, c);
    for (int i = 0; i < m; i++) if (c[i]) key_release(c[i]);
    free(c);
    return r;
}
/* the key of a cell as it stands: its term, under the substitution a thunk or reference carries (a
 * reference carries its box's key, or the key of the box it was itself held in). The caller owns
 * the result */
static Key *eff_key(Cell *c) {
    /* an interval or a face is named by what it holds: rules change these in place (negation,
     * meet and join, substitution), and a key kept from before would name another formula */
    if (c->kind == V_IV || c->kind == V_FACE) return c->p ? key0(c->kind == V_IV ? K_IV : K_FACE, 0, c->p) : NULL;
    Key *k = c->key;
    if (!k) return NULL;
    if ((c->kind == V_THUNK || c->kind == V_REF) && c->a) {
        Key *sk = subst_key((Subst *)(uintptr_t)c->a);
        if (!sk) return NULL;
        Key *r = key2(K_SUB, 0, NULL, k, sk); key_release(sk);
        return r;
    }
    return key_retain(k);
}
/* the key of the value named k under the substitution s (the caller owns it), or NULL */
static Key *key_under(Key *k, const Subst *s) {
    if (!s || !s->n) return key_retain(k);
    Key *sk = subst_key(s);
    if (!sk) return NULL;
    Key *r = key2(K_SUB, 0, NULL, k, sk); key_release(sk);
    return r;
}
/* the key the output port of a keyed operation carries (a duplicator's outputs carry their input's
 * key); the caller owns the result */
static Key *key_of_aux(Port p) {
    Cell *c = PC(p);
    if (!c->key) return NULL;
    if (c->kind == O_DUP || c->kind == O_FWD || c->kind == O_SHARE) return (PI(p) == 1 || PI(p) == 2) ? key_retain(c->key) : NULL;
    return PI(p) == c->n - 1 ? key_retain(c->key) : NULL;
}
/* the key of what a port provides: a value or reference at its principal port, the output of a
 * keyed operation, or a wire made from one (the variable keeps it). A variable is read by a holder
 * of one of its sides, so it is live. The caller owns the result */
static Key *key_of_port(Port p) {
    if (!p) return NULL;
    if (is_var(p)) { Key *k = PV(p)->key; return k ? key_retain(k) : NULL; }
    Cell *c = PC(p);
    if (PI(p) == 0) return is_value(c->kind) ? eff_key(c) : NULL;
    return key_of_aux(p);
}

/* ---------------- memo: one value per live key ----------------
 * A key names a term over the values it is built on: a held value (a box) is named by what was
 * put in it, and a value copied out of a box, or rebuilt from the fields a case took from it,
 * carries the box's key, so one value reached by any route has one name. A cell that will
 * produce the value of a term (a thunk, a saturated call, an application of a known λ, a case) is
 * pending on its key from its creation. When one fires while another is pending, the value goes
 * into a box and both read it (the second waits on the wire for it, uncounted, as any read of a
 * value not yet there); the entry and its box live exactly while a producer is pending or a reader
 * holds the box (archive/HANDOFF.md, v13, 16 Oct 2026, sha256 04025bd9f961…, §36 “Ownership, cache retention, and teardown”: retention is reachability). When the firing producer is the only
 * one pending, it builds directly: nothing holds the value. */
struct Entry { Key *k; Box *box; long pending; bool hold; struct Entry *next; };   /* hold: the entry holds one reference on its box while a producer is pending */
static Entry *memo_find(Shard *s, Key *k, bool make) {   /* under the shard's lock */
    if (s->MTcap == 0) { s->MTcap = 1024; s->MT = calloc(s->MTcap, sizeof(Entry *)); }
    size_t i = k->h & (s->MTcap - 1);
    for (Entry *e = s->MT[i]; e; e = e->next) if (e->k == k) return e;
    if (!make) return NULL;
    if (s->MTn * 2 >= s->MTcap) {
        size_t nc = s->MTcap * 2; Entry **nt = calloc(nc, sizeof(Entry *));
        for (size_t j = 0; j < s->MTcap; j++) for (Entry *e = s->MT[j], *nx; e; e = nx) { nx = e->next; size_t q = e->k->h & (nc - 1); e->next = nt[q]; nt[q] = e; }
        free(s->MT); s->MT = nt; s->MTcap = nc; i = k->h & (s->MTcap - 1);
    }
    Entry *e = calloc(1, sizeof(Entry)); e->k = key_retain(k); e->next = s->MT[i]; s->MT[i] = e; s->MTn++;
    return e;
}
/* under the shard's lock: no producer pending, no box. Returns the entry's key if this was its last
 * reference, to destroy after the unlock */
static Key *memo_drop(Shard *s, Entry *e) {
    size_t i = e->k->h & (s->MTcap - 1);
    for (Entry **pp = &s->MT[i]; *pp; pp = &(*pp)->next) if (*pp == e) { *pp = e->next; break; }
    s->MTn--; Key *k = key_release_locked(s, e->k); free(e);
    return k;
}
static void box_release(Box *b);
/* Holding a box through its memo entry races with its last holder releasing it: the count reaches
 * zero in one thread while another, under the shard lock, still sees the entry's box. The releaser
 * claims the zero (rc 0 → -1) before touching the entry; an increment through the entry refuses a
 * claimed box and detaches it from the entry itself, so no reference to a dying box is ever made */
static bool box_try_hold(Box *b) {
    if (net_solo) { if (b->rc < 0) return false; b->rc++; return true; }
    long v = __atomic_load_n(&b->rc, __ATOMIC_RELAXED);
    while (v >= 0) if (__atomic_compare_exchange_n(&b->rc, &v, v + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) return true;
    return false;
}
/* under the shard lock: the entry's box, held once more; or NULL when it is dying (and detached) */
static Box *entry_hold_box(Entry *e) {
    Box *b = e->box;
    if (!b) return NULL;
    if (box_try_hold(b)) return b;
    Entry *was = net_solo ? b->e : __atomic_exchange_n(&b->e, NULL, __ATOMIC_ACQ_REL);
    if (net_solo) b->e = NULL;
    /* the releaser took the entry first: it reads the entry before it locks, then detaches the box
     * and drops the entry itself, so the entry stays (with its dying box) until then */
    if (!was) return NULL;
    e->box = NULL; e->hold = false;
    return NULL;
}
/* a producer cell with a key: pending on it */
static void memo_register(Cell *c) {
    if (no_memo) return;
    Key *k = eff_key(c);
    if (!k) return;
    if (key_trace) { fprintf(stderr, "register %d ", c->kind); key_print(stderr, k); fputc('\n', stderr); }
    Shard *s = shard_of(k->h);
    klock(s);
    Entry *e = memo_find(s, k, true); e->pending++; c->e = e;
    if (e->box && !e->hold && entry_hold_box(e)) e->hold = true;
    kunlock(s);
    key_release(k);
}
static void memo_leave(Cell *c) {   /* a pending producer consumed without producing (erased) or re-keyed */
    Entry *e = c->e; c->e = NULL;
    Shard *s = shard_of(e->k->h);
    Key *gone = NULL;
    klock(s);
    if (--e->pending == 0) {
        if (e->box) { Box *b = e->box; bool h = e->hold; e->hold = false; kunlock(s); if (h) box_release(b); return; }
        gone = memo_drop(s, e);
    }
    kunlock(s);
    if (gone) key_destroy(gone);
}
/* a pending producer fires. Returns the port to give its consumer when the value is (or will be)
 * shared, and *out = the box to build into (NULL when the value is already there or being built);
 * returns 0 when the producer builds directly */
static Port memo_begin(Cell *c, Box **out, bool builder) {
    *out = NULL;
    Entry *e = c->e;
    if (!e) return 0;
    c->e = NULL;
    Shard *s = shard_of(e->k->h);
    klock(s);
    e->pending--;
    if (key_trace) { fprintf(stderr, "fire %d %s pending=%ld box=%d ", c->kind, builder ? "builder" : "thunk", e->pending, e->box != NULL); key_print(stderr, e->k); fputc('\n', stderr); }
    Box *b = entry_hold_box(e);   /* NULL: none, or dying (then detached: build anew) */
    if (b) {
        /* the value, there or being built: read it */
        bool last = e->pending == 0 && e->hold;
        if (last) e->hold = false;
        kunlock(s);
        local()->hits++;
        Cell *r = cell(V_REF, 1); r->p = b; r->key = key_retain(b->key);
        if (last) box_release(b);   /* the entry's own hold */
        return P(r, 0);
    }
    if (!builder || e->pending == 0) {
        /* a thunk: its body's producer, which has the same key, decides (it registers as pending);
         * a builder nobody else waits for builds directly */
        Key *gone = e->pending == 0 && !e->box ? memo_drop(s, e) : NULL;   /* (a dying box's releaser drops it) */
        kunlock(s);
        if (gone) key_destroy(gone);
        return 0;
    }
    /* others wait for this value: it is held once and read by reference. Its root holds the wire
     * the value will be linked into (memo_end); a reader finding nothing there yet waits on it */
    b = malloc(sizeof(Box)); b->rc = 2; b->root = cell(O_ROOT, 1); b->key = key_retain(e->k); b->e = e; b->fd = NULL; b->fdtries = 0;
    { Var *t = var_new(); t->key = key_retain(e->k); b->in = VARP(t); b->root->port[0] = VARP(t); }
    e->box = b; e->hold = true;
    kunlock(s);
    local()->keys++;
    *out = b;
    Cell *r = cell(V_REF, 1); r->p = b; r->key = key_retain(b->key);
    return P(r, 0);
}
/* CUBICAL_KEYS: the entries still live at the end (what holds values across readings) */
void memo_dump(void) {
    if (!key_trace) return;
    size_t en = 0, kn = 0;
    for (int j = 0; j < KSH; j++) {
        Shard *s = &SH[j];
        klock(s);
        for (size_t i = 0; i < s->MTcap; i++) for (Entry *e = s->MT[i]; e; e = e->next) {
            fprintf(stderr, "entry pending=%ld box=%d ", e->pending, e->box != NULL);
            key_print(stderr, e->k); fputc('\n', stderr);
        }
        en += s->MTn; kn += s->KTn;
        kunlock(s);
    }
    fprintf(stderr, "%zu entries, %zu keys\n", en, kn);
}
/* the box of a memo entry is gone: the entry no longer has a value */
static void memo_box_gone(Box *b) {
    Entry *e = net_solo ? b->e : __atomic_exchange_n(&b->e, NULL, __ATOMIC_ACQ_REL);
    if (net_solo) b->e = NULL;
    if (!e) return;   /* already detached by a reader that found the box dying */
    Shard *s = shard_of(e->k->h);
    Key *gone = NULL;
    klock(s);
    if (e->box == b) { e->box = NULL; e->hold = false; if (e->pending == 0) gone = memo_drop(s, e); }
    kunlock(s);
    if (gone) key_destroy(gone);
}
/* ---------------- small constructors ---------------- */
/* profile: where each duplicator and eraser comes from (a record's definition, or a rule) */
static __thread long dup_origin;
long prof_odup[65536], prof_oera[65536], prof_rdup[256], prof_rera[256];
long prof_pair[4][256];   /* CUBICAL_PROFILE: SUBST, ERA, DUP, PAPPP steps by the value kind met */
/* a value held once: references copy the box; the last one erases the value */
static void box_hold(Box *b);
static Key *key_of_port(Port p);
static Box *box_new(Port val) {
    Box *b = malloc(sizeof(Box)); b->rc = 1; b->root = cell(O_ROOT, 1); b->e = NULL; b->in = 0; b->fd = NULL; b->fdtries = 0;
    b->key = key_of_port(val);
    link(val, P(b->root, 0));
    return b;
}
static Cell *ref_to(Box *b) { Cell *r = cell(V_REF, 1); r->p = b; r->key = b->key ? key_retain(b->key) : NULL; return r; }
/* reading a stored value (held by root): its top cell is copied, each part a reference to the
 * stored part (wrapped in a box when first read), so a part is copied only when an operation
 * reads it, one level per interaction. A reader of a value not yet there (or held unbuilt) waits
 * for it (O_FWD) and gets the same copy when it arrives. No lock: the root's slot and the stored
 * value's slots are changed by compare-and-swap, by the one reader that sees them unchanged;
 * another sees the change and reads what it finds */
static void box_hold(Box *b) { rcl_inc(&b->rc); }
Port thunk_build(Cell *v);
Port unbuilt_build(Cell *v);
static bool unbuilt(int k);
static Port sub(Port prov, Subst *s);
static Port sub_read(Port prov, Subst *s);
static Dim atom_find(const Cell *v);
static bool is_atom_dim(Dim d);
static Dnf *atom_clause(const Subst *s);
extern long atom_res_global, atom_res0_global;
static Port face_cell(Dnf *f);
static bool is_neutral(int k);
static Port copy_top(Key *as, Cell *v);
/* as: the key of what is held (the box's; a definition's name): a copy carries it */
Port lazy_copy(Key *as, Cell *root) {
    for (;;) {
        Port p = sload(&root->port[0]);
        Cell *v = cell_at(p);
        if (v && is_value(v->kind) && !unbuilt(v->kind)) return copy_top(as, v);
        /* the value is not there yet, or is held unbuilt: the reader waits on the wire for it
         * (O_FWD), and is served the same one-level copy when it arrives; an unbuilt value is built
         * by that read, in place, once. Waiting costs no step, so what a read costs does not depend
         * on whether the value had arrived */
        Cell *d = cell(O_FWD, 3); d->key = as ? key_retain(as) : NULL;
        Var *t = var_new(); if (d->key) t->key = key_retain(d->key);
        d->port[1] = VARP(t);
        if (!claim(&root->port[0], p, VARP(t))) { var_free(t); cell_free(d); continue; }
        link(p, P(d, 0));
        return P(d, 2);
    }
}
/* one level of a stored value: its top cell copied, each part with parts of its own a reference to a
 * box around the stored part (the stored value's slot then refers to that box too). The copy is
 * named by what is held (as) */
static Port copy_top(Key *as, Cell *v) {
    Cell *c = clone(v);
    if (as && c->key != as) { if (c->key) key_release(c->key); c->key = key_retain(as); }
    for (int k = 1; k < v->n; k++) for (;;) {
        Port q = sload(&v->port[k]);
        Port r = peek(q);
        Cell *qc = r ? PC(r) : NULL;
        if (!q) { break; }
        if (qc && is_value(qc->kind) && qc->n == 1 && !unbuilt(qc->kind)) {
            /* a part with no parts of its own (a reference, a word, a constant): itself */
            Cell *q2 = clone(qc);
            c->port[k] = P(q2, 0);
            break;
        }
        if (!qc || !is_value(qc->kind)) {
            /* a part not there yet: shared as it will be when it arrives (O_SHARE) — copied if it
             * has no parts, else held once with a reference on each side — so the copy does not
             * depend on whether it had arrived. The stored value's slot is pointed at the sharer's
             * output first, then the sharer is put on the part's wire */
            Cell *d = cell(O_SHARE, 3); d->key = key_of_port(q);
            Var *t = var_new(); if (d->key) t->key = key_retain(d->key);
            d->port[1] = VARP(t);
            if (!claim(&v->port[k], q, VARP(t))) { var_free(t); cell_free(d); continue; }
            link(q, P(d, 0));
            link(P(d, 2), P(c, k));
            break;
        }
        /* a part with parts: held once in a box, a reference on each side */
        Box *b = malloc(sizeof(Box)); b->rc = 2; b->root = cell(O_ROOT, 1); b->e = NULL; b->in = 0; b->fd = NULL; b->fdtries = 0;
        b->key = eff_key(qc);
        b->root->port[0] = r;
        Cell *r1 = ref_to(b), *r2 = ref_to(b);
        if (!claim(&v->port[k], q, P(r1, 0))) {
            /* another reader got there first: it holds the part now */
            r1->p = NULL; r2->p = NULL; cell_free(r1); cell_free(r2);
            cell_free(b->root); if (b->key) key_release(b->key); free(b);
            continue;
        }
        if (is_var(q)) take(q);   /* the wire delivered the part; now the box holds it */
        c->port[k] = P(r2, 0);
        break;
    }
    return P(c, 0);
}

/* a subterm held unbuilt, built (its environment moved into the body) */
/* an unbuilt subterm (a thunk, an induction hypothesis, a word's hypothesis), built */
static bool unbuilt(int k) { return k == V_THUNK || k == V_SUSP || k == V_WSUSP || k == V_DEFER; }
static Port case_of_susp(Cell *v);
static Port welim_of_wsusp(Cell *v);
/* a deferred operation, run: its inputs moved onto the operation (the principal last, so the pair
 * forms with every port wired); the cell is consumed */
/* Call by need (CBN): an input the operation always reads (its principal; defer_reads) that is
 * itself a deferred operation held by this one alone (wired straight to it) is run with it, as
 * building the operation strict would have built it; no interaction is skipped, none happens. The
 * operation takes the deferred cell's key and its pending memo entry */
static bool defer_reads(const Cell *d, int k);
static Port defer_build(Cell *d) {
    int nin = d->n - 1;
    Cell *c = cell((int)d->ty, nin + 1); c->x = d->x; c->a = d->a; c->p = d->p; d->p = NULL;
    c->key = d->key; d->key = NULL; c->e = d->e; d->e = NULL;
    Port in[nin > 0 ? nin : 1];
    for (int k = 0; k < nin; k++) {
        Port q = d->port[k + 1];
        if (q && !is_var(q) && PI(q) == 0 && PC(q)->kind == V_DEFER && defer_reads(d, k)) q = defer_build(PC(q));
        in[k] = q;
    }
    for (int k = 1; k < nin; k++) link(in[k], P(c, k));
    link(in[0], P(c, 0));
    cell_free(d);
    return P(c, nin);
}
/* an operation of kind op with nin inputs, deferred: its inputs go to ports 1..nin */
static Cell *defer_cell(int op, int nin) { Cell *d = cell(V_DEFER, nin + 1); d->ty = (uint32_t)op; return d; }
/* does the deferred operation always read its input k (its port k), in every outcome but a stuck
 * one? As syn_reads, from the cell: its principal; a plain path application's path; a meet or
 * join's second operand; a call's parameters its definition always reads */
static bool defer_reads(const Cell *d, int k) {
    if (k == 0) return true;
    switch ((int)d->ty) {
    case O_PAPPR: return !d->x && k == 1;
    case O_BIN: return k == 1;
    case O_TRANSD1: return k == d->n - 2;
    case O_WEQ: case O_WLT: case O_WADD: case O_WMAX: case O_WSUB: case O_WMUL: case O_WDIV: case O_WMOD: case O_WAND: case O_WOR: case O_WXOR: case O_WSHL: case O_WSHR: return k == 1;   /* as syn_reads */
    case O_CALL: {
        Port g = d->port[1];
        if (!g || is_var(g) || PI(g) != 0 || PC(g)->kind != V_GLOBAL) return false;
        extern uint64_t global_strict(long g); int q = k - 1;
        return q < 64 && (global_strict((long)PC(g)->a) >> q & 1);
    }
    default: return false;
    }
}
Port unbuilt_build(Cell *v) {
    if (v->kind == V_DEFER) return defer_build(v);
    if (v->kind == V_SUSP) return case_of_susp(v);
    if (v->kind == V_WSUSP) return welim_of_wsusp(v);
    return thunk_build(v);
}
static Port memo_begin(Cell *c, Box **out, bool builder);
/* a pending producer's result: built into the memo's box when the value is shared */
static Port memo_end(Port shared, Box *box, Port built) {
    if (!shared) return built;
    if (box) link(built, box->in);
    return shared;
}
Port thunk_build(Cell *v) {
    if (prof_on) ADD(prof_count[((Record *)v->p)->where], 1);
    Box *bx; Port sh = memo_begin(v, &bx, false);
    if (sh && !bx) { for (int k = 1; k < v->n; k++) era(v->port[k]); cell_free(v); return sh; }
    Port r = record_build(v->p, 0, &v->port[1]);
    /* a substitution that reached it applies to its value */
    if (v->a) r = sub(r, (Subst *)(uintptr_t)v->a);
    cell_free(v);
    return memo_end(sh, bx, r);
}
static Port word_of(uint64_t n) { Cell *c = cell(V_WORD, 1); c->a = n; return P(c, 0); }
/* the number a word cell holds (a copy), and a word cell holding a number (taking it) */
static WN cell_wn(const Cell *c) { WN r = { c->a, c->p }; return wn_copy(&r); }
static Port word_wn(WN k) { Cell *c = cell(V_WORD, 1); c->a = k.s; c->p = k.b; return P(c, 0); }
Port word_port(WN k) { return word_wn(k); }
/* a reference reads its value under the substitution it carries (a=Subst or 0): the top cell
 * copied and substituted, each part a reference carrying the substitution on */
/* With a pending substitution the copy of what the box holds is named by the box (the content,
 * which may have free dimensions the substitution closes); the reference's own key names the
 * substituted value, not the content */
/* CUBICAL_PROFILE: box reads, and how many were of a (box, substitution) pair read before */
long prof_reads, prof_reads_rep, prof_reads_sub, prof_reads_sub_rep, prof_reads_sub_boxes;
static uint64_t *rr_tab; static size_t rr_cap, rr_n;
static uint64_t subst_fp(const Subst *s) {
    uint64_t h = 1469598103934665603ULL;
    for (uint32_t k = 0; k < s->n; k++) {
        uint64_t e = (uint64_t)s->d[k] * 0x9e3779b97f4a7c15ULL;
        const Dnf *v = s->v[k]; uint64_t dv = 0;
        if (v) for (uint32_t c = 0; c < v->n; c++) { uint64_t cl = 0x51ed27; for (uint32_t q = 0; q < v->c[c]->n; q++) cl += v->c[c]->l[q] * 0xff51afd7ed558ccdULL ^ (v->c[c]->l[q] >> 7); dv += cl * 0xc4ceb9fe1a85ec53ULL; }
        h += e ^ (dv + 0x632be5ab);
    }
    return h;
}
static bool rr_seen(uint64_t h) {
    if (!h) h = 1;
    if (rr_n * 2 >= rr_cap) {
        size_t nc = rr_cap ? rr_cap * 2 : 1 << 16; uint64_t *nt = calloc(nc, sizeof *nt);
        for (size_t i = 0; i < rr_cap; i++) if (rr_tab[i]) { size_t j = rr_tab[i] & (nc - 1); while (nt[j]) j = (j + 1) & (nc - 1); nt[j] = rr_tab[i]; }
        free(rr_tab); rr_tab = nt; rr_cap = nc;
    }
    size_t j = h & (rr_cap - 1);
    while (rr_tab[j]) { if (rr_tab[j] == h) return true; j = (j + 1) & (rr_cap - 1); }
    rr_tab[j] = h; rr_n++; return false;
}
/* A8 (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): the free dimensions of a box's value, found once the value is built all through (a walk of
 * the value, reading nested boxes' sets), and kept on the box. A substitution of other dimensions
 * does not change the value, so a reference's pending substitution is cut down to the box's
 * dimensions (rule_subst on V_REF, ref_read): reads of one box at the dimensions it does not
 * mention are then reads of the box itself, named by its key, and the memo shares them. Whether the
 * set is known when a read happens depends on what has been built, so this runs only with the memo
 * (whose sharing depends on that already); without it every step is as before. Answers are the
 * same either way: the substitution is restricted to a superset of the value's free dimensions */
typedef struct { Dim d[256]; int n; bool taint, inexact; } FdSet;
static void fd_add(FdSet *x, Dim d) { for (int i = 0; i < x->n; i++) if (x->d[i] == d) return; if (x->n < 256) x->d[x->n++] = d; else x->n = 257; }
static void fd_dnf(FdSet *x, const Dnf *v) { if (v) for (uint32_t c = 0; c < v->n; c++) for (uint32_t q = 0; q < v->c[c]->n; q++) fd_add(x, (Dim)(v->c[c]->l[q] >> 1)); }
static void fd_merge(FdSet *x, const FdSet *y) { for (int i = 0; i < y->n && x->n <= 256; i++) fd_add(x, y->d[i]); if (y->n > 256) x->n = 257; x->taint |= y->taint; x->inexact |= y->inexact; }
static __thread long fd_budget;
static __thread int fd_why;   /* why a set is unknown (profile): 1 root not there, 2 an unbuilt part, 3 a part not there, 4 budget, 5 tries, 6 a pending part may make a dimension */
long prof_fd_why[7], prof_fd_pending;
static int a8b_off = -1;
static inline bool a8b_on(void) { if (a8b_off < 0) a8b_off = getenv("CUBICAL_NOA8B") != NULL; return net_solo && !a8b_off; }
static FdInfo *box_fd(Box *b);
static bool fd_port(Port p, FdSet *x, int depth);
/* may this value's record (or table, or global) make a dimension when built or applied? Asked only
 * on one worker (the analysis' caches are not shared); elsewhere taken to (conservative) */
static bool fd_taint_of(const Cell *v) {
    if (!a8b_on()) return true;
    switch (v->kind) {
    case V_LAM: case V_THUNK: return record_mkdim((Record *)v->p);
    case V_SUSP: case V_CASEF: return casetab_mkdim((const CaseTab *)v->p);
    case V_GLOBAL: return global_mkdim((long)v->a);
    case V_REC: return true;
    case V_DEFER:
        /* as fd_pending refuses a pending operation */
        switch ((int)v->ty) {
        case O_CASE: return casetab_mkdim((const CaseTab *)v->p);
        case O_MKDIM: case O_MKFRESH: case O_FRESHT: case O_GLOB: case O_ERA: return true;
        case O_NATIVE: return global_mkdim((long)v->a);
        default: return false;
        }
    default: return false;
    }
}
/* A8b (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”, its part A8b): a pending operation's result mentions only its inputs' dimensions (and those its payload
 * names), unless something it may run can make one: O_MKDIM itself, the checker's tables, a
 * record, table or global among the inputs that reaches %dim (record_mkdim). Binders (O_MKPLAM,
 * O_FORALL) remove their dimension. A bound, not the set: the value is still being built */
static bool fd_pending(Cell *o, int arrive, FdSet *x, int depth) {
    if (o->kind == O_MKDIM || o->kind == O_MKFRESH || o->kind == O_FRESHT || o->kind == O_GLOB || o->kind == O_ERA) { fd_why = 6; return false; }
    FdSet *y = malloc(sizeof *y); y->n = 0; y->taint = false; y->inexact = true;
    bool ok = true;
    if (o->kind == O_DUP) ok = arrive != 0 && fd_port(o->port[0], y, depth + 1);
    else for (int k = 0; k < o->n && ok; k++) if (k != arrive) ok = fd_port(o->port[k], y, depth + 1);
    if (ok) {
        if (owns_dnf(o->kind)) fd_dnf(y, o->p);
        if (o->kind == O_SUBST && o->p) { const Subst *s = o->p; for (uint32_t k = 0; k < s->n; k++) fd_dnf(y, s->v[k]); }
        if (o->kind == O_CASE && casetab_mkdim((const CaseTab *)o->p)) y->taint = true;
        if (o->kind == O_NATIVE && global_mkdim((long)o->a)) y->taint = true;
        if (y->taint) { fd_why = 6; ok = false; }
    }
    if (ok && y->n <= 256) {
        if (o->kind == O_MKPLAM || o->kind == O_FORALL) { Dim i = (Dim)o->a; for (int q = 0; q < y->n; q++) if (y->d[q] == i) { y->d[q] = y->d[--y->n]; break; } }
        fd_merge(x, y);
    } else ok = false;
    free(y);
    if (ok && prof_on) prof_fd_pending++;
    return ok && x->n <= 256;
}
static bool fd_cell(Cell *v, FdSet *x, int depth) {
    if (--fd_budget < 0 || depth > 200 || x->n > 256) { fd_why = 4; return false; }
    if (!is_value(v->kind)) { fd_why = 3; return false; }
    if (unbuilt(v->kind)) {
        /* an unbuilt subterm: its environment's dimensions bound what building it gives (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”, its part A8b) */
        if (!a8b_on()) { fd_why = 2; return false; }
        if (fd_taint_of(v)) { fd_why = 6; return false; }
        x->inexact = true;
        /* a thunk's pending substitution (applied when it is built) brings its values' dimensions */
        if (v->kind == V_THUNK && v->a) { const Subst *s = (const Subst *)(uintptr_t)v->a; for (uint32_t k = 0; k < s->n; k++) fd_dnf(x, s->v[k]); }
        if (v->kind == V_DEFER) {
            /* a deferred operation: its inputs' dimensions and its payload's, less a binder's own;
             * refused when running it may make a dimension (a record, table or global among its
             * inputs reaching %dim), as a pending operation is (fd_pending) */
            FdSet *y = malloc(sizeof *y); y->n = 0; y->taint = false; y->inexact = true;
            bool ok = true;
            for (int k = 1; k < v->n && ok; k++) ok = fd_port(v->port[k], y, depth + 1);
            if (ok && y->taint) { fd_why = 6; ok = false; }
            if (ok && owns_dnf((int)v->ty)) fd_dnf(y, v->p);
            if (ok && y->n <= 256) {
                if (v->ty == O_MKPLAM || v->ty == O_FORALL) for (int q = 0; q < y->n; q++) if (y->d[q] == (Dim)v->a) { y->d[q] = y->d[--y->n]; break; }
                fd_merge(x, y);
            } else ok = false;
            free(y);
            return ok && x->n <= 256;
        }
        for (int k = 1; k < v->n; k++) if (!fd_port(v->port[k], x, depth + 1)) return false;
        return x->n <= 256;
    }
    if (fd_taint_of(v)) x->taint = true;
    if (v->kind == V_REF) {
        FdInfo *fi = v->p ? box_fd(v->p) : NULL;
        if (!fi) { if (!fd_why) fd_why = 1; return false; }
        if (fi->taint) x->taint = true;
        if (fi->inexact) x->inexact = true;
        Subst *s = (Subst *)(uintptr_t)v->a;
        for (uint32_t i = 0; i < fi->n; i++) {
            bool hit = false;
            if (s) for (uint32_t k = 0; k < s->n; k++) if (s->d[k] == fi->d[i]) { fd_dnf(x, s->v[k]); hit = true; break; }
            if (!hit) fd_add(x, fi->d[i]);
        }
        return x->n <= 256;
    }
    if (owns_dnf(v->kind)) fd_dnf(x, v->p);
    if (is_neutral(v->kind) && v->key) { Dim d = atom_find(v); if (d) fd_add(x, d); }
    if (v->kind == V_PLAM) {
        FdSet *y = malloc(sizeof *y); y->n = 0; y->taint = false; y->inexact = false;
        bool ok = true;
        for (int k = 1; k < v->n && ok; k++) if (!fd_port(v->port[k], y, depth + 1)) ok = false;
        if (ok && y->n <= 256) { for (int i = 0; i < y->n; i++) if (y->d[i] == (Dim)v->a) { y->d[i] = y->d[--y->n]; break; } fd_merge(x, y); }
        else ok = false;
        free(y);
        return ok && x->n <= 256;
    }
    for (int k = 1; k < v->n; k++) if (!fd_port(v->port[k], x, depth + 1)) return false;
    return x->n <= 256;
}
long prof_fd_var, prof_fd_prin[512];
static bool fd_port(Port p, FdSet *x, int depth) {
    Port q = peek(sload(&p));
    if (!q || is_var(q)) { fd_why = 3; if (prof_on) prof_fd_var++; return false; }
    Cell *c = PC(q);
    if (PI(q) != 0 || !is_value(c->kind)) {
        /* the result of an operation still pending, or its input */
        if (!a8b_on() || PI(q) == 0) { fd_why = 3; if (prof_on && c->kind < 512) prof_fd_prin[c->kind]++; return false; }
        if (--fd_budget < 0 || depth > 200) { fd_why = 4; return false; }
        return fd_pending(c, PI(q), x, depth);
    }
    return fd_cell(c, x, depth);
}
/* the box's set, or NULL while unknown (a part not yet built); tried a few times. A bound from
 * pending parts (inexact) is kept until a later try finds the exact set */
static FdInfo *box_fd(Box *b) {
    FdInfo *fi = net_solo ? b->fd : __atomic_load_n(&b->fd, __ATOMIC_ACQUIRE);
    if (fi && !fi->inexact) return fi;
    /* The walk reads a held value's parts non-atomically; with workers another reader
       may claim, free and reuse them under it (the TSan race at cell()'s memset). */
    if (!net_solo) { if (!fi) fd_why = 1; return fi; }
    if (__atomic_load_n(&b->fdtries, __ATOMIC_RELAXED) >= 8) { if (!fi) fd_why = 5; return fi; }
    __atomic_fetch_add(&b->fdtries, 1, __ATOMIC_RELAXED);
    Port q = sload(&b->root->port[0]);
    if (!q || is_var(q)) { q = peek(q); if (!q || is_var(q)) { fd_why = 1; return fi; } }
    if (PI(q) != 0 && !a8b_on()) { fd_why = 1; return fi; }
    FdSet *y = malloc(sizeof *y); y->n = 0; y->taint = false; y->inexact = false;
    long saved = fd_budget; fd_budget = 4000;
    bool ok = PI(q) != 0 ? fd_pending(PC(q), PI(q), y, 0) : fd_cell(PC(q), y, 0);
    fd_budget = saved;
    if (ok && y->n <= 256) {
        FdInfo *nf = malloc(sizeof(FdInfo) + (size_t)y->n * sizeof(Dim)); nf->n = (uint32_t)y->n;
        nf->taint = y->taint; nf->inexact = y->inexact;
        memcpy(nf->d, y->d, (size_t)y->n * sizeof(Dim));
        if (net_solo) { b->fd = nf; fi = nf; }   /* an earlier bound is not freed: a walk in progress may hold it */
        else {
            FdInfo *none = NULL;
            if (!__atomic_compare_exchange_n(&b->fd, &none, nf, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) { free(nf); fi = none; }
            else fi = nf;
        }
    }
    free(y);
    return fi;
}
/* s cut down to the dimensions of fi (NULL when none is left); borrows s, returns owned or NULL */
static Subst *subst_to_fd(Subst *s, const FdInfo *fi) {
    Subst *t = subst_retain(s);
    for (uint32_t k = 0; k < s->n; k++) {
        bool in = false;
        for (uint32_t i = 0; i < fi->n && !in; i++) in = fi->d[i] == s->d[k];
        /* an atom is kept: a neutral built later from what is held may be read as it */
        if (!in && is_atom_dim(s->d[k])) in = true;
        if (!in) { Subst *t2 = subst_without(t, s->d[k]); subst_release(t); t = t2; }
    }
    if (!t->n) { subst_release(t); return NULL; }
    return t;
}
long prof_fd_cut, prof_fd_dropped;
static Port ref_read(Cell *r) {
    Box *b = r->p;
    if (prof_on) {
        uint64_t h = (uint64_t)(uintptr_t)b * 0x2545F4914F6CDD1DULL;
        if (r->a) { if (!rr_seen((uint64_t)(uintptr_t)b * 0x9E3779B97F4A7C15ULL + 77)) prof_reads_sub_boxes++; h ^= subst_fp((Subst *)(uintptr_t)r->a); prof_reads_sub++; if (rr_seen(h)) prof_reads_sub_rep++; }
        else { prof_reads++; if (rr_seen(h)) prof_reads_rep++; }
    }
    if (r->a && !no_memo && !getenv_a8_off()) {
        /* the substitution cut to the box's dimensions (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): none left, a read of the box itself */
        fd_why = 0;
        FdInfo *fi = box_fd(b);
        if (!fi && prof_on) prof_fd_why[fd_why < 7 ? fd_why : 0]++;
        if (fi) {
            Subst *t = subst_to_fd((Subst *)(uintptr_t)r->a, fi);
            if (!t) { if (prof_on) prof_fd_dropped++; return lazy_copy(b->key, b->root); }
            if (t->n < ((Subst *)(uintptr_t)r->a)->n && prof_on) prof_fd_cut++;
            Port p = lazy_copy(b->key, b->root);
            Port o = sub_read(p, t); subst_release(t);
            return o;
        }
    }
    Port p = lazy_copy(r->a ? b->key : r->key, b->root);
    return r->a ? sub_read(p, (Subst *)(uintptr_t)r->a) : p;
}
Port ref_copy(Cell *c) { return ref_read(c); }
/* a word's suspended elimination, computed (for readback) */
Port wsusp_force(Cell *c) {
    Cell *e = cell(O_WELIM, c->n);
    link(c->port[3], P(e, 0)); link(c->port[1], P(e, 1)); link(c->port[2], P(e, 2));
    if (c->n == 5) link(c->port[4], P(e, 3));
    return P(e, c->n - 1);
}
static Port ref_of(Port val) { Cell *c = ref_to(box_new(val)); return P(c, 0); }
static void era(Port prov);
/* The last reference to a box gone: what it holds is erased, once the rule that dropped it is done
 * (retire_boxes): the root is then private to this thread */
static void memo_box_gone(Box *b);
static void box_release(Box *b) {
    if (rcl_dec(&b->rc)) return;
    /* claim the zero: a reader that raced us through the memo entry has raised the count again
     * and owns the box; only the claimant retires it */
    if (net_solo) { if (b->rc != 0) return; b->rc = -1; }
    else { long z = 0; if (!__atomic_compare_exchange_n(&b->rc, &z, -1, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) return; }
    memo_box_gone(b);
    Local *l = local();
    if (l->ndead == l->capdead) { l->capdead = l->capdead ? l->capdead * 2 : 16; l->dead = realloc(l->dead, l->capdead * sizeof(Box *)); }
    l->dead[l->ndead++] = b;
}
static void retire_boxes(void) {
    Local *l = local();
    while (l->ndead) {
        Box *b = l->dead[--l->ndead];
        Cell *r = b->root;
        Port p = r->port[0];
        if (p) era(p);
        cell_free(r);
        if (b->key) key_release(b->key);
        free(b->fd);
        free(b);
    }
}
static void era(Port prov) { Cell *e = cell(O_ERA, 1); e->a = (uint64_t)dup_origin; link(prov, P(e, 0)); }
/* an interval or face value changed in place: its key follows its payload */
static void rekey_formula(Cell *v) {
    if (!v->key) return;
    key_release(v->key);
    v->key = key0(v->kind == V_IV ? K_IV : K_FACE, 0, v->p);
}
/* A substitution is pending wherever it meets a value with parts (rule_subst; PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): the value is held
 * in a box and a reference carries the substitution, applied one level when the reference is read
 * (ref_read: the copy's top cell substituted by an O_SUBST with x = 1, each part a reference
 * carrying the substitution on). sub: a substitution to apply (pending where it meets parts);
 * sub_read: one read level, applied at once. Both are steps where they meet a value */
static Port sub(Port prov, Subst *s) { Cell *c = cell(O_SUBST, 2); c->p = subst_retain(s); link(prov, P(c, 0)); return P(c, 1); }  /* borrows s */
static Port sub_read(Port prov, Subst *s) { Cell *c = cell(O_SUBST, 2); c->p = subst_retain(s); c->x = 1; link(prov, P(c, 0)); return P(c, 1); }  /* borrows s */
static Port mkplam_of(Port prov, Dim i) { Cell *m = cell(O_MKPLAM, 2); m->a = i; link(prov, P(m, 0)); return P(m, 1); }
static Port iv_cell(Dnf *r) { Cell *c = cell(V_IV, 1); c->p = r; return P(c, 0); }  /* takes r */
/* m providers sharing one value (a chain of duplicators) */
static void split(Port prov, int m, Port *out) {
    if (m == 0) { era(prov); return; }
    if (m == 1) { out[0] = prov; return; }
    Key *k = key_of_port(prov);
    for (int i = 0; i < m - 1; i++) { Cell *d = cell(O_DUP, 3); d->a = (uint64_t)dup_origin; d->key = k ? key_retain(k) : NULL; link(prov, P(d, 0)); out[i] = P(d, 1); prov = P(d, 2); }
    out[m - 1] = prov;
    if (k) key_release(k);
}
void port_split(Port prov, int m, Port *out) { split(prov, m, out); }
static void split_restrict(Port prov, const Dnf *cl, int m, Port *out) {
    split(prov, m, out);
    for (int k = 0; k < m; k++) { Subst *t = subst_of_clause(cl->c[k]); out[k] = sub(out[k], t); subst_release(t); }
}

/* ---------------- types of neutrals ----------------
 * A neutral carries its type on its last port. Eliminating a neutral of
 * known type computes the result's type from it (CCHM: the endpoints of a
 * path are part of its type, so t 0 = u0 for t : Path A u0 u1 even when t
 * is neutral). The type is known when the neutral's head was declared with
 * one (ty = 1); otherwise it is V_UNK. */
static Port unk(void) { return P(cell(V_UNK, 1), 0); }
static bool is_neutral(int k) { return k >= V_NVAR && k <= V_NPROJ; }
static Port tpart(Port T, int k) { Cell *c = cell(O_TPART, 2); c->a = (uint64_t)k; link(T, P(c, 0)); return P(c, 1); }
static Port app2(Port f, Port x) { Cell *a = cell(O_APP, 3); link(f, P(a, 0)); link(x, P(a, 1)); return P(a, 2); }
static Port ntype_of(Port neu) { Cell *c = cell(O_NTYPE, 2); link(neu, P(c, 0)); return P(c, 1); }
static Port papp_at(Port line, Dnf *r) { Cell *a = cell(O_PAPPR, 3); link(iv_cell(r), P(a, 0)); link(line, P(a, 1)); return P(a, 2); }  /* takes r */
static void split2(Port prov, Port *a, Port *b) { Cell *d = cell(O_DUP, 3); d->a = (uint64_t)dup_origin; d->key = key_of_port(prov); link(prov, P(d, 0)); *a = P(d, 1); *b = P(d, 2); }

/* the stuck head of a copattern definition: a neutral of its declared type (when it has one), named
 * by the definition; projection unfolds it */
Port cofix_head(long g, long tyg) {
    Cell *c = cell(V_NVAR, 2); c->a = (uint64_t)g; c->ty = TY_COFIX | (global_tickar(g) ? TY_TICK : 0); c->key = key_global(g);
    if (tyg >= 0) { Cell *t = cell(V_GLOBAL, 1); t->a = (uint64_t)tyg; t->key = key_global(tyg); link(P(t, 0), P(c, 1)); c->ty |= TY_KNOWN; }
    else link(unk(), P(c, 1));
    return P(c, 0);
}
Port apply_gid(long g, const Port *args, int n) {
    Cell *gc = cell(V_GLOBAL, 1); gc->a = (uint64_t)g; gc->key = key_global(g);
    Port f = P(gc, 0);
    Key *fk = key_retain(gc->key);
    for (int k = 0; k < n; k++) {
        Cell *a = cell(O_APP, 3); link(f, P(a, 0)); link(args[k], P(a, 1)); f = P(a, 2);
        if (fk) { Key *ak = key_of_port(args[k]); Key *nk = key2(K_APP, 0, NULL, fk, ak); key_release(fk); if (ak) key_release(ak); fk = nk; }
        if (fk) { a->key = key_retain(fk); memo_register(a); }
    }
    if (fk) key_release(fk);
    return f;
}
static Port apply_global(const char *name, Port *args, int n) {
    long g = global_lookup(name);
    if (g < 0) { fprintf(stderr, "missing library definition %s\n", name); exit(2); }
    return apply_gid(g, args, n);
}
/* the induction hypotheses of fields recursive under paths, Π codomains and Σ components (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.18 “E28b — Strict positivity under Π codomains and Σ components”),
 * by the field's shape (ConInfo.rsh): the number of D leaves; whether the shape is a path line only
 * (built directly by the case rule); the maker of the hypothesis from the field, a function value:
 * at a leaf the elimination as a function (V_CASEF) over its own copies of the motive and the
 * environment, above it the prelude's %ih-* on the makers below */
static int ih_leaves(const char *s) { int n = 0; for (; *s; s++) if (*s == 'D') n++; return n; }
static bool ih_pure_path(const char *s) { for (; *s; s++) if (*s != 'P' && *s != 'D') return false; return true; }
static Port ih_maker(const char **sp, CaseTab *tb, int x, Port *mc, Port *ec, int stride, int ne, int *leaf) {
    char c = *(*sp)++;
    switch (c) {
    case 'D': {
        int mb = x ? 1 : 0;
        Cell *f = cell(V_CASEF, ne + 1 + mb); f->x = x; f->p = tb;
        if (mb) link(mc[*leaf], P(f, 1));
        for (int e = 0; e < ne; e++) link(ec[e * stride + *leaf + 1], P(f, 1 + mb + e));
        (*leaf)++;
        return P(f, 0);
    }
    case 'P': { Port a[1] = { ih_maker(sp, tb, x, mc, ec, stride, ne, leaf) }; return apply_global("%ih-path", a, 1); }
    case 'F': { Port a[1] = { ih_maker(sp, tb, x, mc, ec, stride, ne, leaf) }; return apply_global("%ih-pi", a, 1); }
    case '1': { Port a[1] = { ih_maker(sp, tb, x, mc, ec, stride, ne, leaf) }; return apply_global("%ih-fst", a, 1); }
    case '2': { Port a[1] = { ih_maker(sp, tb, x, mc, ec, stride, ne, leaf) }; return apply_global("%ih-snd", a, 1); }
    case 'B': { Port a[2]; a[0] = ih_maker(sp, tb, x, mc, ec, stride, ne, leaf); a[1] = ih_maker(sp, tb, x, mc, ec, stride, ne, leaf); return apply_global("%ih-pair", a, 2); }
    default: fprintf(stderr, "bad hypothesis shape\n"); exit(2);
    }
}
static Port face_cell(Dnf *f) { Cell *c = cell(V_FACE, 1); c->p = f; return P(c, 0); }  /* takes f */

/* a constructor applied to its fields and dimensions: c v r is its boundary
 * e(v, r) where the boundary's face holds at r (judgmentally), and otherwise
 * a constructor value. Built again after every substitution. */
/* O_DEEP's accumulator: a constructor's fields forced in turn (c the constructor, got of nf done) */
typedef struct { int c, nf, got; Port f[]; } DeepAcc;
/* ---- #jit (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §5.5 “Bytes, the boundary, and what is trusted”): a compiled definition's bytes, called from the net ----
 * The compiled code's values are 64 bits each (lib/capacities/x86.cub): a word or primitive by its bits; a
 * constructor with no fields by its index (below 4096); a constructor with fields, a pair, bytes, by
 * the address of a cell in the call's arena. A value of the net crosses when it is closed data (built
 * all the way down, no dimensions, every word within 64 bits); the result is read back by the
 * definition's type. The arena is the calling thread's, reset at each call: compiled code allocates
 * nothing else and writes nothing outside it (stores copy), so a call that leaves through the
 * overflow exit is computed again on the net with nothing done twice. */
typedef struct { uint64_t flag; unsigned char *cur, *end; uintptr_t stack_lim; } JitCtx;   /* stack_lim: below it a compiled definition's prologue takes the overflow exit */
static Port pair_of(Port a, Port b);
static __thread unsigned char *jit_arena;
static const size_t JIT_ARENA = (size_t)1 << 30;
static bool jit_ready(Cell *c) {
    size_t cap = 64, sp = 0; Cell **st = malloc(cap * sizeof(Cell *)); st[sp++] = c; bool ok = true;
    while (sp && ok) {
        Cell *x = st[--sp];
        if (!x) { ok = false; break; }
        switch (x->kind) {
        case V_WORD: if (x->p) ok = false; break;
        case V_PVAL: case V_BYTES: break;
        case V_CON:
            if (CON[x->a].ndims) { ok = false; break; }
            for (int j = 0; j < CON[x->a].nfields; j++) { if (sp == cap) { cap *= 2; st = realloc(st, cap * sizeof(Cell *)); } st[sp++] = cell_at(x->port[1 + j]); }
            break;
        case V_PAIR:
            if (sp + 2 > cap) { cap *= 2; st = realloc(st, cap * sizeof(Cell *)); }
            st[sp++] = cell_at(x->port[1]); st[sp++] = cell_at(x->port[2]); break;
        default: ok = false;
        }
    }
    free(st); return ok;
}
static uint64_t *jit_alloc(JitCtx *x, size_t words) {
    size_t sz = words * 8;
    if ((size_t)(x->end - x->cur) < sz) { x->flag = 1; return NULL; }
    uint64_t *p = (uint64_t *)x->cur; x->cur += sz; return p;
}
/* a value of the net into the arena; a data type's last field followed by a loop (a list's tail) */
static uint64_t jit_in(Cell *c, JT *t, JitCtx *x) {
    uint64_t r = 0, *slot = &r;
    for (;;) {
        if (x->flag) return 0;
        switch (t->k) {
        case JT_WORD: case JT_PRIM: *slot = c->a; return r;
        case JT_ERASED: *slot = 0; return r;
        case JT_NAT: {
            /* a numeral counted to the word it is (Nat by Word) */
            uint64_t k = 0;
            while (c && c->kind == V_CON && CON[c->a].nfields == 1) { k++; c = cell_at(c->port[1]); }
            *slot = k; return r;
        }
        case JT_BYTES: {
            Bytes *b = c->p; size_t n = b ? b->n : 0;
            uint64_t *p = jit_alloc(x, 1 + (n + 7) / 8); if (!p) return 0;
            p[0] = n; if (n) memcpy(p + 1, b->b, n);
            *slot = (uint64_t)(uintptr_t)p; return r;
        }
        case JT_PAIR: {
            uint64_t *p = jit_alloc(x, 3); if (!p) return 0;
            p[0] = 0; p[1] = jit_in(cell_at(c->port[1]), t->a, x);
            *slot = (uint64_t)(uintptr_t)p; slot = &p[2]; c = cell_at(c->port[2]); t = t->b; continue;
        }
        case JT_DATA: {
            ConInfo *k = &CON[c->a]; int nf = k->nfields;
            if (nf == 0) { *slot = (uint64_t)k->index; return r; }
            uint64_t *p = jit_alloc(x, 1 + (size_t)nf); if (!p) return 0;
            p[0] = (uint64_t)k->index;
            for (int j = 0; j < nf - 1; j++) p[1 + j] = jit_in(cell_at(c->port[1 + j]), jt_field(t, k->index, j), x);
            *slot = (uint64_t)(uintptr_t)p; slot = &p[nf];
            JT *tl = jt_field(t, k->index, nf - 1); c = cell_at(c->port[nf]); t = tl; continue;
        }
        default: x->flag = 1; return 0;
        }
    }
}
/* a compiled value back into cells by its type; along the last field the spine is collected first and
 * built from its end, so a long list is read without deep recursion */
static Port jit_read(uint64_t v, JT *t);
static Port jit_read_one(uint64_t v, JT *t, Port last) {
    switch (t->k) {
    case JT_PAIR: { uint64_t *p = (uint64_t *)(uintptr_t)v; return pair_of(jit_read(p[1], t->a), last); }
    case JT_DATA: {
        uint64_t *p = (uint64_t *)(uintptr_t)v; int ci = (int)p[0], c = DATA[t->d].cons[ci], nf = CON[c].nfields;
        Port *f = malloc((size_t)(nf + 1) * sizeof(Port));
        for (int j = 0; j < nf - 1; j++) f[j] = jit_read(p[1 + j], jt_field(t, ci, j));
        f[nf - 1] = last;
        Port r = con_build(c, f, NULL); free(f); return r;
    }
    default: return word_of(0);
    }
}
static Port jit_read(uint64_t v, JT *t) {
    size_t cap = 16, n = 0; uint64_t *vs = malloc(cap * sizeof(uint64_t)); JT **ts = malloc(cap * sizeof(JT *));
    Port tail;
    for (;;) {
        if (t->k == JT_WORD) { tail = word_of(v); break; }
        if (t->k == JT_NAT) {
            /* the word as a numeral: zero, then suc v times */
            Port none[1]; tail = con_build(DATA[t->d].cons[0], none, NULL);
            for (uint64_t i = 0; i < v; i++) { Port f[1] = { tail }; tail = con_build(DATA[t->d].cons[1], f, NULL); }
            break;
        }
        if (t->k == JT_PRIM) { Cell *c = cell(V_PVAL, 1); c->x = (uint32_t)t->pt; c->a = v; tail = P(c, 0); break; }
        if (t->k == JT_BYTES) {
            uint64_t *p = (uint64_t *)(uintptr_t)v; size_t bn = (size_t)p[0];
            Bytes *b = calloc(1, sizeof(Bytes) + bn); b->rc = 1; b->n = bn; if (bn) memcpy(b->b, p + 1, bn);
            Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b; tail = P(c, 0); break;
        }
        if (t->k == JT_DATA && v < 4096) { Port none[1]; tail = con_build(DATA[t->d].cons[v], none, NULL); break; }
        if (t->k != JT_DATA && t->k != JT_PAIR) { tail = word_of(0); break; }
        if (n == cap) { cap *= 2; vs = realloc(vs, cap * sizeof(uint64_t)); ts = realloc(ts, cap * sizeof(JT *)); }
        vs[n] = v; ts[n] = t; n++;
        uint64_t *p = (uint64_t *)(uintptr_t)v;
        if (t->k == JT_PAIR) { v = p[2]; t = t->b; }
        else { int ci = (int)p[0], nf = CON[DATA[t->d].cons[ci]].nfields; v = p[nf]; t = jt_field(t, ci, nf - 1); }
    }
    while (n) { n--; tail = jit_read_one(vs[n], ts[n], tail); }
    free(vs); free(ts); return tail;
}
/* the call: the arguments converted, the code entered as jit(args, n, ctx); the flag set means the
 * call is the definition's on the net (the fallback, run p f ≡ f) */
static bool jit_call(const Ffi *e, Port *args, int n, Port *out) {
    if (!jit_arena) {
        jit_arena = mmap(NULL, JIT_ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (jit_arena == MAP_FAILED) { jit_arena = NULL; return false; }
    }
    /* the lowest stack address compiled code may reach: the thread's stack bottom plus a margin
     * (compiled code calls no C, so the margin only needs to cover one frame's pushes) */
    static __thread uintptr_t stack_lim;
    if (!stack_lim) {
        void *lo = NULL;
#ifdef __APPLE__
        /* 2026-11-13: Darwin reports the stack's high address and size. */
        lo = (char *)pthread_get_stackaddr_np(pthread_self()) - pthread_get_stacksize_np(pthread_self());
#else
        pthread_attr_t at; size_t sz = 0;
        if (pthread_getattr_np(pthread_self(), &at) == 0) { pthread_attr_getstack(&at, &lo, &sz); pthread_attr_destroy(&at); }
#endif
        stack_lim = lo ? (uintptr_t)lo + ((size_t)1 << 18) : (uintptr_t)__builtin_frame_address(0) - ((size_t)1 << 22);
    }
    JitCtx x = { 0, jit_arena, jit_arena + JIT_ARENA, stack_lim };
    uint64_t a[NATIVE_MAXW];
    for (int k = 0; k < n; k++) a[k] = e->jt[k]->k == JT_ERASED ? 0 : jit_in(cell_at(args[k]), e->jt[k], &x);
    if (x.flag) return false;
    uint64_t r = ((uint64_t (*)(const uint64_t *, uint64_t, JitCtx *))e->fn)(a, (uint64_t)n, &x);
    if (x.flag) return false;
    *out = jit_read(r, e->jt[n]);
    return true;
}
/* a foreign entry called on its arguments' bits (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §18.11 “D24–D27 — The world”, its part D27): every argument and the result integers
 * (or pointers), or every one a double */
uint64_t ffi_invoke(const Ffi *e, const uint64_t *a, int n) {
    uint64_t r = 0;
    if (e->ret != PT_F64) {
        typedef uint64_t (*F1)(uint64_t); typedef uint64_t (*F2)(uint64_t, uint64_t); typedef uint64_t (*F3)(uint64_t, uint64_t, uint64_t);
        typedef uint64_t (*F4)(uint64_t, uint64_t, uint64_t, uint64_t); typedef uint64_t (*F5)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t); typedef uint64_t (*F6)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
        typedef uint64_t (*F0)(void);
        switch (n) {
        case 0: r = ((F0)e->fn)(); break;
        case 1: r = ((F1)e->fn)(a[0]); break; case 2: r = ((F2)e->fn)(a[0], a[1]); break; case 3: r = ((F3)e->fn)(a[0], a[1], a[2]); break;
        case 4: r = ((F4)e->fn)(a[0], a[1], a[2], a[3]); break; case 5: r = ((F5)e->fn)(a[0], a[1], a[2], a[3], a[4]); break; default: r = ((F6)e->fn)(a[0], a[1], a[2], a[3], a[4], a[5]); break;
        }
        if (e->ret == PT_U32) r = (uint32_t)r;
    } else {
        typedef double (*D1)(double); typedef double (*D2)(double, double); typedef double (*D3)(double, double, double); typedef double (*D4)(double, double, double, double);
        double d[8], dr; for (int k = 0; k < n; k++) memcpy(&d[k], &a[k], 8);
        switch (n) { case 1: dr = ((D1)e->fn)(d[0]); break; case 2: dr = ((D2)e->fn)(d[0], d[1]); break; case 3: dr = ((D3)e->fn)(d[0], d[1], d[2]); break; default: dr = ((D4)e->fn)(d[0], d[1], d[2], d[3]); break; }
        memcpy(&r, &dr, 8);
    }
    return r;
}
static Port con_build_bdy(int c, const Port *f, const Port *d);
/* a path constructor (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): its dimensions waited for in turn (O_CONW); its boundary's faces are
 * conjunctions of its dimensions' ends, so while no dimension is 0 or 1 none holds and the
 * constructor stands without its boundary being built; a dimension at an end builds it (as before).
 * The waits are steps whatever the arrival order, so counts do not depend on the schedule */
static inline bool a8_conw(void) { static int on = -1; if (on < 0) on = getenv("CUBICAL_NOCONW") == NULL; return on; }
Port con_build(int c, const Port *f, const Port *d) {
    ConInfo *k = &CON[c];
    int nf = k->nfields, nd = k->ndims, n = nf + nd;
    if (k->bdy >= 0 && nd > 0 && a8_conw()) {
        /* 0=dimension 0, 1..nf=fields, nf+1..=the other dimensions in order, out */
        Cell *w = cell(O_CONW, n + 1); w->a = (uint64_t)c; w->x = 0;
        link(d[0], P(w, 0));
        for (int j = 0; j < nf; j++) link(f[j], P(w, 1 + j));
        for (int j = 1; j < nd; j++) link(d[j], P(w, nf + j));
        return P(w, n);
    }
    return con_build_bdy(c, f, d);
}
static Port con_build_bdy(int c, const Port *f, const Port *d) {
    ConInfo *k = &CON[c];
    int nf = k->nfields, nd = k->ndims, n = nf + nd;
    if (k->bdy < 0) {
        Cell *v = cell(V_CON, n + 1); v->a = (uint64_t)c;
        for (int j = 0; j < nf; j++) link(f[j], P(v, 1 + j));
        for (int j = 0; j < nd; j++) link(d[j], P(v, 1 + nf + j));
        return P(v, 0);
    }
    Port *a = malloc((size_t)(2 * n + 1) * sizeof(Port));
    for (int j = 0; j < n; j++) split2(j < nf ? f[j] : d[j - nf], &a[j], &a[n + j]);
    Port S = apply_gid(k->bdy, a, n), s1, s2;
    split2(S, &s1, &s2);
    Cell *fo = cell(O_FACEOF, 2); link(s1, P(fo, 0));
    Cell *ck = cell(O_CONCHK, n + 3); ck->a = (uint64_t)c;
    link(P(fo, 1), P(ck, 0)); link(s2, P(ck, 1));
    for (int j = 0; j < n; j++) link(a[n + j], P(ck, 2 + j));
    free(a);
    return P(ck, n + 2);
}

/* ---------------- structural rules ---------------- */
static void rule_dup(Cell *o, Cell *v) {
    Port o1 = o->port[1], o2 = o->port[2];
    if (prof_on) {
        ADD(prof_dupk[v->kind & 63], 1);
        if (v->kind == V_LAM) ADD(prof_dup[((Record *)v->p)->where], 1);
        if (v->kind == V_CON && v->a < 4096) ADD(prof_con[v->a], 1);
        if (v->kind == V_GLOBAL && v->a < 65536) ADD(prof_glob[v->a], 1);
    }
    if (v->n > 1 || unbuilt(v->kind)) {
        /* a value with parts is held once: each side a reference, read one level at a time (a
         * subterm held unbuilt is built once, by its first reader) */
        Box *bx = box_new(P(v, 0));
        Cell *r1 = ref_to(bx), *r2 = ref_to(bx); box_hold(bx);
        link(P(r1, 0), o1); link(P(r2, 0), o2);
        cell_free(o);
        return;
    }
    Cell *c1 = clone(v), *c2 = clone(v);
    for (int k = 1; k < v->n; k++) {
        Cell *d = cell(O_DUP, 3); d->a = o->a; d->key = key_of_port(v->port[k]);
        link(v->port[k], P(d, 0)); link(P(d, 1), P(c1, k)); link(P(d, 2), P(c2, k));
    }
    link(P(c1, 0), o1); link(P(c2, 0), o2);
    cell_free(o); cell_free(v);
}
static void rule_era(Cell *o, Cell *v) {
    if (prof_on) ADD(prof_erak[v->kind & 63], 1);
    dup_origin = (long)o->a;
    for (int k = 1; k < v->n; k++) era(v->port[k]);
    cell_free(o); cell_free(v);
}

/* substitution of dimensions, one level: the parts get the substitution as pending (sub) */
static void subst_level(Cell *o, Cell *v) {
    Subst *s = o->p;
    Port out = o->port[1];
    /* a deferred operation read under a substitution: run, and its value substituted one level when
     * it arrives (the reader is reading it now: held again it would be read at once) */
    if (v->kind == V_DEFER) { link(sub_read(defer_build(v), s), out); cell_free(o); return; }
    if (is_neutral(v->kind) && v->key) {
        /* a neutral read as an interval is its atom: a substitution moving the atom gives the image */
        Dim d = atom_find(v);
        const Dnf *r = d ? subst_get(s, d) : NULL;
        if (r) {
            link(iv_cell(dnf_retain((Dnf *)r)), out);
            for (int k = 1; k < v->n; k++) era(v->port[k]);
            cell_free(o); cell_free(v); return;
        }
    }
    switch (v->kind) {
    case V_GLOBAL: {
        /* a definition's value has no free dimension, but an atom s assumes may be read from a
         * neutral its value builds: then the value, copied in, is substituted */
        Dnf *ac = atom_clause(s);
        if (ac) {
            dnf_release(ac);
            Port r = global_copy((long)v->a, v->key);
            if (r) { link(sub(r, s), out); cell_free(o); cell_free(v); return; }
        }
        link(P(v, 0), out); cell_free(o); return;
    }
    case V_UNK:
        link(P(v, 0), out); cell_free(o); return;
    case V_THUNK: case V_REF: {
        /* the reference (the unbuilt subterm) carries the substitution: nothing is copied */
        Subst *t = v->a ? subst_compose((Subst *)(uintptr_t)v->a, s) : subst_retain(s);
        if (v->a) subst_release((Subst *)(uintptr_t)v->a);
        if (v->kind == V_REF && !no_memo && !getenv_a8_off()) {
            /* only the box's own dimensions (box_fd; PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): the rest leave its value unchanged */
            FdInfo *fi = box_fd(v->p);
            if (fi) { Subst *t2 = subst_to_fd(t, fi); subst_release(t); t = t2; }
        }
        v->a = (uint64_t)(uintptr_t)t;
        if (v->e) { memo_leave(v); memo_register(v); }
        link(P(v, 0), out); cell_free(o); return;
    }
    case V_NVAR:
        link(sub(v->port[1], s), P(v, 1));
        link(P(v, 0), out); cell_free(o); return;
    case V_IV: { Dnf *n2 = iv_subst(v->p, s); dnf_release(v->p); v->p = n2; rekey_formula(v); link(P(v, 0), out); cell_free(o); return; }
    case V_FACE: { Dnf *n2 = face_subst(v->p, s); dnf_release(v->p); v->p = n2; rekey_formula(v); link(P(v, 0), out); cell_free(o); return; }
    case V_PART: {
        /* Partial r A under s: A itself where s makes r 1 */
        Dnf *n2 = iv_subst(v->p, s);
        if (dnf_is_one(n2)) { dnf_release(n2); link(sub(v->port[1], s), out); cell_free(o); cell_free(v); return; }
        Cell *c = cell(V_PART, 2); c->p = n2; link(sub(v->port[1], s), P(c, 1)); link(P(c, 0), out);
        cell_free(o); cell_free(v); return;
    }
    case V_PLAM: {
        Dim j = (Dim)v->a, j2 = j;
        Subst *t = subst_without(s, j);
        if (subst_range_mentions(t, j)) {
            j2 = fresh_dim();
            Dnf *at = iv_atom(j2); Subst *t2 = subst_with(t, j, at);
            dnf_release(at); subst_release(t); t = t2;
        }
        link(mkplam_of(sub(v->port[1], t), j2), out);
        subst_release(t);
        cell_free(o); cell_free(v); return;
    }
    case V_SYS: {
        int nb = v->n - 1;
        Dnf *cl = v->p;
        Dnf **f = malloc((size_t)(nb + 1) * sizeof(Dnf *));
        int total = 0;
        for (int k = 0; k < nb; k++) { Dnf *fc = face_of_clause(cl->c[k]); f[k] = face_subst(fc, s); dnf_release(fc); total += (int)f[k]->n; }
        for (int k = 0; k < nb; k++) if (dnf_is_one(f[k])) {
            /* the substitution entails this branch's clause: the system is that value */
            link(sub(v->port[k + 1], s), out);
            for (int m = 0; m < nb; m++) if (m != k) era(v->port[m + 1]);
            for (int m = 0; m < nb; m++) dnf_release(f[m]);
            free(f); cell_free(o); cell_free(v); return;
        }
        Cell *ns = cell(V_SYS, total + 1);
        Clause **ncl = malloc((size_t)(total + 1) * sizeof(Clause *));
        int idx = 0;
        for (int k = 0; k < nb; k++) {
            int m = (int)f[k]->n;
            Port *cp = malloc((size_t)(m + 1) * sizeof(Port));
            split(v->port[k + 1], m, cp);
            for (int q = 0; q < m; q++) {
                ncl[idx] = f[k]->c[q];
                Subst *t = subst_then_clause(s, f[k]->c[q]);
                link(sub(cp[q], t), P(ns, idx + 1));
                subst_release(t);
                idx++;
            }
            free(cp);
        }
        ns->p = clause_list(ncl, (uint32_t)total);
        free(ncl);
        link(P(ns, 0), out);
        for (int m = 0; m < nb; m++) dnf_release(f[m]);
        free(f); cell_free(o); cell_free(v); return;
    }
    /* neutrals and reducible constructors become their operations again */
    case V_NAPP: { Cell *a = cell(O_APP, 3); link(sub(v->port[1], s), P(a, 0)); link(sub(v->port[2], s), P(a, 1)); link(P(a, 2), out); era(v->port[3]); break; }
    case V_NFST: case V_NSND: { Cell *a = cell(v->kind == V_NFST ? O_FST : O_SND, 2); link(sub(v->port[1], s), P(a, 0)); link(P(a, 1), out); era(v->port[2]); break; }
    case V_NPROJ: { Cell *a = cell(O_PROJ, 2); a->a = v->a; link(sub(v->port[1], s), P(a, 0)); link(P(a, 1), out); era(v->port[2]); break; }
    case V_NPAPP: {
        Cell *a = cell(O_PAPPR, v->x ? 5 : 3); a->x = v->x;
        link(iv_cell(iv_subst(v->p, s)), P(a, 0));
        link(sub(v->port[1], s), P(a, 1));
        if (v->x) { link(sub(v->port[2], s), P(a, 2)); link(sub(v->port[3], s), P(a, 3)); }
        link(P(a, a->n - 1), out);
        era(v->port[v->n - 1]);
        break;
    }
    case V_NCOMP: { Cell *a = cell(O_COMP, 4); link(sub(v->port[2], s), P(a, 0)); link(sub(v->port[1], s), P(a, 1)); link(sub(v->port[3], s), P(a, 2)); link(P(a, 3), out); era(v->port[4]); break; }
    case V_NUNGLUE: { Cell *a = cell(O_UNGLUE, 3); a->x = v->x; link(sub(v->port[1], s), P(a, 0)); link(sub(v->port[2], s), P(a, 1)); link(P(a, 2), out); era(v->port[3]); break; }
    case V_CON: {
        ConInfo *k = &CON[v->a];
        int n = k->nfields + k->ndims;
        Port *a = malloc((size_t)(n + 1) * sizeof(Port));
        for (int j = 0; j < n; j++) a[j] = sub(v->port[1 + j], s);
        link(con_build((int)v->a, a, a + k->nfields), out);
        free(a);
        break;
    }
    case V_WPLUS: { Cell *a = cell(O_WADD, 3); link(sub(v->port[1], s), P(a, 0)); link(word_wn(cell_wn(v)), P(a, 1)); link(P(a, 2), out); break; }
    case V_NWOP: { Cell *a = cell((int)v->x, 3); link(sub(v->port[1], s), P(a, 0)); link(sub(v->port[2], s), P(a, 1)); link(P(a, 2), out); break; }
    case V_NWELIM: { Cell *a = cell(O_WELIM, v->n); for (int k = 1; k < v->n; k++) link(sub(v->port[k], s), P(a, k - 1)); link(P(a, v->n - 1), out); break; }
    case V_NPRIM: {
        /* the operation again, on the substituted arguments */
        int cls = (int)(v->x >> 8), sb = (int)(v->x & 255), n = v->n - 1;
        int kind = cls == 1 ? O_POP1 : cls == 2 ? O_POP2 : cls == 3 ? O_LOAD : cls == 4 ? O_STORE : cls == 5 ? O_BLEN : O_BYTES;
        Cell *a = cell(kind, n + 1); a->x = (uint32_t)sb;
        for (int k = 0; k < n; k++) link(sub(v->port[k + 1], s), P(a, k));
        link(P(a, n), out);
        break;
    }
    case V_HCOMP: {
        /* rebuilt: a tube the substitution makes total ends, and sides it makes constructors meet */
        Cell *a = cell(O_HCOMP, v->n); link(sub(v->port[1], s), P(a, 0)); link(sub(v->port[2], s), P(a, 1));
        if (v->n == 4) link(sub(v->port[3], s), P(a, 2));
        link(P(a, a->n - 1), out); break;
    }
    case V_TRANSH: {
        /* the parts substituted; the tube re-decided (O_TRANSH): a side the substitution selects is
         * transported alone, at its end */
        int np = v->n - 3;
        Cell *a = cell(O_TRANSH, np + 3); a->a = v->a; a->p = face_subst(v->p, s);
        link(sub(v->port[1], s), P(a, 0)); link(sub(v->port[2], s), P(a, 1));
        for (int j = 0; j < np; j++) link(sub(v->port[3 + j], s), P(a, 2 + j));
        link(P(a, np + 2), out);
        break;
    }
    case V_NCASE: {
        /* 1=scrutinee [2=motive] then the environment, type → 0=scrutinee then the environment, out:
         * the motive is the table's, rebuilt where it is read */
        int mb = v->x ? 1 : 0, ne = v->n - 3 - mb;
        Cell *a = cell(O_CASE, ne + 2); a->x = 0; a->p = v->p;
        link(sub(v->port[1], s), P(a, 0));
        for (int e = 0; e < ne; e++) link(sub(v->port[2 + mb + e], s), P(a, 1 + e));
        link(P(a, a->n - 1), out);
        if (mb) era(v->port[2]);
        era(v->port[v->n - 1]);
        break;
    }
    case V_NTRANS: {
        int np = v->n - 3;
        Cell *a = cell(O_TRANSD1, np + 3); a->a = v->a;
        link(face_cell(face_subst(v->p, s)), P(a, 0));
        for (int j = 0; j < np; j++) link(sub(v->port[2 + j], s), P(a, 1 + j));
        link(sub(v->port[1], s), P(a, np + 1));
        link(P(a, np + 2), out);
        era(v->port[v->n - 1]);
        break;
    }
    case V_GLUE: { Cell *a = cell(O_MKGLUE, 3); a->x = v->x; link(sub(v->port[2], s), P(a, 0)); link(sub(v->port[1], s), P(a, 1)); link(P(a, 2), out); break; }
    case V_GLUEV: { Cell *a = cell(O_MKGLUEV, 3); link(sub(v->port[1], s), P(a, 0)); link(sub(v->port[2], s), P(a, 1)); link(P(a, 2), out); break; }
    default: {
        /* constructors: λ (its environment), pair, Π, Σ, PathP; named as the value under s */
        Cell *c = clone(v);
        if (c->key) { Key *k = key_under(c->key, s); key_release(c->key); c->key = k; }
        for (int k = 1; k < v->n; k++) link(sub(v->port[k], s), P(c, k));
        Dnf *ac;
        int np = v->kind == V_LAM ? ((Record *)v->p)->nparam : v->kind == V_CASEF ? 1 : -1;
        if ((np == 0 || np == 1) && atom_res0_global >= 0 && (ac = atom_clause(s))) {
            /* the atoms s assumes reach what the function or suspension builds, not only its
             * environment */
            Port a[2] = { face_cell(ac), P(c, 0) };
            link(apply_gid(np ? atom_res_global : atom_res0_global, a, 2), out);
        } else link(P(c, 0), out);
        break;
    }
    }
    cell_free(o); cell_free(v);
}
/* A8 (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): does a substitution meeting this value stay pending (a value with parts other than a
 * reference, an unbuilt subterm, a variable or a formula)? */
static bool subst_defers(const Cell *v) {
    if (v->kind == V_DEFER) return true;
    if (v->n <= 1 || unbuilt(v->kind)) return false;
    switch (v->kind) {
    case V_GLOBAL: case V_UNK: case V_REF: case V_NVAR: case V_IV: case V_FACE: return false;
    default: return true;
    }
}
long prof_sub_held, prof_sub_dropped;
static void rule_subst(Cell *o, Cell *v) {
    if (o->x == 1 || !subst_defers(v)) {
        /* a read's level (x = 1), or a value with nothing to hold: substituted now */
        subst_level(o, v);
        return;
    }
    /* the value is held and the substitution pending on a reference to it: nothing is copied or
     * re-dispatched until a reader reads it, one level at a time (ref_read). With the memo, on one
     * worker, the value's free dimensions cut the substitution first (as a box's do, box_fd): one
     * that does not meet them is dropped */
    Subst *s = subst_retain(o->p);
    Port out = o->port[1];
    FdInfo *nf = NULL;
    if (!no_memo && !getenv_a8_off() && net_solo) {
        FdSet *y = malloc(sizeof *y); y->n = 0; y->taint = false; y->inexact = false;
        long saved = fd_budget; fd_budget = 4000; fd_why = 0;
        bool ok = fd_cell(v, y, 0);
        fd_budget = saved;
        if (ok && y->n <= 256) {
            nf = malloc(sizeof(FdInfo) + (size_t)y->n * sizeof(Dim)); nf->n = (uint32_t)y->n;
            nf->taint = y->taint; nf->inexact = y->inexact;
            memcpy(nf->d, y->d, (size_t)y->n * sizeof(Dim));
        }
        free(y);
        if (nf) {
            Subst *t = subst_to_fd(s, nf); subst_release(s); s = t;
            if (!s) {
                if (prof_on) prof_sub_dropped++;
                free(nf);
                link(P(v, 0), out); cell_free(o); return;
            }
        }
    }
    if (prof_on) prof_sub_held++;
    Box *b = box_new(P(v, 0));
    if (nf) { b->fd = nf; b->fdtries = nf->inexact ? 1 : 0; }
    Cell *r = ref_to(b); r->a = (uint64_t)(uintptr_t)s;
    link(P(r, 0), out);
    cell_free(o);
}

/* a value is put under a dimension binder; a system whose clauses avoid the
 * dimension is a system of lines */
static void rule_mkplam(Cell *o, Cell *v) {
    Dim i = (Dim)o->a;
    if (v->kind == V_REF) { link(ref_read(v), P(o, 0)); cell_free(v); return; }
    if (unbuilt(v->kind)) {
        if (o->kind == O_FORCE) { link(unbuilt_build(v), o->port[1]); cell_free(o); }
        else link(unbuilt_build(v), P(o, 0));
        return;
    }
    Port out = o->port[1];
    if (v->kind == V_SYS) {
        Dnf *cl = v->p;
        int nb = v->n - 1;
        bool avoid = true;
        for (int k = 0; k < nb; k++) if (clause_mentions(cl->c[k], i)) avoid = false;
        if (avoid) {
            Cell *ns = cell(V_SYS, nb + 1); ns->p = dnf_retain(cl);
            /* each branch put under the binder when it is read (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”) */
            for (int k = 0; k < nb; k++) { Cell *m = defer_cell(O_MKPLAM, 1); m->a = i; link(v->port[k + 1], P(m, 1)); link(P(m, 0), P(ns, k + 1)); }
            link(P(ns, 0), out);
            cell_free(o); cell_free(v); return;
        }
    }
    Cell *pl = cell(V_PLAM, 2); pl->a = i;
    link(P(v, 0), P(pl, 1));
    link(P(pl, 0), out);
    cell_free(o);
}

/* an eliminator meets a system: it acts branchwise, every other operand
 * restricted to the branch's clause */
/* a formal transport of a composition, distributed one layer (the generated clause: the transport
 * into each side and the cap, the sides' own inner compositions formal again); the cell is consumed */
Port transh_expand(Cell *v) {
    int np = v->n - 3;
    Port *a = malloc((size_t)(np + 4) * sizeof(Port));
    for (int j = 0; j < np; j++) a[j] = v->port[3 + j];
    a[np] = face_cell(dnf_retain(v->p)); a[np + 1] = v->port[1]; a[np + 2] = v->port[2];
    Port r = apply_gid(DATA[v->a].transh, a, np + 3);
    free(a);
    cell_free(v);
    return r;
}
static void distribute(Cell *o, Cell *v) {
    int nb = v->n - 1;
    Dnf *cl = v->p;
    Cell *ns = cell(V_SYS, nb + 1); ns->p = dnf_retain(cl);
    Port ex[o->n]; int ne = 0;
    switch (o->kind) {
    case O_APP: case O_UNGLUE2: case O_SYSMAP: case O_DAPPLY: ex[ne++] = o->port[1]; break;
    case O_PAPPP: if (o->x) { ex[ne++] = o->port[1]; ex[ne++] = o->port[2]; } break;
    case O_COMPA: ex[ne++] = o->port[1]; ex[ne++] = o->port[2]; break;
    case O_CASE: for (int k = 1; k < o->n - 1; k++) ex[ne++] = o->port[k]; break;
    case O_TRANSD2: for (int k = 1; k < o->n - 1; k++) ex[ne++] = o->port[k]; break;
    default: break;
    }
    Port out = out_of(o);
    Port *cp[o->n];
    for (int e = 0; e < ne; e++) { cp[e] = malloc((size_t)(nb + 1) * sizeof(Port)); split_restrict(ex[e], cl, nb, cp[e]); }
    /* each branch's operation is deferred (V_DEFER): run when that branch is read (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”) */
#define DI(c, k) P(c, (k) + 1)
    for (int b = 0; b < nb; b++) {
        Port val = v->port[b + 1], res = P(ns, b + 1);
        Cell *c;
        switch (o->kind) {
        case O_APP: c = defer_cell(O_APP, 2); link(val, DI(c, 0)); link(cp[0][b], DI(c, 1)); break;
        case O_SYSMAP: c = defer_cell(O_APP, 2); link(cp[0][b], DI(c, 0)); link(val, DI(c, 1)); break;
        case O_FST: case O_SND: case O_FORCE: case O_TPART: case O_PROJ: case O_UNFOLD: c = defer_cell(o->kind, 1); c->a = o->a; link(val, DI(c, 0)); break;
        case O_DAPPLY: c = defer_cell(O_DAPPLY, 2); link(val, DI(c, 0)); link(cp[0][b], DI(c, 1)); break;
        case O_FACEOF: c = defer_cell(O_FACEOF, 1); link(val, DI(c, 0)); break;
        case O_PAPPP: {
            /* the interval restricted is formed now: the deferred operation is the application */
            c = defer_cell(O_PAPPR, o->x ? 4 : 2); c->x = o->x;
            { Subst *t = subst_of_clause(cl->c[b]); link(iv_cell(iv_subst(o->p, t)), DI(c, 0)); subst_release(t); }
            link(val, DI(c, 1));
            if (o->x) { link(cp[0][b], DI(c, 2)); link(cp[1][b], DI(c, 3)); }
            break;
        }
        case O_UNGLUE2: c = defer_cell(O_UNGLUE, 2); c->x = o->x; link(cp[0][b], DI(c, 0)); link(val, DI(c, 1)); break;
        case O_COMPA: c = defer_cell(O_COMP, 3); link(cp[0][b], DI(c, 0)); link(val, DI(c, 1)); link(cp[1][b], DI(c, 2)); break;
        case O_CASE:
            c = defer_cell(O_CASE, o->n - 1); c->x = o->x; c->p = o->p;
            link(val, DI(c, 0));
            for (int e = 0; e < ne; e++) link(cp[e][b], DI(c, 1 + e));
            break;
        case O_TRANSD2:
            c = defer_cell(O_TRANSD2, o->n - 1); c->a = o->a;
            { Subst *t = subst_of_clause(cl->c[b]); c->p = face_subst(o->p, t); subst_release(t); }
            link(val, DI(c, 0));
            for (int e = 0; e < ne; e++) link(cp[e][b], DI(c, 1 + e));
            break;
        default: fprintf(stderr, "distribute: kind %d\n", o->kind); exit(2);
        }
        link(P(c, 0), res);
    }
#undef DI
    for (int e = 0; e < ne; e++) free(cp[e]);
    link(P(ns, 0), out);
    cell_free(o); cell_free(v);
}

/* a stuck projection is a neutral retaining its operand; its type is the
 * Σ's first component, or its second at the first projection */
static void neutral_proj(Cell *o, Cell *v) {
    bool first = o->kind == O_FST;
    Cell *n = cell(first ? V_NFST : V_NSND, 3);
    Port out = o->port[1];
    /* its type is computed from the spine when needed (O_NTYPE): stored, the type of a projection
     * would hold the earlier projections each with its own type, exponential in the depth */
    (void)first;
    link(P(v, 0), P(n, 1)); link(unk(), P(n, 2));
    link(P(n, 0), out);
    cell_free(o);
}

/* ---------------- values as data ----------------
 * %view gives a value's head as a constructor of View (lib/core/code.cub) whose fields are the
 * value's own ports: nothing is copied, and a part is read further only by viewing it. */
enum { W_LAM, W_PAIR, W_PI, W_SIGMA, W_U, W_PATH, W_PLAM, W_IV, W_FACE, W_SYS, W_GLUE, W_GLUEV, W_DATA, W_CON,
       W_HCOMP, W_VAR, W_APP, W_FST, W_SND, W_PAPP, W_COMP, W_UNGLUE, W_ELIM, W_TRANS, W_LEVEL, W_OTHER,
       W_WORD, W_WLIT, W_WPLUS, W_WOP, W_WELIM, W_COFIX, W_PROJ, W_REC, W_CLOCK, W_TICK, W_CLOCK0, W_TICK0, W_HCOMPU, W_UNGLUEU,
       W_PTY, W_PLIT, W_BYTES, W_PRIM, W_ITY, W_PART, W_N };
static const char *WNAME[W_N] = { "wlam", "wpair", "wpi", "wsigma", "wu", "wpath", "wplam", "wiv", "wface", "wsys", "wglue",
    "wgluev", "wdata", "wcon", "whcomp", "wvar", "wapp", "wfst", "wsnd", "wpapp", "wcomp", "wunglue", "welim", "wtrans",
    "wlevel", "wother", "wword", "wwlit", "wwplus", "wwop", "wwelim", "wcofix", "wproj", "wrec", "wclock", "wtick",
    "wclock0", "wtick0", "whcompu", "wunglueu", "wpty", "wplit", "wbytes", "wprim", "wity", "wpartial" };
static int WC[W_N], C_LNIL = -1, C_LCONS, C_TRUE, C_FALSE, C_NONE, C_SOME;
static int B_TRUE = -1, B_FALSE;
static void need_bool(void) {
    if (B_TRUE >= 0) return;
    B_FALSE = con_index("false");
    int t = con_index("true");
    if (t < 0 || B_FALSE < 0) { fprintf(stderr, "Bool is missing (lib/core/prelude.cub)\n"); exit(2); }
    __atomic_store_n(&B_TRUE, t, __ATOMIC_RELEASE);
}
static void need_cons(void) {
    if (C_LNIL >= 0) return;
    for (int w = 0; w < W_N; w++) if ((WC[w] = con_index(WNAME[w])) < 0) { fprintf(stderr, "%%view needs the data type View (lib/core/code.cub)\n"); exit(2); }
    C_LCONS = con_index("lcons");
    C_TRUE = con_index("true"); C_FALSE = con_index("false");
    C_NONE = con_index("none"); C_SOME = con_index("some");
    int nil = con_index("lnil");
    if (nil < 0 || C_LCONS < 0 || C_TRUE < 0 || C_FALSE < 0 || C_NONE < 0 || C_SOME < 0) { fprintf(stderr, "the checker's data types are missing\n"); exit(2); }
    __atomic_store_n(&C_LNIL, nil, __ATOMIC_RELEASE);
}
static Port mk(int c, Port a, Port b, Port d, Port e) {
    Port f[4] = { a, b, d, e };
    return con_build(c, f, NULL);
}
static Port list_of(const Port *xs, int n) {
    Port l = con_build(C_LNIL, NULL, NULL);
    for (int k = n - 1; k >= 0; k--) l = mk(C_LCONS, xs[k], l, 0, 0);
    return l;
}
static Port pair_of(Port a, Port b) { Cell *c = cell(V_PAIR, 3); link(a, P(c, 1)); link(b, P(c, 2)); return P(c, 0); }
static Port face_cell(Dnf *f);
static void rule_view(Cell *o, Cell *v) {
    need_cons();
    Port out = o->port[1], r = 0;
    int vk = v->kind;
    bool keep = false;   /* the value itself is the field */
    switch (vk) {
    case V_LAM: case V_CASEF: r = mk(WC[W_LAM], P(v, 0), 0, 0, 0); keep = true; break;
    case V_PLAM: r = mk(WC[W_PLAM], P(v, 0), 0, 0, 0); keep = true; break;
    case V_IV: r = mk(WC[W_IV], P(v, 0), 0, 0, 0); keep = true; break;
    case V_FACE: r = mk(WC[W_FACE], P(v, 0), 0, 0, 0); keep = true; break;
    case V_PAIR: r = mk(WC[W_PAIR], v->port[1], v->port[2], 0, 0); break;
    case V_WTY: r = con_build(WC[W_WORD], NULL, NULL); break;
    case V_ITY: r = con_build(WC[W_ITY], NULL, NULL); break;
    case V_PART: r = mk(WC[W_PART], iv_cell(dnf_retain(v->p)), v->port[1], 0, 0); break;
    case V_PTY: r = mk(WC[W_PTY], word_of(v->a), v->n > 1 ? v->port[1] : word_of(0), 0, 0); break;
    case V_PVAL: r = mk(WC[W_PLIT], word_of(v->x), word_of(v->a), 0, 0); break;
    case V_BYTES: r = mk(WC[W_BYTES], word_of(v->p ? (uint64_t)((Bytes *)v->p)->n : 0), 0, 0, 0); break;
    case V_WORD: r = mk(WC[W_WLIT], P(v, 0), 0, 0, 0); keep = true; break;
    case V_WPLUS: r = mk(WC[W_WPLUS], word_wn(cell_wn(v)), v->port[1], 0, 0); break;
    case V_NWOP: r = mk(WC[W_WOP], word_of((uint64_t)(v->x - O_WEQ)), v->port[1], v->port[2], 0); break;
    case V_NPRIM: r = mk(WC[W_PRIM], word_of((uint64_t)v->x), list_of(&v->port[1], v->n - 1), 0, 0); break;
    case V_NWELIM: {
        /* the motive, when the elimination was written with one (%welimP): some P, else none (the
         * quote reads it: lib/core/check.cub's qneu gives cwelim its motive, the loop's state type) */
        Port mo;
        if (v->n == 5) { Port f[1] = { v->port[4] }; mo = con_build(C_SOME, f, NULL); } else mo = con_build(C_NONE, NULL, NULL);
        r = mk(WC[W_WELIM], v->port[1], v->port[2], v->port[3], mo); break;
    }
    case V_PI: r = mk(WC[W_PI], v->port[1], v->port[2], 0, 0); break;
    case V_SIGMA: r = mk(WC[W_SIGMA], v->port[1], v->port[2], 0, 0); break;
    case V_U: r = mk(WC[W_U], v->port[1], 0, 0, 0); break;
    case V_PATHP: r = mk(WC[W_PATH], v->port[1], v->port[2], v->port[3], 0); break;
    case V_GLUE: r = mk(WC[v->x ? W_HCOMPU : W_GLUE], v->port[1], v->port[2], 0, 0); break;
    case V_GLUEV: r = mk(WC[W_GLUEV], v->port[1], v->port[2], 0, 0); break;
    case V_HCOMP: if (v->n == 4) era(v->port[3]); r = mk(WC[W_HCOMP], v->port[1], v->port[2], 0, 0); break;
    case V_TRANSH: link(transh_expand(v), P(o, 0)); return;
    case V_SYS: {
        int nb = v->n - 1; Dnf *cl = v->p;
        Port *bs = malloc((size_t)(nb + 1) * sizeof(Port));
        for (int k = 0; k < nb; k++) bs[k] = pair_of(face_cell(face_of_clause(cl->c[k])), v->port[k + 1]);
        r = mk(WC[W_SYS], list_of(bs, nb), 0, 0, 0);
        free(bs);
        break;
    }
    case V_DATA: {
        int d = data_checked_index((int)v->a);
        if (!strcmp(DATA[v->a].name, "Level")) { r = con_build(WC[W_LEVEL], NULL, NULL); break; }
        /* clocks and the ticks of a clock: read by their own constructors */
        if (!strcmp(DATA[v->a].name, "Clock") && v->n == 1) { r = con_build(WC[W_CLOCK], NULL, NULL); break; }
        if (!strcmp(DATA[v->a].name, "Tick") && v->n == 2) { r = mk(WC[W_TICK], v->port[1], 0, 0, 0); break; }
        if (d < 0) { for (int k = 1; k < v->n; k++) era(v->port[k]); r = con_build(WC[W_OTHER], NULL, NULL); break; }
        r = mk(WC[W_DATA], word_of((uint64_t)d), list_of(&v->port[1], v->n - 1), 0, 0);
        break;
    }
    case V_CON: {
        /* a constructor of a checked data type, or of Level (as data type 0: levels are read by
         * their constructors only); of any other, nothing the checker reads */
        ConInfo *k = &CON[v->a];
        if (!strcmp(DATA[k->data].name, "Clock")) { r = con_build(WC[W_CLOCK0], NULL, NULL); break; }
        if (!strcmp(DATA[k->data].name, "Tick")) { r = con_build(WC[W_TICK0], NULL, NULL); break; }
        int d = data_checked_index(k->data);
        if (d < 0 && strcmp(DATA[k->data].name, "Level")) { for (int q = 1; q < v->n; q++) era(v->port[q]); r = con_build(WC[W_OTHER], NULL, NULL); break; }
        r = mk(WC[W_CON], word_of((uint64_t)(d < 0 ? 0 : d)), word_of((uint64_t)k->index), list_of(&v->port[1], k->nfields), list_of(&v->port[1 + k->nfields], k->ndims));
        break;
    }
    case V_NVAR:
        if (v->a & LVL_TAG) r = mk(WC[W_VAR], word_of(v->a & ~LVL_TAG), v->port[1], 0, 0);
        else if ((v->ty & TY_COFIX) && global_checked((long)v->a) >= 0)
            /* the stuck head of a checked copattern definition: its index in the signature, its type */
            r = mk(WC[W_COFIX], word_of((uint64_t)global_checked((long)v->a)), v->port[1], 0, 0);
        else { era(v->port[1]); r = con_build(WC[W_OTHER], NULL, NULL); }
        break;
    case V_NPROJ: {
        FldInfo *fl = &FLD[v->a];
        int d = data_checked_index(fl->data);
        if (d < 0) { for (int q = 1; q < v->n; q++) era(v->port[q]); r = con_build(WC[W_OTHER], NULL, NULL); break; }
        r = mk(WC[W_PROJ], word_of((uint64_t)d), word_of((uint64_t)fl->index), v->port[1], v->port[2]);
        break;
    }
    case V_REC: {
        int d = data_checked_index((int)v->a);
        if (d < 0) { for (int q = 1; q < v->n; q++) era(v->port[q]); r = con_build(WC[W_OTHER], NULL, NULL); break; }
        r = mk(WC[W_REC], word_of((uint64_t)d), list_of(&v->port[1], v->n - 1), 0, 0);
        break;
    }
    case V_NAPP: r = mk(WC[W_APP], v->port[1], v->port[2], v->port[3], 0); break;
    case V_NFST: r = mk(WC[W_FST], v->port[1], v->port[2], 0, 0); break;
    case V_NSND: r = mk(WC[W_SND], v->port[1], v->port[2], 0, 0); break;
    case V_NPAPP: {
        Cell *iv = cell(V_IV, 1); iv->p = dnf_retain(v->p);
        if (v->x) { era(v->port[2]); era(v->port[3]); }
        r = mk(WC[W_PAPP], v->port[1], P(iv, 0), v->port[v->n - 1], 0);
        break;
    }
    case V_NCOMP: r = mk(WC[W_COMP], v->port[1], v->port[2], v->port[3], v->port[4]); break;
    case V_NUNGLUE: r = mk(WC[v->x ? W_UNGLUEU : W_UNGLUE], v->port[1], v->port[2], v->port[3], 0); break;
    case V_NCASE: {
        /* the elimination as a function of its scrutinee: its table and environment (its motive,
         * when it has one, is the table's over that environment: %motive) */
        CaseTab *tb = v->p;
        if (data_checked_index(tb->d) < 0) { for (int q = 1; q < v->n; q++) era(v->port[q]); r = con_build(WC[W_OTHER], NULL, NULL); break; }
        int mb = v->x ? 1 : 0, ne = v->n - 3 - mb;
        Cell *f = cell(V_CASEF, ne + 1); f->x = 0; f->p = tb;
        if (mb) era(v->port[2]);
        for (int e = 0; e < ne; e++) link(v->port[2 + mb + e], P(f, 1 + e));
        r = mk(WC[W_ELIM], word_of((uint64_t)data_checked_index(tb->d)), v->port[1], P(f, 0), v->port[v->n - 1]);
        break;
    }
    case V_NTRANS: {
        int np = v->n - 3;
        if (data_checked_index((int)v->a) < 0) { for (int q = 1; q < v->n; q++) era(v->port[q]); r = con_build(WC[W_OTHER], NULL, NULL); break; }
        Cell *fc = cell(V_FACE, 1); fc->p = dnf_retain(v->p);
        Port l = list_of(&v->port[2], np);
        Port f[5] = { word_of((uint64_t)data_checked_index((int)v->a)), P(fc, 0), l, v->port[1], v->port[v->n - 1] };
        r = con_build(WC[W_TRANS], f, NULL);
        break;
    }
    default:
        for (int k = 1; k < v->n; k++) era(v->port[k]);
        r = con_build(WC[W_OTHER], NULL, NULL);
        break;
    }
    link(r, out);
    cell_free(o);
    if (!keep) cell_free(v);
}

/* an induction hypothesis: the elimination of its field (1=field [2=motive] environment) */
static Port case_of_susp(Cell *v) {
    Cell *c = cell(O_CASE, v->n); c->x = v->x; c->p = v->p;
    for (int k = 1; k < v->n; k++) link(v->port[k], P(c, k - 1));
    if (v->key) { c->key = key_retain(v->key); memo_register(c); }
    cell_free(v);
    return P(c, c->n - 1);
}
/* a word's hypothesis: the elimination of its word (1=z 2=s 3=w) */
static Port welim_of_wsusp(Cell *v) {
    Cell *e = cell(O_WELIM, v->n);
    link(v->port[3], P(e, 0)); link(v->port[1], P(e, 1)); link(v->port[2], P(e, 2));
    if (v->n == 5) link(v->port[4], P(e, 3));
    cell_free(v);
    return P(e, v->n - 1);
}
/* words: a literal #k, k successors of a neutral (V_WPLUS), or a neutral. wsplit reads a word as
 * (k, base), base 0 for a literal; it returns the cell to free (the literal or V_WPLUS) */
static Cell *wsplit(Cell *c, WN *k, Port *base) {
    if (c->kind == V_WORD) { *k = cell_wn(c); *base = 0; return c; }
    if (c->kind == V_WPLUS) { *k = cell_wn(c); *base = c->port[1]; return c; }
    *k = wn_small(0); *base = P(c, 0); return NULL;
}
/* k ⊕ base (taking k) */
static Port wplus(WN k, Port base) {
    if (!base) return word_wn(k);
    if (wn_zero(&k)) return base;
    Cell *w = cell(V_WPLUS, 2); w->a = k.s; w->p = k.b; link(base, P(w, 1));
    return P(w, 0);
}
static Port wstuck(int op, Port a, Port b) {
    Cell *n = cell(V_NWOP, 3); n->x = (uint32_t)op;
    link(a, P(n, 1)); link(b, P(n, 2));
    return P(n, 0);
}

/* a primitive operation on a neutral (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.2 “B12 — Representations as a construct”): a neutral of its own, the arguments kept, which a
 * substitution runs again and the checker views (wprim) and compares */
static Port nprim(int cls, int sb, Port *args, int n) {
    Cell *c = cell(V_NPRIM, n + 1); c->x = (uint32_t)(cls << 8 | sb);
    for (int k = 0; k < n; k++) link(args[k], P(c, k + 1));
    return P(c, 0);
}
static Port pval_of(int kind, uint64_t bits) { Cell *c = cell(V_PVAL, 1); c->x = (uint32_t)kind; c->a = bits; return P(c, 0); }
static Port bytes_cell(Bytes *b) { Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b; return P(c, 0); }
static int po_kind(int op) { return op <= PO_I64LT ? PT_I64 : op <= PO_U32LT ? PT_U32 : op <= PO_F64LT ? PT_F64 : op <= PO_U64SHR ? PT_U64 : PT_I64; }   /* a binary operation's operands */
/* an operation on a value it has no rule for (an ill-typed term, run unchecked): the result is
 * unknown, the operands erased — the machine does not stop, and a checker reading the value of a
 * term it rejected reads ? there */
/* Neutral interval atoms. A term of type I stuck on a variable (a call returning an interval, its
 * argument neutral) is read by an interval operation as a dimension of its own, one per key (the
 * neutral's name: a variable, a call on named arguments), so the same neutral is the same atom
 * wherever it is read. Faces over atoms are faces over dimensions: a clause assuming an atom 1 is a
 * substitution of it, and conversion under an assumed face is conversion under that substitution.
 * A neutral with no key gets an atom of its own. */
static uint64_t next_var;   /* fresh variables' levels (%var, %fresh) */
typedef struct { Key *k; Dim d; } AtomE;
static Dim *ATD; static size_t ATDn, ATDcap;   /* the atoms' dimensions */
static AtomE *AT; static size_t ATn, ATcap;
static pthread_mutex_t atommu = PTHREAD_MUTEX_INITIALIZER;
static Dnf *atom_of(Cell *v) {
    Key *k = eff_key(v);
    if (!k) return iv_atom(fresh_dim());
    if (!net_solo) pthread_mutex_lock(&atommu);
    Dim d = 0;
    if (ATcap) for (size_t i = k->h & (ATcap - 1); AT[i].k; i = (i + 1) & (ATcap - 1)) if (AT[i].k == k) { d = AT[i].d; break; }
    if (!d) {
        if ((ATn + 1) * 2 > ATcap) {
            size_t nc = ATcap ? ATcap * 2 : 256; AtomE *N = calloc(nc, sizeof(AtomE));
            for (size_t j = 0; j < ATcap; j++) if (AT[j].k) { size_t i = AT[j].k->h & (nc - 1); while (N[i].k) i = (i + 1) & (nc - 1); N[i] = AT[j]; }
            free(AT); AT = N; ATcap = nc;
        }
        d = fresh_dim();
        size_t i = k->h & (ATcap - 1); while (AT[i].k) i = (i + 1) & (ATcap - 1);
        AT[i].k = key_retain(k); AT[i].d = d; ATn++;
        if (ATDn == ATDcap) { ATDcap = ATDcap ? ATDcap * 2 : 64; ATD = realloc(ATD, ATDcap * sizeof(Dim)); }
        ATD[ATDn] = d; __atomic_store_n(&ATDn, ATDn + 1, __ATOMIC_RELEASE);
    }
    if (!net_solo) pthread_mutex_unlock(&atommu);
    key_release(k);
    return iv_atom(d);
}
/* is a dimension an atom? (atoms are few: a scan) */
static bool is_atom_dim(Dim d) {
    if (!__atomic_load_n(&ATDn, __ATOMIC_ACQUIRE)) return false;
    bool r = false;
    if (!net_solo) pthread_mutex_lock(&atommu);
    for (size_t i = 0; i < ATDn && !r; i++) r = ATD[i] == d;
    if (!net_solo) pthread_mutex_unlock(&atommu);
    return r;
}
/* the part of a substitution that sends atoms to endpoints, as a face clause (NULL if none) */
static Dnf *atom_clause(const Subst *s) {
    Dnf *f = NULL;
    for (uint32_t k = 0; k < s->n; k++) {
        if (!is_atom_dim(s->d[k])) continue;
        const Dnf *r = s->v[k]; int e;
        if (dnf_is_zero(r)) e = 0; else if (dnf_is_one(r)) e = 1; else continue;
        Dnf *a = iv_atom(s->d[k]), *l = face_eq(a, e); dnf_release(a);
        if (!f) f = l; else { Dnf *m = dnf_meet(f, l, true); dnf_release(f); dnf_release(l); f = m; }
    }
    return f;
}
/* the atom a neutral was read as, if it has been (0 if not) */
static Dim atom_find(const Cell *v) {
    if (!v->key || !ATcap) return 0;
    Dim d = 0;
    if (!net_solo) pthread_mutex_lock(&atommu);
    for (size_t i = v->key->h & (ATcap - 1); AT[i].k; i = (i + 1) & (ATcap - 1)) if (AT[i].k == v->key) { d = AT[i].d; break; }
    if (!net_solo) pthread_mutex_unlock(&atommu);
    return d;
}
/* does the operation read its principal as an interval? */
static bool iv_reader(const Cell *o) {
    switch (o->kind) {
    case O_NEG: case O_FEQ: case O_PAPPR: case O_MKPART: case O_IEQ: case O_IEQB: return true;
    case O_BIN: case O_BIN2: return o->x == B_MEET || o->x == B_JOIN;
    default: return false;
    }
}
static void stuck(Cell *o, Cell *v) {
    for (int k = 1; k < v->n; k++) era(v->port[k]);
    for (int k = 1; k < o->n - 1; k++) era(o->port[k]);
    link(unk(), o->port[o->n - 1]);
    cell_free(o); cell_free(v);
}


/* CUBICAL_TRACE: one line per interaction during a (show ...) reading — the operation, the value
 * it met, and what each names (the definition a λ or thunk was compiled in, a constructor, a global,
 * a word, the data type an elimination is over) */
FILE *trace_out;
static const char *const KIND_NAME[K_END] = {
    [V_LAM] = "LAM",
    [V_PAIR] = "PAIR",
    [V_PI] = "PI",
    [V_SIGMA] = "SIGMA",
    [V_U] = "U",
    [V_PATHP] = "PATHP",
    [V_PLAM] = "PLAM",
    [V_IV] = "IV",
    [V_FACE] = "FACE",
    [V_SYS] = "SYS",
    [V_ITY] = "ITY",
    [V_PART] = "PART",
    [V_GLUE] = "GLUE",
    [V_GLUEV] = "GLUEV",
    [V_NVAR] = "NVAR",
    [V_NAPP] = "NAPP",
    [V_NFST] = "NFST",
    [V_NSND] = "NSND",
    [V_NPAPP] = "NPAPP",
    [V_NCOMP] = "NCOMP",
    [V_NUNGLUE] = "NUNGLUE",
    [V_NCASE] = "NCASE",
    [V_NTRANS] = "NTRANS",
    [V_NPROJ] = "NPROJ",
    [V_REC] = "REC",
    [V_GLOBAL] = "GLOBAL",
    [V_UNK] = "UNK",
    [V_DATA] = "DATA",
    [V_CON] = "CON",
    [V_HCOMP] = "HCOMP",
    [V_TRANSH] = "TRANSH",
    [V_CASEF] = "CASEF",
    [V_REF] = "REF",
    [V_WORD] = "WORD",
    [V_WSUSP] = "WSUSP",
    [V_WTY] = "WTY",
    [V_PTY] = "PTY", [V_PVAL] = "PVAL", [V_BYTES] = "BYTES",
    [V_THUNK] = "THUNK", [V_DEFER] = "DEFER",
    [V_WPLUS] = "WPLUS",
    [V_NWOP] = "NWOP",
    [V_NPRIM] = "NPRIM",
    [V_NWELIM] = "NWELIM",
    [V_SUSP] = "SUSP",
    [O_APP] = "APP",
    [O_FORCE] = "FORCE",
    [O_FST] = "FST",
    [O_SND] = "SND",
    [O_PAPPR] = "PAPPR",
    [O_PAPPP] = "PAPPP",
    [O_SUBST] = "SUBST",
    [O_DUP] = "DUP",
    [O_ERA] = "ERA",
    [O_COMP] = "COMP",
    [O_COMPA] = "COMPA",
    [O_COMPH] = "COMPH", [O_COMPW] = "COMPW",
    [O_MKPLAM] = "MKPLAM",
    [O_MKGLUE] = "MKGLUE",
    [O_MKGLUEV] = "MKGLUEV",
    [O_UNGLUE] = "UNGLUE",
    [O_UNGLUE2] = "UNGLUE2",
    [O_FACEOF] = "FACEOF",
    [O_FNEG] = "FNEG",
    [O_NEG] = "NEG",
    [O_BIN] = "BIN",
    [O_BIN2] = "BIN2",
    [O_FEQ] = "FEQ",
    [O_FORALL] = "FORALL",
    [O_SYSCONS] = "SYSCONS",
    [O_SYSCONS2] = "SYSCONS2",
    [O_SYSMAP] = "SYSMAP",
    [O_TPART] = "TPART",
    [O_CONCHK] = "CONCHK", [O_CONW] = "CONW",
    [O_HCOMP] = "HCOMP",
    [O_HCM1] = "HCM1",
    [O_HCM2] = "HCM2",
    [O_HCM3] = "HCM3",
    [O_TRANSD1] = "TRANSD1",
    [O_TRANSD2] = "TRANSD2",
    [O_TRANSH] = "TRANSH",
    [O_CASE] = "CASE",
    [O_VIEW] = "VIEW",
    [O_MKVAR] = "MKVAR",
    [O_MKDIM] = "MKDIM",
    [O_MKFRESH] = "MKFRESH",
    [O_FRESHT] = "FRESHT",
    [O_IEQ] = "IEQ",
    [O_IEQB] = "IEQB",
    [O_FEQL] = "FEQL",
    [O_FEQLB] = "FEQLB",
    [O_CLAUSES] = "CLAUSES",
    [O_DNF] = "DNF",
    [O_MOTIVE] = "MOTIVE",
    [O_RESTRICT] = "RESTRICT",
    [O_GLOB] = "GLOB",
    [O_ABS] = "ABS",
    [O_NTYPE] = "NTYPE",
    [O_CALL] = "CALL",
    [O_WSUCC] = "WSUCC",
    [O_PTY] = "PTY", [O_BYTESTY] = "BYTESTY", [O_PVAL] = "PVAL", [O_PVAL2] = "PVAL2", [O_POP1] = "POP1", [O_POP2] = "POP2", [O_POP2B] = "POP2B", [O_BYTES] = "BYTES", [O_LOAD] = "LOAD", [O_LOADB] = "LOADB", [O_STORE] = "STORE", [O_STOREB] = "STOREB", [O_STOREC] = "STOREC", [O_BLEN] = "BLEN", [O_BEQ] = "BEQ", [O_BEQB] = "BEQB", [O_PTROF] = "PTROF", [O_REGAT] = "REGAT", [O_REGATA] = "REGATA", [O_FFI] = "FFI", [O_NATIVE] = "NATIVE", [O_DEEP] = "DEEP", [O_DEEPC] = "DEEPC",
    [O_PIMULT] = "PIMULT", [O_PIM] = "PIM", [O_FMULT] = "FMULT", [O_FMULT2] = "FMULT2", [O_FMULT3] = "FMULT3", [O_LINEAR] = "LINEAR", [O_GSTEPS] = "GSTEPS", [O_GCODE] = "GCODE", [O_DNIDX] = "DNIDX", [O_DIHT] = "DIHT", [O_DIHN] = "DIHN", [O_GUSAGE] = "GUSAGE", [O_GUSAGE2] = "GUSAGE2",
    [O_WEQ] = "WEQ",
    [O_WLT] = "WLT",
    [O_WADD] = "WADD",
    [O_WMAX] = "WMAX",
    [O_WSUB] = "WSUB", [O_WMUL] = "WMUL", [O_WDIV] = "WDIV", [O_WMOD] = "WMOD", [O_WAND] = "WAND", [O_WOR] = "WOR", [O_WXOR] = "WXOR", [O_WSHL] = "WSHL", [O_WSHR] = "WSHR",
    [O_WOP2] = "WOP2",
    [O_WELIM] = "WELIM",
    [O_PEND] = "PEND",
    [O_CLOS] = "CLOS",
    [O_CLOS2] = "CLOS2",
    [O_BRANCH] = "BRANCH",
    [O_BRANCHC] = "BRANCHC",
    [O_BRANCHL] = "BRANCHL",
    [O_FWD] = "FWD",
    [O_SHARE] = "SHARE",
    [O_ROOT] = "ROOT",
    [O_PROJ] = "PROJ",
    [O_UNFOLD] = "UNFOLD",
    [O_TICKAPP] = "TICKAPP",
    [O_DAPPLY] = "DAPPLY",
};

static void trace_side(Cell *c) {
    switch (c->kind) {
    case V_LAM: case V_THUNK: { Record *r = c->p; if (r && r->where >= 0 && r->where < prof_n) fprintf(trace_out, "<%s>", prof_name[r->where]); break; }
    case V_SUSP: case V_CASEF: case V_NCASE: case O_CASE: { CaseTab *t = c->p; if (t) fprintf(trace_out, "<%s>", DATA[t->d].name); break; }
    case V_GLOBAL: fprintf(trace_out, "<%s>", name_of((long)c->a)); break;
    case V_CON: fprintf(trace_out, "<%s>", CON[c->a].name); break;
    case V_WORD: if (!c->p) fprintf(trace_out, "<#%llu>", (unsigned long long)c->a); break;
    default: break;
    }
}
static void trace_step(Cell *o, Cell *v) {
    fprintf(trace_out, "%ld %s", stat_steps, KIND_NAME[o->kind] ? KIND_NAME[o->kind] : "?");
    trace_side(o);
    fprintf(trace_out, " · %s", KIND_NAME[v->kind] ? KIND_NAME[v->kind] : "?");
    trace_side(v);
    fputc('\n', trace_out);
}
/* ---------------- the rules ---------------- */
const char *kind_name(int k) { return k >= 0 && k < K_END && KIND_NAME[k] ? KIND_NAME[k] : "?"; }
static void interact(Cell *o, Cell *v) {
    if (o->kind == O_FWD) {
        /* a read waiting for its value (lazy_copy): not a step of its own. An unbuilt value is
         * built, as the reader would have built it in place; a value is passed on to where it is
         * held, and the waiting reader gets its one-level copy */
        if (unbuilt(v->kind)) { link(unbuilt_build(v), P(o, 0)); return; }
        Port c = copy_top(o->key, v);
        link(c, o->port[2]);
        link(P(v, 0), o->port[1]);
        cell_free(o);
        return;
    }
    if (o->kind == O_SHARE) {
        /* a part of a read value that arrived after the read (copy_top): shared exactly as the
         * read would have shared it had it been there; not a step of its own */
        rule_dup(o, v);
        return;
    }
    { Local *l = local(); l->steps++; l->kind[o->kind]++;
      /* CUBICAL_PROGRESS=n: every n steps of a thread, its steps and the cells it holds (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”:
       * its measurements of reads too large to finish) */
      static long prog = -1; if (prog < 0) { const char *e = getenv("CUBICAL_PROGRESS"); prog = e ? atol(e) : 0; }
      if (prog > 0 && l->steps % prog == 0) { extern long rb_nodes; fprintf(stderr, "[progress] %ld steps, %ld cells, %ld read back\n", l->steps, (long)l->cells, rb_nodes); } }
    if (prof_on && (o->kind == O_SUBST || o->kind == O_ERA || o->kind == O_DUP || o->kind == O_PAPPP)) ADD(prof_pair[o->kind == O_SUBST ? 0 : o->kind == O_ERA ? 1 : o->kind == O_DUP ? 2 : 3][v->kind & 255], 1);
    if (trace_out) trace_step(o, v);
    if (prof_on) {
        long a = (long)o->a;
        if (o->kind == O_DUP || o->kind == O_ERA) {
            long *rec = o->kind == O_DUP ? prof_odup : prof_oera, *rul = o->kind == O_DUP ? prof_rdup : prof_rera;
            if (a > 0 && a < 65536) ADD(rec[a], 1); else if (a < 0 && -a < 256) ADD(rul[-a], 1); else ADD(rec[0], 1);
        }
        dup_origin = -(long)o->kind;
    }
    switch (o->kind) {
    case O_ERA: rule_era(o, v); return;
    case O_DUP: rule_dup(o, v); return;
    case O_SUBST: rule_subst(o, v); return;
    case O_MKPLAM: rule_mkplam(o, v); return;
    default: break;
    }
    if (v->kind == V_GLOBAL && o->kind == O_CALL) {
        long g = (long)v->a;
        Record *r = global_nary(g);
        int na = o->n - 2;
        if (r && r->nparam <= na) {
            /* all the parameters: the body at once; any further arguments applied to it */
            if (prof_on) ADD(prof_count[r->where], 1);
            Box *bx; Port sh = memo_begin(o, &bx, true);
            if (sh && !bx) { for (int k = 0; k < na; k++) era(o->port[1 + k]); link(sh, o->port[o->n - 1]); cell_free(o); cell_free(v); return; }
            Port res = record_build_n(r, &o->port[1], NULL);
            for (int k = r->nparam; k < na; k++) res = app2(res, o->port[1 + k]);
            link(memo_end(sh, bx, res), o->port[o->n - 1]);
            cell_free(o); cell_free(v);
            return;
        }
        /* otherwise applications, one argument at a time */
        Port f = P(v, 0);
        for (int k = 0; k < na; k++) f = app2(f, o->port[1 + k]);
        link(f, o->port[o->n - 1]);
        cell_free(o);
        return;
    }
    if (v->kind == V_GLOBAL) {
        /* a definition is used: its value is copied in */
        Port r = global_copy((long)v->a, v->key);
        if (!r) { global_park((long)v->a, o, v); return; }
        link(r, P(o, 0));
        cell_free(v);
        return;
    }
    /* an unbuilt subterm meeting an operation is built (an explicit force of a hypothesis too) */
    if (unbuilt(v->kind)) { link(unbuilt_build(v), P(o, 0)); return; }
    if (v->kind == V_REF) {
        /* the value is copied out of its box */
        link(ref_read(v), P(o, 0));
        cell_free(v);
        return;
    }
    int vk = v->kind;
    if (is_neutral(vk) && iv_reader(o)) {
        /* a neutral read as an interval: its atom */
        Cell *iv = cell(V_IV, 1); iv->p = atom_of(v);
        for (int k = 1; k < v->n; k++) era(v->port[k]);
        cell_free(v); v = iv; vk = V_IV;
    }
    if (vk == V_UNK && o->kind != O_VIEW && o->kind != O_CLOS && o->kind != O_CLOS2 && o->kind != O_PEND) {
        /* an eliminator of an unknown type gives an unknown type */
        for (int k = 1; k < o->n - 1; k++) era(o->port[k]);
        link(P(v, 0), out_of(o));
        cell_free(o);
        return;
    }
    switch (o->kind) {
    case O_APP:
        if (vk == V_CASEF) {
            /* the elimination as a function, applied: the case on the argument */
            Cell *c = cell(O_CASE, v->n + 1); c->x = v->x; c->p = v->p;
            mv(o, 1, P(c, 0));
            for (int k = 1; k < v->n; k++) link(v->port[k], P(c, k));
            link(P(c, c->n - 1), o->port[2]);
            cell_free(o); cell_free(v);
            return;
        }
        if (vk == V_LAM && ((Record *)v->p)->nparam == 1) {
            if (prof_on) ADD(prof_count[((Record *)v->p)->where], 1);
            Box *bx; Port sh = memo_begin(o, &bx, true);
            if (sh && !bx) { era(o->port[1]); era(P(v, 0)); link(sh, o->port[2]); cell_free(o); return; }
            Port r = record_build(v->p, o->port[1], &v->port[1]);
            link(memo_end(sh, bx, r), o->port[2]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else if (vk == V_PLAM) {
            /* a line applied as a function of the interval: its path application */
            Cell *pr = cell(O_PAPPR, 3);
            mv(o, 1, P(pr, 0)); link(P(v, 0), P(pr, 1)); link(P(pr, 2), o->port[2]);
            cell_free(o);
        } else {
            /* stuck: a neutral application (its type computed from the spine when needed). A stuck
             * call of a copattern definition stays one, and is named by its head and argument */
            if (is_neutral(vk) && (v->ty & TY_TICK) && (vk == V_NVAR ? global_tickar((long)v->a) : (long)v->a) == 1) {
                /* the argument a tick-unfolding definition unfolds at: its value decides */
                Cell *t = cell(O_TICKAPP, 3);
                mv(o, 1, P(t, 0)); link(P(v, 0), P(t, 1)); link(P(t, 2), o->port[2]);
                cell_free(o);
                return;
            }
            Cell *n = cell(V_NAPP, 4);
            if (is_neutral(vk) && (v->ty & TY_COFIX)) {
                n->ty = TY_COFIX;
                if (v->ty & TY_TICK) { n->ty |= TY_TICK; n->a = (uint64_t)((vk == V_NVAR ? global_tickar((long)v->a) : (long)v->a) - 1); }
                if (v->key) { Key *ak = key_of_port(o->port[1]); n->key = key2(K_APP, 0, NULL, v->key, ak); if (ak) key_release(ak); }
            }
            mv(o, 1, P(n, 2)); link(unk(), P(n, 3));
            link(P(v, 0), P(n, 1));
            link(P(n, 0), o->port[2]);
            cell_free(o);
        }
        return;
    case O_FORCE:
        if (vk == V_LAM && ((Record *)v->p)->nparam == 0) {
            if (prof_on) ADD(prof_count[((Record *)v->p)->where], 1);
            link(record_build(v->p, 0, &v->port[1]), o->port[1]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else { link(P(v, 0), o->port[1]); cell_free(o); }
        return;
    case O_FST: case O_SND:
        if (vk == V_PAIR) {
            int keep = o->kind == O_FST ? 1 : 2;
            link(v->port[keep], o->port[1]); era(v->port[3 - keep]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else neutral_proj(o, v);
        return;
    case O_PAPPR: {
        if (vk != V_IV) { stuck(o, v); return; }
        Dnf *r = v->p;
        if (o->x && (dnf_is_zero(r) || dnf_is_one(r))) {
            /* an endpoint: answered without the path */
            int z = dnf_is_zero(r);
            link(o->port[z ? 2 : 3], o->port[4]);
            era(o->port[1]); era(o->port[z ? 3 : 2]);
        } else {
            Cell *pp = cell(O_PAPPP, o->x ? 4 : 2); pp->x = o->x; pp->p = dnf_retain(r);
            mv(o, 1, P(pp, 0));
            if (o->x) { mv(o, 2, P(pp, 1)); mv(o, 3, P(pp, 2)); }
            link(P(pp, pp->n - 1), out_of(o));
        }
        cell_free(o); cell_free(v);
        return;
    }
    case O_PAPPP:
        if (vk == V_LAM || vk == V_CASEF) {
            /* a function of the interval applied at a formula: its application */
            Cell *ap = cell(O_APP, 3);
            link(P(v, 0), P(ap, 0)); link(iv_cell(dnf_retain(o->p)), P(ap, 1)); link(P(ap, 2), out_of(o));
            if (o->x) { era(o->port[1]); era(o->port[2]); }
            cell_free(o);
            return;
        }
        if (vk == V_PLAM) {
            Subst *t = subst_single((Dim)v->a, o->p);
            link(sub(v->port[1], t), out_of(o));
            subst_release(t);
            if (o->x) { era(o->port[1]); era(o->port[2]); }
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else {
            Dnf *r = o->p;
            if (!o->x && (is_neutral(vk) || (vk == V_NWELIM && v->n == 5)) && (dnf_is_zero(r) || dnf_is_one(r))) {
                /* an endpoint of a neutral path, read from its type (computed from a copy of the
                 * spine): t 0 = u0 for t : Path A u0 u1 */
                Port p1, p2; split2(P(v, 0), &p1, &p2);
                Cell *e = cell(O_PEND, 3); e->p = dnf_retain(r);
                Cell *nt = cell(O_NTYPE, 2); link(p1, P(nt, 0)); link(P(nt, 1), P(e, 0));
                link(p2, P(e, 1)); link(P(e, 2), out_of(o));
                cell_free(o);
                return;
            }
            /* stuck: a neutral path application */
            Cell *n = cell(V_NPAPP, o->x ? 5 : 3); n->x = o->x; n->p = dnf_retain(r);
            if (is_neutral(vk) && (v->ty & TY_COFIX)) {
                n->ty = TY_COFIX;
                /* a tick-unfolding definition may take an interval before its tick (pfix) */
                long ta = (v->ty & TY_TICK) ? (vk == V_NVAR ? global_tickar((long)v->a) : (long)v->a) : 0;
                if (ta > 1) { n->ty |= TY_TICK; n->a = (uint64_t)(ta - 1); }
                if (v->key) { Key *rk = key0(K_IV, 0, r); n->key = key2(K_PAPP, 0, NULL, v->key, rk); if (rk) key_release(rk); }
            }
            link(unk(), P(n, n->n - 1));
            link(P(v, 0), P(n, 1));
            if (o->x) { mv(o, 1, P(n, 2)); mv(o, 2, P(n, 3)); }
            link(P(n, 0), out_of(o));
            cell_free(o);
        }
        return;
    case O_COMP:
        if (vk == V_SYS) {
            /* the tube is partial: look at the family */
            Cell *ca = cell(O_COMPA, 4);
            mv(o, 1, P(ca, 0)); link(P(v, 0), P(ca, 1)); mv(o, 2, P(ca, 2)); link(P(ca, 3), o->port[3]);
        } else {
            /* the tube covers: its end */
            Cell *pr = cell(O_PAPPR, 3);
            link(iv_cell(dnf_one()), P(pr, 0)); link(P(v, 0), P(pr, 1)); link(P(pr, 2), o->port[3]);
            era(o->port[1]); era(o->port[2]);
        }
        cell_free(o);
        return;
    case O_COMPA:
        if (vk == V_PLAM) {
            /* the family at its own fresh dimension; its head decides */
            Cell *ch = cell(O_COMPH, 4); ch->a = v->a;
            link(v->port[1], P(ch, 0)); mv(o, 1, P(ch, 1)); mv(o, 2, P(ch, 2)); link(P(ch, 3), o->port[3]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else {
            /* a neutral family: a neutral composition, of type the family at 1 */
            Cell *n = cell(V_NCOMP, 5);
            link(P(v, 0), P(n, 1)); mv(o, 1, P(n, 2)); mv(o, 2, P(n, 3)); link(unk(), P(n, 4));
            link(P(n, 0), o->port[3]);
            cell_free(o);
        }
        return;
    case O_COMPH: {
        Dim i = (Dim)o->a;
        Port u = o->port[1], a0 = o->port[2], out = o->port[3];
        Port args[5];
        const char *clause = NULL; int n = 0;
        switch (vk) {
        case V_PI: case V_SIGMA:
            args[0] = mkplam_of(v->port[1], i); args[1] = mkplam_of(v->port[2], i); args[2] = u; args[3] = a0; n = 4;
            clause = vk == V_PI ? "%comp-pi" : "%comp-sigma"; break;
        case V_PATHP:
            args[0] = mkplam_of(v->port[1], i); args[1] = mkplam_of(v->port[2], i); args[2] = mkplam_of(v->port[3], i);
            args[3] = u; args[4] = a0; n = 5; clause = "%comp-path"; break;
        case V_U: era(v->port[1]); args[0] = u; args[1] = a0; n = 2; clause = "%comp-u"; break;
        case V_WTY: {
            /* a word: no path in Word is other than constant, so a composition whose tube is empty is
             * its base (a partial tube with faces stays neutral: on a face it is the tube's end) */
            Cell *cw = cell(O_COMPW, 4);
            link(u, P(cw, 0)); link(a0, P(cw, 1)); link(mkplam_of(P(v, 0), i), P(cw, 2)); link(P(cw, 3), out);
            cell_free(o);
            return;
        }
        case V_DATA: {
            /* CHM: comp = hcomp after transport; %comp-D is generated by the declaration */
            int np = v->n - 1;
            Port *da = malloc((size_t)(np + 3) * sizeof(Port));
            for (int k = 0; k < np; k++) da[k] = mkplam_of(v->port[k + 1], i);
            da[np] = u; da[np + 1] = a0;
            link(apply_gid(DATA[v->a].comp, da, np + 2), out);
            free(da);
            cell_free(o); cell_free(v);
            return;
        }
        case V_GLUE:
            /* a Glue type, or a composition of types (its faces lines, unglued by transport) */
            args[0] = mkplam_of(v->port[1], i); args[1] = mkplam_of(v->port[2], i); args[2] = u; args[3] = a0; n = 4;
            clause = v->x ? "%comp-gluel" : "%comp-glue"; break;
        case V_SYS: {
            /* a system of families: each branch whose clause holds along the
             * whole line, ∀i, composes in its own family */
            int nb = v->n - 1; Dnf *cl = v->p;
            int keep = 0;
            for (int k = 0; k < nb; k++) if (!clause_mentions(cl->c[k], i)) keep++;
            Cell *ns = cell(V_SYS, keep + 1);
            Clause **kc = malloc((size_t)(keep + 1) * sizeof(Clause *));
            Port *uc = malloc((size_t)(keep + 1) * sizeof(Port)), *ac = malloc((size_t)(keep + 1) * sizeof(Port));
            int q = 0;
            for (int k = 0; k < nb; k++) if (!clause_mentions(cl->c[k], i)) kc[q++] = cl->c[k];
            Dnf *kl = clause_list(kc, (uint32_t)keep); free(kc);
            ns->p = kl;
            split_restrict(u, kl, keep, uc); split_restrict(a0, kl, keep, ac);
            q = 0;
            for (int k = 0; k < nb; k++) {
                if (clause_mentions(cl->c[k], i)) { era(v->port[k + 1]); continue; }
                /* the branch's composition, and the line it composes in, deferred until the branch is read (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”) */
                Cell *m = defer_cell(O_MKPLAM, 1); m->a = i; link(v->port[k + 1], P(m, 1));
                Cell *c = defer_cell(O_COMP, 3);
                link(uc[q], P(c, 1)); link(P(m, 0), P(c, 2)); link(ac[q], P(c, 3)); link(P(c, 0), P(ns, q + 1));
                q++;
            }
            free(uc); free(ac);
            link(P(ns, 0), out);
            cell_free(o); cell_free(v);
            return;
        }
        default: {
            /* a neutral head: a neutral composition, of type the head at i = 1 */
            Cell *nc = cell(V_NCOMP, 5);
            link(mkplam_of(P(v, 0), i), P(nc, 1)); link(u, P(nc, 2)); link(a0, P(nc, 3)); link(unk(), P(nc, 4)); link(P(nc, 0), out);
            cell_free(o);
            return;
        }
        }
        link(apply_global(clause, args, n), out);
        cell_free(o); cell_free(v);
        return;
    }
    case O_COMPW:
        if (vk == V_SYS && v->n == 1) { link(o->port[1], o->port[3]); era(o->port[2]); cell_free(v); }
        else {
            Cell *nc = cell(V_NCOMP, 5);
            mv(o, 2, P(nc, 1)); link(P(v, 0), P(nc, 2)); mv(o, 1, P(nc, 3)); link(unk(), P(nc, 4)); link(P(nc, 0), o->port[3]);
        }
        cell_free(o);
        return;
    case O_MKGLUE:
        if (vk == V_SYS) { Cell *g = cell(V_GLUE, 3); g->x = o->x; mv(o, 1, P(g, 1)); link(P(v, 0), P(g, 2)); link(P(g, 0), o->port[2]); }
        else if (o->x) {
            /* a composition of types whose tube covers: the tube's end */
            Cell *pr = cell(O_PAPPR, 3);
            link(iv_cell(dnf_one()), P(pr, 0)); link(P(v, 0), P(pr, 1)); link(P(pr, 2), o->port[2]); era(o->port[1]);
        }
        else { Cell *f = cell(O_FST, 2); link(P(v, 0), P(f, 0)); link(P(f, 1), o->port[2]); era(o->port[1]); }
        cell_free(o);
        return;
    case O_MKGLUEV:
        if (vk == V_SYS) { Cell *g = cell(V_GLUEV, 3); link(P(v, 0), P(g, 1)); mv(o, 1, P(g, 2)); link(P(g, 0), o->port[2]); }
        else { link(P(v, 0), o->port[2]); era(o->port[1]); }
        cell_free(o);
        return;
    case O_UNGLUE:
        if (vk == V_SYS) {
            Cell *u2 = cell(O_UNGLUE2, 3); u2->x = o->x;
            mv(o, 1, P(u2, 0)); link(P(v, 0), P(u2, 1)); link(P(u2, 2), o->port[2]);
        } else if (o->x) {
            /* on the face of a composition of types: transport back along the face's line */
            Port a[2] = { P(v, 0), o->port[1] };
            link(apply_global("%unglue-line", a, 2), o->port[2]);
        } else {
            /* on the face: the equivalence's map */
            Cell *sn = cell(O_SND, 2), *f = cell(O_FST, 2), *a = cell(O_APP, 3);
            link(P(v, 0), P(sn, 0)); link(P(sn, 1), P(f, 0)); link(P(f, 1), P(a, 0)); mv(o, 1, P(a, 1)); link(P(a, 2), o->port[2]);
        }
        cell_free(o);
        return;
    case O_UNGLUE2:
        if (vk == V_GLUEV) {
            link(v->port[2], o->port[2]); era(v->port[1]); era(o->port[1]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else {
            /* stuck: a neutral unglue, of type the Glue type's base */
            Cell *n = cell(V_NUNGLUE, 4); n->x = o->x;
            link(unk(), P(n, 3));
            mv(o, 1, P(n, 1)); link(P(v, 0), P(n, 2)); link(P(n, 0), o->port[2]);
            cell_free(o);
        }
        return;
    case O_FNEG: {
        if (vk != V_FACE) { fprintf(stderr, "%%fneg: not a face\n"); exit(2); }
        Cell *f = cell(V_FACE, 1); f->p = face_neg(v->p);
        link(P(f, 0), o->port[1]); era(P(v, 0));
        cell_free(o);
        return;
    }
    case O_MKPART: {
        if (vk != V_IV) { stuck(o, v); return; }
        if (dnf_is_one(v->p)) { link(o->port[1], o->port[2]); cell_free(o); cell_free(v); return; }
        Cell *c = cell(V_PART, 2); c->p = dnf_retain(v->p);
        mv(o, 1, P(c, 1)); link(P(c, 0), o->port[2]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_FACEOF: {
        Cell *f = cell(V_FACE, 1);
        if (vk == V_SYS) {
            f->p = face_of_clauses(v->p);
            for (int k = 1; k < v->n; k++) era(v->port[k]);
            cell_free(v);
        } else { f->p = dnf_one(); era(P(v, 0)); }
        link(P(f, 0), o->port[1]);
        cell_free(o);
        return;
    }
    case O_NEG:
        if (vk != V_IV) { stuck(o, v); return; }
        { Dnf *n2 = iv_neg(v->p); dnf_release(v->p); v->p = n2; rekey_formula(v); }
        link(P(v, 0), o->port[1]); cell_free(o);
        return;
    case O_BIN: {
        if (vk != V_IV && vk != V_FACE) { stuck(o, v); return; }
        Cell *b = cell(O_BIN2, 2); b->x = o->x; b->p = dnf_retain(v->p);
        mv(o, 1, P(b, 0)); link(P(b, 1), o->port[2]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_BIN2: {
        bool face = o->x == B_FAND || o->x == B_FOR;
        if (vk != (face ? V_FACE : V_IV)) { stuck(o, v); return; }
        Dnf *n2 = (o->x == B_MEET || o->x == B_FAND) ? dnf_meet(o->p, v->p, face) : dnf_join(o->p, v->p, face);
        dnf_release(v->p); v->p = n2; rekey_formula(v);
        link(P(v, 0), o->port[1]); cell_free(o);
        return;
    }
    case O_FEQ: {
        if (vk != V_IV) { stuck(o, v); return; }
        Cell *f = cell(V_FACE, 1); f->p = face_eq(v->p, (int)o->a);
        link(P(f, 0), o->port[1]); cell_free(o); cell_free(v);
        return;
    }
    case O_FORALL:
        if (vk != V_FACE) { stuck(o, v); return; }
        { Dnf *n2 = face_forall(v->p, (Dim)o->a); dnf_release(v->p); v->p = n2; }
        link(P(v, 0), o->port[1]); cell_free(o);
        return;
    case O_SYSCONS: {
        if (vk != V_FACE) { stuck(o, v); return; }
        Dnf *f = v->p;
        if (dnf_is_one(f)) {
            Cell *fo = cell(O_FORCE, 2); mv(o, 1, P(fo, 0)); link(P(fo, 1), o->port[3]); era(o->port[2]);
        } else if (dnf_is_zero(f)) {
            link(o->port[2], o->port[3]); era(o->port[1]);
        } else {
            Cell *s2 = cell(O_SYSCONS2, 3); s2->p = dnf_retain(f);
            mv(o, 2, P(s2, 0)); mv(o, 1, P(s2, 1)); link(P(s2, 2), o->port[3]);
        }
        cell_free(o); cell_free(v);
        return;
    }
    case O_SYSCONS2:
        if (vk == V_SYS) {
            /* the branch, forced under each clause of its face, joins the system */
            Dnf *f = o->p;
            int nb = v->n - 1, m = (int)f->n;
            Cell *ns = cell(V_SYS, nb + m + 1);
            Clause **ncl = malloc((size_t)(nb + m + 1) * sizeof(Clause *));
            Dnf *cl = v->p;
            for (int k = 0; k < nb; k++) { ncl[k] = cl->c[k]; mv(v, k + 1, P(ns, k + 1)); }
            Port *cp = malloc((size_t)(m + 1) * sizeof(Port));
            split_restrict(o->port[1], f, m, cp);
            for (int q = 0; q < m; q++) {
                ncl[nb + q] = f->c[q];
                /* forced when that branch is read (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”) */
                Cell *fo = defer_cell(O_FORCE, 1); link(cp[q], P(fo, 1)); link(P(fo, 0), P(ns, nb + q + 1));
            }
            ns->p = clause_list(ncl, (uint32_t)(nb + m));
            free(ncl);
            free(cp);
            link(P(ns, 0), o->port[2]);
            cell_free(o); cell_free(v);
        } else {
            /* the rest is total */
            link(P(v, 0), o->port[2]); era(o->port[1]);
            cell_free(o);
        }
        return;
    case O_TPART: {
        int k = (int)o->a;
        if ((vk == V_PI || vk == V_SIGMA || vk == V_PATHP || vk == V_GLUE) && k < v->n) {
            link(v->port[k], o->port[1]);
            for (int m = 1; m < v->n; m++) if (m != k) era(v->port[m]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else { link(unk(), o->port[1]); era(P(v, 0)); cell_free(o); }
        return;
    }
    case O_CONW: {
        /* a path constructor's dimension x arrived (con_build) */
        int c = (int)o->a; ConInfo *k = &CON[c];
        int nf = k->nfields, nd = k->ndims, x = (int)o->x;
        Port out = o->port[nf + nd];
        Port fl[256], dm[64];
        if (nf > 256 || nd > 64) { fprintf(stderr, "CONW: a constructor too wide\n"); abort(); }
        for (int j = 0; j < nf; j++) fl[j] = o->port[1 + j];
        for (int j = 0; j < nd; j++) dm[j] = j < x ? o->port[nf + 1 + j] : j == x ? P(v, 0) : o->port[nf + j];
        bool end = vk != V_IV || dnf_is_zero(v->p) || dnf_is_one(v->p);
        if (end) link(con_build_bdy(c, fl, dm), out);
        else if (x == nd - 1) {
            Cell *cc = cell(V_CON, nf + nd + 1); cc->a = (uint64_t)c;
            for (int j = 0; j < nf; j++) link(fl[j], P(cc, 1 + j));
            for (int j = 0; j < nd; j++) link(dm[j], P(cc, 1 + nf + j));
            link(P(cc, 0), out);
        } else {
            Cell *w = cell(O_CONW, nf + nd + 1); w->a = (uint64_t)c; w->x = (uint32_t)(x + 1);
            link(dm[x + 1], P(w, 0));
            for (int j = 0; j < nf; j++) link(fl[j], P(w, 1 + j));
            for (int j = 0, q = 0; j < nd; j++) if (j != x + 1) link(dm[j], P(w, nf + 1 + q++));
            link(P(w, nf + nd), out);
        }
        cell_free(o);
        return;
    }
    case O_CONCHK: {
        int n = o->n - 3;
        if (vk == V_FACE && dnf_is_one(v->p)) {
            /* the boundary's face holds: the constructor is its boundary */
            link(o->port[1], o->port[n + 2]);
            for (int j = 0; j < n; j++) era(o->port[2 + j]);
        } else {
            Cell *c = cell(V_CON, n + 1); c->a = o->a;
            for (int j = 0; j < n; j++) mv(o, 2 + j, P(c, 1 + j));
            era(o->port[1]);
            link(P(c, 0), o->port[n + 2]);
        }
        cell_free(o); cell_free(v);
        return;
    }
    case O_HCOMP: {
        Port out = o->port[o->n - 1];
        if (vk == V_SYS && o->n == 4) {
            /* a data type without path constructors: look at the cap, then at the sides */
            Port t1, t2; split2(P(v, 0), &t1, &t2);
            Cell *m = cell(O_HCM1, 5);
            link(o->port[1], P(m, 0)); link(t1, P(m, 1)); link(o->port[2], P(m, 2)); link(out, P(m, 3)); link(t2, P(m, 4));
        } else if (vk == V_SYS) {
            /* a partial tube: a formal hcomp, a value of the data type */
            Cell *h = cell(V_HCOMP, 3);
            link(P(v, 0), P(h, 1)); mv(o, 1, P(h, 2)); link(P(h, 0), out);
        } else {
            Cell *pr = cell(O_PAPPR, 3);
            link(iv_cell(dnf_one()), P(pr, 0)); link(P(v, 0), P(pr, 1)); link(P(pr, 2), out);
            era(o->port[1]);
            if (o->n == 4) era(o->port[2]);
        }
        cell_free(o);
        return;
    }
    case O_HCM1:
        if (vk == V_CON) {
            /* the cap is a constructor: are the sides? */
            Cell *m = cell(O_HCM2, 5); m->a = v->a;
            link(o->port[4], P(m, 0)); link(o->port[1], P(m, 1)); link(P(v, 0), P(m, 2)); link(o->port[2], P(m, 3)); link(o->port[3], P(m, 4));
        } else {
            /* a formal hcomp, which a substitution may make meet */
            Cell *h = cell(V_HCOMP, 4);
            link(o->port[1], P(h, 1)); link(P(v, 0), P(h, 2)); link(o->port[2], P(h, 3)); link(P(h, 0), o->port[3]);
            era(o->port[4]);
        }
        cell_free(o);
        return;
    case O_HCM2: {
        /* each side of the tube at a fresh dimension, one after another */
        if (vk != V_SYS) { fprintf(stderr, "hcomp: the tube is not a system\n"); exit(2); }
        int nb = v->n - 1;
        Port tube = o->port[1], cap = o->port[2], F = o->port[3], out = o->port[4];
        if (nb == 0) link(app2(app2(F, tube), cap), out);
        else {
            Cell *m = cell(O_HCM3, nb + 4); m->a = o->a;   /* the first side, the nb - 1 left, 4 more */
            link(papp_at(v->port[1], iv_atom(fresh_dim())), P(m, 0));
            for (int b = 1; b < nb; b++) link(v->port[b + 1], P(m, b));
            link(tube, P(m, nb)); link(cap, P(m, nb + 1)); link(F, P(m, nb + 2)); link(out, P(m, nb + 3));
        }
        cell_free(o); cell_free(v);
        return;
    }
    case O_HCM3: {
        int left = o->n - 5;   /* the sides not yet looked at */
        Port tube = o->port[left + 1], cap = o->port[left + 2], F = o->port[left + 3], out = o->port[left + 4];
        bool meet = vk == V_CON && v->a == o->a;
        era(P(v, 0));
        if (meet && left == 0) link(app2(app2(F, tube), cap), out);
        else if (meet) {
            Cell *m = cell(O_HCM3, left + 4); m->a = o->a;
            link(papp_at(o->port[1], iv_atom(fresh_dim())), P(m, 0));
            for (int b = 1; b < left; b++) link(o->port[b + 1], P(m, b));
            link(tube, P(m, left)); link(cap, P(m, left + 1)); link(F, P(m, left + 2)); link(out, P(m, left + 3));
        } else {
            for (int b = 1; b <= left; b++) era(o->port[b]);
            Cell *h = cell(V_HCOMP, 4);
            link(tube, P(h, 1)); link(cap, P(h, 2)); link(F, P(h, 3)); link(P(h, 0), out);
        }
        cell_free(o);
        return;
    }
    case O_TRANSD1: {
        int np = o->n - 3;
        if (vk != V_FACE) { stuck(o, v); return; }
        if (dnf_is_one(v->p)) {
            /* on ψ the family is constant: transport is the identity */
            link(o->port[np + 1], o->port[np + 2]);
            for (int k = 1; k <= np; k++) era(o->port[k]);
        } else {
            Cell *t = cell(O_TRANSD2, np + 2); t->a = o->a; t->p = dnf_retain(v->p);
            mv(o, np + 1, P(t, 0));
            for (int k = 1; k <= np; k++) mv(o, k, P(t, k));
            link(P(t, np + 1), o->port[np + 2]);
        }
        cell_free(o); cell_free(v);
        return;
    }
    case O_TRANSH: {
        /* a substituted formal transport: the tube still a system, the transport stays formal; the
         * substitution made it one side, that side is transported at its end; ψ true, the
         * transport is the identity and the composition is rebuilt */
        int np = o->n - 3;
        Port out = o->port[np + 2];
        if (dnf_is_one(o->p)) {
            Cell *h = cell(O_HCOMP, 3);
            link(P(v, 0), P(h, 0)); mv(o, 1, P(h, 1)); link(P(h, 2), out);
            for (int j = 0; j < np; j++) era(o->port[2 + j]);
        } else if (vk == V_SYS) {
            Cell *h = cell(V_TRANSH, np + 3); h->a = o->a; h->p = dnf_retain(o->p);
            link(P(v, 0), P(h, 1)); mv(o, 1, P(h, 2));
            for (int j = 0; j < np; j++) mv(o, 2 + j, P(h, 3 + j));
            link(P(h, 0), out);
        } else {
            Cell *t = cell(O_TRANSD1, np + 3); t->a = o->a;
            link(face_cell(dnf_retain(o->p)), P(t, 0));
            for (int j = 0; j < np; j++) mv(o, 2 + j, P(t, 1 + j));
            link(papp_at(P(v, 0), dnf_one()), P(t, np + 1));
            link(P(t, np + 2), out);
            era(o->port[1]);
        }
        cell_free(o);
        return;
    }
    case O_TRANSD2: {
        int np = o->n - 2;
        Port out = o->port[np + 1];
        if (vk == V_CON) {
            ConInfo *k = &CON[v->a];
            int n = k->nfields + k->ndims;
            Port *a = malloc((size_t)(np + n + 2) * sizeof(Port));
            for (int j = 0; j < np; j++) a[j] = o->port[1 + j];
            a[np] = face_cell(dnf_retain(o->p));
            for (int j = 0; j < n; j++) a[np + 1 + j] = v->port[1 + j];
            link(apply_gid(k->trans, a, np + 1 + n), out);
            free(a);
            cell_free(o); cell_free(v);
        } else if (vk == V_HCOMP && v->n == 3) {
            /* a formal composition in a type with path constructors: the transport stays formal
             * (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): the sides are transported only when a substitution selects one, or when a
             * reading demands the structure (transh_expand) */
            Cell *h = cell(V_TRANSH, np + 3); h->a = o->a; h->p = dnf_retain(o->p);
            link(v->port[1], P(h, 1)); link(v->port[2], P(h, 2));
            for (int j = 0; j < np; j++) mv(o, 1 + j, P(h, 3 + j));
            link(P(h, 0), out);
            cell_free(o); cell_free(v);
        } else if (vk == V_HCOMP) {
            Port *a = malloc((size_t)(np + 4) * sizeof(Port));
            for (int j = 0; j < np; j++) a[j] = o->port[1 + j];
            a[np] = face_cell(dnf_retain(o->p)); a[np + 1] = v->port[1]; a[np + 2] = v->port[2];
            if (v->n == 4) era(v->port[3]);
            link(apply_gid(DATA[o->a].transh, a, np + 3), out);
            free(a);
            cell_free(o); cell_free(v);
        } else if (vk == V_TRANSH) { link(transh_expand(v), P(o, 0)); return; }
        else if (vk == V_SYS) distribute(o, v);
        else {
            Cell *n = cell(V_NTRANS, np + 3); n->a = o->a; n->p = dnf_retain(o->p);
            link(P(v, 0), P(n, 1));
            for (int j = 0; j < np; j++) mv(o, 1 + j, P(n, 2 + j));
            link(unk(), P(n, np + 2));
            link(P(n, 0), out);
            cell_free(o);
        }
        return;
    }
    case O_CASE: {
        /* 0=scrutinee [1=motive] then the environment, out */
        CaseTab *tb = o->p;
        int ne = tb->nenv, mb = o->x ? 1 : 0, e0 = 1 + mb;
        if (vk == V_TRANSH) { link(transh_expand(v), P(o, 0)); return; }
        if (vk == V_CON) {
            /* the selected branch's body is built with the constructor's fields and dimensions
             * and the shared variables: one interaction. A branch taking induction hypotheses
             * gets, per recursive field, its elimination by the same case, suspended */
            ConInfo *ci = &CON[v->a];
            int k = ci->index, nf = ci->nfields, nd = ci->ndims, nr = ci->nrec;
            /* a case meets a constructor of its own data type, whose branch it has; anything else
             * is a value of the wrong type reaching the case, reported where it happens */
            if (ci->data != tb->d || !tb->br[k]) {
                fflush(stdout);
                fprintf(stderr, "internal error: a case on data type %d (the type of %s) met constructor %s (data type %d, index %d)%s\n",
                        tb->d, data_first_con(tb->d), ci->name ? ci->name : "?", ci->data, k, tb->br[k] ? "" : ", which has no branch");
                abort();
            }
            bool ih = nr && (tb->mask >> k & 1);
            int np = nf + (ih ? nr : 0) + nd;
            Port prm[np + 1], env[ne + 1];
            /* the hypotheses' leaves: one elimination per D at the leaf of a field's shape (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.18 “E28b — Strict positivity under Π codomains and Σ components”) */
            int nleaf = 0;
            if (ih) for (int j = 0; j < nf; j++) if (ci->recmask >> j & 1) nleaf += ci->rsh[j] ? (ci->rsh[j][0] == 'N' ? 1 : ih_leaves(ci->rsh[j])) : 1;
            if (!ih) {
                for (int j = 0; j < nf + nd; j++) prm[j] = v->port[1 + j];
                for (int e = 0; e < ne; e++) env[e] = o->port[e0 + e];
                if (mb) era(o->port[1]);
            } else {
                Port mc[nleaf + 1], ec[ne + 1][nleaf + 2];
                if (mb) split(o->port[1], nleaf, mc);
                for (int e = 0; e < ne; e++) { split(o->port[e0 + e], nleaf + 1, ec[e]); env[e] = ec[e][0]; }
                int q = 0, leaf = 0;
                for (int j = 0; j < nf; j++) {
                    if (ci->recmask >> j & 1) {
                        Port fa, fb; split2(v->port[1 + j], &fa, &fb);
                        prm[j] = fa;
                        if (ci->rsh[j] && ci->rsh[j][0] == 'N') {
                            /* a field of type L D, L a container (2026-11-15): the hypothesis is
                             * %ihn-L _ _ _ h x, h the elimination as a function (its level, element
                             * type and motive arguments are types only: unknown here) */
                            int L = atoi(ci->rsh[j] + 1);
                            const char *sp = "D";
                            Port hm = ih_maker(&sp, tb, o->x, mb ? mc : NULL, &ec[0][0], nleaf + 2, ne, &leaf);
                            long gi = L >= 0 ? DATA[L].ihng - 1 : -1;
                            if (gi >= 0) { Port a[5] = { unk(), unk(), unk(), hm, fb }; prm[nf + q] = apply_gid(gi, a, 5); }
                            else { era(hm); era(fb); prm[nf + q] = unk(); }
                            q++;
                            continue;
                        }
                        if (ci->rsh[j] && !ih_pure_path(ci->rsh[j])) {
                            /* a function- or pair-shaped field: the hypothesis is the field passed through the
                             * shape's maker, an elimination as a function at each leaf (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.18 “E28b — Strict positivity under Π codomains and Σ components”) */
                            const char *sp = ci->rsh[j];
                            Port hm = ih_maker(&sp, tb, o->x, mb ? mc : NULL, &ec[0][0], nleaf + 2, ne, &leaf);
                            Cell *ap = cell(O_APP, 3); link(hm, P(ap, 0)); link(fb, P(ap, 1));
                            prm[nf + q] = P(ap, 2);
                            q++;
                            continue;
                        }
                        /* 1=field [2=motive] then the environment */
                        /* a path-typed field (rdim dimensions deep): the hypothesis is the line of
                         * the eliminations of its points, λ m ... → F (p @ m ...) (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.17 “E28a — Induction hypotheses for path-typed recursive fields”) */
                        int rd = ci->rdim[j]; Dim ds[16];
                        for (int t = 0; t < rd; t++) { ds[t] = fresh_dim(); fb = papp_at(fb, iv_atom(ds[t])); }
                        Cell *h = cell(V_SUSP, ne + 2 + mb); h->x = o->x; h->p = tb;
                        link(fb, P(h, 1));
                        if (mb) link(mc[leaf], P(h, 2));
                        for (int e = 0; e < ne; e++) link(ec[e][leaf + 1], P(h, 2 + mb + e));
                        /* its key: the case term on the field (the motive is not part of the term) */
                        if (!rd) { Key **ck = malloc(((size_t)ne + 2) * sizeof(Key *)); ck[0] = key_of_port(fb);
                          for (int e = 0; e < ne; e++) ck[1 + e] = key_of_port(ec[e][leaf + 1]);
                          h->key = key(K_CASE, 0, tb, ne + 1, ck);
                          for (int e = 0; e < ne + 1; e++) if (ck[e]) key_release(ck[e]);
                          free(ck); }
                        Port hp = P(h, 0);
                        for (int t = rd - 1; t >= 0; t--) { Cell *m = cell(O_MKPLAM, 2); m->a = ds[t]; link(hp, P(m, 0)); hp = P(m, 1); }
                        prm[nf + q] = hp;
                        q++; leaf++;
                    } else prm[j] = v->port[1 + j];
                }
                for (int j = 0; j < nd; j++) prm[nf + nr + j] = v->port[1 + nf + j];
            }
            Box *bx; Port sh = memo_begin(o, &bx, true);
            if (sh && !bx) { for (int j = 0; j < np; j++) era(prm[j]); for (int e = 0; e < ne; e++) era(env[e]); link(sh, o->port[o->n - 1]); cell_free(o); cell_free(v); return; }
            scrut_key = v->key ? key_retain(v->key) : NULL;   /* the branch's rebuilt scrutinee is named by this value */
            link(memo_end(sh, bx, record_build_n(tb->br[k], prm, env)), o->port[o->n - 1]);
            if (scrut_key) { key_release(scrut_key); scrut_key = NULL; }
            cell_free(o); cell_free(v);
        } else if (vk == V_HCOMP && tb->motive) {
            /* f (hcomp^i [φ ↦ u] u0) = comp^i P(hfill) [φ ↦ f u] (f u0), f this elimination; the
             * motive P is built here, from the environment, since this rule reads it */
            Port me[ne + 1];
            Cell *f = cell(V_CASEF, ne + 1); f->x = 0; f->p = tb;
            for (int e = 0; e < ne; e++) { Port a1, a2; split2(o->port[e0 + e], &a1, &a2); me[e] = a1; link(a2, P(f, 1 + e)); }
            Port a[4] = { record_build_n(tb->motive, NULL, me), P(f, 0), v->port[1], v->port[2] };
            if (v->n == 4) era(v->port[3]);
            link(apply_gid(case_hcomp_global, a, 4), o->port[o->n - 1]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else {
            /* stuck: a neutral elimination, of type the motive at the scrutinee */
            /* 1=scrutinee [2=motive] then the environment, type */
            /* its motive, which types it, is built here from the environment */
            int nm = tb->motive ? 1 : 0;
            Cell *n = cell(V_NCASE, ne + 3 + nm); n->x = (uint32_t)nm; n->p = tb;
            /* named as the elimination: the same stuck elimination is the same neutral (an
             * interval read from it is one atom) */
            /* (by its table, scrutinee and environment, as its syntax names it: not by the name of a
             * call it came from, which another route to it does not carry) */
            if (v->key) {
                Key **c = malloc(((size_t)ne + 2) * sizeof(Key *)); bool ok = true;
                c[0] = key_retain(v->key);
                for (int e = 0; e < ne; e++) { c[1 + e] = key_of_port(o->port[e0 + e]); if (!c[1 + e]) ok = false; }
                if (ok) n->key = key(K_CASE, 0, tb, ne + 1, c);
                for (int e = 0; e < ne + 1; e++) if (c[e]) key_release(c[e]);
                free(c);
            }
            link(P(v, 0), P(n, 1)); link(unk(), P(n, n->n - 1));
            if (nm) {
                Port me[ne + 1];
                for (int e = 0; e < ne; e++) { Port a1, a2; split2(o->port[e0 + e], &a1, &a2); me[e] = a1; link(a2, P(n, 3 + e)); }
                link(record_build_n(tb->motive, NULL, me), P(n, 2));
            } else for (int e = 0; e < ne; e++) mv(o, e0 + e, P(n, 2 + e));
            link(P(n, 0), o->port[o->n - 1]);
            cell_free(o);
        }
        return;
    }
    case O_VIEW: rule_view(o, v); return;
    case O_WEQ: case O_WLT: case O_WADD: case O_WMAX: case O_WSUB: case O_WMUL: case O_WDIV: case O_WMOD: case O_WAND: case O_WOR: case O_WXOR: case O_WSHL: case O_WSHR: {
        /* a is read: wait for b */
        Cell *c = cell(O_WOP2, 3); c->x = o->kind;
        mv(o, 1, P(c, 0)); link(P(v, 0), P(c, 1)); mv(o, 2, P(c, 2));
        cell_free(o);
        return;
    }
    case O_PTY: {
        /* (%pty k): the primitive type of kind k (a word) */
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%pty: a kind is a word\n"); exit(2); }
        Cell *t = cell(V_PTY, 1); t->a = v->a; link(P(t, 0), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_BYTESTY: {
        /* (%bytesty n): the type Bytes n, n held */
        Cell *t = cell(V_PTY, 2); t->a = PT_BYTES; link(P(v, 0), P(t, 1)); link(P(t, 0), o->port[1]);
        cell_free(o);
        return;
    }
    case O_PVAL: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%pval: a kind is a word\n"); exit(2); }
        Cell *c = cell(O_PVAL2, 2); c->a = v->a; mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        cell_free(o); cell_free(v);
        return;
    }
    case O_PVAL2: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%pval: bits are a word\n"); exit(2); }
        Cell *c = cell(V_PVAL, 1); c->x = (uint32_t)o->a; c->a = v->a;
        link(P(c, 0), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_POP1: {
        /* a conversion */
        if (vk != V_PVAL && !(vk == V_WORD && !v->p)) { Port a[1] = { P(v, 0) }; link(nprim(1, (int)o->x, a, 1), o->port[1]); cell_free(o); return; }
        uint64_t a = v->a, r = 0; uint32_t k = 0; double d; int64_t i;
        switch ((int)o->x) {
        case PO_I64OFW: r = a; k = PT_I64; break;
        case PO_WOFI64: r = a; break;
        case PO_U32OFW: r = (uint32_t)a; k = PT_U32; break;
        case PO_WOFU32: r = a; break;
        case PO_F64OFW: d = (double)a; memcpy(&r, &d, 8); k = PT_F64; break;
        case PO_F64OFI64: i = (int64_t)a; d = (double)i; memcpy(&r, &d, 8); k = PT_F64; break;
        case PO_I64OFF64: memcpy(&d, &a, 8); i = (int64_t)d; r = (uint64_t)i; k = PT_I64; break;
        case PO_F64BITS: r = a; k = PT_I64; break;      /* a Float64's bits as an Int64, and back (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C22, and §12 “The build list”, “C. Memory and state”, item 22: Float64 stored by the byte layer) */
        case PO_F64OFBITS: r = a; k = PT_F64; break;
        case PO_F32BITS: r = (uint32_t)a; k = PT_U32; break;      /* a Float32's bits as a UInt32, and back (Float32 stored by the 4-byte layer) */
        case PO_F32OFBITS: r = (uint32_t)a; k = PT_F32; break;
        case PO_U64OFW: r = a; k = PT_U64; break;
        case PO_WOFU64: r = a; break;
        default: fprintf(stderr, "primitive operation: not unary\n"); exit(2);
        }
        if (o->x == PO_WOFI64 || o->x == PO_WOFU32 || o->x == PO_WOFU64) link(word_of(r), o->port[1]);
        else { Cell *c = cell(V_PVAL, 1); c->x = k; c->a = r; link(P(c, 0), o->port[1]); }
        cell_free(o); cell_free(v);
        return;
    }
    case O_POP2: {
        if (vk != V_PVAL) { Port a[2] = { P(v, 0), o->port[1] }; link(nprim(2, (int)o->x, a, 2), o->port[2]); cell_free(o); return; }
        Cell *c = cell(O_POP2B, 2); c->x = o->x; c->a = v->a;
        mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        cell_free(o); cell_free(v);
        return;
    }
    case O_POP2B: {
        if (vk != V_PVAL) { Port a[2] = { pval_of(po_kind((int)o->x), o->a), P(v, 0) }; link(nprim(2, (int)o->x, a, 2), o->port[1]); cell_free(o); return; }
        uint64_t a = o->a, b = v->a, r = 0; int k = -1; bool bl = false, res = false;
        int64_t ia = (int64_t)a, ib = (int64_t)b; uint32_t ua = (uint32_t)a, ub = (uint32_t)b; double da, db, dr;
        memcpy(&da, &a, 8); memcpy(&db, &b, 8);
        switch ((int)o->x) {
        /* two's complement through the unsigned operations: the machine itself has no signed overflow */
        case PO_I64ADD: r = a + b; k = PT_I64; break;
        case PO_I64SUB: r = a - b; k = PT_I64; break;
        case PO_I64MUL: r = a * b; k = PT_I64; break;
        case PO_I64DIV: r = ib == 0 ? 0 : (ia == INT64_MIN && ib == -1) ? a : (uint64_t)(ia / ib); k = PT_I64; break;
        case PO_I64MOD: r = ib == 0 ? a : (ia == INT64_MIN && ib == -1) ? 0 : (uint64_t)(ia % ib); k = PT_I64; break;
        case PO_I64AND: r = a & b; k = PT_I64; break;
        case PO_I64OR: r = a | b; k = PT_I64; break;
        case PO_I64XOR: r = a ^ b; k = PT_I64; break;
        case PO_I64SHL: r = a << (b & 63); k = PT_I64; break;
        case PO_I64SHR: r = (uint64_t)(ia >> (b & 63)); k = PT_I64; break;   /* arithmetic: gcc's >> on a negative int64_t is arithmetic (implementation-defined in C, pinned) */
        case PO_U64ADD: r = a + b; k = PT_U64; break;
        case PO_U64SUB: r = a - b; k = PT_U64; break;
        case PO_U64MUL: r = a * b; k = PT_U64; break;
        case PO_U64EQ: bl = true; res = a == b; break;
        case PO_U64LT: bl = true; res = a < b; break;
        case PO_U64DIV: r = b == 0 ? 0 : a / b; k = PT_U64; break;
        case PO_U64MOD: r = b == 0 ? a : a % b; k = PT_U64; break;
        case PO_U64AND: r = a & b; k = PT_U64; break;
        case PO_U64OR: r = a | b; k = PT_U64; break;
        case PO_U64XOR: r = a ^ b; k = PT_U64; break;
        case PO_U64SHL: r = a << (b & 63); k = PT_U64; break;
        case PO_U64SHR: r = a >> (b & 63); k = PT_U64; break;
        case PO_I64EQ: bl = true; res = ia == ib; break;
        case PO_I64LT: bl = true; res = ia < ib; break;
        case PO_U32ADD: r = (uint32_t)(ua + ub); k = PT_U32; break;
        case PO_U32SUB: r = (uint32_t)(ua - ub); k = PT_U32; break;
        case PO_U32MUL: r = (uint32_t)(ua * ub); k = PT_U32; break;
        case PO_U32EQ: bl = true; res = ua == ub; break;
        case PO_U32LT: bl = true; res = ua < ub; break;
        case PO_F64ADD: dr = da + db; memcpy(&r, &dr, 8); k = PT_F64; break;
        case PO_F64SUB: dr = da - db; memcpy(&r, &dr, 8); k = PT_F64; break;
        case PO_F64MUL: dr = da * db; memcpy(&r, &dr, 8); k = PT_F64; break;
        case PO_F64DIV: dr = da / db; memcpy(&r, &dr, 8); k = PT_F64; break;
        case PO_F64EQ: bl = true; res = da == db; break;
        case PO_F64LT: bl = true; res = da < db; break;
        default: fprintf(stderr, "primitive operation: not binary\n"); exit(2);
        }
        if (bl) { need_bool(); link(con_build(res ? B_TRUE : B_FALSE, NULL, NULL), o->port[1]); }
        else { Cell *c = cell(V_PVAL, 1); c->x = (uint32_t)k; c->a = r; link(P(c, 0), o->port[1]); }
        cell_free(o); cell_free(v);
        return;
    }
    case O_BYTES: case O_LOADB: case O_STOREB: case O_REGAT: {
        /* a length or an offset: a word, or a numeral (zero, suc k): counted down to a word */
        long n = -1;
        if (vk == V_WORD && !v->p) n = (long)v->a;
        else if (vk == V_CON && CON[v->a].nfields == 0 && !strcmp(CON[v->a].name, "zero")) n = 0;
        else if (vk == V_CON && CON[v->a].nfields == 1 && !strcmp(CON[v->a].name, "suc")) {
            Cell *c = cell(o->kind, o->n); c->x = o->x; c->a = o->a + 1; c->p = o->p; o->p = NULL;
            for (int k = 1; k < o->n; k++) mv(o, k, P(c, k));
            link(v->port[1], P(c, 0));
            cell_free(o); cell_free(v);
            return;
        } else if (o->kind == O_REGAT || o->a) { stuck(o, v); return; }
        else {
            /* a neutral length or offset: the operation stuck on it */
            if (o->kind == O_BYTES) { Port a[1] = { P(v, 0) }; link(nprim(6, 0, a, 1), o->port[1]); }
            else if (o->kind == O_LOADB) { Port a[2] = { bytes_cell(o->p), P(v, 0) }; o->p = NULL; link(nprim(3, (int)o->x, a, 2), o->port[1]); }
            else { Port a[3] = { bytes_cell(o->p), P(v, 0), o->port[1] }; o->p = NULL; link(nprim(4, (int)o->x, a, 3), o->port[2]); }
            cell_free(o);
            return;
        }
        n += (long)o->a;
        if (vk == V_CON) cell_free(v); else cell_free(v);
        if (o->kind == O_BYTES) {
            Bytes *b = calloc(1, sizeof(Bytes) + (size_t)n); b->rc = 1; b->n = (size_t)n;
            Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b;
            link(P(c, 0), o->port[1]);
        } else if (o->kind == O_REGAT) {
            /* the address next */
            Cell *c = cell(O_REGATA, 1); c->a = (uint64_t)n;
            mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        } else if (o->kind == O_LOADB) {
            Bytes *b = o->p; int w = (int)o->x; uint64_t r = 0;
            if (b && (size_t)n + (size_t)w <= b->n) memcpy(&r, b->b + n, (size_t)w);
            Cell *c = cell(V_PVAL, 1); c->x = w == 8 ? PT_I64 : PT_U32; c->a = r;
            link(P(c, 0), o->port[1]);
            if (b && rcl_dec(&b->rc) == 0) free(b);
            o->p = NULL;
        } else {
            /* the value next */
            Cell *c = cell(O_STOREC, 2); c->x = o->x; c->a = (uint64_t)n; c->p = o->p; o->p = NULL;
            mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        }
        cell_free(o);
        return;
    }
    case O_LOAD: case O_STORE: {
        if (vk != V_BYTES) {
            int n = o->n - 1; Port a[3] = { P(v, 0), o->port[1], n > 2 ? o->port[2] : 0 };
            link(nprim(o->kind == O_LOAD ? 3 : 4, (int)o->x, a, n), o->port[n]); cell_free(o); return;
        }
        Cell *c = cell(o->kind == O_LOAD ? O_LOADB : O_STOREB, o->n - 1); c->x = o->x; c->a = 0;
        c->p = v->p; v->p = NULL;   /* the bytes, taken */
        for (int k = 1; k < o->n; k++) mv(o, k, P(c, k - 1));
        cell_free(o); cell_free(v);
        return;
    }
    case O_STOREC: {
        if (vk != V_PVAL) { Port a[3] = { bytes_cell(o->p), word_of(o->a), P(v, 0) }; o->p = NULL; link(nprim(4, (int)o->x, a, 3), o->port[1]); cell_free(o); return; }
        Bytes *b = o->p; int w = (int)o->x; long n = (long)o->a;
        Bytes *nb = malloc(sizeof(Bytes) + (b ? b->n : 0)); nb->rc = 1; nb->n = b ? b->n : 0; if (b) memcpy(nb->b, b->b, b->n);
        if ((size_t)n + (size_t)w <= nb->n) memcpy(nb->b + n, &v->a, (size_t)w);
        if (b && rcl_dec(&b->rc) == 0) free(b);
        o->p = NULL;
        Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = nb;
        link(P(c, 0), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_BEQ: case O_BEQB: {
        /* byte strings compared by contents; anything not yet a byte string (a neutral) is not
         * known equal, so false (conversion is conservative) */
        need_bool();
        if (o->kind == O_BEQ) {
            if (vk != V_BYTES) { era(o->port[1]); era(P(v, 0)); link(con_build(B_FALSE, NULL, NULL), o->port[2]); cell_free(o); return; }
            Cell *c = cell(O_BEQB, 2); c->p = v->p; v->p = NULL;
            mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
            cell_free(o); cell_free(v);
            return;
        }
        Bytes *a = o->p; o->p = NULL; bool eq = false;
        if (vk == V_BYTES) {
            Bytes *b = v->p; size_t na = a ? a->n : 0, nb = b ? b->n : 0;
            eq = na == nb && (na == 0 || !memcmp(a->b, b->b, na));
        }
        if (a && rcl_dec(&a->rc) == 0) free(a);
        era(P(v, 0));
        link(con_build(eq ? B_TRUE : B_FALSE, NULL, NULL), o->port[1]);
        cell_free(o);
        return;
    }
    case O_BLEN: {
        if (vk != V_BYTES) { Port a[1] = { P(v, 0) }; link(nprim(5, 0, a, 1), o->port[1]); cell_free(o); return; }
        link(word_of(v->p ? (uint64_t)((Bytes *)v->p)->n : 0), o->port[1]);
        era(P(v, 0)); cell_free(o);
        return;
    }
    case O_FFI: {
        /* a foreign call (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.11 “D24–D27 — The world”, its part D27): the arguments read in turn, each a primitive value, a word or a byte
         * string (its address passed, the string kept until the call returns); then the call, by
         * the entry's kinds: every argument and the result integers (or pointers), or every one a
         * double */
        typedef struct { int n; uint64_t v[8]; Bytes *keep[8]; } FfiArgs;
        Ffi *e = &FFI[o->x];
        FfiArgs *A = o->p; if (!A) { A = calloc(1, sizeof(FfiArgs)); }
        o->p = NULL;
        uint64_t bits;
        if (vk == V_PVAL) bits = v->a;
        else if (vk == V_WORD && !v->p) bits = v->a;
        else if (vk == V_BYTES) { Bytes *b = v->p; v->p = NULL; A->keep[A->n] = b; bits = b ? (uint64_t)(uintptr_t)b->b : 0; }
        else { o->p = A; stuck(o, v); return; }
        A->v[A->n++] = bits;
        cell_free(v);
        if (o->n > 2) {
            Cell *c = cell(O_FFI, o->n - 1); c->x = o->x; c->p = A;
            for (int k = 1; k < o->n; k++) mv(o, k, P(c, k - 1));
            cell_free(o);
            return;
        }
        uint64_t r = ffi_invoke(e, A->v, A->n); int n = A->n;
        for (int k = 0; k < n; k++) if (A->keep[k] && rcl_dec(&A->keep[k]->rc) == 0) free(A->keep[k]);
        free(A);
        Port out;
        if (e->ret == 4) out = word_of(r);
        else { Cell *c = cell(V_PVAL, 1); c->x = e->ret; c->a = r; out = P(c, 0); }
        link(out, o->port[1]);
        cell_free(o);
        return;
    }
    case O_DEEP: {
        /* a constructor (no dimensions) with fields: its fields forced one after another; anything else
         * is delivered as it is (a word, a primitive value, a byte string, a native value) */
        if (vk != V_CON || CON[v->a].ndims || CON[v->a].nfields == 0) { link(P(v, 0), o->port[1]); cell_free(o); return; }
        int nf = CON[v->a].nfields;
        DeepAcc *D = calloc(1, sizeof(DeepAcc) + (size_t)nf * sizeof(Port)); D->c = (int)v->a; D->nf = nf;
        Cell *dc = cell(O_DEEPC, nf + 1); dc->p = D;
        Cell *d0 = cell(O_DEEP, 2); mv(v, 1, P(d0, 0)); link(P(d0, 1), P(dc, 0));
        for (int j = 1; j < nf; j++) mv(v, 1 + j, P(dc, j));
        mv(o, 1, P(dc, nf));
        for (int j = nf + 1; j < v->n; j++) era(v->port[j]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_DEEPC: {
        DeepAcc *D = o->p; o->p = NULL;
        D->f[D->got++] = P(v, 0);
        if (D->got < D->nf) {
            int rest = D->nf - D->got;   /* the fields still unforced: ports 1..rest, then out */
            Cell *dc = cell(O_DEEPC, rest + 1); dc->p = D;
            Cell *d0 = cell(O_DEEP, 2); mv(o, 1, P(d0, 0)); link(P(d0, 1), P(dc, 0));
            for (int j = 2; j <= rest; j++) mv(o, j, P(dc, j - 1));
            mv(o, rest + 1, P(dc, rest));
            cell_free(o);
            return;
        }
        Port r = con_build(D->c, D->f, NULL);
        link(r, o->port[1]);
        free(D); cell_free(o);
        return;
    }
    case O_NATIVE: {
        /* a #jit definition called from the net (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §5.5 “Bytes, the boundary, and what is trusted”): the arguments read in turn and kept as
         * values. Each crosses by its type (jit_in): a word, a primitive value, a value converted by
         * its type (kind 8, read whole first: O_DEEP), or not read (9). When every argument crosses,
         * the bytes compute the result; otherwise, and on the overflow exit, the definition itself is
         * applied to them (the net computes what the bytes do not) */
        typedef struct { int n; bool deep; Port v[NATIVE_MAXW]; } NArgs;
        NArgs *A = o->p; if (!A) A = calloc(1, sizeof(NArgs));
        o->p = NULL;
        A->v[A->n++] = P(v, 0);
        if (o->n > 2) {
            Cell *c = cell(O_NATIVE, o->n - 1); c->x = o->x; c->a = o->a; c->p = A;
            for (int k = 1; k < o->n; k++) mv(o, k, P(c, k - 1));
            cell_free(o);
            return;
        }
        int n = A->n; bool words = n <= NATIVE_MAXW; Ffi *e = &FFI[o->x]; int unbuilt = 0;
        for (int k = 0; k < n && words; k++) {
            Cell *c = cell_at(A->v[k]);
            if (!c) { words = false; break; }
            if (e->kind[k] == 5) { if (c->kind != V_PVAL) words = false; }
            else if (e->kind[k] == 9) { }
            else if (e->kind[k] == 8) { if (!jit_ready(c)) { if (c->kind == V_CON && !A->deep) unbuilt++; else words = false; } }
            else if (c->kind != V_WORD || c->p) words = false;
        }
        if (words && unbuilt) {
            /* an argument (a constructor) with fields not yet values: read whole (O_DEEP) and the
             * call made again, once */
            Cell *c = cell(O_NATIVE, n + 1); c->x = o->x; c->a = o->a;
            NArgs *B = calloc(1, sizeof(NArgs)); B->deep = true; c->p = B;
            for (int k = 0; k < n; k++) {
                Cell *ak = cell_at(A->v[k]);
                if (e->kind[k] == 8 && ak && ak->kind == V_CON && !jit_ready(ak)) { Cell *d = cell(O_DEEP, 2); link(A->v[k], P(d, 0)); link(P(d, 1), P(c, k)); }
                else link(A->v[k], P(c, k));
            }
            mv(o, 1, P(c, n));
            free(A); cell_free(o);
            return;
        }
        Port out;
        if (words && jit_call(e, A->v, n, &out)) { for (int k = 0; k < n; k++) era(A->v[k]); }
        else out = apply_gid((long)o->a, A->v, n);
        free(A); link(out, o->port[1]); cell_free(o);
        return;
    }
    case O_PTROF: {
        /* the address of the bytes (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C20): memory the program does not own is reached by it. The bytes
         * whose address is taken are pinned (never freed), so the address stays valid; trusted: a
         * store makes a copy, which the address does not see */
        if (vk != V_BYTES) { stuck(o, v); return; }
        if (v->p) rcl_inc(&((Bytes *)v->p)->rc);
        link(word_of(v->p ? (uint64_t)(uintptr_t)((Bytes *)v->p)->b : 0), o->port[1]);
        era(P(v, 0)); cell_free(o);
        return;
    }
    case O_REGATA: {
        /* the n bytes at an address, copied into a byte string (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C20; trusted: the program vouches
         * for the address) */
        if (vk != V_WORD || v->p) { stuck(o, v); return; }
        size_t n = (size_t)o->a;
        Bytes *b = calloc(1, sizeof(Bytes) + n); b->rc = 1; b->n = n;
        if (v->a && n) memcpy(b->b, (const void *)(uintptr_t)v->a, n);
        Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b;
        link(P(c, 0), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_PIMULT: {
        /* the multiplicity of a Π type's binder, a word: 2 (ω) for anything that is not a Π */
        link(word_of(vk == V_PI ? (uint64_t)v->x : 2), o->port[1]);
        era(P(v, 0)); cell_free(o);
        return;
    }
    case O_PIM: {
        /* (%pim m A B): a Π type whose binder's multiplicity is the word m */
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%pim: a multiplicity is a word 0, 1 or 2\n"); exit(2); }
        Cell *c = cell(V_PI, 3); c->x = (uint32_t)(v->a > 2 ? 2 : v->a);
        mv(o, 1, P(c, 1)); mv(o, 2, P(c, 2)); link(P(c, 0), o->port[3]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_FMULT: case O_FMULT2: {
        /* (%fmult d c k): the words in turn, kept in a (d) and x (c) */
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%fmult: an index is a word\n"); exit(2); }
        Cell *c = cell(o->kind + 1, o->n - 1);
        if (o->kind == O_FMULT) { c->a = v->a; } else { c->a = o->a; c->x = (uint32_t)v->a; }
        for (int k = 1; k < o->n; k++) mv(o, k, P(c, k - 1));
        cell_free(o); cell_free(v);
        return;
    }
    case O_FMULT3: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%fmult: an index is a word\n"); exit(2); }
        int D = checked_data((long)o->a); unsigned m = 2;
        if (D >= 0 && (long)o->x < DATA[D].ncons) { ConInfo *ci = &CON[DATA[D].cons[o->x]]; if ((long)v->a < ci->nfields) m = ci->fmult[v->a]; }
        link(word_of(m), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_GUSAGE: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%gusage: an index is a word\n"); exit(2); }
        Cell *c = cell(O_GUSAGE2, 2); c->a = v->a;
        mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        cell_free(o); cell_free(v);
        return;
    }
    case O_GSTEPS: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%gsteps: an index is a word\n"); exit(2); }
        link(word_of((uint64_t)global_steps((long)v->a)), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_GCODE: {
        extern Port gcode_build(long n);
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%gcode: an index is a word\n"); exit(2); }
        Port c = gcode_build((long)v->a);
        need_cons();
        if (!c) link(con_build(C_NONE, NULL, NULL), o->port[1]);
        else { Port f[1] = { c }; link(con_build(C_SOME, f, NULL), o->port[1]); }
        cell_free(o); cell_free(v);
        return;
    }
    case O_GUSAGE2: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%gusage: an index is a word\n"); exit(2); }
        link(word_of((uint64_t)global_usage((long)o->a, (long)v->a)), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_DNIDX: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%dnidx: an index is a word\n"); exit(2); }
        int D = checked_data((long)v->a);
        link(word_of(D >= 0 ? (uint64_t)DATA[D].nidx : 0), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_DIHT: case O_DIHN: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%diht/%%dihn: an index is a word\n"); exit(2); }
        int D = checked_data((long)v->a);
        link(word_of(D >= 0 ? (uint64_t)(o->kind == O_DIHT ? DATA[D].iht : DATA[D].ihn) : 0), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_LINEAR: {
        /* is the checked data type linear: a constructor with a field at multiplicity 1 */
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%linear: an index is a word\n"); exit(2); }
        need_bool();
        int D = checked_data((long)v->a); bool lin = false;
        if (D >= 0) for (int ci = 0; ci < DATA[D].ncons && !lin; ci++) { ConInfo *c = &CON[DATA[D].cons[ci]]; for (int k = 0; k < c->nfields; k++) if (c->fmult[k] == 1) lin = true; }
        link(con_build(lin ? B_TRUE : B_FALSE, NULL, NULL), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_WSUCC: {
        WN k; Port b; Cell *fr = wsplit(v, &k, &b);
        WN k1 = wn_succ(&k); wn_drop(&k);
        link(wplus(k1, b), o->port[1]);
        if (fr) cell_free(fr);
        cell_free(o);
        return;
    }
    case O_WELIM: {
        /* 0=w 1=z 2=s [3=P, the motive, kept so a stuck elimination's type is known: its endpoint
         * at 0 or 1 when the motive is a path type] out */
        bool hasP = o->n == 5; Port out = o->port[o->n - 1];
        WN k; Port b; Cell *fr = wsplit(v, &k, &b);
        if (fr) cell_free(fr);
        if (wn_zero(&k) && !b) { link(o->port[1], out); era(o->port[2]); if (hasP) era(o->port[3]); }
        else if (wn_zero(&k)) {
            /* a neutral word: the elimination is stuck */
            Cell *e = cell(V_NWELIM, o->n);
            link(b, P(e, 1)); mv(o, 1, P(e, 2)); mv(o, 2, P(e, 3)); if (hasP) mv(o, 3, P(e, 4)); link(P(e, 0), out);
        } else {
            /* s (k-1 ⊕ b) h, h the elimination of k-1 ⊕ b, computed when used */
            WN one = wn_small(1), km = wn_sub(&k, &one);
            Port s1, s2, p1, p2;
            split2(o->port[2], &s1, &s2);
            split2(wplus(km, b), &p1, &p2);
            Cell *h = cell(V_WSUSP, o->n);
            link(o->port[1], P(h, 1)); link(s2, P(h, 2)); link(p2, P(h, 3)); if (hasP) mv(o, 3, P(h, 4));
            link(app2(app2(s1, p1), P(h, 0)), out);
        }
        wn_drop(&k);
        cell_free(o);
        return;
    }
    case O_WOP2: {
        need_bool();
        Cell *ac = PC(o->port[1]);
        WN ka, kb; Port ba, bb;
        Cell *fa = wsplit(ac, &ka, &ba), *fb = wsplit(v, &kb, &bb);
        if (fa) cell_free(fa);
        if (fb) cell_free(fb);
        int op = (int)o->x;
        Port r = 0;
        if (op >= O_WMUL) {
            /* multiplication, division, remainder, bitwise and shifts: on numerals; stuck otherwise */
            if (ba || bb) r = wstuck(op, wplus(wn_copy(&ka), ba), wplus(wn_copy(&kb), bb));
            else if (op == O_WMUL) r = word_wn(wn_mul(&ka, &kb));
            else if (op == O_WDIV || op == O_WMOD) { WN q, m; wn_divmod(&ka, &kb, &q, &m); r = word_wn(op == O_WDIV ? q : m); wn_drop(op == O_WDIV ? &m : &q); }
            else if (op == O_WSHR) r = word_wn(kb.b ? wn_small(0) : wn_shr(&ka, kb.s));
            /* a left shift by more than 2^32 bits: 0 shifted is 0; any other numeral would have more
             * than 2^32 bits, which the machine does not build, so the application stays as it is,
             * a normal form (as on a non-numeral), and nothing ends the run (2026-11-12) */
            else if (op == O_WSHL && (kb.b || kb.s > ((uint64_t)1 << 32))) r = !ka.b && ka.s == 0 ? word_wn(wn_small(0)) : wstuck(op, word_wn(wn_copy(&ka)), word_wn(wn_copy(&kb)));
            else if (op == O_WSHL) r = word_wn(wn_shl(&ka, kb.s));
            else r = word_wn(wn_bits(&ka, &kb, op == O_WAND ? 0 : op == O_WOR ? 1 : 2));
        } else if (!ba && !bb) {
            int c = wn_cmp(&ka, &kb);
            switch (op) {
            case O_WEQ: r = con_build(c == 0 ? B_TRUE : B_FALSE, NULL, NULL); break;
            case O_WLT: r = con_build(c < 0 ? B_TRUE : B_FALSE, NULL, NULL); break;
            case O_WADD: r = word_wn(wn_add(&ka, &kb)); break;
            case O_WMAX: r = word_wn(wn_copy(c > 0 ? &ka : &kb)); break;
            default: r = word_wn(wn_sub(&ka, &kb)); break;
            }
        } else if (op == O_WADD) {
            /* by recursion on b */
            if (!bb) r = wplus(wn_add(&ka, &kb), ba);
            else r = wplus(wn_copy(&kb), wstuck(O_WADD, wplus(wn_copy(&ka), ba), bb));
        } else {
            /* by recursion on both: successors cancel */
            WN c = wn_cmp(&ka, &kb) < 0 ? wn_copy(&ka) : wn_copy(&kb);
            WN a1 = wn_sub(&ka, &c), b1 = wn_sub(&kb, &c);
            bool za = wn_zero(&a1) && !ba, zb = wn_zero(&b1) && !bb;   /* literally zero */
            bool pa = !wn_zero(&a1), pb = !wn_zero(&b1);               /* a successor */
            switch (op) {
            case O_WEQ:
                if ((za && pb) || (zb && pa)) { r = con_build(B_FALSE, NULL, NULL); if (ba) era(ba); if (bb) era(bb); break; }
                r = wstuck(op, wplus(wn_copy(&a1), ba), wplus(wn_copy(&b1), bb)); break;
            case O_WLT:
                if (zb) { r = con_build(B_FALSE, NULL, NULL); if (ba) era(ba); break; }
                if (za && pb) { r = con_build(B_TRUE, NULL, NULL); if (bb) era(bb); break; }
                r = wstuck(op, wplus(wn_copy(&a1), ba), wplus(wn_copy(&b1), bb)); break;
            case O_WMAX:
                if (za) { r = wplus(wn_add(&c, &b1), bb); break; }
                if (zb) { r = wplus(wn_add(&c, &a1), ba); break; }
                r = wplus(wn_copy(&c), wstuck(op, wplus(wn_copy(&a1), ba), wplus(wn_copy(&b1), bb))); break;
            default:
                if (zb) { r = wplus(wn_copy(&a1), ba); break; }
                if (za) { r = word_of(0); if (bb) era(bb); break; }
                r = wstuck(op, wplus(wn_copy(&a1), ba), wplus(wn_copy(&b1), bb)); break;
            }
            wn_drop(&c); wn_drop(&a1); wn_drop(&b1);
        }
        wn_drop(&ka); wn_drop(&kb);
        link(r, o->port[2]);
        cell_free(o);
        return;
    }
    case O_MKFRESH: {
        /* the type decides: read it */
        uint32_t nm = vk == V_WORD && !v->p ? (uint32_t)v->a + 1 : 0;
        for (int k = 1; k < v->n; k++) era(v->port[k]);
        Cell *f = cell(O_FRESHT, 2); f->a = nm;
        mv(o, 1, P(f, 0)); link(P(f, 1), o->port[2]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_FRESHT: {
        uint32_t nm = (uint32_t)o->a;
        if (vk == V_ITY) {
            Dim d = fresh_dim();
            if (nm) dim_named(d, nm - 1);
            Cell *iv = cell(V_IV, 1); iv->p = iv_atom(d); link(P(iv, 0), o->port[1]);
            cell_free(v);
        } else {
            Cell *n = cell(V_NVAR, 2); n->a = LVL_TAG | __atomic_fetch_add(&next_var, 1, __ATOMIC_RELAXED); n->ty = 1; n->x = nm;
            n->key = key0(K_VAR, n->a, NULL);
            link(ref_of(P(v, 0)), P(n, 1)); link(P(n, 0), o->port[1]);
        }
        cell_free(o);
        return;
    }
    case O_MKVAR: case O_MKDIM: {
        /* a fresh variable (of the type at port 1) or dimension, each time; 0 is its source name (a
         * word) or only the trigger */
        uint32_t nm = vk == V_WORD && !v->p ? (uint32_t)v->a + 1 : 0;
        for (int k = 1; k < v->n; k++) era(v->port[k]);
        if (o->kind == O_MKVAR) {
            /* its type is held once: every copy of the variable refers to it */
            Cell *n = cell(V_NVAR, 2); n->a = LVL_TAG | __atomic_fetch_add(&next_var, 1, __ATOMIC_RELAXED); n->ty = 1; n->x = nm;
            n->key = key0(K_VAR, n->a, NULL);   /* fresh for the whole run: its own key */
            link(ref_of(o->port[1]), P(n, 1)); link(P(n, 0), o->port[2]);
        } else {
            Dim d = fresh_dim();
            if (nm) dim_named(d, nm - 1);
            Cell *iv = cell(V_IV, 1); iv->p = iv_atom(d); link(P(iv, 0), o->port[1]);
        }
        cell_free(o); cell_free(v);
        return;
    }
    case O_ABS: {
        /* (%abs i v): the line binding the dimension i */
        Dnf *d = v->p;
        if (vk != V_IV || !d || d->n != 1 || d->c[0]->n != 1 || (d->c[0]->l[0] & 1)) { fprintf(stderr, "%%abs: not a dimension\n"); exit(2); }
        Cell *m = cell(O_MKPLAM, 2); m->a = d->c[0]->l[0] >> 1;
        mv(o, 1, P(m, 0)); link(P(m, 1), o->port[2]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_GLOB: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%gval/%%gty/%%dtab/%%dent: an index is a word\n"); exit(2); }
        o->a = v->a;
        long g = table_global((int)o->x, (long)o->a);
        if (g < 0) { fprintf(stderr, "checker table %u has no entry %llu\n", o->x, (unsigned long long)o->a); exit(2); }
        if (o->x == TB_VAL) {
            /* a checked definition's own value */
            Port val = global_copy(g, NULL);
            long aty = val ? -2 : global_axiom_ty(g);
            if (!val && aty != -2) {
                /* a constant with no computation (a law stated as a primitive): its neutral, carrying
                 * its type, as the elaborated term meets it (S_AXIOM): a path among them reads its
                 * endpoints from that type */
                Cell *c = cell(V_NVAR, 2); c->a = (uint64_t)g; c->key = key0(K_VAR, (uint64_t)g, NULL);
                if (aty >= 0) { Cell *t = cell(V_GLOBAL, 1); t->a = (uint64_t)aty; link(P(t, 0), P(c, 1)); c->ty = 1; }
                else link(unk(), P(c, 1));
                val = P(c, 0);
            }
            if (!val) { fprintf(stderr, "a checked definition without a value\n"); exit(2); }
            link(val, o->port[1]);
        } else { Cell *r = cell(V_GLOBAL, 1); r->a = (uint64_t)g; link(P(r, 0), o->port[1]); }
        cell_free(o); cell_free(v);
        return;
    }
    case O_NTYPE: {
        Port out = o->port[1], T;
        switch (vk) {
        case V_NVAR: T = (v->ty & TY_KNOWN) ? v->port[1] : (era(v->port[1]), unk()); break;
        case V_NPROJ: {
            /* the field's type at the parameters of the neutral's type, the earlier fields its own */
            era(v->port[2]);
            Port n1, n2; split2(v->port[1], &n1, &n2);
            Cell *da = cell(O_DAPPLY, 3); link(ntype_of(n1), P(da, 0));
            Cell *g = cell(V_GLOBAL, 1); g->a = (uint64_t)FLD[v->a].fty; g->key = key_global(FLD[v->a].fty);
            link(P(g, 0), P(da, 1));
            T = app2(P(da, 2), n2);
            break;
        }
        case V_NAPP: era(v->port[3]); T = app2(tpart(ntype_of(v->port[1]), 2), v->port[2]); break;
        case V_NFST: era(v->port[2]); T = tpart(ntype_of(v->port[1]), 1); break;
        case V_NSND: {
            era(v->port[2]);
            Port p1, p2; split2(v->port[1], &p1, &p2);
            Cell *f = cell(O_FST, 2); link(p2, P(f, 0));
            T = app2(tpart(ntype_of(p1), 2), P(f, 1));
            break;
        }
        case V_NPAPP:
            if (v->x) { era(v->port[2]); era(v->port[3]); }
            era(v->port[v->n - 1]);
            T = papp_at(tpart(ntype_of(v->port[1]), 1), dnf_retain(v->p));
            break;
        case V_NCOMP: era(v->port[2]); era(v->port[3]); era(v->port[4]); T = papp_at(v->port[1], dnf_one()); break;
        case V_NUNGLUE: era(v->port[1]); era(v->port[3]); T = tpart(ntype_of(v->port[2]), 1); break;
        case V_NCASE:
            for (int k = 2 + (v->x ? 1 : 0); k < v->n; k++) era(v->port[k]);
            T = v->x ? app2(v->port[2], v->port[1]) : (era(v->port[1]), unk());
            break;
        case V_NWELIM:
            /* a stuck word elimination with its motive: the motive at the word */
            era(v->port[2]); era(v->port[3]);
            T = v->n == 5 ? app2(v->port[4], v->port[1]) : (era(v->port[1]), unk());
            break;
        case V_NTRANS: {
            int np = v->n - 3;
            Cell *d = cell(V_DATA, np + 1); d->a = v->a;
            for (int j = 0; j < np; j++) link(papp_at(v->port[2 + j], dnf_one()), P(d, 1 + j));
            era(v->port[1]); era(v->port[v->n - 1]);
            T = P(d, 0);
            break;
        }
        default: for (int k = 1; k < v->n; k++) era(v->port[k]); T = unk(); break;
        }
        link(T, out);
        cell_free(o);
        cell_free(v);
        return;
    }
    case O_PEND: {
        /* 0=the type, 1=the neutral path, 2=out */
        if (vk == V_PATHP) {
            int z = dnf_is_zero(o->p);
            link(v->port[z ? 2 : 3], o->port[2]); era(v->port[1]); era(v->port[z ? 3 : 2]); era(o->port[1]);
            cell_free(o); cell_free(v);
            return;
        }
        for (int k = 1; k < v->n; k++) era(v->port[k]);
        Cell *n = cell(V_NPAPP, 3); n->p = dnf_retain(o->p);
        mv(o, 1, P(n, 1)); link(unk(), P(n, 2)); link(P(n, 0), o->port[2]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_CLOS: {
        Cell *c = cell(O_CLOS2, 3);
        mv(o, 1, P(c, 0)); link(P(v, 0), P(c, 1)); mv(o, 2, P(c, 2));
        cell_free(o);
        return;
    }
    case O_CLOS2: {
        need_cons();
        static int C_SAME = -1, C_DIFF;
        if (C_SAME < 0) { C_DIFF = con_index("cdiff"); __atomic_store_n(&C_SAME, con_index("csame"), __ATOMIC_RELEASE); }
        if (C_SAME < 0 || C_DIFF < 0) { fprintf(stderr, "%%clos needs CloCmp (lib/core/code.cub)\n"); exit(2); }
        Cell *f = PC(o->port[1]);
        /* two λs of one record, or two eliminations of one table */
        bool fl = PI(o->port[1]) == 0 && (f->kind == V_LAM || f->kind == V_CASEF);
        if (fl && vk == f->kind && f->p == v->p && f->n == v->n && (vk == V_LAM || f->x == v->x)) {
            /* copies of the two environments, paired; the λs themselves are kept for applying
             * when the environments differ */
            int ne = v->n - 1;
            Port *ps = malloc((size_t)(ne + 1) * sizeof(Port));
            for (int k = 0; k < ne; k++) {
                Port a1, a2, b1, b2;
                split2(f->port[k + 1], &a1, &a2); split2(v->port[k + 1], &b1, &b2);
                link(a2, P(f, k + 1)); link(b2, P(v, k + 1));
                ps[k] = pair_of(a1, b1);
            }
            Port fs[3] = { list_of(ps, ne), P(f, 0), P(v, 0) };
            link(con_build(C_SAME, fs, NULL), o->port[2]);
            free(ps);
            cell_free(o);
            return;
        }
        Port fs[2] = { o->port[1], P(v, 0) };
        link(con_build(C_DIFF, fs, NULL), o->port[2]);
        cell_free(o);
        return;
    }
    case O_BRANCH: {
        /* the branch of an elimination at given fields, hypotheses and dimensions */
        if (vk != V_CASEF) { fprintf(stderr, "%%branch: not an elimination\n"); exit(2); }
        CaseTab *tb = v->p;
        int mb = v->x ? 1 : 0, ne = tb->nenv;
        Cell *c = cell(O_BRANCHC, ne + 3); c->p = tb; c->a = 0;
        mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        if (mb) era(v->port[1]);
        for (int e = 0; e < ne; e++) link(v->port[1 + mb + e], P(c, 2 + e));
        link(P(c, ne + 2), o->port[3]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_BRANCHC: {
        if (vk != V_WORD || v->p) { fprintf(stderr, "%%branch: a constructor index is a word\n"); exit(2); }
        /* 0=rest of the list, environment, out */
        Cell *c = cell(O_BRANCHL, o->n - 1); c->p = o->p; c->a = v->a; c->x = 0;
        for (int k = 2; k < o->n; k++) mv(o, k, P(c, k - 1));
        mv(o, 1, P(c, 0));
        cell_free(o); cell_free(v);
        return;
    }
    case O_BRANCHL: {
        need_cons();
        CaseTab *tb = o->p;
        int ne = tb->nenv, np = (int)o->x;
        if (vk != V_CON) { fprintf(stderr, "%%branch: parameters are a list\n"); exit(2); }
        if ((int)v->a == C_LCONS) {
            Cell *c = cell(O_BRANCHL, o->n + 1); c->p = tb; c->a = o->a; c->x = (uint32_t)np + 1;
            for (int k = 1; k <= ne + np; k++) mv(o, k, P(c, k));
            link(v->port[1], P(c, ne + np + 1));
            mv(o, o->n - 1, P(c, c->n - 1));
            link(v->port[2], P(c, 0));
            cell_free(o); cell_free(v);
            return;
        }
        /* the parameters are fields, a hypothesis per recursive field, dimensions; a branch not
         * taking hypotheses is given the others */
        ConInfo *ci = &CON[DATA[tb->d].cons[o->a]];
        bool ih = ci->nrec && (tb->mask >> o->a & 1);
        Port env[ne + 1], prm[np + 1];
        int m = 0;
        for (int e = 0; e < ne; e++) env[e] = o->port[1 + e];
        for (int q = 0; q < np; q++) {
            Port x = o->port[1 + ne + q];
            if (!ih && q >= ci->nfields && q < ci->nfields + ci->nrec) era(x); else prm[m++] = x;
        }
        if (m != tb->br[o->a]->nparam) { fprintf(stderr, "%%branch: %d parameters for a branch of %d\n", m, tb->br[o->a]->nparam); exit(2); }
        link(record_build_n(tb->br[o->a], prm, env), o->port[o->n - 1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_IEQ: case O_FEQL: {
        if (vk != (o->kind == O_IEQ ? V_IV : V_FACE)) { fprintf(stderr, "%%ieq/%%feq: not an interval or face\n"); exit(2); }
        Cell *c = cell(o->kind == O_IEQ ? O_IEQB : O_FEQLB, 2); c->p = dnf_retain(v->p);
        mv(o, 1, P(c, 0)); mv(o, 2, P(c, 1));
        cell_free(o); cell_free(v);
        return;
    }
    case O_IEQB: case O_FEQLB: {
        need_bool();
        if (vk != (o->kind == O_IEQB ? V_IV : V_FACE)) { fprintf(stderr, "%%ieq/%%feq: not an interval or face\n"); exit(2); }
        link(con_build(dnf_equal(o->p, v->p) ? B_TRUE : B_FALSE, NULL, NULL), o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_CLAUSES: {
        need_cons();
        if (vk != V_FACE) { fprintf(stderr, "%%clauses: not a face\n"); exit(2); }
        Dnf *d = v->p; int n = d ? (int)d->n : 0;
        Port *cs = malloc((size_t)(n + 1) * sizeof(Port));
        for (int k = 0; k < n; k++) cs[k] = face_cell(face_of_clause(d->c[k]));
        link(list_of(cs, n), o->port[1]);
        free(cs);
        cell_free(o); cell_free(v);
        return;
    }
    case O_MOTIVE: {
        /* an elimination as a function (V_CASEF): its motive built over its environment, (some P),
         * or none when it was written without one */
        need_cons();
        if (vk != V_CASEF || v->x) { fprintf(stderr, "%%motive: not an elimination\n"); exit(2); }
        CaseTab *tb = v->p; int ne = v->n - 1;
        Port r;
        if (tb->motive) { Port me[ne + 1]; for (int e = 0; e < ne; e++) me[e] = v->port[1 + e]; r = mk(C_SOME, record_build_n(tb->motive, NULL, me), 0, 0, 0); }
        else { for (int e = 0; e < ne; e++) era(v->port[1 + e]); r = con_build(C_NONE, NULL, NULL); }
        link(r, o->port[1]);
        cell_free(o); cell_free(v);
        return;
    }
    case O_DNF: {
        /* an interval or a face as data: its clauses (a list), each a list of its literals
         * (pair atom b), atom the dimension as an interval; b: negated (an interval), or the
         * endpoint 1 (a face). No clauses: 0, ff; a clause without literals: 1, tt */
        need_cons(); need_bool();
        if (vk != V_IV && vk != V_FACE) { fprintf(stderr, "%%dnf: not an interval or a face\n"); exit(2); }
        Dnf *d = v->p; int n = d ? (int)d->n : 0;
        Port *cs = malloc((size_t)(n + 1) * sizeof(Port));
        for (int k = 0; k < n; k++) {
            Clause *cl = d->c[k]; int m = (int)cl->n;
            Port *ls = malloc((size_t)(m + 1) * sizeof(Port));
            for (int j = 0; j < m; j++) {
                Cell *a = cell(V_IV, 1); a->p = iv_atom((Dim)(cl->l[j] >> 1));
                ls[j] = pair_of(P(a, 0), con_build((cl->l[j] & 1) ? B_TRUE : B_FALSE, NULL, NULL));
            }
            cs[k] = list_of(ls, m);
            free(ls);
        }
        link(list_of(cs, n), o->port[1]);
        free(cs);
        cell_free(o); cell_free(v);
        return;
    }
    case O_RESTRICT: {
        if (vk != V_FACE || !v->p || ((Dnf *)v->p)->n != 1) { fprintf(stderr, "%%restrict: not a clause\n"); exit(2); }
        Subst *t = subst_of_clause(((Dnf *)v->p)->c[0]);
        link(sub(o->port[1], t), o->port[2]);
        subst_release(t);
        cell_free(o); cell_free(v);
        return;
    }
    case O_SYSMAP:
        if (vk == V_SYS) distribute(o, v);
        else { Cell *a = cell(O_APP, 3); mv(o, 1, P(a, 0)); link(P(v, 0), P(a, 1)); link(P(a, 2), o->port[2]); cell_free(o); }
        return;
    case O_PROJ: {
        /* a field: of a copattern definition's body, the field's term (held unbuilt); of a stuck
         * call of a copattern definition, the call unfolded and then the field; of any other
         * neutral, a neutral projection */
        Port out = o->port[1];
        int k = FLD[o->a].index;
        if (vk == V_REC && k + 1 < v->n) {
            for (int m = 1; m < v->n; m++) if (m != k + 1) era(v->port[m]);
            link(v->port[k + 1], out);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else if (is_neutral(vk) && (v->ty & TY_COFIX)) {
            Cell *u = cell(O_UNFOLD, 2);
            link(P(v, 0), P(u, 0)); link(P(u, 1), P(o, 0));
        } else if (is_neutral(vk)) {
            Cell *n = cell(V_NPROJ, 3); n->a = o->a;
            link(P(v, 0), P(n, 1)); link(unk(), P(n, 2)); link(P(n, 0), out);
            cell_free(o);
        } else stuck(o, v);
        return;
    }
    case O_UNFOLD: {
        /* a stuck call taken apart from its head: the body of the definition applied to the same
         * arguments, at the same dimensions. The application is named by the call it unfolds, so two
         * unfoldings of one call while it is pending share the body */
        Port out = o->port[1];
        if (vk == V_NAPP) {
            Cell *u = cell(O_UNFOLD, 2), *a = cell(O_APP, 3);
            link(v->port[1], P(u, 0)); link(P(u, 1), P(a, 0)); link(v->port[2], P(a, 1)); link(P(a, 2), out);
            era(v->port[3]);
            if (v->key) { a->key = key1(K_UNFOLD, 0, NULL, v->key); if (a->key) memo_register(a); }
            cell_free(o); cell_free(v);
        } else if (vk == V_NPAPP) {
            Cell *u = cell(O_UNFOLD, 2), *pr = cell(O_PAPPR, 3);
            link(v->port[1], P(u, 0));
            link(iv_cell(dnf_retain(v->p)), P(pr, 0)); link(P(u, 1), P(pr, 1)); link(P(pr, 2), out);
            if (v->x) { era(v->port[2]); era(v->port[3]); }
            era(v->port[v->n - 1]);
            cell_free(o); cell_free(v);
        } else if (vk == V_NVAR && (v->ty & TY_COFIX)) {
            long b = global_unfold((long)v->a);
            Cell *g = cell(V_GLOBAL, 1); g->a = (uint64_t)b; g->key = key_global(b);
            link(P(g, 0), out); era(v->port[1]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else { link(P(v, 0), out); cell_free(o); }
        return;
    }
    case O_TICKAPP: {
        /* the tick a stuck call of a tick-unfolding definition is applied to: ⋄ unfolds it (the
         * body applied to the same arguments and ⋄); a tick variable leaves a stuck application,
         * no longer a call that unfolds */
        static int diamond = -2;
        if (diamond == -2) diamond = con_index("⋄");
        Port out = o->port[2];
        if (vk == V_CON && (int)v->a == diamond) {
            Cell *u = cell(O_UNFOLD, 2), *a = cell(O_APP, 3);
            link(o->port[1], P(u, 0)); link(P(u, 1), P(a, 0)); link(P(v, 0), P(a, 1)); link(P(a, 2), out);
            cell_free(o);
        } else {
            Cell *n = cell(V_NAPP, 4);
            mv(o, 1, P(n, 1)); link(P(v, 0), P(n, 2)); link(unk(), P(n, 3));
            link(P(n, 0), out);
            cell_free(o);
        }
        return;
    }
    case O_DAPPLY:
        if (vk == V_DATA) {
            Port f = o->port[1];
            for (int k = 1; k < v->n; k++) f = app2(f, v->port[k]);
            link(f, o->port[2]);
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else stuck(o, v);
        return;
    default:
        stuck(o, v); return;
    }
}

/* ---------------- rewriting one active pair ----------------
 * The pair's two cells are reachable from nothing but the pair (their principal ports met, and a
 * principal port is held in one place), and a rule writes only them, cells it makes, variables (by
 * exchange) and held values' slots (by compare-and-swap): so no lock, and a popped pair is never
 * stale. The pairs a rule makes are published when it is done. */
static inline void finished(void) { Local *l = local(); __atomic_store_n(&l->done, l->done + 1, __ATOMIC_RELEASE); }
static void rewrite(Redex r) {
    interact(r.op, r.v);
    retire_boxes();
    flush_frees();
    if (net_solo) pending--;
    else { publish(); finished(); }
}

/* ---------------- workers ---------------- */
static int nworkers;
static pthread_t *workers;
static pthread_mutex_t wmu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wcv = PTHREAD_COND_INITIALIZER, pcv = PTHREAD_COND_INITIALIZER;
static int running, parked;
/* a thread finding no pair: spins briefly, then yields, then sleeps a little (an idle thread does
 * not keep a core busy; it picks up new work within a sleep) */
static void idle_wait(int *idle) {
    int k = ++*idle;
    if (k < 128) {
        /* 2026-11-13: use the host's spin hint, including Apple Silicon. */
#if defined(__i386__) || defined(__x86_64__)
        __builtin_ia32_pause();
#elif defined(__aarch64__)
        __asm__ volatile("yield");
#else
        sched_yield();
#endif
    }
    else if (k < 1024) sched_yield();
    else { struct timespec ts = { 0, k < 4096 ? 20000 : 200000 }; nanosleep(&ts, NULL); }
}
static void *worker(void *arg) {
    myq = (int)(intptr_t)arg;
    int idle = 0;
    for (;;) {
        if (!__atomic_load_n(&running, __ATOMIC_ACQUIRE)) {
            pthread_mutex_lock(&wmu);
            parked++; pthread_cond_broadcast(&pcv);
            while (!__atomic_load_n(&running, __ATOMIC_ACQUIRE)) pthread_cond_wait(&wcv, &wmu);
            parked--;
            pthread_mutex_unlock(&wmu);
            idle = 0;
        }
        Redex r;
        if (pop(&r, idle)) { rewrite(r); idle = 0; } else idle_wait(&idle);
    }
    return NULL;
}
void net_start(int n) {
    key_trace = getenv("CUBICAL_KEYS") != NULL;
    no_memo = getenv("CUBICAL_NOMEMO") != NULL;
    shards_init();
    queues_init(n + 1);
    nworkers = n;
    net_solo = n == 0;   /* stays false when some workers fail to start: the scheduler serves those that did */
    workers = calloc((size_t)n + 1, sizeof(pthread_t));
    /* a worker that cannot be created (each gets the process's stack size, 1 GB when main raised
     * it) is not waited for: the run goes on with the workers that started (2026-11-12: the loop
     * below waited forever for a worker that did not exist) */
    int made = 0;
    for (int i = 0; i < n; i++) {
        int e = pthread_create(&workers[i], NULL, worker, (void *)(intptr_t)(i + 1));
        if (e) { fprintf(stderr, "warning: %d of %d workers started (pthread_create: %s)\n", made, n, strerror(e)); break; }
        made++;
    }
    pthread_mutex_lock(&wmu);
    nworkers = made;
    pthread_mutex_unlock(&wmu);
    pthread_mutex_lock(&wmu);
    while (parked < nworkers) pthread_cond_wait(&pcv, &wmu);
    pthread_mutex_unlock(&wmu);
}
static bool observed(Cell *root) {
    if (!root) return false;
    Cell *v = cell_at(sload(&root->port[0]));
    return v && is_value(v->kind);
}
/* an observation: reduce, every worker taking pairs in arrival order, until
 * the observed port holds a value; the rest of the net stands */
/* The workers are woken only when there is work for them (several pairs pending), and parked
 * again before the observation returns, since the reader then walks the value without locks.
 * Reading back a value is one observation per node, most of them finding the node already a
 * value: those cost no wake and no park (each is two futex round trips and a context switch) */
#define WAKE_AT 8
bool net_observe(Cell *root, long limit) {
    retire_boxes(); flush_frees();
    publish();
    long n = 0; int idle = 0;
    bool ok, woke = false;
    for (;;) {
        if ((ok = observed(root))) break;
        if (limit >= 0 && n >= limit) break;
        if (net_solo) {
            if (pending == 0) break;
        } else if (nworkers && !woke && Q[myq].bottom - __atomic_load_n(&Q[myq].top, __ATOMIC_ACQUIRE) >= WAKE_AT) {
            pthread_mutex_lock(&wmu); __atomic_store_n(&running, 1, __ATOMIC_RELEASE); pthread_cond_broadcast(&wcv); pthread_mutex_unlock(&wmu);
            woke = true;
        }
        Redex r;
        if (pop(&r, idle)) { rewrite(r); n++; idle = 0; }
        else { idle_wait(&idle); if (!net_solo && (idle & 63) == 0 && quiescent()) break; }   /* the sums are read now and then: reading them pulls the workers' lines */
    }
    if (woke) {
        pthread_mutex_lock(&wmu);
        __atomic_store_n(&running, 0, __ATOMIC_RELEASE);
        while (parked < nworkers) pthread_cond_wait(&pcv, &wmu);
        pthread_mutex_unlock(&wmu);
    }
    stat_gather();
    arenas_release();
    if (!ok && root && limit < 0 && key_trace) {
        /* the reading stopped short of a value: what is at the root (an operation waiting there, or
         * nothing yet) */
        Port q = peek(root->port[0]); Cell *c = q ? PC(q) : NULL;
        fprintf(stderr, "stuck: root -> kind %d", c ? c->kind : -1);
        if (c && c->key) { fputc(' ', stderr); key_print(stderr, c->key); }
        fputc('\n', stderr);
    }
    return ok || observed(root);
}
/* reduce every pending pair: the workload's normal form, or what waits on an input */
void net_drain(void) { net_observe(NULL, -1); }
/* every payload pointer the net holds, a cell's to cellf and a key's to keyf (NULL: not walked): the
 * roots lang.c's records_sweep marks from. Run with every worker parked. A freed cell's p is NULL
 * (payload_release), so a cell on a free list hands nothing; a slab of variables holds no payloads */
void net_scan_payloads(void (*cellf)(void *), void (*keyf)(void *)) {
    if (cellf) {
        pthread_mutex_lock(&allmu);
        for (Local *l = ALL; l; l = l->next)
            for (Slab *x = l->slabs; x; x = x->next) {
                if (x->n == ~0u) continue;
                size_t sz = cell_bytes((int)x->n);
                for (size_t off = SLAB_HDR; off + sz <= x->used; off += sz) { Cell *c = (Cell *)((char *)x + off); if (c->p) cellf(c->p); }
            }
        pthread_mutex_unlock(&allmu);
        pthread_mutex_lock(&bigmu);
        for (size_t i = 0; i < nbigs; i++) if (BIGS[i]->p) cellf(BIGS[i]->p);
        pthread_mutex_unlock(&bigmu);
    }
    if (keyf) for (int i = 0; i < KSH; i++) for (size_t j = 0; j < SH[i].KTcap; j++) if (SH[i].KT[j] && SH[i].KT[j]->p) keyf(SH[i].KT[j]->p);
}
/* memory accounting (CUBICAL_MEMDEF): slabs held over every thread, keys interned */
void net_mem(long *slabs, long *keys) {
    long sl = 0, k = 0;
    pthread_mutex_lock(&allmu);
    for (Local *l = ALL; l; l = l->next) for (Slab *x = l->slabs; x; x = x->next) sl++;
    pthread_mutex_unlock(&allmu);
    for (int i = 0; i < KSH; i++) k += (long)SH[i].KTn;
    *slabs = sl; *keys = k;
}

/* ---------------- building a net from syntax ---------------- */
typedef struct { Port *sup; int cur, n; Key *key; Port inl[4]; } Supply;   /* inl: the providers of a slot used at most 3 times */
typedef struct { Supply *s; int nslots; const int *uses; Key *scrut; } Frame;   /* scrut: a case branch's scrutinee's key (the value it rebuilds is named by it) */

static void bind(Frame *f, int slot, Port prov) {
    int u = f->uses[slot];
    Supply *s = &f->s[slot];
    s->sup = u + 1 <= 4 ? s->inl : malloc((size_t)(u + 1) * sizeof(Port)); s->n = u; s->cur = 0;
    s->key = key_of_port(prov);
    split(prov, u, s->sup);
}
static void bind_dim(Frame *f, int slot, Dim i) {
    int u = f->uses[slot];
    Supply *s = &f->s[slot];
    s->sup = u + 1 <= 4 ? s->inl : malloc((size_t)(u + 1) * sizeof(Port)); s->n = u; s->cur = 0;
    Dnf *at = iv_atom(i); s->key = key0(K_IV, 0, at); dnf_release(at);
    for (int k = 0; k < u; k++) s->sup[k] = iv_cell(iv_atom(i));
}
/* the key of a compiled term in a frame, without building it: the term with its slots' keys.
 * NULL where a part has no key or the form is not keyed. The caller owns the result */
static Key *synkey(Syn *t, Frame *f) {
    Key *r = NULL, *x = NULL, *y = NULL, *z = NULL;
    switch (t->tag) {
    case S_VAR: { Key *k = f->s[t->slot].key; return k ? key_retain(k) : NULL; }
    case S_LAM: case S_THUNK: {
        /* a thunk is its body's term; a λ is its record over its environment */
        Key **env = malloc(((size_t)t->n + 1) * sizeof(Key *)); bool ok = true;
        for (int k = 0; k < t->n; k++) { env[k] = synkey(t->k[k], f); if (!env[k]) ok = false; }
        if (ok) {
            if (t->tag == S_LAM) r = key(K_LAM, 0, t->rec, t->n, env);
            else {
                Record *rc = t->rec;
                Frame g = { calloc((size_t)rc->nslots + 1, sizeof(Supply)), rc->nslots, rc->uses, NULL };
                for (int k = 0; k < rc->nenv; k++) g.s[rc->env_slot[k]].key = env[k];
                r = synkey(rc->body, &g);
                free(g.s);
            }
        }
        for (int k = 0; k < t->n; k++) if (env[k]) key_release(env[k]);
        free(env);
        return r;
    }
    case S_APP: x = synkey(t->k[0], f); y = x ? synkey(t->k[1], f) : NULL; r = key2(K_APP, 0, NULL, x, y); break;
    case S_CALL: {
        r = key0(K_GLOBAL, (uint64_t)t->id, NULL);
        for (int k = 0; k < t->n && r; k++) { Key *a = synkey(t->k[k], f); Key *n = key2(K_APP, 0, NULL, r, a); key_release(r); if (a) key_release(a); r = n; }
        return r;
    }
    case S_PAIR: case S_PI: case S_SIGMA: case S_PATHP: case S_DATA: case S_CON: {
        if (t->tag == S_CON && t->x) return f->scrut ? key_retain(f->scrut) : NULL;   /* the scrutinee rebuilt: its key */
        int tag = t->tag == S_PAIR ? K_PAIR : t->tag == S_PI ? K_PI : t->tag == S_SIGMA ? K_SIGMA : t->tag == S_PATHP ? K_PATHP : t->tag == S_DATA ? K_DATA : K_CON;
        Key **c = malloc(((size_t)t->n + 1) * sizeof(Key *)); bool ok = true;
        for (int k = 0; k < t->n; k++) { c[k] = synkey(t->k[k], f); if (!c[k]) ok = false; }
        if (ok) r = key(tag, (uint64_t)t->id, NULL, t->n, c);
        for (int k = 0; k < t->n; k++) if (c[k]) key_release(c[k]);
        free(c);
        return r;
    }
    case S_FST: x = synkey(t->k[0], f); r = key1(K_FST, 0, NULL, x); break;
    case S_SND: x = synkey(t->k[0], f); r = key1(K_SND, 0, NULL, x); break;
    case S_FORCE: return synkey(t->k[0], f);   /* forcing a suspension is the suspended term */
    case S_PAPP: if (t->x) return NULL; x = synkey(t->k[0], f); y = x ? synkey(t->k[1], f) : NULL; r = key2(K_PAPP, 0, NULL, x, y); break;
    case S_U: { if (t->n) { x = synkey(t->k[0], f); r = key1(K_U, 0, NULL, x); } else r = key0(K_U, 0, NULL); break; }
    case S_WORD: { Big *b = t->big; r = key0(K_WORD, (uint64_t)t->id, b); return r; }
    case S_WORDTY: return key0(K_WTY, 0, NULL);
    case S_IV0: { Dnf *d = dnf_zero(); r = key0(K_IV, 0, d); dnf_release(d); return r; }
    case S_IV1: { Dnf *d = dnf_one(); r = key0(K_IV, 0, d); dnf_release(d); return r; }
    case S_FTT: { Dnf *d = dnf_one(); r = key0(K_FACE, 0, d); dnf_release(d); return r; }
    case S_FFF: { Dnf *d = dnf_zero(); r = key0(K_FACE, 0, d); dnf_release(d); return r; }
    case S_GLOBAL: return key0(K_GLOBAL, (uint64_t)t->id, NULL);
    case S_AXIOM: return key0(K_VAR, (uint64_t)t->id, NULL);
    case S_CASE: {
        /* the table, the scrutinee and the shared variables */
        int ne = t->tab->nenv;
        Key **c = malloc(((size_t)ne + 2) * sizeof(Key *)); bool ok = true;
        c[0] = synkey(t->k[0], f); if (!c[0]) ok = false;
        for (int e = 0; e < ne; e++) { c[1 + e] = synkey(t->k[2 + e], f); if (!c[1 + e]) ok = false; }
        if (ok) r = key(K_CASE, 0, t->tab, ne + 1, c);
        for (int e = 0; e < ne + 1; e++) if (c[e]) key_release(c[e]);
        free(c);
        return r;
    }
    case S_LET: {
        Key *saved = f->s[t->slot].key;
        f->s[t->slot].key = synkey(t->k[0], f);
        r = synkey(t->k[1], f);
        if (f->s[t->slot].key) key_release(f->s[t->slot].key);
        f->s[t->slot].key = saved;
        return r;
    }
    default: return NULL;
    }
    if (x) key_release(x);
    if (y) key_release(y);
    if (z) key_release(z);
    return r;
}
/* Call by need (CBN; PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”). Every operand is compiled unbuilt: an operation in an operand position is
 * built as a V_DEFER cell (its kind, payload and inputs, the operation not yet run), run by its
 * reader (an operation meeting it), held once by a duplicator, erased unrun if nobody reads it. Its
 * own operands are deferred the same way, so nothing below an unread operand runs. Built strict
 * (`lazy` false) are only what is read as soon as the building is: the result of a body being built
 * (its reader asked for it) and, through it, the inputs its operation always reads (op_reads); in
 * a deferred operation those are deferred too and are run with it, without a step, when it is run
 * (defer_build), as building it strict would have. Values are built as they are, their parts
 * deferred. */
static Port build_m(Syn *t, Frame *f, bool lazy);
static Port build(Syn *t, Frame *f) { return build_m(t, f, false); }
/* does the operation of syntax t always read its operand q (the port it goes to), in every outcome
 * but a stuck one? Its principal; a plain path application's path (x = 0: a typed one's endpoints
 * answer at 0 and 1 without it); a meet or join's second operand; a transport's value (read, or
 * passed on as the result where ψ holds: whoever runs the transport reads it); a call's parameters
 * its definition always reads (lang.c's analysis, G.strict) */
static bool syn_reads(Syn *t, int port) {
    if (port == 0) return true;
    switch (t->tag) {
    case S_PAPP: return !t->x && port == 1;
    case S_BIN: return port == 1;
    case S_TRANSD: return port == t->n - 1;   /* the value transported: read, or the result itself where ψ holds */
    /* a word operation's second operand (it reads the first, then the second, unless the first is
     * neutral): built strict, not deferred */
    case S_PRIM: return port == 1 && (t->x == O_WEQ || t->x == O_WLT || t->x == O_WADD || t->x == O_WMAX || (t->x >= O_WSUB && t->x <= O_WSHR));
    case S_CALL: { extern uint64_t global_strict(long g); int q = port - 1; return q < 64 && (global_strict((long)t->id) >> q & 1); }
    default: return false;
    }
}
#define OPC(kind, n) (lazy ? defer_cell((kind), (n) - 1) : cell((kind), (n)))
#define IN(c, k) P((c), lazy ? (k) + 1 : (k))
#define OUT(c) (lazy ? P((c), 0) : P((c), (c)->n - 1))
/* an operand of t at the operation's port k: built strict when t's operation is and reads it always */
#define OPND(s, k) build_m((s), f, lazy || !syn_reads(t, (k)))
static Port op1(int kind, Syn *a, Frame *f, bool lazy) { Cell *c = OPC(kind, 2); link(build_m(a, f, lazy), IN(c, 0)); return OUT(c); }
static Port build_m(Syn *t, Frame *f, bool lazy) {
    Cell *c;
    switch (t->tag) {
    case S_VAR: {
        Supply *s = &f->s[t->slot];
        if (s->cur >= s->n) { fprintf(stderr, "internal: slot %d used beyond its count\n", t->slot); exit(2); }
        return s->sup[s->cur++];
    }
    case S_LAM: case S_THUNK:
        c = cell(t->tag == S_LAM ? V_LAM : V_THUNK, t->n + 1); c->p = t->rec;
        for (int k = 0; k < t->n; k++) link(build_m(t->k[k], f, true), P(c, k + 1));
        c->key = synkey(t, f);
        if (t->tag == S_THUNK && c->key) memo_register(c);
        return P(c, 0);
    case S_APP:
        c = OPC(O_APP, 3); link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1));
        c->key = synkey(t, f); if (c->key) memo_register(c);
        return OUT(c);
    case S_PAIR: c = cell(V_PAIR, 3); goto fields;
    case S_PI: c = cell(V_PI, 3); c->x = t->x; goto fields;
    case S_SIGMA: c = cell(V_SIGMA, 3); goto fields;
    case S_PATHP: c = cell(V_PATHP, 4); goto fields;
    fields:
        for (int k = 0; k < c->n - 1; k++) link(build_m(t->k[k], f, true), P(c, k + 1));
        c->key = synkey(t, f);
        return P(c, 0);
    case S_FST: { Port r = op1(O_FST, t->k[0], f, lazy); PC(r)->key = synkey(t, f); return r; }
    case S_SND: { Port r = op1(O_SND, t->k[0], f, lazy); PC(r)->key = synkey(t, f); return r; }
    case S_U: {
        /* U at its level: lzero when none is written */
        c = cell(V_U, 2);
        if (t->n) link(build_m(t->k[0], f, true), P(c, 1));
        else { int z = con_index("lzero"); if (z < 0) { fprintf(stderr, "U needs Level (lib/core/prelude.cub)\n"); exit(2); } link(con_build(z, NULL, NULL), P(c, 1)); }
        return P(c, 0);
    }
    case S_BOX: return ref_of(build_m(t->k[0], f, true));
    case S_WORD: c = cell(V_WORD, 1); c->a = (uint64_t)t->id; c->p = big_retain(t->big); c->key = synkey(t, f); return P(c, 0);
    case S_WORDTY: c = cell(V_WTY, 1); c->key = synkey(t, f); return P(c, 0);
    case S_ITY: c = cell(V_ITY, 1); return P(c, 0);
    case S_PART: c = OPC(O_MKPART, 3); link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1)); return OUT(c);
    case S_CALL: {
        c = OPC(O_CALL, t->n + 2);
        for (int k = 0; k < t->n; k++) link(OPND(t->k[k], 1 + k), IN(c, 1 + k));
        Cell *gv = cell(V_GLOBAL, 1); gv->a = (uint64_t)t->id; gv->key = key_global(t->id);
        link(P(gv, 0), IN(c, 0));
        c->key = synkey(t, f); if (c->key) memo_register(c);
        return OUT(c);
    }
    case S_GLOBTAB:
        c = OPC(O_GLOB, 2); c->x = t->x;
        link(OPND(t->k[0], 0), IN(c, 0));
        return OUT(c);
    case S_PRIM:
        c = OPC((int)t->x, t->n + 1);
        if (t->x == O_POP1 || t->x == O_POP2 || t->x == O_LOAD || t->x == O_STORE || t->x == O_FFI || t->x == O_NATIVE) c->x = (uint32_t)t->id;   /* the operation, the width, or the foreign entry */
        if (t->x == O_NATIVE) c->a = (uint64_t)t->slot;   /* the definition the bytes compute */
        for (int k = 0; k < t->n; k++) link(OPND(t->k[k], k), IN(c, k));
        return OUT(c);
    case S_PLAM: case S_FORALL: {
        Dim i = fresh_dim();
        bind_dim(f, t->slot, i);
        Cell *m = OPC(t->tag == S_PLAM ? O_MKPLAM : O_FORALL, 2); m->a = i;
        link(OPND(t->k[0], 0), IN(m, 0));
        return OUT(m);
    }
    case S_PAPP:
        c = OPC(O_PAPPR, t->x ? 5 : 3); c->x = t->x;
        link(OPND(t->k[1], 0), IN(c, 0)); link(OPND(t->k[0], 1), IN(c, 1));
        if (t->x) { link(OPND(t->k[2], 2), IN(c, 2)); link(OPND(t->k[3], 3), IN(c, 3)); }
        return OUT(c);
    case S_IV0: { Port r = iv_cell(dnf_zero()); PC(r)->key = synkey(t, f); return r; }
    case S_IV1: { Port r = iv_cell(dnf_one()); PC(r)->key = synkey(t, f); return r; }
    case S_NEG: return op1(O_NEG, t->k[0], f, lazy);
    case S_BIN: c = OPC(O_BIN, 3); c->x = t->x; link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1)); return OUT(c);
    case S_FEQ: c = OPC(O_FEQ, 2); c->a = t->x; link(OPND(t->k[0], 0), IN(c, 0)); return OUT(c);
    case S_FTT: c = cell(V_FACE, 1); c->p = dnf_one(); c->key = synkey(t, f); return P(c, 0);
    case S_FFF: c = cell(V_FACE, 1); c->p = dnf_zero(); c->key = synkey(t, f); return P(c, 0);
    case S_FACEOF: return op1(O_FACEOF, t->k[0], f, lazy);
    case S_FNEG: return op1(O_FNEG, t->k[0], f, lazy);
    case S_SYS0: c = cell(V_SYS, 1); c->p = NULL; return P(c, 0);
    case S_SYSCONS:
        c = OPC(O_SYSCONS, 4);
        link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1)); link(OPND(t->k[2], 2), IN(c, 2));
        return OUT(c);
    case S_SYSMAP: c = OPC(O_SYSMAP, 3); link(OPND(t->k[1], 0), IN(c, 0)); link(OPND(t->k[0], 1), IN(c, 1)); return OUT(c);
    case S_COMP:
        c = OPC(O_COMP, 4);
        link(OPND(t->k[1], 0), IN(c, 0)); link(OPND(t->k[0], 1), IN(c, 1)); link(OPND(t->k[2], 2), IN(c, 2));
        return OUT(c);
    case S_GLUE: c = OPC(O_MKGLUE, 3); c->x = t->x; link(OPND(t->k[1], 0), IN(c, 0)); link(OPND(t->k[0], 1), IN(c, 1)); return OUT(c);
    case S_GLUEV: c = OPC(O_MKGLUEV, 3); link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1)); return OUT(c);
    case S_UNGLUE: c = OPC(O_UNGLUE, 3); c->x = t->x; link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1)); return OUT(c);
    case S_GLOBAL: c = cell(V_GLOBAL, 1); c->a = (uint64_t)t->id; c->key = key_global(t->id); return P(c, 0);
    case S_AXIOM:
        c = cell(V_NVAR, 2); c->a = (uint64_t)t->id; c->key = synkey(t, f);
        if (t->slot >= 0) { Cell *g = cell(V_GLOBAL, 1); g->a = (uint64_t)t->slot; link(P(g, 0), P(c, 1)); c->ty = 1; }
        else link(unk(), P(c, 1));
        return P(c, 0);
    case S_FORCE: { Port r = op1(O_FORCE, t->k[0], f, lazy); PC(r)->key = synkey(t, f); return r; }
    case S_DATA: c = cell(V_DATA, t->n + 1); c->a = (uint64_t)t->id; goto fields;
    case S_CON: {
        ConInfo *ci = &CON[t->id];
        int nf = ci->nfields, nd = ci->ndims;
        if (lazy && ci->bdy >= 0 && nd > 0 && a8_conw()) {
            /* a path constructor waiting for its dimensions (con_build), deferred: 0=dimension 0,
             * 1..nf=fields, nf+1..=the other dimensions */
            c = defer_cell(O_CONW, nf + nd); c->a = (uint64_t)t->id; c->x = 0;
            link(build_m(t->k[nf], f, true), P(c, 1));
            for (int j = 0; j < nf; j++) link(build_m(t->k[j], f, true), P(c, 2 + j));
            for (int j = 1; j < nd; j++) link(build_m(t->k[nf + j], f, true), P(c, 1 + nf + j));
            return P(c, 0);
        }
        Port *a = malloc((size_t)(t->n + 1) * sizeof(Port));
        /* a path constructor's first dimension is what its building reads first */
        for (int k = 0; k < t->n; k++) a[k] = build_m(t->k[k], f, !(k == nf && ci->bdy >= 0 && nd > 0 && a8_conw()) || lazy);
        Port r = con_build((int)t->id, a, a + nf);
        free(a);
        if (PI(r) == 0) PC(r)->key = synkey(t, f);   /* a plain constructor (one with a boundary is checked first) */
        return r;
    }
    case S_PROJ: c = OPC(O_PROJ, 2); c->a = (uint64_t)t->id; link(OPND(t->k[0], 0), IN(c, 0)); return OUT(c);
    case S_REC:
        c = cell(V_REC, t->n + 1); c->a = (uint64_t)t->id;
        for (int k = 0; k < t->n; k++) link(build_m(t->k[k], f, true), P(c, k + 1));
        return P(c, 0);
    case S_HCOMPD:
        c = OPC(O_HCOMP, t->n + 1); link(OPND(t->k[0], 0), IN(c, 0)); link(OPND(t->k[1], 1), IN(c, 1));
        if (t->n == 3) link(OPND(t->k[2], 2), IN(c, 2));
        return OUT(c);
    case S_TRANSD:
        c = OPC(O_TRANSD1, t->n + 1); c->a = (uint64_t)t->id;
        for (int k = 0; k < t->n; k++) link(OPND(t->k[k], k), IN(c, k));
        return OUT(c);
    case S_CASE: {
        /* k[0]=scrutinee k[1]=motive (or none) k[2..]=the shared variables */
        int ne = t->tab->nenv;
        c = OPC(O_CASE, ne + 2); c->x = 0; c->p = t->tab;
        link(OPND(t->k[0], 0), IN(c, 0));
        for (int e = 0; e < ne; e++) link(OPND(t->k[2 + e], 1 + e), IN(c, 1 + e));
        c->key = synkey(t, f); if (c->key) memo_register(c);
        return OUT(c);
    }
    case S_LET: {
        bind(f, t->slot, build_m(t->k[0], f, true));
        return build_m(t->k[1], f, lazy);
    }
    }
    fprintf(stderr, "build: tag %d\n", t->tag); exit(2);
}
#undef OPC
#undef IN
#undef OUT
#undef OPND

static Port run_frame(Syn *body, int nslots, const int *uses, int nparam, const Port *params, int nenv, const int *env_slot, const Port *env, int where) {
    Supply local_s[16];
    Supply *ss = nslots + 1 <= 16 ? (memset(local_s, 0, (size_t)(nslots + 1) * sizeof(Supply)), local_s) : calloc((size_t)nslots + 1, sizeof(Supply));
    Frame f = { ss, nslots, uses, scrut_key };
    scrut_key = NULL;
    long saved = dup_origin;
    if (prof_on) dup_origin = where + 1;
    for (int k = 0; k < nparam; k++) bind(&f, k, params[k]);
    for (int k = 0; k < nenv; k++) bind(&f, env_slot[k], env[k]);
    Port r = build(body, &f);
    for (int k = 0; k < nslots; k++) {
        if (f.s[k].cur != f.s[k].n) { fprintf(stderr, "internal: slot %d built %d of %d uses\n", k, f.s[k].cur, f.s[k].n); exit(2); }
        if (f.s[k].sup != f.s[k].inl) free(f.s[k].sup);
        if (f.s[k].key) key_release(f.s[k].key);
    }
    if (f.scrut) key_release(f.scrut);
    if (f.s != local_s) free(f.s);
    dup_origin = saved;
    return r;
}
Port record_build(const Record *r, Port param, const Port *env) {
    return run_frame(r->body, r->nslots, r->uses, r->nparam, &param, r->nenv, r->env_slot, env, r->where);
}
Port record_build_n(const Record *r, const Port *params, const Port *env) {
    return run_frame(r->body, r->nslots, r->uses, r->nparam, params, r->nenv, r->env_slot, env, r->where);
}
Port top_build(Syn *body, int nslots, const int *uses) {
    return run_frame(body, nslots, uses, 0, NULL, 0, NULL, NULL, -1);
}
