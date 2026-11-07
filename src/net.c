/* net.c — cells, wiring, the interaction rules, and building a net from
 * compiled syntax. */
#include "core.h"
#include <string.h>
#include <pthread.h>
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

Cell *cell(int kind, int n) {
    Local *l = local();
    Cell *c;
    if (n > MAXFREE) {
        c = malloc(cell_bytes(n));
        if (!c) { fputs("out of memory\n", stderr); exit(2); }
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
static inline bool owns_dnf(int k) { switch (k) { case O_PEND: case V_IV: case V_FACE: case V_SYS: case V_NPAPP: case O_PAPPP: case O_BIN2: case O_SYSCONS2: case V_NTRANS: case O_TRANSD2: case V_TRANSH: case O_TRANSH: case O_IEQB: case O_FEQLB: return true; default: return false; } }
static bool owns_subst(int k) { return k == O_SUBST; }
static void box_release(Box *b);
static void payload_retain(Cell *c) {
    if (owns_dnf(c->kind)) dnf_retain(c->p); else if (owns_subst(c->kind)) subst_retain(c->p);
    else if (c->kind == V_REF) { rcl_inc(&((Box *)c->p)->rc); if (c->a) subst_retain((Subst *)(uintptr_t)c->a); }
    else if (c->kind == V_WORD || c->kind == V_WPLUS) big_retain(c->p);
    else if (c->kind == V_BYTES && c->p) rcl_inc(&((Bytes *)c->p)->rc);
    else if (c->kind == V_THUNK && c->a) subst_retain((Subst *)(uintptr_t)c->a);
}
static void payload_release(Cell *c) {
    if (owns_dnf(c->kind)) dnf_release(c->p); else if (owns_subst(c->kind)) subst_release(c->p);
    else if (c->kind == V_REF && c->p) { box_release(c->p); if (c->a) subst_release((Subst *)(uintptr_t)c->a); c->a = 0; }
    else if (c->kind == V_WORD || c->kind == V_WPLUS) big_release(c->p);
    else if (c->kind == V_BYTES && c->p) { Bytes *b = c->p; if (rcl_dec(&b->rc) == 0) free(b); }
    else if (c->kind == V_THUNK && c->a) { subst_release((Subst *)(uintptr_t)c->a); c->a = 0; }
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
        while (l->big) { Cell *c = l->big; l->big = (Cell *)(uintptr_t)c->a; free(c); }
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
/* Σdone read before Σpushed: d = D(t1) ≤ P(t1) ≤ P(t2) = p, so d == p means P(t1) == D(t1), every
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
 * holds the box (HANDOFF §36: retention is reachability). When the firing producer is the only
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
    (void)was;
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
        Key *gone = e->pending == 0 ? memo_drop(s, e) : NULL;
        kunlock(s);
        if (gone) key_destroy(gone);
        return 0;
    }
    /* others wait for this value: it is held once and read by reference. Its root holds the wire
     * the value will be linked into (memo_end); a reader finding nothing there yet waits on it */
    b = malloc(sizeof(Box)); b->rc = 2; b->root = cell(O_ROOT, 1); b->key = key_retain(e->k); b->e = e;
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
/* a value held once: references copy the box; the last one erases the value */
static void box_hold(Box *b);
static Key *key_of_port(Port p);
static Box *box_new(Port val) {
    Box *b = malloc(sizeof(Box)); b->rc = 1; b->root = cell(O_ROOT, 1); b->e = NULL; b->in = 0;
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
        Box *b = malloc(sizeof(Box)); b->rc = 2; b->root = cell(O_ROOT, 1); b->e = NULL; b->in = 0;
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
static bool unbuilt(int k) { return k == V_THUNK || k == V_SUSP || k == V_WSUSP; }
static Port case_of_susp(Cell *v);
static Port welim_of_wsusp(Cell *v);
Port unbuilt_build(Cell *v) {
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
static Port ref_read(Cell *r) {
    Box *b = r->p;
    Port p = lazy_copy(r->a ? b->key : r->key, b->root);
    return r->a ? sub(p, (Subst *)(uintptr_t)r->a) : p;
}
Port ref_copy(Cell *c) { return ref_read(c); }
/* a word's suspended elimination, computed (for readback) */
Port wsusp_force(Cell *c) {
    Cell *e = cell(O_WELIM, 4);
    link(c->port[3], P(e, 0)); link(c->port[1], P(e, 1)); link(c->port[2], P(e, 2));
    return P(e, 3);
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
static Port sub(Port prov, Subst *s) { Cell *c = cell(O_SUBST, 2); c->p = subst_retain(s); link(prov, P(c, 0)); return P(c, 1); }  /* borrows s */
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
/* the induction hypotheses of fields recursive under paths, Π codomains and Σ components (18.18),
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
/* the emitted runtime's boxed values (B18): tag 0 a word in n, tag 2 bytes (n of them after the header), tag c+1
 * constructor c with n fields. A value of the net is boxable when it is a word, a byte string, or a constructor
 * whose fields are such values already (a field not yet a value is not forced here: the call falls back to the
 * definition on the net) */
typedef struct VB { uint64_t tag, n; struct VB *f[]; } VB;
Port unbox_value(VB *v, int d);
static bool boxable(Cell *c) {
    if (!c) return false;
    if (c->kind == V_WORD) return !c->p;
    if (c->kind == V_BYTES) return true;
    if (c->kind == V_CON) { ConInfo *k = &CON[c->a]; if (k->ndims) return false; for (int j = 0; j < k->nfields; j++) if (!boxable(cell_at(c->port[1 + j]))) return false; return true; }
    return false;
}
static VB *box_value(Cell *c) {
    if (c->kind == V_WORD) { VB *v = calloc(1, sizeof(VB)); v->tag = 0; v->n = c->a; return v; }
    if (c->kind == V_BYTES) { Bytes *b = c->p; size_t bn = b ? b->n : 0; VB *v = calloc(1, sizeof(VB) + bn); v->tag = 2; v->n = bn; if (bn) memcpy((unsigned char *)v->f, b->b, bn); return v; }
    ConInfo *k = &CON[c->a];
    VB *v = calloc(1, sizeof(VB) + (size_t)k->nfields * sizeof(VB *)); v->tag = (uint64_t)k->index + 1; v->n = (uint64_t)k->nfields;
    for (int j = 0; j < k->nfields; j++) v->f[j] = box_value(cell_at(c->port[1 + j]));
    return v;
}
static void free_boxed(VB *v) { if (!v) return; if (v->tag != 0 && v->tag != 2) for (uint64_t j = 0; j < v->n; j++) free_boxed(v->f[j]); free(v); }
/* a boxed result into cells; d the data type (machine index) of a constructor, -1 when unknown (then a word) */
Port unbox_value(VB *v, int d) {
    /* d: -2 a word, -3 bytes, a data type's machine index for a constructor, -1 unknown (a word by
     * tag 0, else bytes). The tags alone do not decide: constructor 1 and bytes share tag 2 */
    if (!v) return word_of(0);
    if (d == -2 || (d == -1 && v->tag == 0) || (d >= 0 && v->tag == 0)) return word_of(v->n);
    if (d == -3 || d == -1) { size_t bn = (size_t)v->n; Bytes *b = calloc(1, sizeof(Bytes) + bn); b->rc = 1; b->n = bn; if (bn) memcpy(b->b, (unsigned char *)v->f, bn); Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b; return P(c, 0); }
    int ci = (int)v->tag - 1;
    if (ci < 0 || ci >= DATA[d].ncons) return word_of(v->n);
    int c = DATA[d].cons[ci]; int nf = CON[c].nfields;
    Port *f = malloc((size_t)(nf + 1) * sizeof(Port));
    for (int j = 0; j < nf; j++) { int fd = data_field_type(d, ci, j); f[j] = unbox_value(j < (int)v->n ? v->f[j] : NULL, fd); }
    Port r = con_build(c, f, NULL);
    free(f);
    return r;
}
/* a foreign entry called on its arguments' bits (D27, B18): every argument and the result integers
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
Port con_build(int c, const Port *f, const Port *d) {
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

/* substitution of dimensions */
static void rule_subst(Cell *o, Cell *v) {
    Subst *s = o->p;
    Port out = o->port[1];
    switch (v->kind) {
    case V_GLOBAL: case V_UNK:
        link(P(v, 0), out); cell_free(o); return;
    case V_THUNK: case V_REF: {
        /* the reference (the unbuilt subterm) carries the substitution: nothing is copied */
        Subst *t = v->a ? subst_compose((Subst *)(uintptr_t)v->a, s) : subst_retain(s);
        if (v->a) subst_release((Subst *)(uintptr_t)v->a);
        v->a = (uint64_t)(uintptr_t)t;
        if (v->e) { memo_leave(v); memo_register(v); }
        link(P(v, 0), out); cell_free(o); return;
    }
    case V_NVAR:
        link(sub(v->port[1], s), P(v, 1));
        link(P(v, 0), out); cell_free(o); return;
    case V_IV: { Dnf *n2 = iv_subst(v->p, s); dnf_release(v->p); v->p = n2; rekey_formula(v); link(P(v, 0), out); cell_free(o); return; }
    case V_FACE: { Dnf *n2 = face_subst(v->p, s); dnf_release(v->p); v->p = n2; rekey_formula(v); link(P(v, 0), out); cell_free(o); return; }
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
    case V_NWELIM: { Cell *a = cell(O_WELIM, 4); for (int k = 1; k <= 3; k++) link(sub(v->port[k], s), P(a, k - 1)); link(P(a, 3), out); break; }
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
        /* constructors: λ (its environment), pair, Π, Σ, PathP */
        Cell *c = clone(v);
        for (int k = 1; k < v->n; k++) link(sub(v->port[k], s), P(c, k));
        link(P(c, 0), out);
        break;
    }
    }
    cell_free(o); cell_free(v);
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
            for (int k = 0; k < nb; k++) link(mkplam_of(v->port[k + 1], i), P(ns, k + 1));
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
    for (int b = 0; b < nb; b++) {
        Port val = v->port[b + 1], res = P(ns, b + 1);
        Cell *c;
        switch (o->kind) {
        case O_APP: c = cell(O_APP, 3); link(val, P(c, 0)); link(cp[0][b], P(c, 1)); link(P(c, 2), res); break;
        case O_SYSMAP: c = cell(O_APP, 3); link(cp[0][b], P(c, 0)); link(val, P(c, 1)); link(P(c, 2), res); break;
        case O_FST: case O_SND: case O_FORCE: case O_TPART: case O_PROJ: case O_UNFOLD: c = cell(o->kind, 2); c->a = o->a; link(val, P(c, 0)); link(P(c, 1), res); break;
        case O_DAPPLY: c = cell(O_DAPPLY, 3); link(val, P(c, 0)); link(cp[0][b], P(c, 1)); link(P(c, 2), res); break;
        case O_FACEOF: c = cell(O_FACEOF, 2); link(val, P(c, 0)); link(P(c, 1), res); break;
        case O_PAPPP:
            c = cell(O_PAPPR, o->x ? 5 : 3); c->x = o->x;
            { Subst *t = subst_of_clause(cl->c[b]); link(iv_cell(iv_subst(o->p, t)), P(c, 0)); subst_release(t); }
            link(val, P(c, 1));
            if (o->x) { link(cp[0][b], P(c, 2)); link(cp[1][b], P(c, 3)); }
            link(P(c, c->n - 1), res);
            break;
        case O_UNGLUE2: c = cell(O_UNGLUE, 3); c->x = o->x; link(cp[0][b], P(c, 0)); link(val, P(c, 1)); link(P(c, 2), res); break;
        case O_COMPA: c = cell(O_COMP, 4); link(cp[0][b], P(c, 0)); link(val, P(c, 1)); link(cp[1][b], P(c, 2)); link(P(c, 3), res); break;
        case O_CASE:
            c = cell(O_CASE, o->n); c->x = o->x; c->p = o->p;
            link(val, P(c, 0));
            for (int e = 0; e < ne; e++) link(cp[e][b], P(c, 1 + e));
            link(P(c, o->n - 1), res);
            break;
        case O_TRANSD2:
            c = cell(O_TRANSD2, o->n); c->a = o->a;
            { Subst *t = subst_of_clause(cl->c[b]); c->p = face_subst(o->p, t); subst_release(t); }
            link(val, P(c, 0));
            for (int e = 0; e < ne; e++) link(cp[e][b], P(c, 1 + e));
            link(P(c, o->n - 1), res);
            break;
        default: fprintf(stderr, "distribute: kind %d\n", o->kind); exit(2);
        }
    }
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
 * %view gives a value's head as a constructor of View (lib/code.cub) whose fields are the
 * value's own ports: nothing is copied, and a part is read further only by viewing it. */
enum { W_LAM, W_PAIR, W_PI, W_SIGMA, W_U, W_PATH, W_PLAM, W_IV, W_FACE, W_SYS, W_GLUE, W_GLUEV, W_DATA, W_CON,
       W_HCOMP, W_VAR, W_APP, W_FST, W_SND, W_PAPP, W_COMP, W_UNGLUE, W_ELIM, W_TRANS, W_LEVEL, W_OTHER,
       W_WORD, W_WLIT, W_WPLUS, W_WOP, W_WELIM, W_COFIX, W_PROJ, W_REC, W_CLOCK, W_TICK, W_CLOCK0, W_TICK0, W_HCOMPU, W_UNGLUEU,
       W_PTY, W_PLIT, W_BYTES, W_PRIM, W_N };
static const char *WNAME[W_N] = { "wlam", "wpair", "wpi", "wsigma", "wu", "wpath", "wplam", "wiv", "wface", "wsys", "wglue",
    "wgluev", "wdata", "wcon", "whcomp", "wvar", "wapp", "wfst", "wsnd", "wpapp", "wcomp", "wunglue", "welim", "wtrans",
    "wlevel", "wother", "wword", "wwlit", "wwplus", "wwop", "wwelim", "wcofix", "wproj", "wrec", "wclock", "wtick",
    "wclock0", "wtick0", "whcompu", "wunglueu", "wpty", "wplit", "wbytes", "wprim" };
static int WC[W_N], C_LNIL = -1, C_LCONS, C_TRUE, C_FALSE, C_NONE, C_SOME;
static int B_TRUE = -1, B_FALSE;
static void need_bool(void) {
    if (B_TRUE >= 0) return;
    B_FALSE = con_index("false");
    int t = con_index("true");
    if (t < 0 || B_FALSE < 0) { fprintf(stderr, "Bool is missing (lib/prelude.cub)\n"); exit(2); }
    __atomic_store_n(&B_TRUE, t, __ATOMIC_RELEASE);
}
static void need_cons(void) {
    if (C_LNIL >= 0) return;
    for (int w = 0; w < W_N; w++) if ((WC[w] = con_index(WNAME[w])) < 0) { fprintf(stderr, "%%view needs the data type View (lib/code.cub)\n"); exit(2); }
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
    case V_PTY: r = mk(WC[W_PTY], word_of(v->a), v->n > 1 ? v->port[1] : word_of(0), 0, 0); break;
    case V_PVAL: r = mk(WC[W_PLIT], word_of(v->x), word_of(v->a), 0, 0); break;
    case V_BYTES: r = mk(WC[W_BYTES], word_of(v->p ? (uint64_t)((Bytes *)v->p)->n : 0), 0, 0, 0); break;
    case V_WORD: r = mk(WC[W_WLIT], P(v, 0), 0, 0, 0); keep = true; break;
    case V_WPLUS: r = mk(WC[W_WPLUS], word_wn(cell_wn(v)), v->port[1], 0, 0); break;
    case V_NWOP: r = mk(WC[W_WOP], word_of((uint64_t)(v->x - O_WEQ)), v->port[1], v->port[2], 0); break;
    case V_NPRIM: r = mk(WC[W_PRIM], word_of((uint64_t)v->x), list_of(&v->port[1], v->n - 1), 0, 0); break;
    case V_NWELIM: r = mk(WC[W_WELIM], v->port[1], v->port[2], v->port[3], 0); break;
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
    Cell *e = cell(O_WELIM, 4);
    link(v->port[3], P(e, 0)); link(v->port[1], P(e, 1)); link(v->port[2], P(e, 2));
    cell_free(v);
    return P(e, 3);
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

/* a primitive operation on a neutral (B12): a neutral of its own, the arguments kept, which a
 * substitution runs again and the checker views (wprim) and compares */
static Port nprim(int cls, int sb, Port *args, int n) {
    Cell *c = cell(V_NPRIM, n + 1); c->x = (uint32_t)(cls << 8 | sb);
    for (int k = 0; k < n; k++) link(args[k], P(c, k + 1));
    return P(c, 0);
}
static Port pval_of(int kind, uint64_t bits) { Cell *c = cell(V_PVAL, 1); c->x = (uint32_t)kind; c->a = bits; return P(c, 0); }
static Port bytes_cell(Bytes *b) { Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b; return P(c, 0); }
static int po_kind(int op) { return op <= PO_I64LT ? PT_I64 : op <= PO_U32LT ? PT_U32 : PT_F64; }   /* a binary operation's operands */
/* an operation on a value it has no rule for (an ill-typed term, run unchecked): the result is
 * unknown, the operands erased — the machine does not stop, and a checker reading the value of a
 * term it rejected reads ? there */
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
    [V_THUNK] = "THUNK",
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
    [O_COMPH] = "COMPH",
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
    [O_CONCHK] = "CONCHK",
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
    [O_PTY] = "PTY", [O_BYTESTY] = "BYTESTY", [O_PVAL] = "PVAL", [O_PVAL2] = "PVAL2", [O_POP1] = "POP1", [O_POP2] = "POP2", [O_POP2B] = "POP2B", [O_BYTES] = "BYTES", [O_LOAD] = "LOAD", [O_LOADB] = "LOADB", [O_STORE] = "STORE", [O_STOREB] = "STOREB", [O_STOREC] = "STOREC", [O_BLEN] = "BLEN", [O_PTROF] = "PTROF", [O_REGAT] = "REGAT", [O_REGATA] = "REGATA", [O_FFI] = "FFI", [O_NATIVE] = "NATIVE",
    [O_PIMULT] = "PIMULT", [O_PIM] = "PIM", [O_FMULT] = "FMULT", [O_FMULT2] = "FMULT2", [O_FMULT3] = "FMULT3", [O_LINEAR] = "LINEAR", [O_GSTEPS] = "GSTEPS", [O_DNIDX] = "DNIDX", [O_GUSAGE] = "GUSAGE", [O_GUSAGE2] = "GUSAGE2",
    [O_WEQ] = "WEQ",
    [O_WLT] = "WLT",
    [O_WADD] = "WADD",
    [O_WMAX] = "WMAX",
    [O_WSUB] = "WSUB",
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
    { Local *l = local(); l->steps++; l->kind[o->kind]++; }
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
        else {
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
        if (vk == V_PLAM) {
            Subst *t = subst_single((Dim)v->a, o->p);
            link(sub(v->port[1], t), out_of(o));
            subst_release(t);
            if (o->x) { era(o->port[1]); era(o->port[2]); }
            cell_free(o); cell_free(v);
        } else if (vk == V_SYS) distribute(o, v);
        else {
            Dnf *r = o->p;
            if (!o->x && is_neutral(vk) && (dnf_is_zero(r) || dnf_is_one(r))) {
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
                Cell *c = cell(O_COMP, 4);
                link(uc[q], P(c, 0)); link(mkplam_of(v->port[k + 1], i), P(c, 1)); link(ac[q], P(c, 2)); link(P(c, 3), P(ns, q + 1));
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
                Cell *fo = cell(O_FORCE, 2); link(cp[q], P(fo, 0)); link(P(fo, 1), P(ns, nb + q + 1));
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
             * (18.16): the sides are transported only when a substitution selects one, or when a
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
            bool ih = nr && (tb->mask >> k & 1);
            int np = nf + (ih ? nr : 0) + nd;
            Port prm[np + 1], env[ne + 1];
            /* the hypotheses' leaves: one elimination per D at the leaf of a field's shape (18.18) */
            int nleaf = 0;
            if (ih) for (int j = 0; j < nf; j++) if (ci->recmask >> j & 1) nleaf += ci->rsh[j] ? ih_leaves(ci->rsh[j]) : 1;
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
                        if (ci->rsh[j] && !ih_pure_path(ci->rsh[j])) {
                            /* a function- or pair-shaped field: the hypothesis is the field passed through the
                             * shape's maker, an elimination as a function at each leaf (18.18) */
                            const char *sp = ci->rsh[j];
                            Port hm = ih_maker(&sp, tb, o->x, mb ? mc : NULL, &ec[0][0], nleaf + 2, ne, &leaf);
                            Cell *ap = cell(O_APP, 3); link(hm, P(ap, 0)); link(fb, P(ap, 1));
                            prm[nf + q] = P(ap, 2);
                            q++;
                            continue;
                        }
                        /* 1=field [2=motive] then the environment */
                        /* a path-typed field (rdim dimensions deep): the hypothesis is the line of
                         * the eliminations of its points, λ m ... → F (p @ m ...) (18.17) */
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
    case O_WEQ: case O_WLT: case O_WADD: case O_WMAX: case O_WSUB: {
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
        default: fprintf(stderr, "primitive operation: not unary\n"); exit(2);
        }
        if (o->x == PO_WOFI64 || o->x == PO_WOFU32) link(word_of(r), o->port[1]);
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
        case PO_I64ADD: r = (uint64_t)(ia + ib); k = PT_I64; break;
        case PO_I64SUB: r = (uint64_t)(ia - ib); k = PT_I64; break;
        case PO_I64MUL: r = (uint64_t)(ia * ib); k = PT_I64; break;
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
    case O_BLEN: {
        if (vk != V_BYTES) { Port a[1] = { P(v, 0) }; link(nprim(5, 0, a, 1), o->port[1]); cell_free(o); return; }
        link(word_of(v->p ? (uint64_t)((Bytes *)v->p)->n : 0), o->port[1]);
        era(P(v, 0)); cell_free(o);
        return;
    }
    case O_FFI: {
        /* a foreign call (D27): the arguments read in turn, each a primitive value, a word or a byte
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
    case O_NATIVE: {
        /* a compiled definition called from the net (B18): the arguments read in turn and kept as
         * values; all words, the compiled code is called with them; otherwise the definition itself
         * is applied to them (the net computes what the native form does not cover) */
        typedef struct { int n; Port v[8]; } NArgs;
        NArgs *A = o->p; if (!A) A = calloc(1, sizeof(NArgs));
        o->p = NULL;
        A->v[A->n++] = P(v, 0);
        if (o->n > 2) {
            Cell *c = cell(O_NATIVE, o->n - 1); c->x = o->x; c->a = o->a; c->p = A;
            for (int k = 1; k < o->n; k++) mv(o, k, P(c, k - 1));
            cell_free(o);
            return;
        }
        int n = A->n; bool words = n <= 8; Ffi *e = &FFI[o->x];
        for (int k = 0; k < n && words; k++) {
            Cell *c = cell_at(A->v[k]);
            if (!c) { words = false; break; }
            if (e->kind[k] == 6) { if (!boxable(c)) words = false; }
            else if (c->kind != V_WORD || c->p) words = false;
        }
        Port out;
        if (words) {
            /* words by value; a boxed argument (a byte string, or a constructor whose fields are values
             * already) as the emitted runtime's V, copied (the compiled code may update it in place);
             * a V * result unboxed into fresh cells (a constructor by the result's data type) */
            uint64_t a[8]; VB *boxed[8] = { 0 };
            for (int k = 0; k < n; k++) {
                Cell *c = cell_at(A->v[k]);
                if (e->kind[k] == 6) { boxed[k] = box_value(c); a[k] = (uint64_t)(uintptr_t)boxed[k]; }
                else a[k] = c->a;
                era(A->v[k]);
            }
            uint64_t r = ffi_invoke(e, a, n);
            if (e->ret == 6) out = unbox_value((VB *)(uintptr_t)r, e->rdata);
            else out = word_of(r);
            for (int k = 0; k < n; k++) free_boxed(boxed[k]);
        } else out = apply_gid((long)o->a, A->v, n);
        free(A);
        link(out, o->port[1]);
        cell_free(o);
        return;
    }
    case O_PTROF: {
        /* the address of the bytes (C20): memory the program does not own is reached by it. The bytes
         * whose address is taken are pinned (never freed), so the address stays valid; trusted: a
         * store makes a copy, which the address does not see */
        if (vk != V_BYTES) { stuck(o, v); return; }
        if (v->p) rcl_inc(&((Bytes *)v->p)->rc);
        link(word_of(v->p ? (uint64_t)(uintptr_t)((Bytes *)v->p)->b : 0), o->port[1]);
        era(P(v, 0)); cell_free(o);
        return;
    }
    case O_REGATA: {
        /* the n bytes at an address, copied into a byte string (C20, trusted: the program vouches
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
        WN k; Port b; Cell *fr = wsplit(v, &k, &b);
        if (fr) cell_free(fr);
        if (wn_zero(&k) && !b) { link(o->port[1], o->port[3]); era(o->port[2]); }
        else if (wn_zero(&k)) {
            /* a neutral word: the elimination is stuck */
            Cell *e = cell(V_NWELIM, 4);
            link(b, P(e, 1)); mv(o, 1, P(e, 2)); mv(o, 2, P(e, 3)); link(P(e, 0), o->port[3]);
        } else {
            /* s (k-1 ⊕ b) h, h the elimination of k-1 ⊕ b, computed when used */
            WN one = wn_small(1), km = wn_sub(&k, &one);
            Port s1, s2, p1, p2;
            split2(o->port[2], &s1, &s2);
            split2(wplus(km, b), &p1, &p2);
            Cell *h = cell(V_WSUSP, 4);
            link(o->port[1], P(h, 1)); link(s2, P(h, 2)); link(p2, P(h, 3));
            link(app2(app2(s1, p1), P(h, 0)), o->port[3]);
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
        if (!ba && !bb) {
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
    case O_MKVAR: case O_MKDIM: {
        /* a fresh variable (of the type at port 1) or dimension, each time; 0 is its source name (a
         * word) or only the trigger */
        uint32_t nm = vk == V_WORD && !v->p ? (uint32_t)v->a + 1 : 0;
        for (int k = 1; k < v->n; k++) era(v->port[k]);
        if (o->kind == O_MKVAR) {
            static uint64_t next_var;
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
        if (o->x == TB_REP) {
            /* (%rtab d): some R, the data type's first representation, or none */
            need_cons();
            if (g < 0) link(con_build(C_NONE, NULL, NULL), o->port[1]);
            else { Cell *r = cell(V_GLOBAL, 1); r->a = (uint64_t)g; Port f[1] = { P(r, 0) }; link(con_build(C_SOME, f, NULL), o->port[1]); }
            cell_free(o); cell_free(v);
            return;
        }
        if (g < 0) { fprintf(stderr, "checker table %u has no entry %llu\n", o->x, (unsigned long long)o->a); exit(2); }
        if (o->x == TB_VAL) {
            /* a checked definition's own value */
            Port val = global_copy(g, NULL);
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
        if (C_SAME < 0 || C_DIFF < 0) { fprintf(stderr, "%%clos needs CloCmp (lib/code.cub)\n"); exit(2); }
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
    if (k < 128) __builtin_ia32_pause();
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
    net_solo = n == 0;
    workers = calloc((size_t)n + 1, sizeof(pthread_t));
    for (int i = 0; i < n; i++) pthread_create(&workers[i], NULL, worker, (void *)(intptr_t)(i + 1));
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
static Port build(Syn *t, Frame *f);
static Port op1(int kind, Syn *a, Frame *f) { Cell *c = cell(kind, 2); link(build(a, f), P(c, 0)); return P(c, 1); }

static Port build(Syn *t, Frame *f) {
    Cell *c;
    switch (t->tag) {
    case S_VAR: {
        Supply *s = &f->s[t->slot];
        if (s->cur >= s->n) { fprintf(stderr, "internal: slot %d used beyond its count\n", t->slot); exit(2); }
        return s->sup[s->cur++];
    }
    case S_LAM: case S_THUNK:
        c = cell(t->tag == S_LAM ? V_LAM : V_THUNK, t->n + 1); c->p = t->rec;
        for (int k = 0; k < t->n; k++) link(build(t->k[k], f), P(c, k + 1));
        c->key = synkey(t, f);
        if (t->tag == S_THUNK && c->key) memo_register(c);
        return P(c, 0);
    case S_APP:
        c = cell(O_APP, 3); link(build(t->k[0], f), P(c, 0)); link(build(t->k[1], f), P(c, 1));
        c->key = synkey(t, f); if (c->key) memo_register(c);
        return P(c, 2);
    case S_PAIR: c = cell(V_PAIR, 3); goto fields;
    case S_PI: c = cell(V_PI, 3); c->x = t->x; goto fields;
    case S_SIGMA: c = cell(V_SIGMA, 3); goto fields;
    case S_PATHP: c = cell(V_PATHP, 4); goto fields;
    fields:
        for (int k = 0; k < c->n - 1; k++) link(build(t->k[k], f), P(c, k + 1));
        c->key = synkey(t, f);
        return P(c, 0);
    case S_FST: { Port r = op1(O_FST, t->k[0], f); PC(r)->key = synkey(t, f); return r; }
    case S_SND: { Port r = op1(O_SND, t->k[0], f); PC(r)->key = synkey(t, f); return r; }
    case S_U: {
        /* U at its level: lzero when none is written */
        c = cell(V_U, 2);
        if (t->n) link(build(t->k[0], f), P(c, 1));
        else { int z = con_index("lzero"); if (z < 0) { fprintf(stderr, "U needs Level (lib/prelude.cub)\n"); exit(2); } link(con_build(z, NULL, NULL), P(c, 1)); }
        return P(c, 0);
    }
    case S_BOX: return ref_of(build(t->k[0], f));
    case S_WORD: c = cell(V_WORD, 1); c->a = (uint64_t)t->id; c->p = big_retain(t->big); c->key = synkey(t, f); return P(c, 0);
    case S_WORDTY: c = cell(V_WTY, 1); c->key = synkey(t, f); return P(c, 0);
    case S_CALL: {
        c = cell(O_CALL, t->n + 2);
        for (int k = 0; k < t->n; k++) link(build(t->k[k], f), P(c, 1 + k));
        Cell *gv = cell(V_GLOBAL, 1); gv->a = (uint64_t)t->id; gv->key = key_global(t->id);
        link(P(gv, 0), P(c, 0));
        c->key = synkey(t, f); if (c->key) memo_register(c);
        return P(c, t->n + 1);
    }
    case S_GLOBTAB:
        c = cell(O_GLOB, 2); c->x = t->x;
        link(build(t->k[0], f), P(c, 0));
        return P(c, 1);
    case S_PRIM:
        c = cell((int)t->x, t->n + 1);
        if (t->x == O_POP1 || t->x == O_POP2 || t->x == O_LOAD || t->x == O_STORE || t->x == O_FFI || t->x == O_NATIVE) c->x = (uint32_t)t->id;   /* the operation, the width, or the foreign entry */
        if (t->x == O_NATIVE) c->a = (uint64_t)t->slot;   /* the definition the native form stands for */
        for (int k = 0; k < t->n; k++) link(build(t->k[k], f), P(c, k));
        return P(c, t->n);
    case S_PLAM: {
        Dim i = fresh_dim();
        bind_dim(f, t->slot, i);
        Cell *m = cell(O_MKPLAM, 2); m->a = i;
        link(build(t->k[0], f), P(m, 0));
        return P(m, 1);
    }
    case S_FORALL: {
        Dim i = fresh_dim();
        bind_dim(f, t->slot, i);
        Cell *m = cell(O_FORALL, 2); m->a = i;
        link(build(t->k[0], f), P(m, 0));
        return P(m, 1);
    }
    case S_PAPP:
        c = cell(O_PAPPR, t->x ? 5 : 3); c->x = t->x;
        link(build(t->k[1], f), P(c, 0)); link(build(t->k[0], f), P(c, 1));
        if (t->x) { link(build(t->k[2], f), P(c, 2)); link(build(t->k[3], f), P(c, 3)); }
        return P(c, c->n - 1);
    case S_IV0: { Port r = iv_cell(dnf_zero()); PC(r)->key = synkey(t, f); return r; }
    case S_IV1: { Port r = iv_cell(dnf_one()); PC(r)->key = synkey(t, f); return r; }
    case S_NEG: return op1(O_NEG, t->k[0], f);
    case S_BIN: c = cell(O_BIN, 3); c->x = t->x; link(build(t->k[0], f), P(c, 0)); link(build(t->k[1], f), P(c, 1)); return P(c, 2);
    case S_FEQ: c = cell(O_FEQ, 2); c->a = t->x; link(build(t->k[0], f), P(c, 0)); return P(c, 1);
    case S_FTT: c = cell(V_FACE, 1); c->p = dnf_one(); c->key = synkey(t, f); return P(c, 0);
    case S_FFF: c = cell(V_FACE, 1); c->p = dnf_zero(); c->key = synkey(t, f); return P(c, 0);
    case S_FACEOF: return op1(O_FACEOF, t->k[0], f);
    case S_FNEG: return op1(O_FNEG, t->k[0], f);
    case S_SYS0: c = cell(V_SYS, 1); c->p = NULL; return P(c, 0);
    case S_SYSCONS:
        c = cell(O_SYSCONS, 4);
        link(build(t->k[0], f), P(c, 0)); link(build(t->k[1], f), P(c, 1)); link(build(t->k[2], f), P(c, 2));
        return P(c, 3);
    case S_SYSMAP: c = cell(O_SYSMAP, 3); link(build(t->k[1], f), P(c, 0)); link(build(t->k[0], f), P(c, 1)); return P(c, 2);
    case S_COMP:
        c = cell(O_COMP, 4);
        link(build(t->k[1], f), P(c, 0)); link(build(t->k[0], f), P(c, 1)); link(build(t->k[2], f), P(c, 2));
        return P(c, 3);
    case S_GLUE: c = cell(O_MKGLUE, 3); c->x = t->x; link(build(t->k[1], f), P(c, 0)); link(build(t->k[0], f), P(c, 1)); return P(c, 2);
    case S_GLUEV: c = cell(O_MKGLUEV, 3); link(build(t->k[0], f), P(c, 0)); link(build(t->k[1], f), P(c, 1)); return P(c, 2);
    case S_UNGLUE: c = cell(O_UNGLUE, 3); c->x = t->x; link(build(t->k[0], f), P(c, 0)); link(build(t->k[1], f), P(c, 1)); return P(c, 2);
    case S_GLOBAL: c = cell(V_GLOBAL, 1); c->a = (uint64_t)t->id; c->key = key_global(t->id); return P(c, 0);
    case S_AXIOM:
        c = cell(V_NVAR, 2); c->a = (uint64_t)t->id; c->key = synkey(t, f);
        if (t->slot >= 0) { Cell *g = cell(V_GLOBAL, 1); g->a = (uint64_t)t->slot; link(P(g, 0), P(c, 1)); c->ty = 1; }
        else link(unk(), P(c, 1));
        return P(c, 0);
    case S_FORCE: { Port r = op1(O_FORCE, t->k[0], f); PC(r)->key = synkey(t, f); return r; }
    case S_DATA: c = cell(V_DATA, t->n + 1); c->a = (uint64_t)t->id; goto fields;
    case S_CON: {
        Port *a = malloc((size_t)(t->n + 1) * sizeof(Port));
        for (int k = 0; k < t->n; k++) a[k] = build(t->k[k], f);
        Port r = con_build((int)t->id, a, a + CON[t->id].nfields);
        free(a);
        if (PI(r) == 0) PC(r)->key = synkey(t, f);   /* a plain constructor (one with a boundary is checked first) */
        return r;
    }
    case S_PROJ: c = cell(O_PROJ, 2); c->a = (uint64_t)t->id; link(build(t->k[0], f), P(c, 0)); return P(c, 1);
    case S_REC:
        c = cell(V_REC, t->n + 1); c->a = (uint64_t)t->id;
        for (int k = 0; k < t->n; k++) link(build(t->k[k], f), P(c, k + 1));
        return P(c, 0);
    case S_HCOMPD:
        c = cell(O_HCOMP, t->n + 1); link(build(t->k[0], f), P(c, 0)); link(build(t->k[1], f), P(c, 1));
        if (t->n == 3) link(build(t->k[2], f), P(c, 2));
        return P(c, t->n);
    case S_TRANSD:
        c = cell(O_TRANSD1, t->n + 1); c->a = (uint64_t)t->id;
        for (int k = 0; k < t->n; k++) link(build(t->k[k], f), P(c, k));
        return P(c, t->n);
    case S_CASE: {
        /* k[0]=scrutinee k[1]=motive (or none) k[2..]=the shared variables */
        int ne = t->tab->nenv;
        c = cell(O_CASE, ne + 2); c->x = 0; c->p = t->tab;
        link(build(t->k[0], f), P(c, 0));
        for (int e = 0; e < ne; e++) link(build(t->k[2 + e], f), P(c, 1 + e));
        c->key = synkey(t, f); if (c->key) memo_register(c);
        return P(c, c->n - 1);
    }
    case S_LET: {
        bind(f, t->slot, build(t->k[0], f));
        return build(t->k[1], f);
    }
    }
    fprintf(stderr, "build: tag %d\n", t->tag); exit(2);
}

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
