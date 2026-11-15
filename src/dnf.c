/* dnf.c — the interval (free De Morgan algebra) and the face lattice.
 *
 * Both are joins of clauses (meets of literals), kept as an antichain.
 * An interval literal is a dimension or its negation; i ∧ ~i is NOT zero.
 * A face literal is (i = 0) or (i = 1); (i=0) ∧ (i=1) is false.
 * The free bounded distributive lattice on the literals has the antichain
 * of clauses as canonical form, so 0, 1 and equality are read off directly.
 *
 * Payloads are immutable and reference counted. Every function borrows its
 * arguments and returns a reference its caller owns.
 */
#include "core.h"
#include <string.h>
#include <pthread.h>

static Dim next_dim = 1;
Dim fresh_dim(void) { return __atomic_fetch_add(&next_dim, 1, __ATOMIC_RELAXED); }

long stat_payloads;
#define LIVE(d) cnt_add(&stat_payloads, (d))
/* Immortal payloads: the constants 0 and 1 and every single literal (a dimension or its negation,
 * an endpoint face) live once, made on first use and never freed; retaining or releasing one does
 * nothing. These are most of the formulas a run makes. */
#define IMMORTAL 0x40000000u
long stat_dnf_allocs, stat_cl_allocs, stat_cl_lits, stat_dnf_clauses;
static Clause *cl_new(uint32_t n) { Clause *c = malloc(sizeof(Clause) + n * sizeof(uint64_t)); c->rc = 1; c->n = n; LIVE(1); cnt_add(&stat_cl_allocs, 1); cnt_add(&stat_cl_lits, n); return c; }
static Dnf *dnf_new(uint32_t n) { Dnf *d = malloc(sizeof(Dnf) + n * sizeof(Clause *)); d->rc = 1; d->n = n; LIVE(1); cnt_add(&stat_dnf_allocs, 1); cnt_add(&stat_dnf_clauses, n); return d; }
static inline bool imm(const uint32_t *rc) { return (net_solo ? *rc : __atomic_load_n(rc, __ATOMIC_RELAXED)) & IMMORTAL; }   /* the bit never changes once set */
Clause *clause_retain(Clause *c) { if (!imm(&c->rc)) rc_inc(&c->rc); return c; }
void clause_release(Clause *c) { if (c && !imm(&c->rc) && rc_dec(&c->rc) == 0) { free(c); LIVE(-1); } }
Dnf *dnf_retain(Dnf *d) { if (d && !imm(&d->rc)) rc_inc(&d->rc); return d; }
void dnf_release(Dnf *d) {
    if (d && !imm(&d->rc) && rc_dec(&d->rc) == 0) {
        for (uint32_t i = 0; i < d->n; i++) clause_release(d->c[i]);
        free(d); LIVE(-1);
    }
}
/* a list of clauses taken as given (a system's branches): retains each */
Dnf *clause_list(Clause *const *c, uint32_t n) {
    Dnf *d = dnf_new(n);
    for (uint32_t i = 0; i < n; i++) d->c[i] = clause_retain(c[i]);
    return d;
}

static int cl_cmp(const Clause *a, const Clause *b) {
    if (a->n != b->n) return a->n < b->n ? -1 : 1;
    for (uint32_t i = 0; i < a->n; i++) if (a->l[i] != b->l[i]) return a->l[i] < b->l[i] ? -1 : 1;
    return 0;
}
static int cl_qcmp(const void *x, const void *y) { return cl_cmp(*(Clause *const *)x, *(Clause *const *)y); }
static bool cl_subset(const Clause *a, const Clause *b) {
    uint32_t i = 0, j = 0;
    while (i < a->n && j < b->n) {
        if (a->l[i] == b->l[j]) { i++; j++; }
        else if (a->l[i] > b->l[j]) j++;
        else return false;
    }
    return i == a->n;
}
/* union of two clauses; for faces NULL when a dimension gets both endpoints */
static Clause *cl_union(const Clause *a, const Clause *b, bool face) {
    Clause *c = cl_new(a->n + b->n);
    uint32_t i = 0, j = 0, k = 0;
    while (i < a->n || j < b->n) {
        uint64_t x;
        if (j >= b->n || (i < a->n && a->l[i] < b->l[j])) x = a->l[i++];
        else if (i >= a->n || b->l[j] < a->l[i]) x = b->l[j++];
        else { x = a->l[i]; i++; j++; }
        if (face && k > 0 && (c->l[k - 1] >> 1) == (x >> 1)) { clause_release(c); return NULL; }
        c->l[k++] = x;
    }
    c->n = k;
    return c;
}
/* normalize, consuming the references in a: drop NULLs, sort, dedup, absorb */
static Dnf *normalize(Clause **a, uint32_t n) {
    uint32_t m = 0;
    for (uint32_t i = 0; i < n; i++) if (a[i]) a[m++] = a[i];
    qsort(a, m, sizeof(Clause *), cl_qcmp);
    Dnf *d = dnf_new(m);
    uint32_t k = 0;
    for (uint32_t i = 0; i < m; i++) {
        bool absorbed = false;
        for (uint32_t j = 0; j < k && !absorbed; j++) if (cl_subset(d->c[j], a[i])) absorbed = true;
        if (absorbed) clause_release(a[i]); else d->c[k++] = a[i];
    }
    d->n = k;
    return d;
}

static Dnf *ZERO, *ONE;
typedef struct { Dnf **t; size_t cap; } LitTab;   /* by literal, made on first use; the table is replaced whole when it grows */
static LitTab *LT;
static pthread_mutex_t litmu = PTHREAD_MUTEX_INITIALIZER;
static Dnf *immortal(Dnf *d) { d->rc |= IMMORTAL; for (uint32_t i = 0; i < d->n; i++) d->c[i]->rc |= IMMORTAL; return d; }
Dnf *dnf_zero(void) { if (!ZERO) ZERO = immortal(dnf_new(0)); return ZERO; }
Dnf *dnf_one(void) { if (!ONE) { Dnf *d = dnf_new(1); d->c[0] = cl_new(0); ONE = immortal(d); } return ONE; }
static Dnf *dnf_lit(uint64_t lit) {
    LitTab *T = __atomic_load_n(&LT, __ATOMIC_ACQUIRE);
    if (T && lit < T->cap) { Dnf *d = __atomic_load_n(&T->t[lit], __ATOMIC_ACQUIRE); if (d) return d; }
    if (!net_solo) pthread_mutex_lock(&litmu);
    T = LT;
    if (!T || lit >= T->cap) {
        size_t nc = T ? T->cap : 1024; while (nc <= lit) nc *= 2;
        LitTab *N = malloc(sizeof(LitTab)); N->t = calloc(nc, sizeof(Dnf *)); N->cap = nc;
        if (T) memcpy(N->t, T->t, T->cap * sizeof(Dnf *));
        __atomic_store_n(&LT, N, __ATOMIC_RELEASE); T = N;   /* the old table is kept: a reader may hold it */
    }
    Dnf *d = T->t[lit];
    if (!d) { d = dnf_new(1); d->c[0] = cl_new(1); d->c[0]->l[0] = lit; immortal(d); __atomic_store_n(&T->t[lit], d, __ATOMIC_RELEASE); }
    if (!net_solo) pthread_mutex_unlock(&litmu);
    return d;
}
Dnf *iv_atom(Dim i) { return dnf_lit(i << 1); }
bool dnf_is_zero(const Dnf *x) { return x->n == 0; }
bool dnf_is_one(const Dnf *x) { return x->n == 1 && x->c[0]->n == 0; }
bool clause_mentions(const Clause *c, Dim i) {
    for (uint32_t k = 0; k < c->n; k++) if ((c->l[k] >> 1) == i) return true;
    return false;
}
bool dnf_mentions(const Dnf *x, Dim i) {
    for (uint32_t j = 0; j < x->n; j++) if (clause_mentions(x->c[j], i)) return true;
    return false;
}
Dnf *dnf_join(const Dnf *x, const Dnf *y, bool face) {
    (void)face;
    /* units and absorbing elements: no new formula */
    if (x->n == 0 || dnf_is_one(y)) return dnf_retain((Dnf *)y);
    if (y->n == 0 || dnf_is_one(x)) return dnf_retain((Dnf *)x);
    Clause *buf[16], **a = x->n + y->n + 1 <= 16 ? buf : malloc((x->n + y->n + 1) * sizeof(Clause *));
    uint32_t n = 0;
    for (uint32_t i = 0; i < x->n; i++) a[n++] = clause_retain(x->c[i]);
    for (uint32_t i = 0; i < y->n; i++) a[n++] = clause_retain(y->c[i]);
    Dnf *d = normalize(a, n);
    if (a != buf) free(a);
    return d;
}
Dnf *dnf_meet(const Dnf *x, const Dnf *y, bool face) {
    if (dnf_is_one(x) || y->n == 0) return dnf_retain((Dnf *)y);
    if (dnf_is_one(y) || x->n == 0) return dnf_retain((Dnf *)x);
    size_t need = (size_t)x->n * y->n + 1;
    Clause *buf[16], **a = need <= 16 ? buf : malloc(need * sizeof(Clause *));
    uint32_t n = 0;
    for (uint32_t i = 0; i < x->n; i++)
        for (uint32_t j = 0; j < y->n; j++) a[n++] = cl_union(x->c[i], y->c[j], face);
    Dnf *d = normalize(a, n);
    if (a != buf) free(a);
    return d;
}
/* acc := acc ∧ d / acc ∨ d, releasing both */
static Dnf *meet_into(Dnf *acc, Dnf *d, bool face) { Dnf *r = dnf_meet(acc, d, face); dnf_release(acc); dnf_release(d); return r; }
static Dnf *join_into(Dnf *acc, Dnf *d, bool face) { Dnf *r = dnf_join(acc, d, face); dnf_release(acc); dnf_release(d); return r; }

/* ~(∨_c ∧_l l) = ∧_c ∨_l ~l */
Dnf *iv_neg(const Dnf *x) {
    if (x->n == 1 && x->c[0]->n == 1) return dnf_lit(x->c[0]->l[0] ^ 1);
    Dnf *acc = dnf_one();
    for (uint32_t i = 0; i < x->n; i++) {
        Dnf *d = dnf_zero();
        for (uint32_t k = 0; k < x->c[i]->n; k++) d = join_into(d, dnf_lit(x->c[i]->l[k] ^ 1), false);
        acc = meet_into(acc, d, false);
    }
    return acc;
}
/* the complement of a face: ¬(∨_c ∧_l l) = ∧_c ∨_l ¬l, met as faces (a dimension at both ends
 * is no face), so a face and its complement meet in ⊥ and join to ⊤ on every closed point */
Dnf *face_neg(const Dnf *x) {
    Dnf *acc = dnf_one();
    for (uint32_t i = 0; i < x->n; i++) {
        Dnf *d = dnf_zero();
        for (uint32_t k = 0; k < x->c[i]->n; k++) d = join_into(d, dnf_lit(x->c[i]->l[k] ^ 1), true);
        acc = meet_into(acc, d, true);
    }
    return acc;
}
const Dnf *subst_get(const Subst *s, Dim j) {
    for (uint32_t i = 0; i < s->n; i++) if (s->d[i] == j) return s->v[i];
    return NULL;
}
/* does s move any dimension x mentions? (most substitutions reaching a formula do not) */
static bool dnf_moved(const Dnf *x, const Subst *s) {
    for (uint32_t i = 0; i < x->n; i++)
        for (uint32_t k = 0; k < x->c[i]->n; k++) if (subst_get(s, x->c[i]->l[k] >> 1)) return true;
    return false;
}
Dnf *iv_subst(const Dnf *x, const Subst *s) {
    if (!dnf_moved(x, s)) return dnf_retain((Dnf *)x);
    if (x->n == 1 && x->c[0]->n == 1) {
        /* a single literal: its image (negated) */
        uint64_t lit = x->c[0]->l[0];
        Dnf *v = (Dnf *)subst_get(s, lit >> 1);
        return (lit & 1) ? iv_neg(v) : dnf_retain(v);
    }
    Dnf *acc = dnf_zero();
    for (uint32_t i = 0; i < x->n; i++) {
        Dnf *m = dnf_one();
        for (uint32_t k = 0; k < x->c[i]->n; k++) {
            uint64_t lit = x->c[i]->l[k];
            const Dnf *v = subst_get(s, lit >> 1);
            Dnf *w = v ? dnf_retain((Dnf *)v) : iv_atom(lit >> 1);
            if (lit & 1) { Dnf *nw = iv_neg(w); dnf_release(w); w = nw; }
            m = meet_into(m, w, false);
        }
        acc = join_into(acc, m, false);
    }
    return acc;
}
/* (r = 0) is the meet over clauses of the join of (l = 0);
 * (r = 1) is the join over clauses of the meet of (l = 1).
 * (i = e) is the face literal i<<1|e; (~i = e) is (i = 1-e). */
Dnf *face_eq(const Dnf *r, int eps) {
    if (eps == 0) {
        Dnf *acc = dnf_one();
        for (uint32_t i = 0; i < r->n; i++) {
            Dnf *d = dnf_zero();
            for (uint32_t k = 0; k < r->c[i]->n; k++) d = join_into(d, dnf_lit(r->c[i]->l[k]), true);
            acc = meet_into(acc, d, true);
        }
        return acc;
    }
    Dnf *acc = dnf_zero();
    for (uint32_t i = 0; i < r->n; i++) {
        Dnf *m = dnf_one();
        for (uint32_t k = 0; k < r->c[i]->n; k++) m = meet_into(m, dnf_lit(r->c[i]->l[k] ^ 1), true);
        acc = join_into(acc, m, true);
    }
    return acc;
}
Dnf *face_subst(const Dnf *f, const Subst *s) {
    if (!dnf_moved(f, s)) return dnf_retain((Dnf *)f);
    Dnf *acc = dnf_zero();
    for (uint32_t i = 0; i < f->n; i++) {
        Dnf *m = dnf_one();
        for (uint32_t k = 0; k < f->c[i]->n; k++) {
            uint64_t lit = f->c[i]->l[k];
            const Dnf *v = subst_get(s, lit >> 1);
            m = meet_into(m, v ? face_eq(v, (int)(lit & 1)) : dnf_lit(lit), true);
        }
        acc = join_into(acc, m, true);
    }
    return acc;
}
Dnf *face_of_clause(const Clause *c) { Dnf *d = dnf_new(1); d->c[0] = clause_retain((Clause *)c); return d; }
Dnf *face_of_clauses(const Dnf *list) {
    uint32_t n = list ? list->n : 0;
    Clause **a = malloc((n + 1) * sizeof(Clause *));
    for (uint32_t i = 0; i < n; i++) a[i] = clause_retain(list->c[i]);
    Dnf *d = normalize(a, n);
    free(a);
    return d;
}
Dnf *face_forall(const Dnf *f, Dim i) {
    Clause **a = malloc((f->n + 1) * sizeof(Clause *));
    uint32_t n = 0;
    for (uint32_t k = 0; k < f->n; k++) if (!clause_mentions(f->c[k], i)) a[n++] = clause_retain(f->c[k]);
    Dnf *d = normalize(a, n);
    free(a);
    return d;
}

/* one block: the header, then the dimensions, then their values */
static Subst *subst_new(uint32_t n) {
    Subst *s = malloc(sizeof(Subst) + (size_t)(n + 1) * (sizeof(Dim) + sizeof(Dnf *)));
    s->rc = 1; s->n = n; s->d = (Dim *)(s + 1); s->v = (Dnf **)(s->d + n + 1);
    LIVE(1);
    return s;
}
Subst *subst_retain(Subst *s) { if (s) rc_inc(&s->rc); return s; }
void subst_release(Subst *s) {
    if (s && rc_dec(&s->rc) == 0) {
        for (uint32_t i = 0; i < s->n; i++) dnf_release(s->v[i]);
        free(s); LIVE(-1);
    }
}
Subst *subst_single(Dim i, const Dnf *r) { Subst *s = subst_new(1); s->d[0] = i; s->v[0] = dnf_retain((Dnf *)r); return s; }
Subst *subst_of_clause(const Clause *c) {
    Subst *s = subst_new(c->n);
    for (uint32_t k = 0; k < c->n; k++) { s->d[k] = c->l[k] >> 1; s->v[k] = (c->l[k] & 1) ? dnf_one() : dnf_zero(); }
    return s;
}
Subst *subst_then_clause(const Subst *s, const Clause *d) {
    Subst *ds = subst_of_clause(d);
    Subst *t = subst_new(s->n + d->n);
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->n; i++) { t->d[n] = s->d[i]; t->v[n] = iv_subst(s->v[i], ds); n++; }
    for (uint32_t k = 0; k < ds->n; k++)
        if (!subst_get(s, ds->d[k])) { t->d[n] = ds->d[k]; t->v[n] = dnf_retain(ds->v[k]); n++; }
    t->n = n;
    subst_release(ds);
    return t;
}
bool subst_range_mentions(const Subst *s, Dim j) {
    for (uint32_t i = 0; i < s->n; i++) if (dnf_mentions(s->v[i], j)) return true;
    return false;
}
Subst *subst_without(const Subst *s, Dim j) {
    Subst *t = subst_new(s->n);
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->n; i++) if (s->d[i] != j) { t->d[n] = s->d[i]; t->v[n] = dnf_retain(s->v[i]); n++; }
    t->n = n;
    return t;
}
Subst *subst_with(const Subst *s, Dim j, const Dnf *r) {
    Subst *t = subst_new(s->n + 1);
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->n; i++) if (s->d[i] != j) { t->d[n] = s->d[i]; t->v[n] = dnf_retain(s->v[i]); n++; }
    t->d[n] = j; t->v[n] = dnf_retain((Dnf *)r); n++;
    t->n = n;
    return t;
}
/* s, then t: x ↦ s(x)[t], and t's entries for the dimensions s leaves alone */
Subst *subst_compose(const Subst *s, const Subst *t) {
    /* t adds nothing to s and moves nothing in its range: s itself (the usual case: a substitution
     * reaching a reference that already carries one for the same dimensions) */
    bool same = true;
    for (uint32_t k = 0; k < t->n && same; k++) if (!subst_get(s, t->d[k])) same = false;
    for (uint32_t i = 0; i < s->n && same; i++) if (dnf_moved(s->v[i], t)) same = false;
    if (same) return subst_retain((Subst *)s);
    Subst *u = subst_new(s->n + t->n);
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->n; i++) { u->d[n] = s->d[i]; u->v[n] = iv_subst(s->v[i], t); n++; }
    for (uint32_t k = 0; k < t->n; k++)
        if (!subst_get(s, t->d[k])) { u->d[n] = t->d[k]; u->v[n] = dnf_retain(t->v[k]); n++; }
    u->n = n;
    return u;
}
Subst *subst_id(void) { return subst_new(0); }

void clause_print(FILE *o, const Clause *c, DimName nm) {
    if (c->n == 0) { fputs("tt", o); return; }
    if (c->n > 1) fputs("(&", o);
    for (uint32_t k = 0; k < c->n; k++) fprintf(o, "%s(= %s %u)", c->n > 1 ? " " : "", nm(c->l[k] >> 1), (unsigned)(c->l[k] & 1));
    if (c->n > 1) fputs(")", o);
}
void dnf_print(FILE *o, const Dnf *x, bool face, DimName nm) {
    if (face) {
        if (x->n == 0) { fputs("ff", o); return; }
        if (x->n > 1) fputs("(|", o);
        for (uint32_t i = 0; i < x->n; i++) { if (x->n > 1) fputc(' ', o); clause_print(o, x->c[i], nm); }
        if (x->n > 1) fputs(")", o);
        return;
    }
    if (x->n == 0) { fputs("0", o); return; }
    if (dnf_is_one(x)) { fputs("1", o); return; }
    if (x->n > 1) fputs("(\\/", o);
    for (uint32_t i = 0; i < x->n; i++) {
        const Clause *c = x->c[i];
        if (x->n > 1) fputc(' ', o);
        if (c->n > 1) fputs("(/\\", o);
        for (uint32_t k = 0; k < c->n; k++) {
            if (c->n > 1) fputc(' ', o);
            if (c->l[k] & 1) fprintf(o, "(~ %s)", nm(c->l[k] >> 1)); else fputs(nm(c->l[k] >> 1), o);
        }
        if (c->n > 1) fputs(")", o);
    }
    if (x->n > 1) fputs(")", o);
}

/* equality of canonical forms: the same antichain of clauses, each the same set of literals */
static bool clause_equal(const Clause *a, const Clause *b) {
    if (a->n != b->n) return false;
    for (uint32_t i = 0; i < a->n; i++) {
        bool found = false;
        for (uint32_t j = 0; j < b->n && !found; j++) found = a->l[i] == b->l[j];
        if (!found) return false;
    }
    return true;
}
bool dnf_equal(const Dnf *x, const Dnf *y) {
    uint32_t nx = x ? x->n : 0, ny = y ? y->n : 0;
    if (nx != ny) return false;
    for (uint32_t i = 0; i < nx; i++) {
        bool found = false;
        for (uint32_t j = 0; j < ny && !found; j++) found = clause_equal(x->c[i], y->c[j]);
        if (!found) return false;
    }
    return true;
}
