#include <malloc.h>
#include <setjmp.h>
#include <signal.h>
#include <dlfcn.h>
/* lang.c — reading terms, compiling them to binder records, definitions,
 * reading values back, and the driver. */
#include "core.h"
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/stat.h>
extern int access(const char *, int);
#include <sys/resource.h>
#include <limits.h>
extern int execv(const char *path, char *const argv[]);   /* <unistd.h>, whose link(2) the net's link shadows */
#include <stdarg.h>
extern int dup(int); extern int dup2(int, int); extern int close(int); extern FILE *fdopen(int, const char *);

/* grow an array to hold at least n elements */
#define GROW(arr, cap, n) do { if ((size_t)(n) > (size_t)(cap)) { size_t nc_ = (cap) ? (size_t)(cap) * 2 : 16; while (nc_ < (size_t)(n)) nc_ *= 2; (arr) = realloc((arr), nc_ * sizeof *(arr)); (cap) = nc_; } } while (0)

/* a failure where the caller has a way out (printing a result in the surface syntax): back to it */
static jmp_buf *die_jb;
static char die_msg[512];
static void die(const char *m, const char *x) {
    if (die_jb) { snprintf(die_msg, sizeof die_msg, "%s%s%s", m, x ? " " : "", x ? x : ""); jmp_buf *j = die_jb; die_jb = NULL; longjmp(*j, 1); }
    /* on stderr, and on stdout too when that is a different file (a run read through its stdout
     * alone saw a silent exit: E28 finding iii) */
    fflush(stdout);
    struct stat so, se; bool same = fstat(1, &so) == 0 && fstat(2, &se) == 0 && so.st_dev == se.st_dev && so.st_ino == se.st_ino;
    if (!same) { printf("error: %s%s%s\n", m, x ? " " : "", x ? x : ""); fflush(stdout); }
    fprintf(stderr, "error: %s%s%s\n", m, x ? " " : "", x ? x : ""); exit(1);
}

/* ---------------- s-expressions ---------------- */
typedef struct Node { int list; char *s; int n; struct Node **it; char *cmt; short hk; unsigned lvgen, fvgen; struct Names *fv, *lv; } Node;   /* lv: the names it may refer to (lvs), valid while lvgen is the arena's generation */   /* hk: a symbol's binder/head class, once found (hk_of); fv: the free names of a desugared term, once found (free_vars). s: interned, one pointer per spelling */
/* nodes are never freed: they come from a bump arena of zeroed chunks */
/* The node arena: bump allocation in 1 MB chunks. Compiling a term makes nodes nothing keeps
 * (desugaring, generated binders; the compiled form is Syn and Records, which hold no node): a
 * mark taken before and released after frees them. Parsed source and elaborated forms, which
 * tables and the module cache keep, are made outside any mark */
typedef struct Chunk { struct Chunk *prev; size_t cap, used; char data[]; } Chunk;   /* used: how far this chunk was ever filled */
typedef struct { Chunk *c; char *p; } ArMark;
static Chunk *ar_cur, *ar_spare; static char *ar_p, *ar_end;
static size_t ar_nspare;   /* chunks on the spare list: past ar_spare_max, a released chunk goes back to malloc */
#define AR_SPARE_MAX 64
static void *ar_alloc(size_t n) {
    n = (n + 15) & ~(size_t)15;
    if ((size_t)(ar_end - ar_p) < n) {
        if (ar_cur) ar_cur->used = (size_t)(ar_p - ar_cur->data);
        Chunk *k = NULL;
        if (n <= (1u << 20) && ar_spare) { k = ar_spare; ar_spare = k->prev; ar_nspare--; }   /* a released chunk: zeroed where it was used */
        else {
            size_t c = n > (1u << 20) ? n : (1u << 20);
            k = calloc(1, sizeof(Chunk) + c); if (!k) { fputs("out of memory\n", stderr); exit(2); }
            k->cap = c;
        }
        k->prev = ar_cur; k->used = 0; ar_cur = k;
        ar_p = k->data; ar_end = ar_p + k->cap;
    }
    void *r = ar_p; ar_p += n; return r;
}
static ArMark ar_mark(void) { return (ArMark){ ar_cur, ar_p }; }
/* nodes are made from zeroed memory, so what was used since the mark is zeroed again: the mark's
 * chunk from the mark to where its use reached, and each later chunk over what it used; those
 * chunks are kept for the next allocations (a chunk a megabyte wide is not zeroed whole again) */
static unsigned ar_gen = 1;   /* bumped at every release: a cache made inside a mark is stale after it */
static void ar_release(ArMark m) {
    ar_gen++;
    if (ar_cur) ar_cur->used = (size_t)(ar_p - ar_cur->data);
    static int poison = -1;
    if (poison < 0) { poison = getenv("CUBICAL_ARPOISON") != NULL; if (poison) mallopt(M_MMAP_THRESHOLD, 256 * 1024); }
    while (ar_cur != m.c) {
        Chunk *k = ar_cur; ar_cur = k->prev;
        if (poison) { free(k); continue; }   /* mapped on its own (the threshold fixed): unmapped, a read of it faults */
        if (k->cap > (1u << 20)) free(k);   /* an oversized one-off */
        else if (ar_nspare >= AR_SPARE_MAX) free(k);   /* a release far past the next ones' need: returned (a big mark held hundreds of megabytes spare for good) */
        else { memset(k->data, 0, k->used); k->used = 0; k->prev = ar_spare; ar_spare = k; ar_nspare++; }
    }
    if (poison && ar_cur) {
        /* the mark's own chunk: garbage from the mark on, and nothing after the mark reused */
        if (ar_cur->data + ar_cur->used > m.p) memset(m.p, 0xA5, (size_t)(ar_cur->data + ar_cur->used - m.p));
        ar_cur->used = ar_cur->cap; ar_p = ar_end = ar_cur->data + ar_cur->cap;
        return;
    }
    ar_p = m.p; ar_end = ar_cur ? ar_cur->data + ar_cur->cap : NULL;
    if (ar_cur && ar_cur->data + ar_cur->used > ar_p) memset(ar_p, 0, (size_t)(ar_cur->data + ar_cur->used - ar_p));
}
/* memory accounting (CUBICAL_MEMDEF): the node arena's chunks in use and spare, in bytes */
static void ar_mem(long *use, long *spare) {
    long u = 0, sp = 0;
    for (Chunk *k = ar_cur; k; k = k->prev) u += (long)k->cap;
    for (Chunk *k = ar_spare; k; k = k->prev) sp += (long)k->cap;
    *use = u; *spare = sp;
}
static long stat_syn_bytes, stat_perm_bytes, stat_rec_bytes;
static void mem_report(const char *name) {
    static int on = -1; if (on < 0) on = getenv("CUBICAL_MEMDEF") != NULL;
    if (!on) return;
    long rss = 0, hwm = 0; char line[256];
    FILE *st = fopen("/proc/self/status", "r");
    if (st) { while (fgets(line, sizeof line, st)) { if (!strncmp(line, "VmRSS:", 6)) rss = atol(line + 6); else if (!strncmp(line, "VmHWM:", 6)) hwm = atol(line + 6); } fclose(st); }
    long sl, ks, au, as; net_mem(&sl, &ks); ar_mem(&au, &as);
    stat_gather();
    struct mallinfo2 mi = mallinfo2();
    fprintf(stderr, "mem %s rss=%ld hwm=%ld cells=%ld slabsMB=%ld keys=%ld payloads=%ld arena=%ld spare=%ld heap=%zu free=%zu mmap=%zu syn=%ld perm=%ld rec=%ld\n",
            name, rss / 1024, hwm / 1024, stat_cells, sl * 64 / 1024, ks, stat_payloads, au >> 20, as >> 20,
            mi.uordblks >> 20, mi.fordblks >> 20, mi.hblkhd >> 20, stat_syn_bytes >> 20, stat_perm_bytes >> 20, stat_rec_bytes >> 20);
}
/* Symbols are interned: one copy of each spelling, kept for the run, so names compare by
 * pointer (the binder lookups, the name sets of free_vars and local_free) */
static size_t kw_hash(const char *s);
static char **SYMS; static size_t symcap, symn;
static char *intern(const char *s) {
    if (symn * 2 >= symcap) {
        size_t nc = symcap ? symcap * 2 : 4096; char **nt = calloc(nc, sizeof(char *));
        for (size_t i = 0; i < symcap; i++) if (SYMS[i]) { size_t q = kw_hash(SYMS[i]) & (nc - 1); while (nt[q]) q = (q + 1) & (nc - 1); nt[q] = SYMS[i]; }
        free(SYMS); SYMS = nt; symcap = nc;
    }
    size_t q = kw_hash(s) & (symcap - 1);
    for (; SYMS[q]; q = (q + 1) & (symcap - 1)) if (SYMS[q] == s || (SYMS[q][0] == s[0] && !strcmp(SYMS[q], s))) return SYMS[q];
    size_t l = strlen(s) + 1; unsigned *m = malloc(sizeof(unsigned) * 4 + l); m[0] = m[1] = m[2] = m[3] = 0; char *r = (char *)(m + 4); memcpy(r, s, l);
    SYMS[q] = r; symn++;
    return r;
}
/* the hash of an interned name, kept in its header once computed (lookups from maxlev, which
 * runs at every node compiled, hashed the name each time) */
/* (the header of an interned name: [-4] its mark, [-3] its hash, [-2] its latest binder + 1) */
static inline size_t sym_hash(const char *x) { unsigned *m = (unsigned *)x - 3; if (!*m) *m = (unsigned)kw_hash(x) | 1; return *m; }
static inline unsigned sym_bind(const char *x) { return ((const unsigned *)x)[-2]; }
static inline unsigned sym_bind_epoch(const char *x) { return ((const unsigned *)x)[-1]; }
static unsigned bind_epoch = 1;   /* bumped whenever binders are popped: what a header remembers is from one epoch */
static inline void sym_bind_set(const char *x, unsigned i) { ((unsigned *)x)[-2] = i; ((unsigned *)x)[-1] = bind_epoch; }
/* a mark on an interned name (kept just before its characters): the merge of name sets */
static inline unsigned sym_mark(const char *x) { return ((const unsigned *)x)[-4]; }
static inline void sym_mark_set(const char *x, unsigned e) { ((unsigned *)x)[-4] = e; }
static Node *nsym(const char *s) { Node *x = ar_alloc(sizeof(Node)); x->s = intern(s); return x; }
/* a string literal's interned pointer, by the literal's address (a literal's contents never
 * change, so the address identifies it); not for buffers */
static const char *intern_lit(const char *lit) {
    static const char *K[1024], *V[1024];
    size_t i = ((uintptr_t)lit >> 3) & 1023;
    for (int k = 0; k < 8; k++, i = (i + 1) & 1023) { if (K[i] == lit) return V[i]; if (!K[i]) { K[i] = lit; return V[i] = intern(lit); } }
    return intern(lit);
}
static Node *nsyml(const char *lit) { Node *x = ar_alloc(sizeof(Node)); x->s = intern_lit(lit); return x; }
#define IS(k, lit) ((k) == intern_lit(lit))   /* k: a symbol's interned string */
/* the type written on a λ's binder, (x : A), kept on the binder's name node (a symbol's `it` is
 * otherwise unused), so it survives the passes that rebuild the λ around the name */
static void bnd_slots(Node *x) { if (!x->it) x->it = ar_alloc(3 * sizeof(Node *)); }
static void bnd_set(Node *x, Node *A) { bnd_slots(x); x->it[0] = A; }
static Node *bnd_ty(Node *x) { return !x->list && x->it ? x->it[0] : NULL; }
/* a binder's multiplicity (B11): 0 erased, 1 linear, 2 unconstrained (ω, the default), kept on the
 * name node of a Π binder or a field, as a word symbol */
static void bnd_set_mult(Node *x, int m) { bnd_slots(x); x->it[1] = m == 0 ? nsyml("0") : m == 1 ? nsyml("1") : NULL; }
static int bnd_mult(Node *x) { return !x->list && x->it && x->it[1] ? (x->it[1]->s[0] == '0' ? 0 : 1) : 2; }
/* an instance binder {{x : C A}} (E28): an implicit argument found by instance search */
static void bnd_set_inst(Node *x) { bnd_slots(x); x->it[2] = nsyml("inst"); }
static bool bnd_inst(Node *x) { return !x->list && x->it && x->it[2]; }
static Node *nlist(int n) { Node *x = ar_alloc(sizeof(Node)); x->list = 1; x->n = n; x->it = ar_alloc(((size_t)n + 1) * sizeof(Node *)); return x; }
static Node *nl2(const char *h, Node *a) { Node *x = nlist(2); x->it[0] = nsyml(h); x->it[1] = a; return x; }   /* h: a literal */
static Node *nl3(const char *h, Node *a, Node *b) { Node *x = nlist(3); x->it[0] = nsyml(h); x->it[1] = a; x->it[2] = b; return x; }
static Node *nl4(const char *h, Node *a, Node *b, Node *c) { Node *x = nlist(4); x->it[0] = nsyml(h); x->it[1] = a; x->it[2] = b; x->it[3] = c; return x; }

static const char *src;
/* comments, kept (for printing a file in the surface language) when asked */
static bool keep_cmt; static char *pend_cmt;
static void skip(void) {
    for (;;) {
        while (*src && isspace((unsigned char)*src)) src++;
        if (*src == ';') {
            const char *b = src; while (*src && *src != '\n') src++;
            if (keep_cmt) {
                while (*b == ';') b++; if (*b == ' ') b++;
                size_t ol = pend_cmt ? strlen(pend_cmt) : 0, nl = (size_t)(src - b);
                pend_cmt = realloc(pend_cmt, ol + nl + 2); memcpy(pend_cmt + ol, b, nl); pend_cmt[ol + nl] = '\n'; pend_cmt[ol + nl + 1] = 0;
            }
            continue;
        }
        return;
    }
}
/* an s-expression. The nesting is kept on an explicit stack (a read-back numeral is as deep as it
 * is large), each open list collecting its items until its ')' */
typedef struct { Node **items; size_t cap; int n; char *cmt; } Open;
static Node *read_atom(void) {
    const char *b = src;
    while (*src && !isspace((unsigned char)*src) && *src != '(' && *src != ')' && *src != ';') src++;
    size_t len = (size_t)(src - b);
    char *buf = malloc(len + 1);
    memcpy(buf, b, len); buf[len] = 0;
    Node *x = nsym(buf);
    free(buf);
    return x;
}
static Node *read_node(void) {
    skip();
    if (!*src) return NULL;
    Open *st = NULL; size_t cap = 0; int d = 0;
    for (;;) {
        skip();
        Node *x;
        if (*src == ')') { if (d == 0) die("unexpected )", NULL); src++; Open *o = &st[--d]; x = nlist(o->n); if (o->n) memcpy(x->it, o->items, (size_t)o->n * sizeof(Node *)); free(o->items); x->cmt = o->cmt; }
        else if (!*src) { if (d) die("unclosed (", NULL); free(st); return NULL; }
        else {
            char *cm = pend_cmt; pend_cmt = NULL;   /* the comment before a node is the node's */
            if (*src == '(') { src++; GROW(st, cap, (size_t)d + 1); st[d++] = (Open){ NULL, 0, 0, cm }; continue; }
            x = read_atom(); x->cmt = cm;
        }
        if (d == 0) { free(st); return x; }
        Open *o = &st[d - 1];
        GROW(o->items, o->cap, (size_t)o->n + 1);
        o->items[o->n++] = x;
    }
}
static bool is(Node *x, const char *s) { return x && !x->list && (x->s == s || strcmp(x->s, s) == 0); }
/* the head classes the compiler's walks dispatch on, found once per symbol node (not by a chain of
 * strcmp at every visit) */
enum { H_NONE = 1, H_LAM, H_PLAM, H_FORALL, H_LET, H_THE, H_CON, H_DATA, H_TRANSD, H_RECORD, H_BRANCHES, H_EQ, H_VAR, H_DIM, H_HOLE };
static int hk_of(Node *h) {
    if (h->list) return H_NONE;
    if (!h->hk) {
        static const char *nm[] = { "lam", "plam", "forall", "let", "the", "%con", "%data", "%transD", "%record", "%branches", "=", "%var", "%dim", "%hole" };
        static const char *in[sizeof nm / sizeof *nm]; static bool built;
        if (!built) { for (int i = 0; i < (int)(sizeof nm / sizeof *nm); i++) in[i] = intern(nm[i]); built = true; }
        int k = H_NONE;
        for (int i = 0; i < (int)(sizeof nm / sizeof *nm); i++) if (h->s == in[i]) { k = H_LAM + i; break; }   /* symbols are interned */
        h->hk = (short)k;
    }
    return h->hk;
}
static void print_node(FILE *o, Node *x) {
    if (!x->list) { fputs(x->s, o); return; }
    fputc('(', o);
    for (int i = 0; i < x->n; i++) { if (i) fputc(' ', o); print_node(o, x->it[i]); }
    fputc(')', o);
}
static bool node_eq(Node *a, Node *b) {
    if (a->list != b->list) return false;
    if (!a->list) return strcmp(a->s, b->s) == 0;
    if (a->n != b->n) return false;
    for (int i = 0; i < a->n; i++) if (!node_eq(a->it[i], b->it[i])) return false;
    return true;
}

/* ---------------- desugaring to the core forms ---------------- */
static const char *KW[] = { "%raw", "%welimP", "%rebuilt", "%hole", "%guess", "%spine", "lam", "thunk", "%delay", "delay", "force", "case", "elim", "if", "rec", "the", "code", "%data", "%con", "hcompD", "%hcompF", "%transD", "%case", "%branches", "record", "%record", "plam", "forall", "let", "app", "pair", "fst", "snd", "Pi", "Sigma", "U",
    "PathP", "@", "~", "/\\", "\\/", "=", "&", "|", "faceof", "%fneg", "sys", "sys0", "sys+", "sysmap", "comp", "Glue", "%hcompU", "%unglueU", "glue",
    "unglue", "%view", "%var", "%dim", "%ieq", "%feq", "%clauses", "%dnf", "%motive", "%restrict", "%gval", "%gty", "%dtab", "%dent", "%dtrans", "%abs", "%branch", "%clos", "%box", "%wsucc", "%weq", "%wlt", "%wadd", "%wmul", "%wdiv", "%wmod", "%wand", "%wor", "%wxor", "%wshl", "%wshr", "%wmax", "%wsub", "%welim", "%pimult", "%pim", "%fmult", "%linear", "%gusage", "%gsteps", "%gcode", "%dnidx", "%pty", "%bytesty", "%pval", "%bytes", "%blen", "%beq", "%ptr-of", "%region-at", "%ffi", "%native", "%natrep",
    "%i64add", "%i64sub", "%i64mul", "%i64eq", "%i64lt", "%u32add", "%u32sub", "%u32mul", "%u32eq", "%u32lt", "%f64add", "%f64sub", "%f64mul", "%f64div", "%f64eq", "%f64lt",
    "%u64add", "%u64sub", "%u64mul", "%u64eq", "%u64lt", "%u64div", "%u64mod", "%u64and", "%u64or", "%u64xor", "%u64shl", "%u64shr", "%i64div", "%i64mod", "%i64and", "%i64or", "%i64xor", "%i64shl", "%i64shr",
    "%i64ofw", "%wofi64", "%u32ofw", "%wofu32", "%f64ofw", "%f64ofi64", "%i64off64", "%f64bits", "%f64ofbits", "%f32bits", "%f32ofbits", "%u64ofw", "%wofu64", "%load8", "%load16", "%load32", "%load64", "%store8", "%store16", "%store32", "%store64", NULL };
/* the keywords, in an open-addressed table (asked of every symbol the compiler walks) */
static size_t kw_hash(const char *s);
static size_t kw_hash(const char *s) { size_t h = 1469598103934665603ull; for (; *s; s++) h = (h ^ (unsigned char)*s) * 1099511628211ull; return h; }
static bool keyword(const char *s) {
    enum { KWCAP = 512 };
    static const char *T[KWCAP]; static bool built;
    if (!built) {
        for (int i = 0; KW[i]; i++) { size_t j = kw_hash(KW[i]) & (KWCAP - 1); while (T[j]) j = (j + 1) & (KWCAP - 1); T[j] = KW[i]; }
        built = true;
    }
    for (size_t j = kw_hash(s) & (KWCAP - 1); T[j]; j = (j + 1) & (KWCAP - 1)) if (!strcmp(T[j], s)) return true;
    return false;
}
static Node *param_name(Node *p) { return p->list ? p->it[0] : p; }
/* a field's type is the data type D (0), or a path line whose fibre is, k dimensions deep (k); -1 otherwise */
static int rec_depth(Node *A, const char *D) {
    if (!A) return -1;
    if (!A->list) return !strcmp(A->s, D) ? 0 : -1;
    if (A->n >= 1 && !A->it[0]->list && !strcmp(A->it[0]->s, D)) return 0;
    if (A->n == 4 && !A->it[0]->list && !strcmp(A->it[0]->s, "PathP")) {
        Node *L = A->it[1];
        if (L->list && L->n == 3 && !L->it[0]->list && (!strcmp(L->it[0]->s, "plam") || !strcmp(L->it[0]->s, "lam"))) { int r = rec_depth(L->it[2], D); return r < 0 ? -1 : r + 1; }
    }
    return -1;
}
static bool mentions_name(Node *t, const char *x);
/* the shape of a recursive field's type (ConInfo.rsh): D itself, a path line into one, a function
 * (its domain not mentioning D, not a tick: a later D takes no hypothesis) into one, a pair with
 * recursive components (18.18); false when the type is not recursive */
static bool rec_shape(Node *A, const char *D, char *out, int *n) {
    if (!A || *n > 240) return false;
    if (!A->list) { if (strcmp(A->s, D)) return false; out[(*n)++] = 'D'; return true; }
    if (A->n < 1 || A->it[0]->list) return false;
    const char *h = A->it[0]->s;
    if (!strcmp(h, D)) { out[(*n)++] = 'D'; return true; }
    if (A->n == 4 && !strcmp(h, "PathP")) {
        Node *L = A->it[1];
        if (!(L->list && L->n == 3 && !L->it[0]->list && (!strcmp(L->it[0]->s, "plam") || !strcmp(L->it[0]->s, "lam")))) return false;
        int save = *n; out[(*n)++] = 'P';
        if (rec_shape(L->it[2], D, out, n)) return true;
        *n = save; return false;
    }
    if (!strcmp(h, "Pi") || !strcmp(h, "Sigma")) {
        Node *x = NULL, *dom = NULL, *cod = NULL;
        if (A->n == 4) { x = A->it[1]; dom = A->it[2]; cod = A->it[3]; }
        else if (A->n == 3 && A->it[1]->list && A->it[1]->n == 2 && !A->it[1]->it[0]->list) { x = A->it[1]->it[0]; dom = A->it[1]->it[1]; cod = A->it[2]; }
        else if (A->n == 3 && A->it[1]->list && A->it[1]->n >= 1 && A->it[1]->it[0]->list) {
            /* a telescope (Pi ((x A) (y B)) C): nested */
            Node *tel = A->it[1], *r = A->it[2];
            for (int q = tel->n - 1; q >= 0; q--) { Node *b = tel->it[q]; if (!b->list || b->n != 2) return false; Node *m = nlist(4); m->it[0] = A->it[0]; m->it[1] = b->it[0]; m->it[2] = b->it[1]; m->it[3] = r; r = m; }
            return rec_shape(r, D, out, n);
        } else if (A->n == 3 && A->it[2]->list && A->it[2]->n == 3 && !A->it[2]->it[0]->list && !strcmp(A->it[2]->it[0]->s, "lam")) { x = A->it[2]->it[1]; dom = A->it[1]; cod = A->it[2]->it[2]; }   /* desugared */
        else return false;
        (void)x;
        if (!strcmp(h, "Pi")) {
            if (dom->list && dom->n == 2 && !dom->it[0]->list && !strcmp(dom->it[0]->s, "Tick")) return false;
            if (mentions_name(dom, D)) return false;
            int save = *n; out[(*n)++] = 'F';
            if (rec_shape(cod, D, out, n)) return true;
            *n = save; return false;
        }
        char s1[256], s2[256]; int n1 = 0, n2 = 0;
        bool r1 = rec_shape(dom, D, s1, &n1), r2 = rec_shape(cod, D, s2, &n2);
        if (!r1 && !r2) return false;
        if (*n + n1 + n2 + 1 > 250) return false;
        out[(*n)++] = r1 && r2 ? 'B' : r1 ? '1' : '2';
        if (r1) { memcpy(out + *n, s1, (size_t)n1); *n += n1; }
        if (r2) { memcpy(out + *n, s2, (size_t)n2); *n += n2; }
        return true;
    }
    return false;
}

/* data types: higher inductive types (Coquand–Huber–Mörtberg), declared by
 * (data ...) below; constructors are values, eliminated by %case */
ConInfo *CON; static size_t capcon; static int ncon;
DataInfo *DATA; static size_t capdata; static int ndata;
/* codata types' fields: a field applied to a value is its projection */
FldInfo *FLD; static size_t capfld; static int nfld;
static int fld_lookup(const char *s) { for (int i = nfld - 1; i >= 0; i--) if (!strcmp(FLD[i].name, s)) return i; return -1; }
long case_hcomp_global = -1;
/* "%%c" names the first constructor declared c: the checker's own (lib/check.cub), referred to by
 * the code the checker is given, whatever the program declares later under the same name */
/* by name: an open-addressed table of the latest constructor of each name, rebuilt when constructors
 * have been declared since (declarations are rare, lookups constant) */
static int *CONH; static size_t capCONH; static int conh_built = -1;
static size_t kw_hash(const char *s);
static int con_lookup(const char *s) {
    if (s[0] == '%' && s[1] == '%') { for (int i = 0; i < ncon; i++) if (!strcmp(CON[i].name, s + 2)) return i; return -1; }
    if (conh_built != ncon) {
        size_t cap = 64; while (cap < (size_t)ncon * 2 + 2) cap *= 2;
        if (cap != capCONH) { free(CONH); CONH = malloc(cap * sizeof(int)); capCONH = cap; }
        for (size_t j = 0; j < cap; j++) CONH[j] = -1;
        for (int i = 0; i < ncon; i++) {
            size_t j = kw_hash(CON[i].name) & (cap - 1);
            while (CONH[j] >= 0 && strcmp(CON[CONH[j]].name, CON[i].name)) j = (j + 1) & (cap - 1);
            CONH[j] = i;   /* a later constructor of the same name replaces the earlier */
        }
        conh_built = ncon;
    }
    for (size_t j = kw_hash(s) & (capCONH - 1); CONH[j] >= 0; j = (j + 1) & (capCONH - 1)) if (CON[CONH[j]].name == s || !strcmp(CON[CONH[j]].name, s)) return CONH[j];
    return -1;
}
int con_index(const char *name) { return con_lookup(name); }
static void ford_rename(const char *old, const char *nw);
static int rec_retract(const char *name, char ***fn);
static int data_lookup(const char *s) { for (int i = ndata - 1; i >= 0; i--) if (!strcmp(DATA[i].name, s)) return i; return -1; }
int data_field_type(int d, int c, int k) {
    Node *dd = DATA[d].decl; if (!dd || c < 0 || c >= DATA[d].ncons) return -1;
    Node *cd = dd->it[2 + c]; if (!cd->list || k + 1 >= cd->n) return -1;
    Node *f = cd->it[k + 1]; if (!f->list || f->n != 2) return -1;
    Node *A = f->it[1]; Node *h = A->list && A->n ? A->it[0] : A;
    if (h->list) return -1;
    if (!strcmp(h->s, "Word")) return -2;
    if (!strcmp(h->s, "Bytes")) return -3;
    /* a primitive field (B12): -10 - its PT_* (Int64, UInt32, Float64, Float32), boxed as a word of its bits */
    if (!strcmp(h->s, "Int64")) return -10 - PT_I64;
    if (!strcmp(h->s, "UInt32")) return -10 - PT_U32;
    if (!strcmp(h->s, "Float64")) return -10 - PT_F64;
    if (!strcmp(h->s, "Float32")) return -10 - PT_F32;
    if (!strcmp(h->s, "UInt64")) return -10 - PT_U64;
    return data_lookup(h->s);
}

static Node *ds(Node *t);
static Node *to_code(Node *t);
/* induction hypotheses: within a branch, (rec x) is the elimination of the
 * recursive field x by the same branches and motive; innermost branch first */
static const char **rs_field, **rs_ih; static size_t caprsf, caprsi; static int nrs, nih;
long global_lookup(const char *name);
static bool constant_sym(const char *s);
/* the local variables free in a desugared term, in order of first occurrence: symbols not
 * bound within it, not global, not constants, not keywords */
typedef struct Names { const char **v; int n, cap; } Names;   /* interned names: compared by pointer */
static bool names_has_i(const Names *s, const char *x) { for (int i = 0; i < s->n; i++) if (s->v[i] == x) return true; return false; }   /* x interned */
static bool names_has(const Names *s, const char *x) { return names_has_i(s, intern(x)); }
static void names_add(Names *s, const char *x) { x = intern(x); for (int i = 0; i < s->n; i++) if (s->v[i] == x) return; if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 8; s->v = realloc(s->v, (size_t)s->cap * sizeof(char *)); } s->v[s->n++] = x; }
/* the variables a case alternative's pattern binds, in surface syntax: a name, or a constructor
 * applied to sub-patterns (nested); a numeral and a wildcard bind nothing. Every walker over
 * surface terms that tracks binders takes an alternative's variables from here */
static void pat_bind(Node *p, Names *b) {
    if (!p->list) { if (strcmp(p->s, "_") && strcmp(p->s, "%hole")) names_add(b, p->s); return; }
    if (p->n == 0 || (!p->it[0]->list && !strcmp(p->it[0]->s, "%num"))) return;
    for (int q = 1; q < p->n; q++) pat_bind(p->it[q], b);
}
static bool pat_binds(Node *p, const char *x) {
    if (!p->list) return !strcmp(p->s, x);
    if (p->n == 0 || (!p->it[0]->list && !strcmp(p->it[0]->s, "%num"))) return false;
    for (int q = 1; q < p->n; q++) if (pat_binds(p->it[q], x)) return true;
    return false;
}
/* the first alternative of a surface (case x alts…) or (elim P x alts…); 0 when t is neither */
static int alts_from(Node *t) {
    if (!t->list || t->n == 0 || t->it[0]->list) return 0;
    return !strcmp(t->it[0]->s, "elim") ? 3 : !strcmp(t->it[0]->s, "case") ? 2 : 0;
}
/* the free names of t in order of first occurrence, found once per node (fv) from its parts'
 * sets: a term nested in several case branches is walked once, not once per enclosing case.
 * Names bound inside t are left out; which free names a binder outside t covers is the caller's
 * (free_vars) */
static unsigned mark_epoch;   /* one counter for every merge that marks names (fvs, lvs) */
static Names *fvs(Node *t) {
    if (t->fv && t->fvgen == ar_gen) return t->fv;   /* made in the arena: stale after a release */
    t->fvgen = ar_gen;
    Names *r = ar_alloc(sizeof(Names));
    if (!t->list) {
        const char *x = t->s;
        if (!(constant_sym(x) || keyword(x) || global_lookup(x) >= 0 || (x[0] >= '0' && x[0] <= '9') || x[0] == '-')) {
            r->v = ar_alloc(sizeof(char *)); r->v[0] = x; r->n = r->cap = 1;
        }
        return t->fv = r;
    }
    const char *skip[2] = { NULL, NULL }; int from = 0, to = t->n;
    Node *h = t->n ? t->it[0] : NULL;
    if (h && !h->list) {
        int hk = hk_of(h);
        from = 1;
        if ((hk == H_LAM || hk == H_PLAM || hk == H_FORALL) && t->n == 3) { skip[0] = param_name(t->it[1])->s; from = 2; }
        else if (hk == H_LET && t->n == 4) { skip[1] = t->it[1]->s; from = 2; }   /* bound in the body only */
        else if (hk == H_CON || hk == H_DATA || hk == H_TRANSD || hk == H_RECORD) from = 2;
        else if (hk == H_BRANCHES) from = 4;
        else if (hk == H_EQ) to = 2;
    }
    /* merge the parts' sets, first occurrence first (a mark per name for this merge) */
    int cnt = 0;
    for (int i = from; i < to; i++) cnt += fvs(t->it[i])->n;
    const char **v = cnt ? ar_alloc((size_t)cnt * sizeof(char *)) : NULL; int n = 0;
    if (cnt) {
        unsigned e = ++mark_epoch;
        for (int i = from; i < to; i++) {
            const Names *f = fvs(t->it[i]);
            bool body = skip[1] && i == 3;
            for (int k = 0; k < f->n; k++) {
                const char *x = f->v[k];
                if (x == skip[0] || (body && x == skip[1])) continue;
                if (sym_mark(x) == e) continue;
                sym_mark_set(x, e); v[n++] = x;
            }
        }
    }
    r->v = v; r->n = r->cap = n;
    return t->fv = r;
}
static void free_vars(Node *t, Names *bound, Names *out) {
    const Names *f = fvs(t);
    for (int k = 0; k < f->n; k++) if (!names_has_i(bound, f->v[k])) names_add(out, f->v[k]);
}
/* does t refer to the free name x (not rebound inside)? */
static bool mentions_name(Node *t, const char *x) {
    if (!t->list) return !strcmp(t->s, x);
    if (t->n == 0) return false;
    if (!t->it[0]->list) {
        const char *k = t->it[0]->s;
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && t->n >= 3) {
            for (int i = 1; i < t->n - 1; i++) if (!strcmp(param_name(t->it[i])->s, x)) return false;
            return mentions_name(t->it[t->n - 1], x);
        }
        if ((!strcmp(k, "let") || !strcmp(k, "Pi") || !strcmp(k, "Sigma")) && t->n == 4 && !t->it[1]->list)
            return mentions_name(t->it[2], x) || (strcmp(t->it[1]->s, x) && mentions_name(t->it[3], x));
        int from = alts_from(t);
        if (from) {
            for (int i = 1; i < from && i < t->n; i++) if (mentions_name(t->it[i], x)) return true;
            for (int i = from; i < t->n; i++) {
                Node *a = t->it[i];
                if (a->list && a->n == 2) { if (!pat_binds(a->it[0], x) && mentions_name(a->it[1], x)) return true; }
                else if (mentions_name(a, x)) return true;
            }
            return false;
        }
    }
    for (int i = 0; i < t->n; i++) if (mentions_name(t->it[i], x)) return true;
    return false;
}
static bool mentions_rec(Node *t, Node **names, int n) {
    if (!t->list) return false;
    if (t->n == 2 && is(t->it[0], "rec") && !t->it[1]->list)
        for (int i = 0; i < n; i++) if (names[i] && !strcmp(names[i]->s, t->it[1]->s)) return true;
    for (int i = 0; i < t->n; i++) if (mentions_rec(t->it[i], names, n)) return true;
    return false;
}
static Node *ds_lam(Node **ps, int np, Node *body, const char *b) {
    if (np == 0) return ds(body);
    return nl3(b, param_name(ps[0]), ds_lam(ps + 1, np - 1, body, b));
}
static void arity(Node *t, int n) { if (t->n != n + 1) { print_node(stderr, t); die("wrong number of operands in", t->it[0]->s); } }
static Node *ds(Node *t) {
    if (!t->list) return t;
    if (t->n == 0) die("empty list", NULL);
    Node *h = t->it[0];
    if (!h->list && keyword(h->s)) {
        const char *k = h->s;
        /* delay e: e held unbuilt (the machine's demand, V_THUNK), transparent — an operation that
         * meets it builds it, force of it is it; typed as e (tthunk). The suspensions the
         * elaborator makes (a system's branches, a copattern's handler) are (thunk …) records */
        if (IS(k, "delay")) { arity(t, 1); return nl2("%delay", ds(t->it[1])); }
        /* a term already desugared (an annotated code) */
        if (IS(k, "%raw")) { arity(t, 1); return t->it[1]; }
        if (IS(k, "force")) { arity(t, 1); return nl2("force", ds(t->it[1])); }
        if (IS(k, "%rebuilt")) { arity(t, 1); return nl2("%rebuilt", ds(t->it[1])); }
        /* elaboration: a hole (an implicit argument to find), a guessed λ or application, a spine
         * with holes */
        if (IS(k, "%hole")) { arity(t, 1); return t; }
        if (IS(k, "%guess")) { arity(t, 3); return nl4("%guess", t->it[1], t->it[2], ds(t->it[3])); }
        if (IS(k, "%spine")) { arity(t, 1); return nl2("%spine", ds(t->it[1])); }
        if (IS(k, "record")) {
            /* (record [D] (f t)...): a copattern, one term per field of the codata type D, in any
             * order; desugared to (%record D t...) in the declared order */
            int from = 1, d = -1;
            if (t->n >= 2 && !t->it[1]->list) { d = data_lookup(t->it[1]->s); if (d < 0 || !DATA[d].co) die("record: not a codata type:", t->it[1]->s); from = 2; }
            for (int i = from; i < t->n; i++) {
                Node *b = t->it[i];
                if (!b->list || b->n != 2 || b->it[0]->list) die("record: a field is (f t)", NULL);
                int fl = fld_lookup(b->it[0]->s);
                if (fl < 0) die("record: not a field:", b->it[0]->s);
                if (d < 0) d = FLD[fl].data;
                else if (FLD[fl].data != d) die("record: fields of different codata types:", b->it[0]->s);
            }
            if (d < 0) die("record: (record D) for a codata type without fields", NULL);
            int nf = DATA[d].nflds;
            Node *r = nlist(nf + 2); r->it[0] = nsym("%record"); r->it[1] = nsym(DATA[d].name);
            for (int k2 = 0; k2 < nf; k2++) {
                Node *v = NULL;
                for (int i = from; i < t->n; i++) if (!strcmp(t->it[i]->it[0]->s, FLD[DATA[d].flds[k2]].name)) { if (v) die("record: a field given twice:", FLD[DATA[d].flds[k2]].name); v = t->it[i]->it[1]; }
                if (!v) die("record: a field is missing:", FLD[DATA[d].flds[k2]].name);
                r->it[k2 + 2] = ds(v);
            }
            if (t->n - from != nf) die("record: a field given twice", NULL);
            return r;
        }
        if (IS(k, "rec")) {
            arity(t, 1);
            if (!t->it[1]->list) for (int i = nrs - 1; i >= 0; i--) if (!strcmp(rs_field[i], t->it[1]->s)) return nl2("force", nsym(rs_ih[i]));
            die("rec of a name that is not a recursive field of its branch:", t->it[1]->list ? "(...)" : t->it[1]->s);
        }
        if (IS(k, "if")) {
            /* elimination of Bool */
            arity(t, 3);
            Node *c = nlist(4); c->it[0] = nsym("case"); c->it[1] = t->it[1];
            Node *bt = nlist(2); bt->it[0] = nlist(1); bt->it[0]->it[0] = nsym("true"); bt->it[1] = t->it[2];
            Node *bf = nlist(2); bf->it[0] = nlist(1); bf->it[0]->it[0] = nsym("false"); bf->it[1] = t->it[3];
            c->it[2] = bt; c->it[3] = bf;
            return ds(c);
        }
        if (IS(k, "case") || IS(k, "elim")) {
            /* (case e branch...) and (elim P e branch...): one branch per constructor,
             * a λ over its fields and dimensions returning a thunk of the body */
            bool el = IS(k, "elim");
            int first = el ? 3 : 2;
            /* t57 (2026-11-12): wrong. An implicit argument left to the elaborator is refused with
             * "reason: (elim P e branch...)" (this die) where its solution contains an elimination with
             * no branches (here Empty-rec applied in the expected type, inside a path over a
             * bool-elim1 of a pcat). Reproducer: at commit 8af9a50, in experiments/nbe/p26-wip.cub's
             * diag-nondim-d, (lcons e-dim sk) clause, replace `Empty-rec {lsuc lzero} {PathP …}
             * (tf-empty hq)` by `Empty-rec {lsuc lzero} (tf-empty hq)` and check with CUBICAL_NOCACHE=1:
             * "FAIL: checks diag-nondim-d / reason: (elim P e branch...)" (run 2026-11-12); with the
             * type given it checks. Smaller cases (Empty-rec's type a stuck Empty-rec, or a path of it
             * applied at a dimension, through an implicit) check, so it is not yet minimised; the same
             * message met kp-lhs written with pcat-i (implicit endpoints containing carry). Not yet
             * located where a solution is read back without its data type's name. Workaround: the
             * implicit given (diag-nondim-d; kp-lhs with pcat). */
            if (t->n < first + 1) die(el ? "(elim P e branch...)" : "(case e branch...)", NULL);
            int d = -1; Node *dflt = NULL; Node *dname = NULL;
            if (t->n == first + 1 && !t->it[first]->list && data_lookup(t->it[first]->s) >= 0 && DATA[data_lookup(t->it[first]->s)].co)
                die("a codata type has no elimination (it is observed by its fields):", t->it[first]->s);
            if (t->n == first + 1 && !t->it[first]->list && data_lookup(t->it[first]->s) >= 0 && DATA[data_lookup(t->it[first]->s)].ncons == 0) {
                /* a type with no constructors: (case e D), (elim P e D), no branches */
                int d0 = data_lookup(t->it[first]->s);
                Node *B = nlist(4); B->it[0] = nsym("%branches"); B->it[1] = nsym(DATA[d0].name); B->it[2] = nsym("0"); B->it[3] = nsym("0");
                Node *env = nl3("lam", nsym("%g"), nsym("%g"));
                if (el) { Node *r = nlist(5); r->it[0] = nsym("%case"); r->it[1] = B; r->it[2] = ds(t->it[1]); r->it[3] = env; r->it[4] = ds(t->it[2]); return r; }
                return nl4("%case", B, env, ds(t->it[1]));
            }
            for (int i = first; i < t->n; i++) {
                Node *br = t->it[i];
                if (!br->list || br->n != 2) die("malformed case branch", NULL);
                if (!br->it[0]->list) {
                    /* a default branch: (_ body), or (x body) with x the scrutinee, rebuilt from the
                     * fields of whichever constructor it is */
                    dflt = br->it[1]; dname = is(br->it[0], "_") ? NULL : br->it[0];
                    continue;
                }
                if (!br->it[0]->list || br->it[0]->n < 1) die("case branch pattern must be (C x...)", NULL);
                int c = con_lookup(br->it[0]->it[0]->s);
                if (c < 0) die("unknown constructor", br->it[0]->it[0]->s);
                if (d >= 0 && CON[c].data != d) die("constructors of different types in one case:", CON[c].name);
                d = CON[c].data;
                if (br->it[0]->n - 1 != CON[c].nfields + CON[c].ndims) die("wrong number of fields and dimensions for", CON[c].name);
            }
            if (d < 0) die("case needs at least one constructor branch", NULL);
            int N = DATA[d].ncons;
            Node *B = nlist(N + 4); B->it[0] = nsym("%branches"); B->it[1] = nsym(DATA[d].name);
            uint64_t mask = 0;
            Node **hb = calloc((size_t)N + 1, sizeof(Node *));
            Names fv = { 0 };
            for (int ci = 0; ci < N; ci++) {
                ConInfo *c = &CON[DATA[d].cons[ci]];
                Node *pat = NULL, *body = NULL;
                for (int i = first; i < t->n; i++) {
                    Node *br = t->it[i];
                    if (br->it[0]->list && !strcmp(br->it[0]->it[0]->s, c->name)) { pat = br->it[0]; body = br->it[1]; }
                }
                /* within branch (C x...) the scrutinee is (C x...): a branch mentioning the
                 * scrutinee's name rebuilds it from its fields instead of a copy of it */
                Node *scr = t->it[first - 1];
                if (body && !scr->list && !dname && mentions_name(body, scr->s)) {
                    bool shadow = false;
                    /* (the elaborator has rebuilt it already, at its type: let scr = the T (%rebuilt (C x…))) */
                    if (body->list && body->n == 4 && !body->it[0]->list && !strcmp(body->it[0]->s, "let") && !body->it[1]->list && !strcmp(body->it[1]->s, scr->s)
                        && body->it[2]->list && body->it[2]->n == 3 && !body->it[2]->it[0]->list && !strcmp(body->it[2]->it[0]->s, "the")
                        && body->it[2]->it[2]->list && body->it[2]->it[2]->n == 2 && !body->it[2]->it[2]->it[0]->list && !strcmp(body->it[2]->it[2]->it[0]->s, "%rebuilt")) shadow = true;
                    for (int q = 1; pat && q < pat->n; q++) if (!pat->it[q]->list && !strcmp(pat->it[q]->s, scr->s)) shadow = true;
                    if (!shadow) {
                        static int nsc;
                        int na = c->nfields + c->ndims;
                        if (!pat) { pat = nlist(na + 1); pat->it[0] = nsym(c->name); for (int q = 0; q < na; q++) { char nm[32]; snprintf(nm, sizeof nm, "%%sc%d", nsc++); pat->it[q + 1] = nsym(nm); } }
                        Node *rebuilt = nsym(c->name);
                        Node *np2 = nlist(na + 1); np2->it[0] = pat->it[0];
                        Node *pre = NULL;
                        for (int q = 0; q < na; q++) {
                            Node *x = pat->it[q + 1];
                            if (!x->list && !strcmp(x->s, "_")) { char nm[32]; snprintf(nm, sizeof nm, "%%sc%d", nsc++); x = nsym(nm); }
                            np2->it[q + 1] = x; rebuilt = nl3("app", rebuilt, x);
                        }
                        (void)pre;
                        pat = np2;
                        /* (%rebuilt (C x...)): the value is the scrutinee, and is named by it */
                        Node *lb = nlist(4); lb->it[0] = nsym("let"); lb->it[1] = scr; lb->it[2] = nl2("%rebuilt", rebuilt); lb->it[3] = body; body = lb;
                    }
                }
                if (!body) {
                    if (!dflt) die("case misses constructor", c->name);
                    body = dflt;
                    if (dname) {
                        static int ndf;
                        int na = c->nfields + c->ndims;
                        pat = nlist(na + 1); pat->it[0] = nsym(c->name);
                        Node *rebuilt = nsym(c->name);
                        for (int q = 0; q < na; q++) { char nm[32]; snprintf(nm, sizeof nm, "%%df%d", ndf++); pat->it[q + 1] = nsym(nm); rebuilt = nl3("app", rebuilt, nsym(nm)); }
                        body = nlist(4); body->it[0] = nsym("let"); body->it[1] = dname; body->it[2] = nl2("%rebuilt", rebuilt); body->it[3] = dflt;
                    }
                }
                /* a branch takes the induction hypotheses of its recursive fields when its body
                 * uses one: λ fields, hypotheses, dimensions, of a thunk of the body, so nothing
                 * in an unselected branch, and no recursive call lifted out of it, is computed */
                Node *rn[64]; int nr = 0;
                if (pat) for (int f = 0; f < c->nfields; f++) if (c->recmask >> f & 1) rn[nr++] = pat->it[f + 1];
                bool ih = nr && mentions_rec(body, rn, nr);
                char ihn[64][24];
                int save = nrs;
                if (ih) {
                    mask |= 1ull << ci;
                    for (int q = 0; q < nr; q++) {
                        snprintf(ihn[q], sizeof ihn[q], "%%ih%d", nih++);
                        GROW(rs_field, caprsf, (size_t)nrs + 1); GROW(rs_ih, caprsi, (size_t)nrs + 1);
                        rs_field[nrs] = rn[q]->s; rs_ih[nrs] = ihn[q]; nrs++;
                    }
                }
                Node *hnd = nl2("thunk", ds(body));
                nrs = save;
                for (int f = c->nfields + c->ndims; f > c->nfields; f--) {
                    Node *x = pat ? pat->it[f] : NULL;
                    char nm[32];
                    if (!x) { snprintf(nm, sizeof nm, "%%_%d", f); x = nsym(nm); }
                    hnd = nl3("lam", x, hnd);
                }
                if (ih) for (int q = nr - 1; q >= 0; q--) hnd = nl3("lam", nsym(ihn[q]), hnd);
                for (int f = c->nfields; f >= 1; f--) {
                    Node *x = pat ? pat->it[f] : NULL;
                    char nm[32];
                    if (!x) { snprintf(nm, sizeof nm, "%%_%d", f); x = nsym(nm); }
                    hnd = nl3("lam", x, hnd);
                }
                Names bound = { 0 };
                free_vars(hnd, &bound, &fv);
                free(bound.v);
                hb[ci] = hnd;
            }
            /* the variables the branches share are not captured by each branch (copied into
             * every one, all but one then erased) but supplied once to the selected one: each
             * branch is λ shared..., and the environment λ g. g shared... applies it */
            for (int ci = 0; ci < N; ci++) {
                Node *hnd = hb[ci];
                for (int q = fv.n - 1; q >= 0; q--) hnd = nl3("lam", nsym(fv.v[q]), hnd);
                B->it[ci + 4] = hnd;
            }
            { char mb[24]; snprintf(mb, sizeof mb, "%llu", (unsigned long long)mask); B->it[2] = nsym(mb);
              snprintf(mb, sizeof mb, "%d", fv.n); B->it[3] = nsym(mb); }
            Node *env = nsym("%g");
            for (int q = 0; q < fv.n; q++) env = nl3("app", env, nsym(fv.v[q]));
            env = nl3("lam", nsym("%g"), env);
            free(hb); free(fv.v);
            if (el) { Node *r = nlist(5); r->it[0] = nsym("%case"); r->it[1] = B; r->it[2] = ds(t->it[1]); r->it[3] = env; r->it[4] = ds(t->it[2]); return r; }
            return nl4("%case", B, env, ds(t->it[1]));
        }
        if (IS(k, "lam") || IS(k, "plam")) { if (t->n < 3) die("binder without body", NULL); return ds_lam(t->it + 1, t->n - 2, t->it[t->n - 1], k); }
        if (IS(k, "forall")) { arity(t, 2); return nl3("forall", t->it[1], ds(t->it[2])); }
        if (IS(k, "let")) { arity(t, 3); return nl4("let", t->it[1], ds(t->it[2]), ds(t->it[3])); }
        if ((IS(k, "Pi") || IS(k, "Sigma")) && t->n == 3 && t->it[1]->list && t->it[1]->n >= 1 && t->it[1]->it[0]->list) {
            /* a telescope: (Pi ((x A) (y B) ...) C) */
            Node *tel = t->it[1], *r = t->it[2];
            for (int q = tel->n - 1; q >= 0; q--) {
                Node *b = tel->it[q];
                if (!b->list || b->n != 2 || b->it[0]->list) die("malformed telescope in", k);
                Node *n = nlist(4); n->it[0] = nsym(k); n->it[1] = b->it[0]; n->it[2] = b->it[1]; n->it[3] = r; r = n;
            }
            return ds(r);
        }
        if (IS(k, "Pi") || IS(k, "Sigma")) {
            Node *x, *A, *B;
            if (t->n == 4) { x = t->it[1]; A = t->it[2]; B = t->it[3]; }
            else if (t->n == 3 && t->it[1]->list && t->it[1]->n == 2) { x = t->it[1]->it[0]; A = t->it[1]->it[1]; B = t->it[2]; }
            else die("malformed", k);
            return nl3(k, ds(A), nl3("lam", x, ds(B)));
        }
        if (IS(k, "sys")) {
            Node *r = nlist(1); r->it[0] = nsym("sys0");
            for (int i = t->n - 1; i >= 1; i--) {
                Node *br = t->it[i];
                if (!br->list || br->n != 2) die("malformed system branch", NULL);
                r = nl4("sys+", ds(br->it[0]), nl2("thunk", ds(br->it[1])), r);
            }
            return r;
        }
        if (IS(k, "sys+")) { arity(t, 3); return nl4("sys+", ds(t->it[1]), nl2("thunk", ds(t->it[2])), ds(t->it[3])); }
        if (IS(k, "U")) {
            /* the level is kept for codes: a numeral, or a level expression */
            if (t->n > 2) die("(U [level])", NULL);
            if (t->n == 2 && !(t->it[1]->s && t->it[1]->s[0] >= '0' && t->it[1]->s[0] <= '9')) return nl2("U", ds(t->it[1]));
            return t;
        }
        if (IS(k, "the")) { arity(t, 2); return nl3("the", ds(t->it[1]), ds(t->it[2])); }
        if (IS(k, "code")) { arity(t, 1); return to_code(ds(t->it[1])); }
        if (IS(k, "/\\") || IS(k, "\\/") || IS(k, "&") || IS(k, "|")) {
            if (t->n < 3) die("too few operands for", k);
            Node *r = ds(t->it[1]);
            for (int i = 2; i < t->n; i++) r = nl3(k, r, ds(t->it[i]));
            return r;
        }
        static const struct { const char *k; int n; } AR[] = { { "pair", 2 }, { "fst", 1 }, { "snd", 1 }, { "PathP", 3 },
            { "~", 1 }, { "=", 2 }, { "faceof", 1 }, { "sysmap", 2 }, { "comp", 3 }, { "Glue", 2 }, { "glue", 2 },
            { "unglue", 2 }, { "app", 2 }, { "hcompD", 2 }, { NULL, 0 } };
        if (IS(k, "@")) { if (t->n != 3 && t->n != 5) die("@ takes a path, an interval and optionally both endpoints", NULL); }
        else for (int i = 0; AR[i].k; i++) if (!strcmp(AR[i].k, k)) arity(t, AR[i].n);
        Node *r = nlist(t->n); r->it[0] = h;
        for (int i = 1; i < t->n; i++) r->it[i] = (IS(k, "=") && i == 2) ? t->it[i] : ds(t->it[i]);
        return r;
    }
    Node *f = ds(h);
    for (int i = 1; i < t->n; i++) f = nl3("app", f, ds(t->it[i]));
    return f;
}

/* ---------------- codes of source terms ----------------
 * (code t) writes the code of a source term as data: constructor
 * applications of lib/code.cub, variables as de Bruijn indices over one
 * context of term and dimension variables, checked definitions as signature
 * indices. This is reading source text, not inspecting a value. */
static const char **cscope; static size_t capcscope; static int ncscope;
static char **CHECKED; static size_t capchecked; static int nchecked;
static int *CHKDATA; static size_t capchkdata; static int nchkdata;   /* checked data types, in signature order */
/* hidden: an entry no source name refers to (a rejected definition, a codata type's generated
 * composition and record) */
static bool *CHKHIDE; static size_t capchkhide;
static char **CHKPOS; static size_t capchkpos;   /* where each checked definition was declared (file:line), for #where */
extern char decl_pos[512];
static char **DATAPOS; static size_t capdatapos;
static int checked_index(const char *s) { for (int i = nchecked - 1; i >= 0; i--) if (!CHKHIDE[i] && (CHECKED[i] == s || !strcmp(CHECKED[i], s))) return i; return -1; }
/* a name a checked term cannot use. A definition of a file loaded before the checker (lib/prelude.cub's
 * map, length, append, add, mul, and, iszero; the checker's own helpers) has no type and cannot be
 * given one there (a signature needs the checker): the message says so and names the checked ones */
static void die_unchecked(const char *s) {
    if (global_lookup(s) >= 0) {
        char m[512]; snprintf(m, sizeof m, "code: %s is a helper defined before the checker was loaded (lib/prelude.cub and the checker's files): it has no type, so a checked definition cannot use it. Checked ones in lib/std.cub: list-map, list-length, list-append (for map, length, append), plus, times (for add, mul)", s);
        die(m, NULL);
    }
    die("code: not a bound variable or checked definition:", s);
}
static int chk_index(int d) { for (int i = 0; i < nchkdata; i++) if (CHKDATA[i] == d) return i; return -1; }
int data_checked_index(int d) { return chk_index(d); }
/* the trusted surface (B12): every postulate and every primitive declared, in order; #trusted lists it */
/* the foreign table (D27) */
Ffi *FFI; int nffi; static size_t capffi;
int ffi_add(const char *sym, int n, const unsigned char *kind, unsigned char ret) {
    /* "sym" in the running program (its C library, and the math library), or "lib.so:sym" in that
     * shared library, opened */
    void *fn = NULL; const char *colon = strchr(sym, ':');
    if (colon) {
        char *lib = strndup(sym, (size_t)(colon - sym)); void *h = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
        if (!h) die("foreign: cannot open the library of", sym);
        fn = dlsym(h, colon + 1); free(lib); sym = colon + 1;
    } else {
        static void *self, *libm; if (!self) self = dlopen(NULL, RTLD_LAZY);
        fn = self ? dlsym(self, sym) : NULL;
        if (!fn) { if (!libm) libm = dlopen("libm.so.6", RTLD_LAZY | RTLD_GLOBAL); if (libm) fn = dlsym(libm, sym); }
    }
    if (!fn) die("foreign: no such symbol in the running program or its libraries (write \"lib.so:sym\" for another):", sym);
    GROW(FFI, capffi, (size_t)nffi + 1);
    FFI[nffi] = (Ffi){ strdup(sym), fn, n, { 0 }, ret, NULL };
    memcpy(FFI[nffi].kind, kind, (size_t)n);
    return nffi++;
}
typedef struct { const char *name; const char *how; } Trusted;
static Trusted *TRUSTED; static size_t captrusted; static int ntrusted;
static void trusted_add(const char *name, const char *how) { GROW(TRUSTED, captrusted, (size_t)ntrusted + 1); TRUSTED[ntrusted++] = (Trusted){ intern(name), how }; }
/* the computed usages of each checked definition's parameters (B11): names (srcname words) and
 * words 0, 1, 2, from the checker after a definition is accepted, or from a cached module's record */
typedef struct { int n; const char **nm; unsigned char *u; } Usages;
static Usages *GUSAGE; static size_t capgusage;
static void usage_set(int k, int n, const char **nm, const unsigned char *u) {
    if ((size_t)k >= capgusage) { size_t c = capgusage ? capgusage : 256; while (c <= (size_t)k) c *= 2; GUSAGE = realloc(GUSAGE, c * sizeof(Usages)); memset(GUSAGE + capgusage, 0, (c - capgusage) * sizeof(Usages)); capgusage = c; }
    GUSAGE[k].n = n; GUSAGE[k].nm = malloc((size_t)n * sizeof(char *)); GUSAGE[k].u = malloc((size_t)n);
    for (int i = 0; i < n; i++) { GUSAGE[k].nm[i] = intern(nm[i]); GUSAGE[k].u[i] = u[i]; }
}
int global_usage(long n, long k) { return n >= 0 && (size_t)n < capgusage && k >= 0 && k < GUSAGE[n].n ? GUSAGE[n].u[k] : 2; }
/* the steps the checker took to accept checked definition n (18.14's cost model, first form: recorded
 * steps per key), from this run or from the store's ok line; 0 when unknown */
static long *GSTEPS; static size_t capgsteps;
static void gsteps_set(long k, long steps) {
    if ((size_t)k >= capgsteps) { size_t c = capgsteps ? capgsteps : 256; while (c <= (size_t)k) c *= 2; GSTEPS = realloc(GSTEPS, c * sizeof(long)); memset(GSTEPS + capgsteps, 0, (c - capgsteps) * sizeof(long)); capgsteps = c; }
    GSTEPS[k] = steps;
}
/* the desugared term of each checked definition, kept (a copy outside the arena) for (%gcode n):
 * its code as written, the globals it names kept as cglob — the back end's view of a definition
 * before normalization unfolds the definitions it calls */
static Node **CHKSRC; static size_t capchksrc;
static long stat_perm_bytes, stat_rec_bytes;   /* CUBICAL_MEMDEF */
static Node *perm_copy(Node *t) {
    Node *x = calloc(1, sizeof(Node)); x->list = t->list; x->s = t->s; x->n = t->n; stat_perm_bytes += (long)sizeof(Node) + (t->list ? (long)((size_t)t->n + 1) * 8 : 0);
    if (t->list) { x->it = calloc((size_t)t->n + 1, sizeof(Node *)); for (int i = 0; i < t->n; i++) x->it[i] = perm_copy(t->it[i]); }
    return x;
}
static void chksrc_set(long ck, Node *dt) {
    if (ck < 0) return;
    { size_t o_ = capchksrc; GROW(CHKSRC, capchksrc, (size_t)ck + 1); if (capchksrc > o_) memset(CHKSRC + o_, 0, (capchksrc - o_) * sizeof *CHKSRC); }   /* (GROW leaves the new entries unset; a definition never given one reads NULL) */
    CHKSRC[ck] = perm_copy(dt);
}
/* and its type, desugared (#jit reads the parameters' and the result's types off it: jt_of) */
static Node **CHKTY; static size_t capchkty;
static void chkty_set(long ck, Node *dT) {
    if (ck < 0) return;
    { size_t o_ = capchkty; GROW(CHKTY, capchkty, (size_t)ck + 1); if (capchkty > o_) memset(CHKTY + o_, 0, (capchkty - o_) * sizeof *CHKTY); }   /* (GROW leaves the new entries unset; a definition never given one reads NULL) */
    CHKTY[ck] = perm_copy(dT);
}
static int chk_index(int d);
/* the domains of a desugared Π chain (up to n) and its codomain */
static int pi_parts(Node *T, Node **doms, int n, Node **cod) {
    int k = 0;
    for (;;) {
        if (k >= n || !T->list || !is(T->it[0], "Pi")) break;
        if (T->n == 4) { doms[k++] = T->it[2]; T = T->it[3]; continue; }
        /* desugared: (Pi A (lam x B)) */
        if (T->n == 3 && T->it[2]->list && T->it[2]->n == 3 && is(T->it[2]->it[0], "lam")) { doms[k++] = T->it[1]; T = T->it[2]->it[2]; continue; }
        break;
    }
    *cod = T; return k;
}
static Node *to_code(Node *t);
static Port build_top(Node *t);
static pthread_mutex_t gcode_mu = PTHREAD_MUTEX_INITIALIZER;
Port gcode_build(long n) {
    if (n < 0 || (size_t)n >= capchksrc || !CHKSRC[n]) return 0;
    pthread_mutex_lock(&gcode_mu);
    ArMark m = ar_mark();
    int save = ncscope; ncscope = 0;
    Node *c = to_code(CHKSRC[n]);
    Port p = build_top(c);
    ncscope = save;
    ar_release(m);
    pthread_mutex_unlock(&gcode_mu);
    return p;
}
long global_steps(long n) { return n >= 0 && (size_t)n < capgsteps ? GSTEPS[n] : 0; }
int checked_data(long k) { return k >= 0 && k < nchkdata ? CHKDATA[k] : -1; }
/* the checker's tables: entry i of each is a global, so a lookup is a reference */
static long *TABLE[TB_N]; static size_t TABCAP[TB_N]; static long TABN[TB_N];
static void table_set(int t, long i, long g) {
    if ((size_t)i >= TABCAP[t]) { size_t nc = TABCAP[t] ? TABCAP[t] * 2 : 64; while (nc <= (size_t)i) nc *= 2; TABLE[t] = realloc(TABLE[t], nc * sizeof(long)); TABCAP[t] = nc; }
    for (long k = TABN[t]; k < i; k++) TABLE[t][k] = -1;   /* the entries skipped are empty */
    TABLE[t][i] = g; if (i >= TABN[t]) TABN[t] = i + 1;
}
long table_global(int t, long i) { return t >= 0 && t < TB_N && i >= 0 && i < TABN[t] ? TABLE[t][i] : -1; }
static Node *word(long n) { char b[32]; snprintf(b, sizeof b, "#%ld", n); return nsym(b); }
/* the code's constructors are named %%c (the first declared), so a program's names cannot capture them */
static Node *K(const char *c) { char b[64]; snprintf(b, sizeof b, "%%%%%s", c); return nsym(b); }
static Node *capp1(const char *c, Node *a) { return nl3("app", K(c), a); }
static Node *capp2(const char *c, Node *a, Node *b) { return nl3("app", nl3("app", K(c), a), b); }
static Node *capp3(const char *c, Node *a, Node *b, Node *d) { return nl3("app", capp2(c, a, b), d); }
static Node *capp4(const char *c, Node *a, Node *b, Node *d, Node *e) { return nl3("app", capp3(c, a, b, d), e); }
/* a list of codes as data */
static Node *code_list(Node **xs, int n) { Node *l = K("lnil"); for (int i = n - 1; i >= 0; i--) l = capp2("lcons", xs[i], l); return l; }
static bool bound_name(const char *s) { for (int i = ncscope - 1; i >= 0; i--) if (!strcmp(cscope[i], s)) return true; return false; }
static int checked_data_of(const char *s, const char *what) {
    int d = data_lookup(s), di = d < 0 ? -1 : chk_index(d);
    if (di < 0) die(what, s);
    return di;
}
/* a branch of an elimination: λ fields, hypotheses (one per recursive field), dimensions,
 * of a thunk of the body. A branch not using its hypotheses has no binders for them in
 * the machine; its code binds them anyway, unused. */
static Node *code_branch(Node *b, ConInfo *c, bool ih, int i) {
    int nf = c->nfields, nr = c->nrec, ar = nf + nr + c->ndims;
    if (i == ar) {
        if (!b->list || b->n != 2 || !is(b->it[0], "thunk")) die("code: malformed branch", NULL);
        return to_code(b->it[1]);
    }
    bool hyp = i >= nf && i < nf + nr;
    GROW(cscope, capcscope, (size_t)ncscope + 1);
    Node *r;
    if (hyp && !ih) {
        cscope[ncscope++] = "%unused-ih";
        r = code_branch(b, c, ih, i + 1);
    } else {
        if (!b->list || b->n != 3 || !is(b->it[0], "lam")) die("code: malformed branch", NULL);
        cscope[ncscope++] = b->it[1]->s;
        r = code_branch(b->it[2], c, ih, i + 1);
    }
    ncscope--;
    return capp1(i < nf + nr ? "clam" : "cdlam", r);
}
static Node *code_bound(const char *x, Node *body) {
    GROW(cscope, capcscope, (size_t)ncscope + 1);
    cscope[ncscope++] = x;
    Node *r = to_code(body);
    ncscope--;
    return r;
}
static Node *to_code(Node *t) {
    /* universe levels: Level, lzero, (lsuc l), (lmax a b) */
    if (is(t, "Level") && !bound_name("Level")) return K("clevel");
    if (is(t, "lzero") && !bound_name("lzero")) return K("clzero");
    if (t->list && t->n == 3 && is(t->it[0], "app")) {
        Node *f = t->it[1];
        if (is(f, "lsuc") && !bound_name("lsuc")) return capp1("clsuc", to_code(t->it[2]));
        if (f->list && f->n == 3 && is(f->it[0], "app") && is(f->it[1], "lmax") && !bound_name("lmax"))
            return capp2("clmax", to_code(f->it[2]), to_code(t->it[2]));
    }
    /* a data type at its parameters, a constructor at its fields and dimensions */
    {
        Node *h = t; int na = 0;
        while (h->list && h->n == 3 && is(h->it[0], "app")) { h = h->it[1]; na++; }
        if (!h->list && !bound_name(h->s)) {
            int c = con_lookup(h->s), d = data_lookup(h->s), fl = c < 0 ? fld_lookup(h->s) : -1;
            /* clocks and ticks: their own codes */
            if (d >= 0 && !strcmp(h->s, "Clock") && na == 0) return K("cclock");
            if (d >= 0 && !strcmp(h->s, "Tick") && na == 1) return capp1("ctick", to_code(t->it[2]));
            if (c >= 0 && !strcmp(DATA[CON[c].data].name, "Clock") && na == 0) return K("cclock0");
            if (c >= 0 && !strcmp(DATA[CON[c].data].name, "Tick") && na == 0) return K("ctick0");
            if (fl >= 0 && na == 0) {
                /* a field alone: the projection as a function */
                static int nfeta; char nm[32]; snprintf(nm, sizeof nm, "%%feta%d", nfeta++);
                return to_code(nl3("lam", nsym(nm), nl3("app", t, nsym(nm))));
            }
            if (fl >= 0 && na == 1) {
                int di = checked_data_of(DATA[FLD[fl].data].name, "code: codata type not checked:");
                return capp3("cproj", word(di), word(FLD[fl].index), to_code(t->it[2]));
            }
            if (c >= 0 && na < CON[c].nfields && !CON[c].ndims) {
                /* a constructor short of fields, as a function of the rest */
                static int neta;
                int rest = CON[c].nfields - na;
                Node *body = t; Node **xs = calloc((size_t)rest + 1, sizeof(Node *));
                for (int q = 0; q < rest; q++) { char nm[32]; snprintf(nm, sizeof nm, "%%eta%d", neta++); xs[q] = nsym(nm); body = nl3("app", body, xs[q]); }
                for (int q = rest - 1; q >= 0; q--) body = nl3("lam", xs[q], body);
                free(xs);
                return to_code(body);
            }
            if (c >= 0 && CON[c].nfields + CON[c].ndims == na) {
                int di = checked_data_of(DATA[CON[c].data].name, "code: data type not checked:");
                Node **a = calloc((size_t)na + 1, sizeof(Node *));
                Node *x = t;
                for (int k = na - 1; k >= 0; k--) { a[k] = to_code(x->it[2]); x = x->it[1]; }
                Node *r = capp4("ccon", word(di), word(CON[c].index), code_list(a, CON[c].nfields), code_list(a + CON[c].nfields, CON[c].ndims));
                free(a);
                return r;
            }
            if (d >= 0 && DATA[d].nparams == na && global_lookup(h->s) >= 0) {
                int di = checked_data_of(h->s, "code: data type not checked:");
                Node **a = calloc((size_t)na + 1, sizeof(Node *));
                Node *x = t;
                for (int k = na - 1; k >= 0; k--) { a[k] = to_code(x->it[2]); x = x->it[1]; }
                Node *r = capp2("cdata", word(di), code_list(a, na));
                free(a);
                return r;
            }
        }
    }
    if (!t->list) {
        const char *s = t->s;
        if (!strcmp(s, "0")) return K("ci0");
        if (!strcmp(s, "1")) return K("ci1");
        if (!strcmp(s, "tt")) return K("cftt");
        if (!strcmp(s, "ff")) return K("cfff");
        if (!strcmp(s, "U")) return capp1("cuniv", K("clzero"));
        if (!strcmp(s, "Word")) return K("cword");
        if (!strcmp(s, "Level")) return K("clevel");
        if (!strcmp(s, "lzero")) return K("clzero");
        if (s[0] == '#' && s[1] >= '0' && s[1] <= '9') return capp1("cwlit", t);   /* a word literal */
        for (int i = ncscope - 1; i >= 0; i--) if (!strcmp(cscope[i], s)) return capp1("cvar", word(ncscope - 1 - i));
        for (int i = 0; i < nchecked; i++) if (!CHKHIDE[i] && !strcmp(CHECKED[i], s)) return capp1("cglob", word(i));
        die_unchecked(s);
    }
    const char *h = t->it[0]->s;
    if (!strcmp(h, "%guess")) return to_code(t->it[3]);
    if (!strcmp(h, "%spine")) return to_code(t->it[1]);
    if (!strcmp(h, "lam")) return capp1("clam", code_bound(t->it[1]->s, t->it[2]));
    if (!strcmp(h, "plam")) return capp1("cplam", code_bound(t->it[1]->s, t->it[2]));
    if (!strcmp(h, "forall")) return capp1("cforall", code_bound(t->it[1]->s, t->it[2]));
    if (!strcmp(h, "let")) return capp2("clet", to_code(t->it[2]), code_bound(t->it[1]->s, t->it[3]));
    if (!strcmp(h, "app")) return capp2("capp", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "pair")) return capp2("cpair", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "fst")) return capp1("cfst", to_code(t->it[1]));
    if (!strcmp(h, "snd")) return capp1("csnd", to_code(t->it[1]));
    if (!strcmp(h, "Pi") || !strcmp(h, "Sigma")) {
        Node *B = t->it[2];   /* (lam x body) */
        if (!strcmp(h, "Pi")) {
            /* (cpi m A B): the binder's multiplicity first (B11) */
            char w[8]; snprintf(w, sizeof w, "#%d", bnd_mult(B->it[1]));
            return nl3("app", capp2("cpi", nsym(w), to_code(t->it[1])), code_bound(B->it[1]->s, B->it[2]));
        }
        return capp2("csigma", to_code(t->it[1]), code_bound(B->it[1]->s, B->it[2]));
    }
    if (!strcmp(h, "U")) {
        if (t->n == 2 && !t->it[1]->list && t->it[1]->s[0] >= '0' && t->it[1]->s[0] <= '9') {
            Node *l = K("clzero");
            for (int q = atoi(t->it[1]->s); q > 0; q--) l = capp1("clsuc", l);
            return capp1("cuniv", l);
        }
        return capp1("cuniv", t->n == 2 ? to_code(t->it[1]) : K("clzero"));
    }
    if (!strcmp(h, "PathP")) return capp3("cpathp", to_code(t->it[1]), to_code(t->it[2]), to_code(t->it[3]));
    if (!strcmp(h, "@")) return capp2("cpapp", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "~")) return capp1("cneg", to_code(t->it[1]));
    if (!strcmp(h, "/\\")) return capp2("cmeet", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "\\/")) return capp2("cjoin", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "=")) return capp1(is(t->it[2], "0") ? "ceq0" : "ceq1", to_code(t->it[1]));
    if (!strcmp(h, "&")) return capp2("cfand", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "|")) return capp2("cfor", to_code(t->it[1]), to_code(t->it[2]));
    /* t55 (2026-11-12): wrong. An implicit argument whose solution contains a composition with a
     * system is refused: the solution reaches to_code as the surface form (sys ...), which only ds
     * turns into sys0/sys+, so to_code dies at the head `sys`. Reproducer (after CORE and lib/std.cub):
     *   isy : (A : Type 1) {x y : A} → PathP (λ i → A) x y → PathP (λ i → A) y x
     *   isy A p = λ i → p (~ i)
     *   T1 : Type 1
     *   T1 = Type
     *   tfill : (a : T1) (x : a) → PathP (λ i → a) x (comp (λ i → a) (λ i → []) x)
     *   tfill a x = λ j → comp (λ i → a) (λ i → [ j = 0 ↦ x ]) x
     *   L1 : Type 1
     *   L1 = Bool → Type
     *   lem4 : (a : T1) (x : a) (G : a → L1) → PathP (λ i → L1) (G (comp (λ i → a) (λ i → []) x)) (G x)
     *   lem4 a x G = isy L1 (λ i → G (tfill a x i))
     * is refused: "FAIL: checks lem4 … reason: code: not a bound variable or checked definition: sys"
     * (run 2026-11-12); the same with the endpoints given, isy L1 {G x} {G (comp (λ i → a) (λ i → [])
     * x)} (…), checks. Not yet located where hole solutions are read back into syntax. Workaround:
     * endpoints given explicitly (experiments/nbe/p26-wip.cub, at pcat-i). */
    if (!strcmp(h, "sys0")) return K("csys0");
    if (!strcmp(h, "sys+")) return capp3("csysc", to_code(t->it[1]), to_code(t->it[2]->it[1]), to_code(t->it[3]));
    if (!strcmp(h, "comp")) return capp3("ccomp", to_code(t->it[1]), to_code(t->it[2]), to_code(t->it[3]));
    if (!strcmp(h, "Glue")) return capp2("cgluet", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "glue")) return capp2("cglue", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "unglue")) return capp2("cunglue", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "%hcompU")) return capp2("chcompu", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "%unglueU")) return capp2("cunglueu", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "the")) return capp2("cann", to_code(t->it[2]), to_code(t->it[1]));
    if (!strcmp(h, "%rebuilt")) return to_code(t->it[1]);
    if (!strcmp(h, "%record")) {
        int di = checked_data_of(t->it[1]->s, "code: codata type not checked:");
        Node **a = calloc((size_t)t->n, sizeof(Node *));
        for (int k = 2; k < t->n; k++) a[k - 2] = to_code(t->it[k]);
        Node *r = capp2("crec", word(di), code_list(a, t->n - 2));
        free(a);
        return r;
    }
    if (!strcmp(h, "hcompD")) return capp2("chcomp", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "force")) return capp1("cforce", to_code(t->it[1]));
    if (!strcmp(h, "%delay")) return capp1("cthunk", to_code(t->it[1]));
    /* words and levels */
    if (!strcmp(h, "%wsucc")) return capp1("cwsucc", to_code(t->it[1]));
    { static const char *WO[] = { "%weq", "%wlt", "%wadd", "%wmax", "%wsub", "%wmul", "%wdiv", "%wmod", "%wand", "%wor", "%wxor", "%wshl", "%wshr" };
      for (int q = 0; q < 13; q++) if (!strcmp(h, WO[q])) return capp3("cwop", word(q), to_code(t->it[1]), to_code(t->it[2])); }
    if (!strcmp(h, "%welimP") && t->n == 5) return capp4("cwelim", to_code(t->it[2]), to_code(t->it[3]), to_code(t->it[4]), capp1("some", to_code(t->it[1])));
    /* a primitive operation (B12): (cprim k args), k = class·256 + sub as lib/code.cub's prim-apply reads it */
    {
        static const char *PO1[] = { "%i64add", "%i64sub", "%i64mul", "%i64eq", "%i64lt", "%u32add", "%u32sub", "%u32mul", "%u32eq", "%u32lt", "%f64add", "%f64sub", "%f64mul", "%f64div", "%f64eq", "%f64lt",
            "%u64add", "%u64sub", "%u64mul", "%u64eq", "%u64lt", "%u64div", "%u64mod", "%u64and", "%u64or", "%u64xor", "%u64shl", "%u64shr",
            "%i64div", "%i64mod", "%i64and", "%i64or", "%i64xor", "%i64shl", "%i64shr",
            "%i64ofw", "%wofi64", "%u32ofw", "%wofu32", "%f64ofw", "%f64ofi64", "%i64off64", "%f64bits", "%f64ofbits", "%f32bits", "%f32ofbits", "%u64ofw", "%wofu64" };
        long k = -1;
        for (int q = 0; q < PO_N; q++) if (!strcmp(h, PO1[q])) { k = (q >= PO_I64OFW ? 1 : 2) * 256 + q; break; }
        if (k < 0 && !strncmp(h, "%load", 5) && t->n == 3) k = 3 * 256 + atoi(h + 5) / 8;
        if (k < 0 && !strncmp(h, "%store", 6) && t->n == 4) k = 4 * 256 + atoi(h + 6) / 8;
        if (k < 0 && !strcmp(h, "%blen")) k = 5 * 256;
        if (k < 0 && !strcmp(h, "%bytes")) k = 6 * 256;
        if (k >= 0) {
            Node **a = calloc((size_t)t->n, sizeof(Node *));
            for (int q = 1; q < t->n; q++) a[q - 1] = to_code(t->it[q]);
            Node *r = capp2("cprim", word(k), code_list(a, t->n - 1));
            free(a);
            return r;
        }
    }
    if (!strcmp(h, "lsuc")) return capp1("clsuc", to_code(t->it[1]));
    if (!strcmp(h, "lmax")) return capp2("clmax", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "%pty")) return capp2("cpty", to_code(t->it[1]), K("clzero"));
    if (!strcmp(h, "%bytesty")) return capp2("cpty", word(PT_BYTES), to_code(t->it[1]));
    if (!strcmp(h, "%pval")) return capp2("cplit", to_code(t->it[1]), to_code(t->it[2]));
    if (!strcmp(h, "%case")) {
        /* (%case (%branches D b...) [P] x) */
        Node *B = t->it[1];
        if (!B->list || B->n < 4 || !is(B->it[0], "%branches")) die("code: elimination without its branches", NULL);
        int d = data_lookup(B->it[1]->s), di = checked_data_of(B->it[1]->s, "code: data type not checked:");
        int N = B->n - 4, m = atoi(B->it[3]->s);
        uint64_t mask = strtoull(B->it[2]->s, NULL, 10);
        Node **bs = calloc((size_t)N + 1, sizeof(Node *));
        for (int i = 0; i < N; i++) {
            /* the shared variables a branch binds first are the variables of the same names
             * where the elimination stands */
            Node *b = B->it[i + 4];
            for (int q = 0; q < m; q++) { if (!b->list || b->n != 3 || !is(b->it[0], "lam")) die("code: malformed branch", NULL); b = b->it[2]; }
            bs[i] = code_branch(b, &CON[DATA[d].cons[i]], mask >> i & 1, 0);
        }
        Node *bl = code_list(bs, N);
        free(bs);
        if (t->n == 5) return capp4("celim", word(di), to_code(t->it[2]), bl, to_code(t->it[4]));
        return capp3("ccase", word(di), bl, to_code(t->it[3]));
    }
    die("code: no code for form", h);
    return NULL;
}

/* ---------------- annotated codes ----------------
 * The checker's input (lib/check.cub `Tm`): the code of a term whose every node carries the
 * term's value, computed by the machine from the compiled subterm, and whose binders are
 * functions: a term binder λ x xT. (code of the body), a dimension binder λ i. (code of the
 * body). Instantiating a binder is applying it; a variable's node is (tv x xT), its value and
 * type. Each node's value is built from its parts' values (let-bound, so each is computed
 * once); a binder's value is the compiled binder itself. A system's branches are thunks, so a
 * branch is computed under its face. Checked definitions are (tg n (%gval n)); indices are words. */
/* source names of binders, by index: a fresh variable or dimension the checker makes for a binder
 * carries its source name, so a failure reads in the source's names */
static char **SRCNAME; static size_t capsrcname; static int nsrcname;
static int srcname(const char *s) {
    for (int i = 0; i < nsrcname; i++) if (!strcmp(SRCNAME[i], s)) return i;
    GROW(SRCNAME, capsrcname, (size_t)nsrcname + 1);
    SRCNAME[nsrcname] = strdup(s);
    return nsrcname++;
}
const char *srcname_of(int i) { return i >= 0 && i < nsrcname ? SRCNAME[i] : "?"; }
typedef struct { Node **x, **e; int n, cap; } Chain;
static Node *kapp(const char *c, Node *a, Node *b) { Node *x = nl3("app", K(c), a); return b ? nl3("app", x, b) : x; }
typedef struct { const char *name; Node *ty; } AScope;   /* ty NULL: a dimension */
static AScope *asc; static size_t capasc; static int nasc;
static long nann;
static Node *afresh(const char *p) { char b[32]; snprintf(b, sizeof b, "%%%s%ld", p, nann++); return nsym(b); }
static Node *bind_in(Chain *c, const char *p, Node *e) {
    if (!e->list) return e;
    if (c->n == c->cap) { c->cap = c->cap ? c->cap * 2 : 16; c->x = realloc(c->x, (size_t)c->cap * sizeof(Node *)); c->e = realloc(c->e, (size_t)c->cap * sizeof(Node *)); }
    Node *x = afresh(p); c->x[c->n] = x; c->e[c->n] = e; c->n++;
    return x;
}
static Node *chain_close(Chain *c, Node *body) {
    for (int i = c->n - 1; i >= 0; i--) body = nl4("let", c->x[i], c->e[i], body);
    free(c->x); free(c->e); c->x = c->e = NULL; c->n = c->cap = 0;
    return body;
}
static AScope *ascope(const char *s) { for (int i = nasc - 1; i >= 0; i--) if (!strcmp(asc[i].name, s)) return &asc[i]; return NULL; }
static void apush(const char *s, Node *ty) { GROW(asc, capasc, (size_t)nasc + 1); asc[nasc++] = (AScope){ s, ty }; }
/* a source subterm as the expression of its value: checked definitions by their own values */
static Node *vsrc(Node *t, Names *bound) {
    if (!t->list) {
        if (names_has(bound, t->s) || ascope(t->s)) return t;
        /* a field or a constructor of the name: as the compiler reads it */
        if (fld_lookup(t->s) >= 0 || con_lookup(t->s) >= 0) return t;
        int i = checked_index(t->s);
        return i >= 0 ? nl2("%gval", word(i)) : t;
    }
    if (t->n == 0) return t;
    Node *r = nlist(t->n);
    const char *h = t->it[0]->list ? NULL : t->it[0]->s;
    if (h && (!strcmp(h, "lam") || !strcmp(h, "plam") || !strcmp(h, "forall")) && t->n == 3) {
        int save = bound->n; names_add(bound, t->it[1]->s);
        r->it[0] = t->it[0]; r->it[1] = t->it[1]; r->it[2] = vsrc(t->it[2], bound);
        bound->n = save; return r;
    }
    if (h && !strcmp(h, "let") && t->n == 4) {
        r->it[0] = t->it[0]; r->it[1] = t->it[1]; r->it[2] = vsrc(t->it[2], bound);
        int save = bound->n; names_add(bound, t->it[1]->s);
        r->it[3] = vsrc(t->it[3], bound); bound->n = save; return r;
    }
    if (h && !strcmp(h, "%branches")) {
        for (int i = 0; i < 4; i++) r->it[i] = t->it[i];
        for (int i = 4; i < t->n; i++) r->it[i] = vsrc(t->it[i], bound);
        return r;
    }
    if (h && keyword(h)) { r->it[0] = t->it[0]; for (int i = 1; i < t->n; i++) r->it[i] = (!strcmp(h, "=") && i == 2) ? t->it[i] : vsrc(t->it[i], bound); return r; }
    for (int i = 0; i < t->n; i++) r->it[i] = vsrc(t->it[i], bound);
    return r;
}
static Node *vsrc0(Node *t) { Names b = { 0 }; Node *r = vsrc(t, &b); free(b.v); return r; }
typedef struct { Node *node, *val; } NV;
static NV ann(Node *t, Chain *C);
static int srcname(const char *s);
/* a hole's value: a fresh variable of an unknown type, named ? */
static Node *hole_value(void) { char b[32]; snprintf(b, sizeof b, "#%d", srcname("?")); return nl3("%var", nsym(b), nl2("U", nsym("lomega"))); }
/* a node: its constructor applied to its parts' nodes, then its value */
static NV anode(Chain *C, const char *con, Node **parts, int n, Node *val) {
    NV r; r.val = bind_in(C, "av", val);
    Node *x = K(con);
    for (int i = 0; i < n; i++) x = nl3("app", x, parts[i]);
    r.node = bind_in(C, "an", nl3("app", x, r.val));
    return r;
}
static Node *alist(Node **xs, int n) { Node *l = K("lnil"); for (int i = n - 1; i >= 0; i--) l = kapp("lcons", xs[i], l); return l; }
/* the code of t in a scope of its own (a binder's body) */
static Node *aclosed(Node *t) { Chain c = { 0 }; NV r = ann(t, &c); return chain_close(&c, r.node); }
static Node *abinder(Node *x, Node *body, bool dim) {
    Node *ty = dim ? NULL : afresh("at");
    apush(x->s, ty);
    Node *b = aclosed(body);
    nasc--;
    return dim ? nl3("lam", x, b) : nl3("lam", x, nl3("lam", ty, b));
}
static Node *abranch(Node *b, ConInfo *c, bool ih, int i) {
    int nf = c->nfields, nr = c->nrec, ar = nf + nr + c->ndims;
    if (i == ar) {
        if (!b->list || b->n != 2 || !is(b->it[0], "thunk")) die("code: malformed branch", NULL);
        return kapp("be", aclosed(b->it[1]), NULL);
    }
    bool hyp = i >= nf && i < nf + nr, dim = i >= nf + nr;
    Node *x, *body;
    if (hyp && !ih) { x = afresh("au"); body = b; }
    else {
        if (!b->list || b->n != 3 || !is(b->it[0], "lam")) die("code: malformed branch", NULL);
        x = b->it[1]; body = b->it[2];
    }
    Node *ty = dim ? NULL : afresh("at");
    apush(x->s, ty);
    Node *r = abranch(body, c, ih, i + 1);
    nasc--;
    return kapp(dim ? "bi" : "bx", word(srcname(x->s)), dim ? nl3("lam", x, r) : nl3("lam", x, nl3("lam", ty, r)));
}
static NV aconst(const char *node, const char *val) { NV r = { K(node), !strcmp(val, "Level") || !strcmp(val, "lzero") ? K(val) : nsym(val) }; return r; }
static NV ann(Node *t, Chain *C) {
    Node *p[8];
    if (is(t, "Level") && !ascope("Level")) return aconst("tlevel", "Level");
    if (is(t, "lzero") && !ascope("lzero")) return aconst("tlzero", "lzero");
    if (t->list && t->n == 3 && is(t->it[0], "app")) {
        Node *f = t->it[1];
        if (is(f, "lsuc") && !ascope("lsuc")) { NV a = ann(t->it[2], C); p[0] = a.node; return anode(C, "tlsuc", p, 1, nl3("app", K("lsuc"), a.val)); }
        if (f->list && f->n == 3 && is(f->it[0], "app") && is(f->it[1], "lmax") && !ascope("lmax")) {
            NV a = ann(f->it[2], C), b = ann(t->it[2], C); p[0] = a.node; p[1] = b.node;
            return anode(C, "tlmax", p, 2, nl3("app", nl3("app", K("lmax"), a.val), b.val));
        }
    }
    {
        Node *h = t; int na = 0;
        while (h->list && h->n == 3 && is(h->it[0], "app")) { h = h->it[1]; na++; }
        if (!h->list && !ascope(h->s)) {
            int c = con_lookup(h->s), d = data_lookup(h->s), fl = c < 0 ? fld_lookup(h->s) : -1;
            /* clocks and ticks */
            if (d >= 0 && !strcmp(h->s, "Clock") && na == 0) return aconst("tclock", "Clock");
            if (d >= 0 && !strcmp(h->s, "Tick") && na == 1) {
                NV k = ann(t->it[2], C); p[0] = k.node;
                return anode(C, "ttick", p, 1, nl3("app", h, k.val));
            }
            if (c >= 0 && !strcmp(DATA[CON[c].data].name, "Clock") && na == 0) return aconst("tclock0", h->s);
            if (c >= 0 && !strcmp(DATA[CON[c].data].name, "Tick") && na == 0) return aconst("ttick0", h->s);   /* rejected by the checker */
            if (fl >= 0 && na == 0) {
                static int nfeta; char nm[32]; snprintf(nm, sizeof nm, "%%afeta%d", nfeta++);
                return ann(nl3("lam", nsym(nm), nl3("app", t, nsym(nm))), C);
            }
            if (fl >= 0 && na == 1) {
                /* a projection: (tproj d k x V) */
                int di = checked_data_of(DATA[FLD[fl].data].name, "code: codata type not checked:");
                NV a = ann(t->it[2], C);
                p[0] = word(di); p[1] = word(FLD[fl].index); p[2] = a.node;
                return anode(C, "tproj", p, 3, nl3("app", h, a.val));
            }
            if (c >= 0 && na < CON[c].nfields && !CON[c].ndims) {
                static int neta;
                int rest = CON[c].nfields - na;
                Node *body = t; Node **xs = calloc((size_t)rest + 1, sizeof(Node *));
                for (int q = 0; q < rest; q++) { char nm[32]; snprintf(nm, sizeof nm, "%%aeta%d", neta++); xs[q] = nsym(nm); body = nl3("app", body, xs[q]); }
                for (int q = rest - 1; q >= 0; q--) body = nl3("lam", xs[q], body);
                free(xs);
                return ann(body, C);
            }
            bool con = c >= 0 && CON[c].nfields + CON[c].ndims == na;
            if (con || (d >= 0 && DATA[d].nparams == na && global_lookup(h->s) >= 0)) {
                int di = checked_data_of(con ? DATA[CON[c].data].name : h->s, "code: data type not checked:");
                Node **a = calloc((size_t)na + 1, sizeof(Node *)), **v = calloc((size_t)na + 1, sizeof(Node *));
                Node *x = t;
                for (int k = na - 1; k >= 0; k--) { NV r = ann(x->it[2], C); a[k] = r.node; v[k] = r.val; x = x->it[1]; }
                Node *val = h;
                for (int k = 0; k < na; k++) val = nl3("app", val, v[k]);
                NV r;
                if (con) { p[0] = word(di); p[1] = word(CON[c].index); p[2] = alist(a, CON[c].nfields); p[3] = alist(a + CON[c].nfields, CON[c].ndims); r = anode(C, "tcon", p, 4, val); }
                else { p[0] = word(di); p[1] = alist(a, na); r = anode(C, "tdata", p, 2, val); }
                free(a); free(v);
                return r;
            }
        }
    }
    if (!t->list) {
        const char *s = t->s;
        if (!strcmp(s, "0")) return aconst("tc0", "0");
        if (!strcmp(s, "1")) return aconst("tc1", "1");
        if (!strcmp(s, "tt")) return aconst("ttt", "tt");
        if (!strcmp(s, "ff")) return aconst("tff", "ff");
        if (!strcmp(s, "U")) { p[0] = K("tlzero"); return anode(C, "tuniv", p, 1, nsym("U")); }
        if (!strcmp(s, "Word")) return aconst("tword", "Word");
        if (s[0] == '#' && s[1] >= '0' && s[1] <= '9') { NV r; r.val = t; r.node = kapp("tlit", t, NULL); return r; }
        AScope *a = ascope(s);
        if (a) { NV r; r.val = t; r.node = a->ty ? kapp("tv", t, a->ty) : kapp("ti", t, NULL); return r; }
        int i = checked_index(s);
        if (i >= 0) { p[0] = word(i); return anode(C, "tg", p, 1, nl2("%gval", word(i))); }
        die_unchecked(s);
    }
    const char *h = t->it[0]->s;
    if (!strcmp(h, "lam") || !strcmp(h, "plam") || !strcmp(h, "forall")) {
        p[0] = word(srcname(t->it[1]->s));
        p[1] = abinder(t->it[1], t->it[2], strcmp(h, "lam") != 0);
        return anode(C, !strcmp(h, "lam") ? "tlam" : !strcmp(h, "plam") ? "tplam" : "tforall", p, 2, vsrc0(t));
    }
    if (!strcmp(h, "let")) {
        NV a = ann(t->it[2], C);
        p[0] = a.node; p[1] = word(srcname(t->it[1]->s)); p[2] = abinder(t->it[1], t->it[3], false);
        Names b = { 0 }; names_add(&b, t->it[1]->s);
        Node *val = nl4("let", t->it[1], a.val, vsrc(t->it[3], &b)); free(b.v);
        return anode(C, "tlet", p, 3, val);
    }
    if (!strcmp(h, "Pi") || !strcmp(h, "Sigma")) {
        Node *B = t->it[2];
        NV a = ann(t->it[1], C);
        p[0] = a.node; p[1] = word(srcname(B->it[1]->s)); p[2] = abinder(B->it[1], B->it[2], false);
        return anode(C, !strcmp(h, "Pi") ? "tpi" : "tsigma", p, 3, nl3(h, a.val, vsrc0(B)));
    }
    if (!strcmp(h, "U")) {
        NV l;
        if (t->n == 2 && !t->it[1]->list && t->it[1]->s[0] >= '0' && t->it[1]->s[0] <= '9') {
            l = aconst("tlzero", "lzero");
            for (int q = atoi(t->it[1]->s); q > 0; q--) { p[0] = l.node; l = anode(C, "tlsuc", p, 1, nl3("app", K("lsuc"), l.val)); }
        } else if (t->n == 2) l = ann(t->it[1], C);
        else l = aconst("tlzero", "lzero");
        p[0] = l.node;
        return anode(C, "tuniv", p, 1, nl2("U", l.val));
    }
    if (!strcmp(h, "sys0") || !strcmp(h, "sys+")) {
        Node *bs[256]; int nb = 0;
        for (Node *x = t; x->list && is(x->it[0], "sys+"); x = x->it[3]) {
            if (nb == 256) die("code: more than 256 system branches", NULL);
            NV f = ann(x->it[1], C);
            bs[nb++] = nl3("pair", f.node, nl2("thunk", aclosed(x->it[2]->it[1])));
        }
        p[0] = alist(bs, nb);
        return anode(C, "tsys", p, 1, vsrc0(t));
    }
    if (!strcmp(h, "%case")) {
        Node *B = t->it[1];
        if (!B->list || B->n < 4 || !is(B->it[0], "%branches")) die("code: elimination without its branches", NULL);
        int d = data_lookup(B->it[1]->s), di = checked_data_of(B->it[1]->s, "code: data type not checked:");
        int N = B->n - 4, m = atoi(B->it[3]->s);
        uint64_t mask = strtoull(B->it[2]->s, NULL, 10);
        bool el = t->n == 5;
        NV P = { 0 };
        if (el) P = ann(t->it[2], C);
        NV x = ann(t->it[el ? 4 : 3], C);
        Node **bs = calloc((size_t)N + 1, sizeof(Node *));
        for (int i = 0; i < N; i++) {
            Node *b = B->it[i + 4];
            for (int q = 0; q < m; q++) { if (!b->list || b->n != 3 || !is(b->it[0], "lam")) die("code: malformed branch", NULL); b = b->it[2]; }
            bs[i] = abranch(b, &CON[DATA[d].cons[i]], mask >> i & 1, 0);
        }
        /* the elimination as a function of its scrutinee */
        Node *sv = afresh("as");
        Node *cs = nlist(t->n); cs->it[0] = t->it[0]; cs->it[1] = vsrc0(B);
        if (el) { cs->it[2] = P.val; cs->it[3] = vsrc0(t->it[3]); cs->it[4] = sv; }
        else { cs->it[2] = vsrc0(t->it[2]); cs->it[3] = sv; }
        Node *F = bind_in(C, "af", nl3("lam", sv, cs));
        int k = 0;
        p[k++] = word(di);
        if (el) p[k++] = P.node;
        p[k++] = alist(bs, N); p[k++] = x.node; p[k++] = F;
        free(bs);
        return anode(C, el ? "telim" : "tcase", p, k, nl3("app", F, x.val));
    }
    static const struct { const char *h, *con; int n; } F[] = {
        { "app", "tapp", 2 }, { "pair", "tpair", 2 }, { "fst", "tfst", 1 }, { "snd", "tsnd", 1 },
        { "PathP", "tpathp", 3 }, { "@", "tpapp", 2 }, { "~", "tneg", 1 }, { "/\\", "tmeet", 2 }, { "\\/", "tjoin", 2 },
        { "&", "tfand", 2 }, { "|", "tfor", 2 }, { "comp", "tcomp", 3 }, { "Glue", "tgluet", 2 }, { "glue", "tglue", 2 },
        { "unglue", "tunglue", 2 }, { "%hcompU", "thcompu", 2 }, { "%unglueU", "tunglueu", 2 }, { "hcompD", "thcomp", 2 }, { "force", "tforce", 1 }, { "%delay", "tthunk", 1 }, { NULL, NULL, 0 } };
    {
        static const char *WO[] = { "%weq", "%wlt", "%wadd", "%wmax", "%wsub", "%wmul", "%wdiv", "%wmod", "%wand", "%wor", "%wxor", "%wshl", "%wshr", NULL };
        for (int q = 0; WO[q]; q++) if (!strcmp(h, WO[q])) {
            NV a = ann(t->it[1], C), b = ann(t->it[2], C);
            p[0] = word(q); p[1] = a.node; p[2] = b.node;
            Node *v = nl3(h, a.val, b.val);
            return anode(C, "twop", p, 3, v);
        }
        if (!strcmp(h, "%wsucc")) { NV a = ann(t->it[1], C); p[0] = a.node; return anode(C, "twsucc", p, 1, nl2(h, a.val)); }
        if (!strcmp(h, "%welimP")) {
            Node *v = nlist(5); v->it[0] = t->it[0];
            for (int i = 0; i < 4; i++) { NV r = ann(t->it[i + 1], C); p[i] = r.node; v->it[i + 1] = r.val; }
            return anode(C, "twelim", p, 4, v);
        }
    }
    if (!strcmp(h, "=")) {
        NV r = ann(t->it[1], C); p[0] = r.node;
        return anode(C, is(t->it[2], "0") ? "teq0" : "teq1", p, 1, nl3("=", r.val, t->it[2]));
    }
    if (!strcmp(h, "the")) {
        NV T = ann(t->it[1], C), a = ann(t->it[2], C); p[0] = a.node; p[1] = T.node;
        return anode(C, "tann", p, 2, a.val);
    }
    if (!strcmp(h, "%rebuilt")) return ann(t->it[1], C);
    if (!strcmp(h, "%hole")) {
        /* (thole k V): V a fresh variable standing for the argument to be found */
        p[0] = nsym(t->it[1]->s[0] == '#' ? t->it[1]->s : "#0");
        return anode(C, "thole", p, 1, hole_value());
    }
    if (!strcmp(h, "%guess")) { NV r = ann(t->it[3], C); p[0] = t->it[1]; p[1] = t->it[2]; p[2] = r.node; return anode(C, "tguess", p, 3, r.val); }
    if (!strcmp(h, "%spine")) { NV r = ann(t->it[1], C); p[0] = r.node; return anode(C, "tspine", p, 1, r.val); }
    if (!strcmp(h, "%record")) {
        /* a copattern: (trec d fields V) */
        int di = checked_data_of(t->it[1]->s, "code: codata type not checked:");
        int nf = t->n - 2;
        Node **a = calloc((size_t)nf + 1, sizeof(Node *));
        Node *v = nlist(t->n); v->it[0] = t->it[0]; v->it[1] = t->it[1];
        for (int k = 0; k < nf; k++) { NV r = ann(t->it[k + 2], C); a[k] = r.node; v->it[k + 2] = r.val; }
        p[0] = word(di); p[1] = alist(a, nf);
        free(a);
        return anode(C, "trec", p, 2, v);
    }
    for (int q = 0; F[q].h; q++) if (!strcmp(h, F[q].h)) {
        Node *v = nlist(F[q].n + 1); v->it[0] = t->it[0];
        for (int i = 0; i < F[q].n; i++) { NV r = ann(t->it[i + 1], C); p[i] = r.node; v->it[i + 1] = r.val; }
        return anode(C, F[q].con, p, F[q].n, v);
    }
    die("code: no code for form", h);
    return (NV){ 0 };
}
/* a data declaration as a telescope (lib/check.cub `Tel`): its parameters, then the list of its
 * constructors, each its fields, its dimensions, then its boundary */
static Node *atel_ctor(Node *cd, ConInfo *c, int i, int k) {
    if (k < c->nfields) {
        Node *fl = cd->it[i];
        if (!fl->list) die("a checked data type needs its fields' types:", c->name);
        Chain C = { 0 }; NV A = ann(ds(fl->it[1]), &C);
        Node *ty = afresh("at");
        apush(fl->it[0]->s, ty);
        Node *rest = atel_ctor(cd, c, i + 1, k + 1);
        nasc--;
        return chain_close(&C, nl3("app", kapp("tx", A.node, word(srcname(fl->it[0]->s))), nl3("lam", fl->it[0], nl3("lam", ty, rest))));
    }
    if (!c->ndims) { Node *s0 = nlist(1); s0->it[0] = nsym("sys"); return kapp("tz", aclosed(ds(s0)), NULL); }
    Node *dl = cd->it[i++];
    int base = nasc;
    for (int q = 1; q < dl->n; q++) apush(dl->it[q]->s, NULL);
    Node *sy = nlist(cd->n - i + 1); sy->it[0] = nsym("sys");
    for (int q = i; q < cd->n; q++) sy->it[q - i + 1] = cd->it[q];
    Node *r = kapp("tz", aclosed(ds(sy)), NULL);
    nasc = base;
    for (int q = dl->n - 1; q >= 1; q--) r = kapp("ty", word(srcname(dl->it[q]->s)), nl3("lam", dl->it[q], r));
    return r;
}
static Node *atel_params(Node *dd, int d, int k) {
    Node *hd = dd->it[1];
    if (k < DATA[d].nparams) {
        Node *p = hd->it[k + 1];
        if (!p->list || p->n != 2) die("a checked data type needs its parameters' types:", DATA[d].name);
        Chain C = { 0 }; NV A = ann(ds(p->it[1]), &C);
        Node *ty = afresh("at");
        apush(p->it[0]->s, ty);
        Node *rest = atel_params(dd, d, k + 1);
        nasc--;
        return chain_close(&C, nl3("app", kapp("tx", A.node, word(srcname(p->it[0]->s))), nl3("lam", p->it[0], nl3("lam", ty, rest))));
    }
    Node **cs = calloc((size_t)DATA[d].ncons + 1, sizeof(Node *));
    for (int ci = 0; ci < DATA[d].ncons; ci++) cs[ci] = atel_ctor(dd->it[ci + 2], &CON[DATA[d].cons[ci]], 1, 0);
    Node *r = kapp("tz", alist(cs, DATA[d].ncons), NULL);
    free(cs);
    return r;
}

/* occurrences of a parameter in a primitive's body (B11): c counts them, direct those that are an
 * argument of a machine operation (a %-headed application); an inner binder of the same name hides it */
static void prim_uses(Node *t, const char *x, int under, int *c, int *direct) {
    if (!t) return;
    if (!t->list) { if (t->s == x || !strcmp(t->s, x)) { (*c)++; if (under) (*direct)++; } return; }
    if (t->n == 3 && (is(t->it[0], "lam") || is(t->it[0], "plam")) && !t->it[1]->list && !strcmp(t->it[1]->s, x)) return;
    int head = t->n > 0 && !t->it[0]->list && t->it[0]->s[0] == '%';
    for (int i = 0; i < t->n; i++) prim_uses(t->it[i], x, head && i > 0, c, direct);
}
static void toplevel(Node *f);
static void x_rec_extra(Node *f);   /* syntax.c: a form added to a module's record */
static Node *cn(Node *c); static int ncn;   /* syntax.c: a code as a surface term */
static size_t name_hash(const char *s);
/* ---------------- definitions ---------------- */
/* G_COFIX: a copattern definition, whose value is its stuck head; unf the definition holding its body */
enum { G_DEF, G_AXIOM, G_INPUT, G_COFIX };
typedef struct {
    char *name; int kind;
    Syn *body; int nslots; int *uses;
    Record *nary;                    /* the body taking all its parameters at once, or NULL */
    int nparams; uint64_t strict;    /* its λ-chain's parameters, and those it always reads */
    Cell *res;                       /* holds the definition's value; a use copies it */
    long tyg;                        /* an axiom's type: the global holding it, or -1 */
    Cell **park_o, **park_v; int npark, parkcap;
    long unf;                        /* a copattern definition's body (a global), or -1 */
    int chk;                         /* its index in the checker's signature, or -1 */
    int tickar;                      /* a stuck call unfolding when its last parameter is ⋄: its parameters, else 0 */
    int file;                        /* the serial of the file defining it (item 17's keys) */
} Global;
static Global *G; static long nG, capG;
/* A8: which compiled syntax may make a free dimension. Only %dim (O_MKDIM) gives a value a free
 * dimension none of its inputs has (every other fresh dimension is bound where it is made), and
 * the checker's tables (O_GLOB) hand out arbitrary values; so building a record, or unfolding a
 * global, can mention only its inputs' dimensions unless its syntax reaches one of those, directly
 * or through the globals it calls. Globals by strongly connected components (a cycle is tainted
 * when one member is), cached once every callee is defined; records by their syntax. */
static signed char *GMK; static long capGMK;   /* 0 unknown, 1 clean, 2 tainted */
static bool syn_walk(const Syn *t, long *calls, int *nc, int cap) {
    if (!t) return false;
    if (t->tag == S_PRIM && t->x == O_MKDIM) return true;
    if (t->tag == S_GLOBTAB) return true;
    if ((t->tag == S_GLOBAL || t->tag == S_CALL) && *nc < cap) calls[(*nc)++] = t->id;
    else if ((t->tag == S_GLOBAL || t->tag == S_CALL)) return true;   /* too many to follow: tainted */
    if (t->rec && syn_walk(t->rec->body, calls, nc, cap)) return true;
    if (t->tab) {
        int N = t->tab->d >= 0 && t->tab->d < ndata ? DATA[t->tab->d].ncons : 0;
        for (int i = 0; i < N; i++) if (t->tab->br && t->tab->br[i] && syn_walk(t->tab->br[i]->body, calls, nc, cap)) return true;
        if (t->tab->motive && syn_walk(t->tab->motive->body, calls, nc, cap)) return true;
    }
    for (int i = 0; i < t->n; i++) if (t->k && syn_walk(t->k[i], calls, nc, cap)) return true;
    return false;
}
/* Tarjan over globals */
static long *TJ_idx, *TJ_low, *TJ_stk; static signed char *TJ_on; static long tj_cap, tj_sp, tj_counter;
static void tj_grow(long g) {
    if (g < tj_cap) return;
    long nc = tj_cap ? tj_cap : 1024; while (nc <= g) nc *= 2;
    TJ_idx = realloc(TJ_idx, sizeof(long) * nc); TJ_low = realloc(TJ_low, sizeof(long) * nc); TJ_stk = realloc(TJ_stk, sizeof(long) * nc); TJ_on = realloc(TJ_on, nc);
    for (long i = tj_cap; i < nc; i++) { TJ_idx[i] = -1; TJ_on[i] = 0; }
    tj_cap = nc;
}
static bool tj_taint;   /* a component's members found tainted (or a callee undefined) */
static void tj_visit(long g) {
    tj_grow(g);
    TJ_idx[g] = TJ_low[g] = tj_counter++; TJ_stk[tj_sp++] = g; TJ_on[g] = 1;
    bool direct = false;
    long *calls = NULL; int nc = 0;
    if (g < 0 || g >= nG) direct = true;
    else if (!G[g].body) direct = G[g].kind != G_AXIOM && G[g].kind != G_INPUT;   /* not yet defined: not known clean */
    else {
        calls = malloc(sizeof(long) * 4096);
        direct = syn_walk(G[g].body, calls, &nc, 4096) || (G[g].nary && syn_walk(G[g].nary->body, calls, &nc, 4096));
        if (G[g].kind == G_COFIX && G[g].unf >= 0 && nc < 4096) calls[nc++] = G[g].unf;
    }
    bool t = direct;
    for (int i = 0; i < nc && !t; i++) {
        long h = calls[i];
        if (h < 0 || h >= nG) { t = true; break; }
        if (h < capGMK && GMK[h]) { if (GMK[h] == 2) t = true; continue; }
        tj_grow(h);
        if (TJ_idx[h] < 0) { tj_visit(h); if (TJ_low[h] < TJ_low[g]) TJ_low[g] = TJ_low[h]; if (h < capGMK && GMK[h] == 2) t = true; }
        else if (TJ_on[h]) { if (TJ_idx[h] < TJ_low[g]) TJ_low[g] = TJ_idx[h]; }
    }
    free(calls);
    if (t) tj_taint = true;
    if (TJ_low[g] == TJ_idx[g]) {
        /* a component: all of it tainted when any member is (tj_taint over the component) */
        long base = tj_sp; while (TJ_stk[base - 1] != g) base--; base--;
        signed char v = tj_taint ? 2 : 1;
        bool undefined = false;
        for (long i = base; i < tj_sp; i++) { long m = TJ_stk[i]; if (m < nG && !G[m].body && G[m].kind != G_AXIOM && G[m].kind != G_INPUT) undefined = true; }
        for (long i = base; i < tj_sp; i++) {
            long m = TJ_stk[i]; TJ_on[m] = 0; TJ_idx[m] = -1;
            if (m >= capGMK) { long nc2 = capGMK ? capGMK : 1024; while (nc2 <= m) nc2 *= 2; GMK = realloc(GMK, (size_t)nc2); memset(GMK + capGMK, 0, (size_t)(nc2 - capGMK)); capGMK = nc2; }
            /* tainted is kept; clean only when nothing in it is still undefined */
            GMK[m] = undefined ? (v == 2 ? 2 : 0) : v;
        }
        tj_sp = base; tj_taint = false;
    }
}
int global_mkdim(long g) {
    if (g < 0 || g >= nG) return 1;
    if (g < capGMK && GMK[g]) return GMK[g] == 2;
    tj_sp = 0; tj_taint = false;
    tj_visit(g);
    return g < capGMK && GMK[g] == 1 ? 0 : 1;
}
int record_mkdim(Record *r) {
    if (!r) return 1;
    int m = __atomic_load_n(&r->mk, __ATOMIC_RELAXED);
    if (m) return m == 2;
    long calls[4096]; int nc = 0;
    bool t = syn_walk(r->body, calls, &nc, 4096);
    for (int i = 0; i < nc && !t; i++) if (global_mkdim(calls[i])) t = true;
    /* clean is cached only when every callee was decided clean (not merely undefined) */
    bool all = true; for (int i = 0; i < nc; i++) if (!(calls[i] < capGMK && GMK[calls[i]] == 1)) all = false;
    if (t) __atomic_store_n(&r->mk, 2, __ATOMIC_RELAXED); else if (all) __atomic_store_n(&r->mk, 1, __ATOMIC_RELAXED);
    return t;
}
int casetab_mkdim(const CaseTab *tb) {
    if (!tb) return 1;
    int N = tb->d >= 0 && tb->d < ndata ? DATA[tb->d].ncons : 0;
    for (int i = 0; i < N; i++) if (!tb->br || !tb->br[i] || record_mkdim(tb->br[i])) return 1;
    if (tb->motive && record_mkdim(tb->motive)) return 1;
    return 0;
}
static char **fresh_names; static long nfresh, capfresh;
#define FRESH_BASE (1L << 40)

/* names to the latest global of that name: open addressing, doubled when half full */
static long *GH; static size_t capGH;
static size_t name_hash(const char *s) { size_t h = 1469598103934665603ull; for (; *s; s++) h = (h ^ (unsigned char)*s) * 1099511628211ull; return h; }
static void gh_put(const char *name, long g) {
    if ((size_t)nG * 2 >= capGH) {
        size_t nc = capGH ? capGH * 2 : 1024;
        long *nh = malloc(nc * sizeof(long));
        for (size_t i = 0; i < nc; i++) nh[i] = -1;
        for (size_t i = 0; i < capGH; i++) if (GH[i] >= 0) {
            size_t j = name_hash(G[GH[i]].name) & (nc - 1);
            while (nh[j] >= 0) j = (j + 1) & (nc - 1);
            nh[j] = GH[i];
        }
        free(GH); GH = nh; capGH = nc;
    }
    size_t j = name_hash(name) & (capGH - 1);
    while (GH[j] >= 0 && strcmp(G[GH[j]].name, name)) j = (j + 1) & (capGH - 1);
    GH[j] = g;
}
long global_lookup(const char *name) {
    if (name[0] == '%' && name[1] == '%') { for (long g = 0; g < nG; g++) if (!strcmp(G[g].name, name + 2)) return g; return -1; }
    if (!capGH) return -1;
    size_t j = name_hash(name) & (capGH - 1);
    while (GH[j] >= 0) { if (!strcmp(G[GH[j]].name, name)) return GH[j]; j = (j + 1) & (capGH - 1); }
    return -1;
}
const char *name_of(long id) {
    if ((uint64_t)id & LVL_TAG) { static __thread char b[8][32]; static __thread int bi; char *r = b[bi++ & 7]; snprintf(r, 32, "v%llu", (unsigned long long)((uint64_t)id & ~LVL_TAG)); return r; }
    return id >= FRESH_BASE ? fresh_names[id - FRESH_BASE] : G[id].name;
}
static long fresh_var(const char *pfx, long k) {
    if (nfresh == capfresh) { capfresh = capfresh ? capfresh * 2 : 64; fresh_names = realloc(fresh_names, (size_t)capfresh * sizeof(char *)); }
    char b[32]; snprintf(b, sizeof b, "%s%ld", pfx, k);
    fresh_names[nfresh] = strdup(b);
    return FRESH_BASE + nfresh++;
}
static int cur_file, file_serial;   /* the file being loaded (its serial), for item 17's keys */
static long add_global(const char *name, int kind) {
    if (nG == capG) { capG = capG ? capG * 2 : 64; G = realloc(G, (size_t)capG * sizeof(Global)); }
    memset(&G[nG], 0, sizeof(Global));
    G[nG].name = strdup(name); G[nG].kind = kind; G[nG].tyg = -1; G[nG].unf = -1; G[nG].chk = -1; G[nG].tickar = 0; G[nG].file = cur_file;
    G[nG].res = cell(O_ROOT, 1);
    nG++;
    gh_put(name, nG - 1);
    return nG - 1;
}
/* a name the source binds is bound once (E28: a second binding would silently rebind the references
 * of everything after it). x_surface: the form being run came from the source (x_top), not from the
 * system (emit: a data type's generated definitions, #jit's rebinding of f to its bytes). A name
 * #retract hid may be bound again */
static bool x_surface;
static int chk_index(int d);
static void decl_fresh(const char *name) {
    if (!x_surface || name[0] == '%') return;
    long g = global_lookup(name);
    bool bound = (g >= 0 && !(G[g].kind == G_INPUT && !G[g].res->port[0])) || data_lookup(name) >= 0 || con_lookup(name) >= 0;
    /* a name whose every checked entry #retract hid may be bound again (checked_index skips hidden
     * entries, so it is -1 exactly then) */
    if (bound && checked_index(name) < 0) for (int i = 0; i < nchecked; i++) if (CHKHIDE[i] && !strcmp(CHECKED[i], name)) return;
    if (bound) die("declared again (a name is bound once):", name);
}
Port global_copy(long g, Key *as) {
    Cell *r = G[g].res;
    if (!r->port[0]) return 0;
    /* a definition whose value is a closed λ or a constant constructor is built from its
     * syntax: unfolding it is the one interaction that meets it */
    Syn *b = G[g].kind == G_DEF ? G[g].body : NULL;
    if (b && b->tag == S_LAM && b->n == 0) { Cell *c = cell(V_LAM, 1); c->p = b->rec; c->key = as ? key_retain(as) : key_global(g); return P(c, 0); }
    if (b && b->tag == S_CON && b->n == 0) { Port c = con_build((int)b->id, NULL, NULL); if (PI(c) == 0) { if (PC(c)->key) key_release(PC(c)->key); PC(c)->key = as ? key_retain(as) : key_con0((int)b->id); } return c; }
    if (b && b->tag == S_DATA && b->n == 0) { Cell *c = cell(V_DATA, 1); c->a = (uint64_t)b->id; return P(c, 0); }
    extern Port lazy_copy(Key *as, Cell *root);
    Key *k = as ? key_retain(as) : key_global(g);   /* a copy of the definition's value is named by the definition, or by what referred to it */
    Port p = lazy_copy(k, r);
    key_release(k);
    return p;
}
Cell *global_res(long g) { return G[g].res; }
/* a constant with no computation (a primitive without one): its type's global, else -2 */
long global_axiom_ty(long g) { return g >= 0 && g < nG && G[g].kind == G_AXIOM ? G[g].tyg : -2; }
static pthread_mutex_t park_mu = PTHREAD_MUTEX_INITIALIZER;
void global_park(long g, Cell *op, Cell *glob) {
    pthread_mutex_lock(&park_mu);
    Global *x = &G[g];
    if (x->npark == x->parkcap) {
        x->parkcap = x->parkcap ? x->parkcap * 2 : 8;
        x->park_o = realloc(x->park_o, (size_t)x->parkcap * sizeof(Cell *));
        x->park_v = realloc(x->park_v, (size_t)x->parkcap * sizeof(Cell *));
    }
    x->park_o[x->npark] = op; x->park_v[x->npark] = glob; x->npark++;
    pthread_mutex_unlock(&park_mu);
}

long global_unfold(long g) { return G[g].unf; }
int global_tickar(long g) { return G[g].tickar; }
int global_checked(long g) { return g >= 0 && g < nG ? G[g].chk : -1; }
uint64_t global_strict(long g) { return g >= 0 && g < nG && G[g].kind == G_DEF ? G[g].strict : 0; }   /* the parameters g always reads (call by need: built strict) */
Record *global_nary(long g) { return g >= 0 && g < nG && G[g].kind == G_DEF ? G[g].nary : NULL; }
/* a copy of the value held by an observation cell */
static Port held_copy(Cell *r) {
    if (!r->port[0]) return 0;
    Cell *d = cell(O_DUP, 3);
    link(r->port[0], P(d, 0)); link(P(d, 1), P(r, 0));
    return P(d, 2);
}

/* ---------------- compiling: binder levels and lifting ---------------- */
typedef struct { const char *name; int level, slot; size_t h; } Bind;
typedef struct {
    int is_thunk, scope_start, nslots, ucap;
    int *uses;
    int nenv, ecap;
    Node **env_nodes; int *env_slots; Syn **env_syns;
} Level;
static Bind *binds; static size_t capbinds; static int nbinds;
static Level *levels; static size_t caplevels; static int nlevels;

static long stat_syn_bytes, stat_perm_bytes, stat_rec_bytes;   /* CUBICAL_MEMDEF: bytes of compiled code made, of permanent node copies, of records */
static Syn *syn(int tag, int n) { Syn *s = calloc(1, sizeof(Syn)); stat_syn_bytes += (long)(sizeof(Syn) + ((size_t)n + 1) * sizeof(Syn *)); s->tag = tag; s->n = n; s->k = calloc((size_t)n + 1, sizeof(Syn *)); return s; }
static Bind *lookup(const char *name) { size_t h = (unsigned)kw_hash(name) | 1; for (int i = nbinds - 1; i >= 0; i--) if (binds[i].h == h && (binds[i].name == name || !strcmp(binds[i].name, name))) return &binds[i]; return NULL; }
/* the same for an interned name: its hash from its header */
/* the same for an interned name: the binder its header remembers when that is still the latest
 * (a binder pushed later updates it, binders are only ever popped from the top), else found and
 * remembered */
static Bind *lookup_i(const char *name) {
    if (sym_bind_epoch(name) == bind_epoch) {
        /* remembered in this epoch: binders were only pushed since, a push of this name updating it */
        unsigned c = sym_bind(name);
        if (getenv("CUB_CHKBIND")) { Bind *b = lookup(name); if (b != (c ? &binds[c - 1] : NULL)) { fprintf(stderr, "bind cache wrong for %s: cached %u (nbinds %d), scan %ld\n", name, c, nbinds, b ? b - binds : -1L); abort(); } }
        return c ? &binds[c - 1] : NULL;
    }
    size_t h = sym_hash(name);
    for (int i = nbinds - 1; i >= 0; i--) if (binds[i].h == h && (binds[i].name == name || !strcmp(binds[i].name, name))) { sym_bind_set(name, (unsigned)i + 1); return &binds[i]; }
    sym_bind_set(name, 0);
    return NULL;
}
static bool constant_sym(const char *s);
static void push_bind(const char *name, int level, int slot) {
    if (constant_sym(name)) die("a constant cannot be bound:", name);
    if (keyword(name)) die("a keyword cannot be bound:", name);
    name = intern(name);   /* (its header remembers this binder) */
    GROW(binds, capbinds, (size_t)nbinds + 1); binds[nbinds++] = (Bind){ name, level, slot, (unsigned)kw_hash(name) | 1 };
    sym_bind_set(name, (unsigned)nbinds); }
static int new_slot(void) {
    Level *L = &levels[nlevels - 1];
    if (L->nslots == L->ucap) { L->ucap = L->ucap ? L->ucap * 2 : 8; L->uses = realloc(L->uses, (size_t)L->ucap * sizeof(int)); }
    L->uses[L->nslots] = 0;
    return L->nslots++;
}
static Syn *var_syn(int slot) { levels[nlevels - 1].uses[slot]++; Syn *s = syn(S_VAR, 0); s->slot = slot; return s; }
static bool constant_sym(const char *s) {
    switch (s[0]) {
    case '0': case '1': case 'U': return s[1] == 0;
    case 't': return s[1] == 't' && s[2] == 0;
    case 'f': return s[1] == 'f' && s[2] == 0;
    case 'W': return !strcmp(s, "Word");
    default: return false;
    }
}

/* the outermost binder level a term refers to (-1: none) */
/* (only whether it is below the current level matters: the walk stops at the first variable of
 * the current level or deeper) */
static int ml_cut = 1 << 30;
/* the names a term may refer to through the binders in force (every symbol but the constants,
 * those bound inside left out), found once per node (lv): maxlev is asked at every node compiled,
 * so walking the subterm each time was quadratic in the term's depth (A9: 24% of a Brunerie
 * run). A fresh variable, dimension or hole form anywhere inside is the mark %fresh. */
static Names *lvs(Node *t) {
    if (t->lv && t->lvgen == ar_gen) return t->lv;
    Names *r = ar_alloc(sizeof(Names)); t->lvgen = ar_gen;
    if (!t->list) {
        if (!constant_sym(t->s)) { r->v = ar_alloc(sizeof(char *)); r->v[0] = t->s; r->n = r->cap = 1; }
        return t->lv = r;
    }
    int hk = t->n ? hk_of(t->it[0]) : H_NONE;
    if (hk == H_VAR || hk == H_DIM || hk == H_HOLE) {
        r->v = ar_alloc(sizeof(char *)); r->v[0] = intern("%fresh"); r->n = r->cap = 1;
        return t->lv = r;
    }
    const char *skip = NULL, *skip_body = NULL; int from = 1, to = t->n;
    if (hk == H_LAM || hk == H_PLAM || hk == H_FORALL) { skip = t->it[1]->s; from = 2; }
    else if (hk == H_LET) { skip_body = t->it[1]->s; from = 2; }
    int cnt = 0;
    for (int i = from; i < to; i++) cnt += lvs(t->it[i])->n;
    const char **v = cnt ? ar_alloc((size_t)cnt * sizeof(char *)) : NULL; int n = 0;
    if (cnt) {
        unsigned e = ++mark_epoch;
        for (int i = from; i < to; i++) {
            const Names *f = lvs(t->it[i]);
            bool body = skip_body && i == 3;
            for (int k = 0; k < f->n; k++) {
                const char *x = f->v[k];
                if (x == skip || (body && x == skip_body)) continue;
                if (sym_mark(x) == e) continue;
                sym_mark_set(x, e); v[n++] = x;
            }
        }
    }
    r->v = v; r->n = r->cap = n;
    return t->lv = r;
}
static int maxlev(Node *t) {
    const Names *f = lvs(t); int r = -1;
    static const char *fresh; if (!fresh) fresh = intern("%fresh");
    for (int k = 0; k < f->n; k++) {
        const char *x = f->v[k];
        /* a fresh variable or dimension is made each time its level runs: never lifted */
        if (x == fresh) return 1 << 30;
        Bind *b = lookup_i(x);
        if (b && b->level > r) { r = b->level; if (r >= ml_cut) return r; }
    }
    return r;
}

static Syn *compile(Node *t);
static Syn *compile_lazy(Node *t);
/* a maximal subterm free of the current level becomes a port of its cell,
 * computed once in the enclosing level */
static int lift(Node *t) {
    int li = nlevels - 1;
    Level *L = &levels[li];
    for (int k = 0; k < L->nenv; k++) if (node_eq(L->env_nodes[k], t)) { return L->env_slots[k]; }
    int sb = L->scope_start, nb = nbinds, nsaved = nb - sb;
    Bind *saved = malloc((size_t)(nsaved + 1) * sizeof(Bind));
    memcpy(saved, binds + sb, (size_t)nsaved * sizeof(Bind));
    Level savedL = *L;
    nbinds = sb; nlevels = li; bind_epoch++;
    Syn *s = compile_lazy(t);
    levels[li] = savedL; nlevels = li + 1;
    memcpy(binds + sb, saved, (size_t)nsaved * sizeof(Bind)); nbinds = nb; bind_epoch++;
    free(saved);
    L = &levels[li];
    int slot = new_slot();
    if (L->nenv == L->ecap) {
        L->ecap = L->ecap ? L->ecap * 2 : 8;
        L->env_nodes = realloc(L->env_nodes, (size_t)L->ecap * sizeof(Node *));
        L->env_slots = realloc(L->env_slots, (size_t)L->ecap * sizeof(int));
        L->env_syns = realloc(L->env_syns, (size_t)L->ecap * sizeof(Syn *));
    }
    L->env_nodes[L->nenv] = t; L->env_slots[L->nenv] = slot; L->env_syns[L->nenv] = s; L->nenv++;
    return slot;
}
long *prof_count, *prof_dup; const char **prof_name; int prof_n, prof_on; static int prof_cur; long prof_dupk[64], prof_erak[64], prof_con[4096], prof_glob[65536];
static size_t capprofc, capprofn, capprofd;
static void prof_enter(const char *name) {
    GROW(prof_count, capprofc, (size_t)prof_n + 1); GROW(prof_name, capprofn, (size_t)prof_n + 1); GROW(prof_dup, capprofd, (size_t)prof_n + 1);
    prof_count[prof_n] = 0; prof_dup[prof_n] = 0; prof_name[prof_n] = strdup(name); prof_cur = prof_n++;
}
/* ---------------- interning compiled code ----------------
 * Equal records are one pointer, and so are equal case tables: a λ's key is its record over the
 * keys of its captures, so equal bodies compiled in different places (two definitions, a
 * generated clause and a written one) name their values alike and share them through the memo.
 * Structural equality of the body: tags, slots, ids, words, and the (already interned) records
 * and tables it mentions */
static uint64_t hmix(uint64_t h, uint64_t v) { return (h ^ (v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2))) * 0x100000001B3ull; }
static uint64_t syn_hash(Syn *t) {
    if (!t) return 0x51;
    if (t->sh) return t->sh;
    uint64_t h = hmix(hmix(hmix(hmix(1, (uint64_t)t->tag), (uint64_t)t->n), (uint64_t)t->slot), t->x);
    h = hmix(h, (uint64_t)t->id); h = hmix(h, (uint64_t)(uintptr_t)t->rec); h = hmix(h, (uint64_t)(uintptr_t)t->tab);
    if (t->big) for (uint32_t i = 0; i < t->big->n; i++) h = hmix(h, t->big->l[i]);
    for (int i = 0; i < t->n; i++) h = hmix(h, syn_hash(t->k[i]));
    if (!h) h = 1;
    return t->sh = h;
}
static bool syn_eq(Syn *a, Syn *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (syn_hash(a) != syn_hash(b)) return false;
    if (a->tag != b->tag || a->n != b->n || a->slot != b->slot || a->x != b->x || a->id != b->id || a->rec != b->rec || a->tab != b->tab) return false;
    if (a->big || b->big) { WN x = { (uint64_t)a->id, a->big }, y = { (uint64_t)b->id, b->big }; if (wn_cmp(&x, &y) != 0) return false; }
    for (int i = 0; i < a->n; i++) if (!syn_eq(a->k[i], b->k[i])) return false;
    return true;
}
static void syn_free(Syn *t) {
    if (!t) return;
    stat_syn_bytes -= (long)(sizeof(Syn) + ((size_t)t->n + 1) * sizeof(Syn *));
    for (int i = 0; i < t->n; i++) syn_free(t->k[i]);
    if (t->big) big_release(t->big);
    free(t->k); free(t);
}
static Record **RT; static size_t RTcap, RTn;
/* CUBICAL_MEMEND: at exit, the compiled code held, split by owner (definitions' bodies, their
 * n-ary records, every interned record's body), in bytes of Syn nodes not counting records reached */
static long syn_bytes_shallow(const Syn *t) {
    if (!t) return 0;
    long b = (long)(sizeof(Syn) + ((size_t)t->n + 1) * sizeof(Syn *));
    for (int i = 0; i < t->n; i++) b += syn_bytes_shallow(t->k[i]);
    return b;
}
static void memend_report(void);
/* a record whose body was dropped (records_sweep): only keys still name it, by pointer. Equal code
 * compiled again must get the same pointer (a key naming it is the name of a value the memo may
 * hold), so what interning compares is kept: the hash, and the SHA-256 of the body as rec_eq
 * reads it (nested records and tables by pointer); and the records and tables the body named, so
 * that they keep their pointers too. Interning equal code again gives it its body back */
typedef struct RecGone { unsigned char dig[32]; Record **rec; CaseTab **tab; int nrec, ntab; } RecGone;
static void rec_digest(const Record *r, unsigned char out[32]);   /* after store.c (sha256) */
static uint64_t rec_hash(const Record *r) {
    if (r->h) return r->h;
    uint64_t h = hmix(hmix(hmix(7, (uint64_t)r->nparam), (uint64_t)r->nslots), (uint64_t)r->nenv);
    for (int i = 0; i < r->nenv; i++) h = hmix(h, (uint64_t)r->env_slot[i]);
    h = hmix(h, syn_hash(r->body));
    if (!h) h = 1;
    return ((Record *)r)->h = h;
}
static bool rec_eq(const Record *a, const Record *b) {
    if (a->nparam != b->nparam || a->nslots != b->nslots || a->nenv != b->nenv) return false;
    for (int i = 0; i < a->nenv; i++) if (a->env_slot[i] != b->env_slot[i]) return false;
    if (a->gone) {
        if (rec_hash(a) != rec_hash(b)) return false;
        unsigned char d[32]; rec_digest(b, d);
        return !memcmp(d, a->gone->dig, 32);
    }
    return syn_eq(a->body, b->body);
}
static long stat_rec_revived;
/* r, or the record equal to it made earlier (r then freed; a dropped one takes r's body back) */
static Record *rec_intern(Record *r) {
    if (RTn * 2 >= RTcap) {
        size_t nc = RTcap ? RTcap * 2 : 4096; Record **nt = calloc(nc, sizeof(Record *));
        for (size_t i = 0; i < RTcap; i++) if (RT[i]) { size_t q = rec_hash(RT[i]) & (nc - 1); while (nt[q]) q = (q + 1) & (nc - 1); nt[q] = RT[i]; }
        free(RT); RT = nt; RTcap = nc;
    }
    uint64_t h = rec_hash(r);
    size_t i = h & (RTcap - 1);
    for (; RT[i]; i = (i + 1) & (RTcap - 1))
        if (rec_eq(RT[i], r)) {
            Record *o = RT[i];
            if (o->gone) {
                o->body = r->body; o->uses = r->uses;
                free(o->gone->rec); free(o->gone->tab); free(o->gone); o->gone = NULL;
                free(r->env_slot); free(r); stat_rec_revived++;
                return o;
            }
            syn_free(r->body); free(r->env_slot); free(r->uses); free(r); return o;
        }
    RT[i] = r; RTn++; stat_rec_bytes += (long)sizeof(Record) + (long)(r->nenv + r->nslots + 2) * 4;
    return r;
}
static CaseTab **TT; static size_t TTcap, TTn;
static int tab_n(const CaseTab *t) { return DATA[t->d].ncons; }
static uint64_t tab_hash(const CaseTab *t) {
    uint64_t h = hmix(hmix(hmix(11, (uint64_t)t->d), (uint64_t)t->nenv), t->mask);
    int N = tab_n(t);
    for (int i = 0; i < N; i++) h = hmix(h, (uint64_t)(uintptr_t)t->br[i]);
    return hmix(h, (uint64_t)(uintptr_t)t->motive);
}
static bool tab_eq(const CaseTab *a, const CaseTab *b) {
    if (a->d != b->d || a->nenv != b->nenv || a->mask != b->mask || a->motive != b->motive) return false;
    int N = tab_n(a);
    for (int i = 0; i < N; i++) if (a->br[i] != b->br[i]) return false;
    return true;
}
static CaseTab *tab_intern(CaseTab *t) {
    if (TTn * 2 >= TTcap) {
        size_t nc = TTcap ? TTcap * 2 : 1024; CaseTab **nt = calloc(nc, sizeof(CaseTab *));
        for (size_t i = 0; i < TTcap; i++) if (TT[i]) { size_t q = tab_hash(TT[i]) & (nc - 1); while (nt[q]) q = (q + 1) & (nc - 1); nt[q] = TT[i]; }
        free(TT); TT = nt; TTcap = nc;
    }
    uint64_t h = tab_hash(t);
    size_t i = h & (TTcap - 1);
    for (; TT[i]; i = (i + 1) & (TTcap - 1))
        if (tab_eq(TT[i], t)) { free(t->br); free(t); return TT[i]; }
    TT[i] = t; TTn++;
    return t;
}
/* ---------------- sweeping compiled code ----------------
 * Records and case tables are interned and were kept for the run, but most are made by the
 * checker's readings of a definition (its quoted code compiled and run once). Between top-level
 * forms (every worker parked, nothing being compiled) they are sorted in three:
 *  - built from: reached from a definition's body or n-ary record or a cell's payload (a λ or
 *    thunk's record, a case's table), and through the bodies and branches of those. Kept whole.
 *  - named only: reached from a key's payload (a λ's key is its record over its captures' keys, a
 *    case's its table), and through what those name (a body's records and tables, a table's
 *    branches, a dropped body's list). Nothing builds them, but equal code compiled again must
 *    get the same pointer, so a key names it as before and the memo finds what it found: the
 *    body is dropped and what interning compares is kept (RecGone); equal code interned again
 *    gives the body back. A table is kept whole (it holds no code).
 *  - the rest: no cell, key, definition or kept code mentions them; freed. Equal code compiled
 *    later gets a fresh pointer, which nothing compares against the old one.
 * Run once the compiled code made since the last sweep has grown past a quarter of what that
 * sweep kept, and 32 MB (CUBICAL_NOSWEEP: never; CUBICAL_SWEEPALL: after every top-level form) */
typedef struct { void **v; size_t cap, n; } PSet;
static size_t ps_slot(const PSet *s, const void *p) { return (size_t)(((uintptr_t)p >> 4) * 0x9E3779B97F4A7C15ull) & (s->cap - 1); }
static void ps_init(PSet *s, size_t n) { s->cap = 64; while (s->cap < 2 * n + 2) s->cap *= 2; s->v = calloc(s->cap, sizeof(void *)); s->n = 0; }
static bool ps_has(const PSet *s, const void *p) { for (size_t i = ps_slot(s, p); s->v[i]; i = (i + 1) & (s->cap - 1)) if (s->v[i] == p) return true; return false; }
/* a set sized for what it was made for grows past half full (a body's records were counted at 16:
 * a full table is probed forever) */
static bool ps_add(PSet *s, void *p) {
    size_t i = ps_slot(s, p); for (; s->v[i]; i = (i + 1) & (s->cap - 1)) if (s->v[i] == p) return false;
    if (2 * (s->n + 1) > s->cap) {
        void **old = s->v; size_t oc = s->cap; s->cap *= 2; s->v = calloc(s->cap, sizeof(void *));
        for (size_t j = 0; j < oc; j++) if (old[j]) { size_t q = ps_slot(s, old[j]); while (s->v[q]) q = (q + 1) & (s->cap - 1); s->v[q] = old[j]; }
        free(old);
        for (i = ps_slot(s, p); s->v[i]; i = (i + 1) & (s->cap - 1)) ;
    }
    s->v[i] = p; s->n++; return true;
}
/* the interned ones; those built from (sw_live*); those named (sw_name*, a superset) */
static PSet sw_recs, sw_tabs, sw_liver, sw_livet, sw_namer, sw_namet;
static PSet *sw_mr, *sw_mt;   /* the sets being marked */
static void **sw_stack; static size_t sw_n, sw_cap;   /* to visit: a record, or a table with its low bit set */
static void sw_rec(Record *r) { if (r && ps_add(sw_mr, r)) { GROW(sw_stack, sw_cap, sw_n + 1); sw_stack[sw_n++] = r; } }
static void sw_tab(CaseTab *t) { if (t && ps_add(sw_mt, t)) { GROW(sw_stack, sw_cap, sw_n + 1); sw_stack[sw_n++] = (void *)((uintptr_t)t | 1); } }
static void sw_syn(const Syn *t) {
    if (!t) return;
    if (t->rec) sw_rec(t->rec);
    if (t->tab) sw_tab(t->tab);
    for (int i = 0; i < t->n; i++) sw_syn(t->k[i]);
}
static bool sw_building;   /* marking what is built from: a dropped record reached is an internal error */
static void sw_drain(void) {
    while (sw_n) {
        void *x = sw_stack[--sw_n];
        if ((uintptr_t)x & 1) {
            CaseTab *t = (CaseTab *)((uintptr_t)x & ~(uintptr_t)1);
            int N = tab_n(t); for (int i = 0; i < N; i++) sw_rec(t->br[i]);
            sw_rec(t->motive);
            continue;
        }
        Record *r = x;
        if (r->gone) {
            if (sw_building) die("internal: code built from a record whose body was dropped", NULL);
            for (int i = 0; i < r->gone->nrec; i++) sw_rec(r->gone->rec[i]);
            for (int i = 0; i < r->gone->ntab; i++) sw_tab(r->gone->tab[i]);
        } else sw_syn(r->body);
    }
}
static void sw_cell_payload(void *p) {
    if (ps_has(&sw_recs, p)) sw_rec(p);
    else if (ps_has(&sw_tabs, p)) sw_tab(p);
}
static void sw_key_payload(void *p) { sw_cell_payload(p); }
/* a body's records and tables, each once, for a record about to lose it */
static PSet sw_seen; static Record **sw_gr; static size_t sw_grn, sw_grcap; static CaseTab **sw_gt; static size_t sw_gtn, sw_gtcap;
/* code is shared (one subterm under many parents): each node is walked once, or a walk is
 * exponential in the nesting */
static PSet sw_synseen;
static void sw_gone_walk(const Syn *t) {
    if (!t || !ps_add(&sw_synseen, (void *)t)) return;
    if (t->rec && ps_add(&sw_seen, t->rec)) { GROW(sw_gr, sw_grcap, sw_grn + 1); sw_gr[sw_grn++] = t->rec; }
    if (t->tab && ps_add(&sw_seen, t->tab)) { GROW(sw_gt, sw_gtcap, sw_gtn + 1); sw_gt[sw_gtn++] = t->tab; }
    for (int i = 0; i < t->n; i++) sw_gone_walk(t->k[i]);
}
static void rec_drop_body(Record *r) {
    RecGone *g = calloc(1, sizeof(RecGone));
    rec_hash(r); rec_digest(r, g->dig);
    ps_init(&sw_seen, 16); ps_init(&sw_synseen, 16); sw_grn = sw_gtn = 0;
    sw_gone_walk(r->body);
    free(sw_seen.v); free(sw_synseen.v);
    if (sw_grn) { g->rec = malloc(sw_grn * sizeof(Record *)); memcpy(g->rec, sw_gr, sw_grn * sizeof(Record *)); g->nrec = (int)sw_grn; }
    if (sw_gtn) { g->tab = malloc(sw_gtn * sizeof(CaseTab *)); memcpy(g->tab, sw_gt, sw_gtn * sizeof(CaseTab *)); g->ntab = (int)sw_gtn; }
    syn_free(r->body); free(r->uses); r->body = NULL; r->uses = NULL; r->gone = g;
}
static long sw_kept_bytes = 0, stat_swept_recs, stat_swept_tabs, stat_dropped_recs, stat_sweeps;
static int toplevel_depth;
/* marks: what is built from (into sw_liver/sw_livet), then what is named (sw_namer/sw_namet, holding
 * the first); the interned sets sw_recs/sw_tabs filled. names: whether to mark the second */
static void sw_mark(bool names) {
    ps_init(&sw_recs, RTn); ps_init(&sw_tabs, TTn);
    for (size_t i = 0; i < RTcap; i++) if (RT[i]) ps_add(&sw_recs, RT[i]);
    for (size_t i = 0; i < TTcap; i++) if (TT[i]) ps_add(&sw_tabs, TT[i]);
    ps_init(&sw_liver, RTn); ps_init(&sw_livet, TTn);
    sw_mr = &sw_liver; sw_mt = &sw_livet; sw_building = true;
    for (long g = 0; g < nG; g++) { sw_syn(G[g].body); sw_rec(G[g].nary); }
    net_scan_payloads(sw_cell_payload, NULL);
    sw_drain();
    sw_building = false;
    if (!names) return;
    ps_init(&sw_namer, RTn); ps_init(&sw_namet, TTn);
    for (size_t i = 0; i < sw_liver.cap; i++) if (sw_liver.v[i]) ps_add(&sw_namer, sw_liver.v[i]);
    for (size_t i = 0; i < sw_livet.cap; i++) if (sw_livet.v[i]) ps_add(&sw_namet, sw_livet.v[i]);
    sw_mr = &sw_namer; sw_mt = &sw_namet;
    net_scan_payloads(NULL, sw_key_payload);
    sw_drain();
}
static void sw_unmark(bool names) {
    free(sw_recs.v); free(sw_tabs.v); free(sw_liver.v); free(sw_livet.v);
    if (names) { free(sw_namer.v); free(sw_namet.v); }
}
static void records_sweep(bool force) {
    static int off = -1; if (off < 0) off = getenv("CUBICAL_NOSWEEP") != NULL;
    if (off || toplevel_depth) return;
    if (!force && stat_syn_bytes < sw_kept_bytes + sw_kept_bytes / 4 + (32L << 20)) return;
    stat_sweeps++;
    sw_mark(true);
    /* tables: kept when named (built from is named too), rebuilt; the rest freed */
    {
        CaseTab **old = TT; size_t oc = TTcap; TT = calloc(oc, sizeof(CaseTab *)); TTn = 0;
        for (size_t i = 0; i < oc; i++) if (old[i]) {
            CaseTab *t = old[i];
            if (!ps_has(&sw_namet, t)) { free(t->br); free(t); stat_swept_tabs++; continue; }
            size_t q = tab_hash(t) & (oc - 1); while (TT[q]) q = (q + 1) & (oc - 1); TT[q] = t; TTn++;
        }
        free(old);
    }
    /* records: built from, kept; named only, the body dropped; the rest freed */
    {
        Record **old = RT; size_t oc = RTcap; RT = calloc(oc, sizeof(Record *)); RTn = 0;
        for (size_t i = 0; i < oc; i++) if (old[i]) {
            Record *r = old[i];
            if (!ps_has(&sw_namer, r)) {
                stat_rec_bytes -= (long)sizeof(Record) + (long)(r->nenv + r->nslots + 2) * 4;
                if (r->gone) { free(r->gone->rec); free(r->gone->tab); free(r->gone); }
                else syn_free(r->body);
                free(r->env_slot); free(r->uses); free(r); stat_swept_recs++; continue;
            }
            if (!r->gone && !ps_has(&sw_liver, r)) { rec_drop_body(r); stat_dropped_recs++; }
            size_t q = rec_hash(r) & (oc - 1); while (RT[q]) q = (q + 1) & (oc - 1); RT[q] = r; RTn++;
        }
        free(old);
    }
    sw_unmark(true);
    sw_kept_bytes = stat_syn_bytes;
}
static Syn *compile_record(Node *param, Node *body, bool thunk) {
    GROW(levels, caplevels, (size_t)nlevels + 1);
    int li = nlevels++;
    levels[li] = (Level){ .is_thunk = thunk, .scope_start = nbinds };
    if (!thunk) push_bind(param->s, li, new_slot());
    Syn *b = compile(body);
    nbinds = levels[li].scope_start; bind_epoch++;
    Level L = levels[li];
    nlevels--;
    Record *r = calloc(1, sizeof(Record));
    r->nparam = thunk ? 0 : 1; r->nslots = L.nslots; r->nenv = L.nenv; r->env_slot = L.env_slots; r->uses = L.uses; r->body = b;
    r->where = prof_cur;
    r = rec_intern(r);
    Syn *s = syn(S_LAM, L.nenv);
    s->rec = r;
    for (int k = 0; k < L.nenv; k++) s->k[k] = L.env_syns[k];
    free(L.env_nodes); free(L.env_syns);   /* the level's lifting tables: the record keeps env_slot, the λ its environment's code */
    return s;
}
/* Strictness: whether evaluating t to its head always reads the variable x. Such an argument is
 * built where it is passed, not suspended: it would be built on the first read anyway, and the
 * suspension would be a step hand reduction does not take. Conservative: a definition not yet
 * analysed (itself, a later one) reads none of its arguments. */
static bool reads(Node *t, const char *x);
static Names rd_bound;   /* the local names in scope (they shadow definitions) */
static bool reads_branch(Node *b, int m, const char *x) {
    /* λ shared(m)... λ fields... (thunk body): the shared variables are the outer ones */
    for (int q = 0; q < m; q++) { if (!b->list || b->n != 3 || !is(b->it[0], "lam")) return false; b = b->it[2]; }
    int save = rd_bound.n; bool r = false;
    while (b->list && b->n == 3 && is(b->it[0], "lam")) {
        if (!strcmp(b->it[1]->s, x)) { rd_bound.n = save; return false; }
        names_add(&rd_bound, b->it[1]->s); b = b->it[2];
    }
    if (b->list && b->n == 2 && is(b->it[0], "thunk")) r = reads(b->it[1], x);
    rd_bound.n = save;
    return r;
}
static bool reads(Node *t, const char *x) {
    if (!t->list) return !strcmp(t->s, x);
    if (t->n == 0 || t->it[0]->list) return false;
    const char *h = t->it[0]->s;
    if (!strcmp(h, "app")) {
        Node *hd = t; int na = 0;
        while (hd->list && hd->n == 3 && is(hd->it[0], "app")) { hd = hd->it[1]; na++; }
        if (!hd->list) {
            if (!strcmp(hd->s, x)) return true;
            if (names_has(&rd_bound, hd->s) || con_lookup(hd->s) >= 0) return false;
            if (fld_lookup(hd->s) >= 0) {
                /* a projection reads its value */
                Node *a = t;
                while (a->it[1]->list && a->it[1]->n == 3 && is(a->it[1]->it[0], "app")) a = a->it[1];
                return reads(a->it[2], x);
            }
            long g = global_lookup(hd->s);
            if (g < 0 || G[g].kind != G_DEF || na < G[g].nparams) return false;
            Node *a = t; Node *args[64]; int k = na;
            while (a->list && a->n == 3 && is(a->it[0], "app")) { if (--k < 64) args[k] = a->it[2]; a = a->it[1]; }
            for (int q = 0; q < G[g].nparams && q < 64; q++) if ((G[g].strict >> q & 1) && reads(args[q], x)) return true;
            return false;
        }
        return reads(hd, x);
    }
    if (!strcmp(h, "%case")) {
        bool el = t->n == 5;
        if (reads(t->it[el ? 4 : 3], x)) return true;
        Node *B = t->it[1];
        if (!B->list || B->n < 5) return false;
        int m = atoi(B->it[3]->s);
        for (int i = 4; i < B->n; i++) if (!reads_branch(B->it[i], m, x)) return false;
        return true;
    }
    if (!strcmp(h, "let")) {
        if (!strcmp(t->it[1]->s, x)) return reads(t->it[2], x) && reads(t->it[3], x);
        int save = rd_bound.n; names_add(&rd_bound, t->it[1]->s);
        /* the body reads x, or reads the let's variable whose value reads x */
        bool r = reads(t->it[3], x) || (reads(t->it[2], x) && reads(t->it[3], t->it[1]->s));
        rd_bound.n = save; return r;
    }
    if (!strcmp(h, "the")) return reads(t->it[2], x);
    if (!strcmp(h, "%rebuilt")) return reads(t->it[1], x);
    if (!strcmp(h, "fst") || !strcmp(h, "snd") || !strcmp(h, "force") || !strcmp(h, "@") || !strcmp(h, "%view")
        || !strcmp(h, "%wsucc") || !strcmp(h, "%welim") || !strcmp(h, "%clauses") || !strcmp(h, "faceof"))
        return reads(t->it[1], x);
    if (!strcmp(h, "%weq") || !strcmp(h, "%wlt") || !strcmp(h, "%wadd") || !strcmp(h, "%wmax") || !strcmp(h, "%wsub")
        || !strcmp(h, "%wmul") || !strcmp(h, "%wdiv") || !strcmp(h, "%wmod") || !strcmp(h, "%wand") || !strcmp(h, "%wor") || !strcmp(h, "%wxor") || !strcmp(h, "%wshl") || !strcmp(h, "%wshr")
        || !strcmp(h, "%ieq") || !strcmp(h, "%feq") || !strcmp(h, "%clos"))
        return reads(t->it[1], x) || reads(t->it[2], x);
    return false;
}
/* Demand. A subterm in a position whose value may not be read (an argument, a field, a let's value,
 * a motive, a lifted subterm) is held as its syntax with its environment (V_THUNK), built when an
 * operation meets it: work nobody reads is never done, and erasing it erases its environment. A
 * subterm whose building does no work (a variable, a constant, a λ, a value former over such
 * positions) is built as it is. */
static bool lazy_trivial(Node *t) {
    if (!t->list) return true;
    if (t->n == 0 || t->it[0]->list) return false;
    const char *h = t->it[0]->s;
    if (!strcmp(h, "lam") || !strcmp(h, "thunk") || !strcmp(h, "pair") || !strcmp(h, "Pi") || !strcmp(h, "Sigma")
        || !strcmp(h, "PathP") || !strcmp(h, "U") || !strcmp(h, "%record")) return true;
    if (!strcmp(h, "the")) return lazy_trivial(t->it[2]);
    if (!strcmp(h, "%rebuilt")) return lazy_trivial(t->it[1]);
    if (!strcmp(h, "app")) {
        Node *hd = t; int na = 0;
        while (hd->list && hd->n == 3 && is(hd->it[0], "app")) { hd = hd->it[1]; na++; }
        int c = !hd->list && !lookup(hd->s) ? con_lookup(hd->s) : -1;
        return c >= 0 && na == CON[c].nfields + CON[c].ndims;
    }
    return false;
}
static Syn *compile_lazy(Node *t) {
    if (lazy_trivial(t)) return compile(t);
    int li = nlevels - 1;
    if (li >= 1) {
        /* free of this level: lifted (lazily) to the level it belongs to */
        ml_cut = li;
        int m = maxlev(t);
        ml_cut = 1 << 30;
        if (m >= 0 && m < li && !levels[li].is_thunk) return var_syn(lift(t));
    }
    Syn *s = compile_record(NULL, t, true);
    s->tag = S_THUNK;
    return s;
}
static Syn *compile_lazy_n(int tag, Node *t, int from, int n) {
    Syn *s = syn(tag, n);
    for (int i = 0; i < n; i++) s->k[i] = compile_lazy(t->it[from + i]);
    return s;
}
static Syn *compile_bound(int tag, Node *name, Node *body) {
    int slot = new_slot();
    push_bind(name->s, nlevels - 1, slot);
    Syn *s = syn(tag, 1); s->slot = slot; s->k[0] = compile(body);
    nbinds--; bind_epoch++;
    return s;
}
static Syn *compile_n(int tag, Node *t, int from, int n) {
    Syn *s = syn(tag, n);
    for (int i = 0; i < n; i++) s->k[i] = compile(t->it[from + i]);
    return s;
}
/* the variables of the enclosing scopes a term refers to, in order of first occurrence */
static void local_free(Node *t, Names *bound, Names *out) {
    if (!t->list) {
        if (!names_has(bound, t->s) && lookup(t->s)) names_add(out, t->s);
        return;
    }
    if (t->n == 0) return;
    if (t->it[0]->list) { for (int i = 0; i < t->n; i++) local_free(t->it[i], bound, out); return; }
    int hk = hk_of(t->it[0]);
    if ((hk == H_LAM || hk == H_PLAM || hk == H_FORALL) && t->n == 3) {
        int save = bound->n; names_add(bound, param_name(t->it[1])->s);
        local_free(t->it[2], bound, out); bound->n = save; return;
    }
    if (hk == H_LET && t->n == 4) {
        local_free(t->it[2], bound, out);
        int save = bound->n; names_add(bound, t->it[1]->s);
        local_free(t->it[3], bound, out); bound->n = save; return;
    }
    if (hk == H_THE && t->n == 3) { local_free(t->it[2], bound, out); return; }
    int from = 1;
    if (hk == H_BRANCHES) from = 4;
    else if (hk == H_CON || hk == H_DATA || hk == H_TRANSD || hk == H_RECORD) from = 2;
    for (int i = from; i < t->n; i++) local_free(t->it[i], bound, out);
}
/* a branch: a record whose parameters (slots 0..np-1) are the constructor's fields, hypotheses
 * and dimensions and whose environment is the case's shared variables. Nothing is lifted out of
 * it: it is built only when selected */
static Record *compile_branch(const Names *env, Node **pn, int np, Node *body) {
    GROW(levels, caplevels, (size_t)nlevels + 1);
    int li = nlevels++;
    levels[li] = (Level){ .is_thunk = 1, .scope_start = nbinds };
    int *es = calloc((size_t)env->n + 1, sizeof(int));
    for (int q = 0; q < np; q++) new_slot();
    for (int e = 0; e < env->n; e++) { es[e] = new_slot(); push_bind(env->v[e], li, es[e]); }
    for (int q = 0; q < np; q++) push_bind(pn[q]->s, li, q);
    Syn *b = compile(body);
    nbinds = levels[li].scope_start; bind_epoch++;
    Level L = levels[li];
    nlevels--;
    if (L.nenv) die("internal: a case branch lifted a term", NULL);
    Record *r = calloc(1, sizeof(Record));
    r->nparam = np; r->nslots = L.nslots; r->nenv = env->n; r->env_slot = es;
    r->uses = L.uses ? L.uses : calloc(1, sizeof(int)); r->body = b; r->where = prof_cur;
    return rec_intern(r);
}
/* (%case (%branches D mask m b...) [P] E x): one cell. Its table holds one record per
 * constructor, and the motive as a record; its ports the scrutinee and the variables the branches
 * and the motive share, each once. A constructor meeting it builds the selected branch: one interaction. A handler is
 * λ shared(m)... λ fields, hypotheses, dimensions. (thunk body), or any term, then applied */
static Syn *compile_case(Node *t) {
    bool el = t->n == 5;
    Node *B = t->it[1];
    if (!B->list || B->n < 4 || !is(B->it[0], "%branches")) die("%case without its branches", NULL);
    int d = data_lookup(B->it[1]->s);
    if (d < 0) die("%branches: unknown data type", B->it[1]->s);
    if (DATA[d].co) die("a codata type has no elimination (it is observed by its fields):", B->it[1]->s);
    uint64_t mask = strtoull(B->it[2]->s, NULL, 10);
    int m = atoi(B->it[3]->s), N = DATA[d].ncons;
    if (B->n - 4 != N) die("%branches: one branch per constructor of", DATA[d].name);
    Node **body = calloc((size_t)N + 1, sizeof(Node *)), ***pn = calloc((size_t)N + 1, sizeof(Node **));
    int *np = calloc((size_t)N + 1, sizeof(int));
    Names env = { 0 };
    for (int c = 0; c < N; c++) {
        ConInfo *ci = &CON[DATA[d].cons[c]];
        Node *h = B->it[4 + c];
        for (int q = 0; q < m; q++) {
            if (!h->list || h->n != 3 || !is(h->it[0], "lam")) die("%branches: a branch binds the shared variables first", NULL);
            h = h->it[2];
        }
        int n = ci->nfields + ((mask >> c & 1) && ci->nrec ? ci->nrec : 0) + ci->ndims;
        np[c] = n; pn[c] = calloc((size_t)n + 1, sizeof(Node *));
        int got = 0;
        while (got < n && h->list && h->n == 3 && is(h->it[0], "lam")) { pn[c][got++] = param_name(h->it[1]); h = h->it[2]; }
        if (got < n) {
            for (int q = got; q < n; q++) { char nm[32]; snprintf(nm, sizeof nm, "%%p%d", q); pn[c][q] = nsym(nm); h = nl3("app", h, nsym(nm)); }
            h = nl2("force", h);
        } else if (h->list && h->n == 2 && is(h->it[0], "thunk")) h = h->it[1];
        else h = nl2("force", h);
        body[c] = h;
        Names bound = { 0 };
        for (int q = 0; q < n; q++) names_add(&bound, pn[c][q]->s);
        local_free(h, &bound, &env);
        free(bound.v);
    }
    /* the motive's variables join the shared ones: it is built from them, only where it is read */
    if (el) { Names none = { 0 }; local_free(t->it[2], &none, &env); free(none.v); }
    CaseTab *tb = calloc(1, sizeof(CaseTab));
    tb->d = d; tb->nenv = env.n; tb->mask = mask; tb->br = calloc((size_t)N + 1, sizeof(Record *));
    for (int c = 0; c < N; c++) tb->br[c] = compile_branch(&env, pn[c], np[c], body[c]);
    tb->motive = el ? compile_branch(&env, NULL, 0, t->it[2]) : NULL;
    tb = tab_intern(tb);
    Syn *s = syn(S_CASE, 2 + env.n);
    s->tab = tb; s->x = 0;
    s->k[0] = compile(t->it[el ? 4 : 3]);
    s->k[1] = NULL;
    for (int e = 0; e < env.n; e++) s->k[2 + e] = compile(nsym(env.v[e]));
    free(body); free(np);
    return s;
}
static Syn *compile(Node *t) {
    int li = nlevels - 1;
    if (li >= 1) {
        ml_cut = li;
        int m = maxlev(t);
        ml_cut = 1 << 30;
        if (m >= 0 && m < li && (!levels[li].is_thunk || !t->list)) return var_syn(lift(t));
    }
    if (!t->list) {
        const char *s = t->s;
        if (IS(s, "0")) return syn(S_IV0, 0);
        if (IS(s, "1")) return syn(S_IV1, 0);
        if (IS(s, "tt")) return syn(S_FTT, 0);
        if (IS(s, "ff")) return syn(S_FFF, 0);
        if (IS(s, "U")) return syn(S_U, 0);
        if (IS(s, "Word")) return syn(S_WORDTY, 0);
        Bind *b = lookup_i(s);
        if (b) {
            if (b->level != li) die("internal: unlifted outer variable", s);
            return var_syn(b->slot);
        }
        { int c = con_lookup(s); if (c >= 0 && CON[c].nfields + CON[c].ndims == 0) { Syn *r = syn(S_CON, 0); r->id = c; return r; } }
        if (s[0] == '#' && s[1] >= '0' && s[1] <= '9') {
            Syn *r = syn(S_WORD, 0); WN w = wn_parse(s + 1);
            r->id = (long)w.s; r->big = w.b;
            return r;
        }
        long g = global_lookup(s);
        if (g < 0) die("unbound name", s);
        Syn *r = syn(G[g].kind == G_AXIOM ? S_AXIOM : S_GLOBAL, 0); r->id = g; r->slot = (int)G[g].tyg;
        return r;
    }
    const char *h = t->it[0]->s;
    if (IS(h, "lam")) return compile_record(t->it[1], t->it[2], false);
    if (IS(h, "thunk")) return compile_record(NULL, t->it[1], true);
    if (IS(h, "%delay")) return compile_lazy(t->it[1]);
    if (IS(h, "force")) return compile_n(S_FORCE, t, 1, 1);
    if (IS(h, "the")) return compile(t->it[2]);
    if (IS(h, "%rebuilt")) { Syn *r = compile(t->it[1]); if (r->tag == S_CON) r->x = 1; return r; }
    if (IS(h, "%hole")) return compile(hole_value());
    if (IS(h, "%guess")) return compile(t->it[3]);
    if (IS(h, "%spine")) return compile(t->it[1]);   /* x: the scrutinee rebuilt: named by it */
    if (IS(h, "plam")) return compile_bound(S_PLAM, t->it[1], t->it[2]);
    if (IS(h, "forall")) return compile_bound(S_FORALL, t->it[1], t->it[2]);
    if (IS(h, "let")) {
        Syn *v = compile_lazy(t->it[2]);
        Syn *s = compile_bound(S_LET, t->it[1], t->it[3]);
        s->k[1] = s->k[0]; s->k[0] = v; s->n = 2;
        return s;
    }
    if (IS(h, "app")) {
        /* a constructor applied to all its fields and dimensions is the constructor */
        Node *hd = t; int na = 0;
        while (hd->list && hd->n == 3 && is(hd->it[0], "app")) { hd = hd->it[1]; na++; }
        int c = !hd->list && !lookup(hd->s) ? con_lookup(hd->s) : -1;
        if (c >= 0 && na == CON[c].nfields + CON[c].ndims) {
            Syn *r = syn(S_CON, na); r->id = c;
            Node *a = t;
            for (int q = na - 1; q >= 0; q--) { r->k[q] = q < CON[c].nfields ? compile_lazy(a->it[2]) : compile(a->it[2]); a = a->it[1]; }
            return r;
        }
        /* a field applied to a value: its projection (then any further arguments) */
        int fl = !hd->list && !lookup(hd->s) && c < 0 ? fld_lookup(hd->s) : -1;
        if (fl >= 0) {
            Node **args = calloc((size_t)na + 1, sizeof(Node *)), *a = t;
            for (int q = na - 1; q >= 0; q--) { args[q] = a->it[2]; a = a->it[1]; }
            Syn *r = syn(S_PROJ, 1); r->id = fl; r->k[0] = compile(args[0]);
            for (int q = 1; q < na; q++) { Syn *ap = syn(S_APP, 2); ap->k[0] = r; ap->k[1] = compile_lazy(args[q]); r = ap; }
            free(args);
            return r;
        }
        /* a definition applied to arguments: one call cell (its body built at once when the
         * arguments cover its parameters) */
        long g = !hd->list && !lookup(hd->s) && c < 0 && !constant_sym(hd->s) ? global_lookup(hd->s) : -1;
        if (g >= 0 && G[g].kind != G_AXIOM && na >= 1) {
            Syn *r = syn(S_CALL, na); r->id = g;
            Node *a = t;
            for (int q = na - 1; q >= 0; q--) {
                bool strict = q < G[g].nparams && q < 64 && (G[g].strict >> q & 1);
                r->k[q] = strict ? compile(a->it[2]) : compile_lazy(a->it[2]);
                a = a->it[1];
            }
            return r;
        }
        { Syn *s = syn(S_APP, 2); s->k[0] = compile(t->it[1]); s->k[1] = compile_lazy(t->it[2]); return s; }
    }
    if (IS(h, "pair")) return compile_lazy_n(S_PAIR, t, 1, 2);
    if (IS(h, "fst")) return compile_n(S_FST, t, 1, 1);
    if (IS(h, "snd")) return compile_n(S_SND, t, 1, 1);
    if (IS(h, "Pi")) { Syn *s = compile_lazy_n(S_PI, t, 1, 2); Node *l = t->it[2]; s->x = (uint32_t)(l->list && l->n == 3 && !l->it[1]->list ? bnd_mult(l->it[1]) : 2); return s; }
    if (IS(h, "Sigma")) return compile_lazy_n(S_SIGMA, t, 1, 2);
    if (IS(h, "U")) {
        if (t->n == 1) return syn(S_U, 0);
        Node *l = t->it[1];
        if (!l->list && l->s[0] >= '0' && l->s[0] <= '9') { Node *z = nsym("lzero"); for (int q = atoi(l->s); q > 0; q--) z = nl3("app", nsym("lsuc"), z); l = z; }
        Syn *s = syn(S_U, 1); s->k[0] = compile(l); return s;
    }
    {
        static const struct { const char *name; int kind, n; } PR[] = { { "%view", O_VIEW, 1 }, { "%ieq", O_IEQ, 2 }, { "%feq", O_FEQL, 2 }, { "%clauses", O_CLAUSES, 1 }, { "%dnf", O_DNF, 1 }, { "%motive", O_MOTIVE, 1 },
            { "%restrict", O_RESTRICT, 2 }, { "%abs", O_ABS, 2 }, { "%branch", O_BRANCH, 3 }, { "%clos", O_CLOS, 2 }, { "%wsucc", O_WSUCC, 1 }, { "%weq", O_WEQ, 2 }, { "%wlt", O_WLT, 2 },
            { "%wadd", O_WADD, 2 }, { "%wmax", O_WMAX, 2 }, { "%wsub", O_WSUB, 2 }, { "%wmul", O_WMUL, 2 }, { "%wdiv", O_WDIV, 2 }, { "%wmod", O_WMOD, 2 }, { "%wand", O_WAND, 2 }, { "%wor", O_WOR, 2 }, { "%wxor", O_WXOR, 2 }, { "%wshl", O_WSHL, 2 }, { "%wshr", O_WSHR, 2 }, { "%welim", O_WELIM, 3 },
            { "%pimult", O_PIMULT, 1 }, { "%pim", O_PIM, 3 }, { "%fmult", O_FMULT, 3 }, { "%linear", O_LINEAR, 1 }, { "%gusage", O_GUSAGE, 2 }, { "%gsteps", O_GSTEPS, 1 }, { "%gcode", O_GCODE, 1 }, { "%dnidx", O_DNIDX, 1 },
            /* primitive types and values (B12) */
            { "%pty", O_PTY, 1 }, { "%bytesty", O_BYTESTY, 1 }, { "%pval", O_PVAL, 2 }, { "%bytes", O_BYTES, 1 }, { "%blen", O_BLEN, 1 }, { "%beq", O_BEQ, 2 }, { "%ptr-of", O_PTROF, 1 }, { "%region-at", O_REGAT, 2 }, { NULL, 0, 0 } };
        /* the primitive operations, (%i64add a b) ...: O_POP1/O_POP2 with the operation in x; the loads and
         * stores by width */
        static const struct { const char *name; int op, n; } PO[] = {
            { "%i64add", PO_I64ADD, 2 }, { "%i64sub", PO_I64SUB, 2 }, { "%i64mul", PO_I64MUL, 2 }, { "%i64eq", PO_I64EQ, 2 }, { "%i64lt", PO_I64LT, 2 },
            { "%u32add", PO_U32ADD, 2 }, { "%u32sub", PO_U32SUB, 2 }, { "%u32mul", PO_U32MUL, 2 }, { "%u32eq", PO_U32EQ, 2 }, { "%u32lt", PO_U32LT, 2 },
            { "%f64add", PO_F64ADD, 2 }, { "%f64sub", PO_F64SUB, 2 }, { "%f64mul", PO_F64MUL, 2 }, { "%f64div", PO_F64DIV, 2 }, { "%f64eq", PO_F64EQ, 2 }, { "%f64lt", PO_F64LT, 2 },
            { "%i64ofw", PO_I64OFW, 1 }, { "%wofi64", PO_WOFI64, 1 }, { "%u32ofw", PO_U32OFW, 1 }, { "%wofu32", PO_WOFU32, 1 }, { "%f64ofw", PO_F64OFW, 1 }, { "%f64ofi64", PO_F64OFI64, 1 }, { "%i64off64", PO_I64OFF64, 1 }, { "%f64bits", PO_F64BITS, 1 }, { "%f64ofbits", PO_F64OFBITS, 1 }, { "%f32bits", PO_F32BITS, 1 }, { "%f32ofbits", PO_F32OFBITS, 1 },
            { "%u64add", PO_U64ADD, 2 }, { "%u64sub", PO_U64SUB, 2 }, { "%u64mul", PO_U64MUL, 2 }, { "%u64eq", PO_U64EQ, 2 }, { "%u64lt", PO_U64LT, 2 }, { "%u64div", PO_U64DIV, 2 }, { "%u64mod", PO_U64MOD, 2 },
            { "%u64and", PO_U64AND, 2 }, { "%u64or", PO_U64OR, 2 }, { "%u64xor", PO_U64XOR, 2 }, { "%u64shl", PO_U64SHL, 2 }, { "%u64shr", PO_U64SHR, 2 },
            { "%i64div", PO_I64DIV, 2 }, { "%i64mod", PO_I64MOD, 2 }, { "%i64and", PO_I64AND, 2 }, { "%i64or", PO_I64OR, 2 }, { "%i64xor", PO_I64XOR, 2 }, { "%i64shl", PO_I64SHL, 2 }, { "%i64shr", PO_I64SHR, 2 },
            { "%u64ofw", PO_U64OFW, 1 }, { "%wofu64", PO_WOFU64, 1 },
            { NULL, 0, 0 } };
        for (int q = 0; PO[q].name; q++) if (!strcmp(h, PO[q].name)) {
            if (t->n != PO[q].n + 1) die("a primitive operation with its arguments:", h);
            Syn *s = compile_n(S_PRIM, t, 1, PO[q].n); s->x = (uint32_t)(PO[q].n == 1 ? O_POP1 : O_POP2);
            s->id = PO[q].op;   /* the operation, in the cell's x once built */
            return s;
        }
        static const struct { const char *name; int kind, w; } PL[] = { { "%load8", O_LOAD, 1 }, { "%load16", O_LOAD, 2 }, { "%load32", O_LOAD, 4 }, { "%load64", O_LOAD, 8 },
            { "%store8", O_STORE, 1 }, { "%store16", O_STORE, 2 }, { "%store32", O_STORE, 4 }, { "%store64", O_STORE, 8 }, { NULL, 0, 0 } };
        for (int q = 0; PL[q].name; q++) if (!strcmp(h, PL[q].name)) {
            int na = PL[q].kind == O_LOAD ? 2 : 3;
            if (t->n != na + 1) die("a load or store with its arguments:", h);
            Syn *s = compile_n(S_PRIM, t, 1, na); s->x = (uint32_t)PL[q].kind; s->id = PL[q].w;
            return s;
        }
        if (IS(h, "%ffi")) {
            /* (%ffi k a1 .. an): the foreign call of entry k (D27) */
            if (t->n < 3 || t->it[1]->list || t->it[1]->s[0] != '#') die("(%ffi k a...)", NULL);
            Syn *s = compile_n(S_PRIM, t, 2, t->n - 2); s->x = O_FFI; s->id = atoi(t->it[1]->s + 1); return s;
        }
        if (IS(h, "%native")) {
            /* (%native k g a1 .. an): the #jit definition g (foreign entry k) by its bytes, g itself otherwise (plan 5.5) */
            if (t->n < 4 || t->it[1]->list || t->it[1]->s[0] != '#' || t->it[2]->list || t->it[2]->s[0] != '#') die("(%native k g a...)", NULL);
            Syn *s = compile_n(S_PRIM, t, 3, t->n - 3); s->x = O_NATIVE; s->id = atoi(t->it[1]->s + 1); s->slot = atoi(t->it[2]->s + 1); return s;
        }
        if (IS(h, "%welimP")) {
            /* (%welimP P w z s): the elimination of a word with its motive, which is for the
             * checker: the machine runs (%welim w z s) */
            if (t->n != 5) die("(%welimP P w z s)", NULL);
            /* the motive travels too (the last operand): a stuck elimination's type, for its
             * endpoints (p 0 is the type's left end) */
            Syn *s = syn(S_PRIM, 4); s->x = O_WELIM;
            s->k[0] = compile(t->it[2]); s->k[1] = compile(t->it[3]); s->k[2] = compile(t->it[4]); s->k[3] = compile(t->it[1]);
            return s;
        }
        if (IS(h, "%var") || IS(h, "%dim")) {
            /* (%var [n] T), (%dim [n]): a fresh variable of type T, a fresh dimension, named by the
             * word n (a source name) or unnamed; the first port triggers the cell */
            bool v = IS(h, "%var");
            int named = t->n - (v ? 2 : 1);
            if (named != 0 && named != 1) die(v ? "(%var [n] T)" : "(%dim [n])", NULL);
            Syn *s = syn(S_PRIM, v ? 2 : 1); s->x = v ? O_MKVAR : O_MKDIM;
            s->k[0] = named ? compile(t->it[1]) : syn(S_FTT, 0);
            if (v) s->k[1] = compile(t->it[1 + named]);
            return s;
        }
        for (int q = 0; PR[q].name; q++) if (!strcmp(h, PR[q].name)) {
            if (t->n != PR[q].n + 1) die("wrong number of operands in", h);
            Syn *s = compile_n(S_PRIM, t, 1, PR[q].n); s->x = (uint32_t)PR[q].kind; return s;
        }
        if (IS(h, "%box")) { if (t->n != 2) die("(%box v)", NULL); Syn *s = syn(S_BOX, 1); s->k[0] = compile(t->it[1]); return s; }
        static const char *TBN[TB_N] = { "%gval", "%gty", "%dtab", "%dent", "%dtrans" };
        for (int q = 0; q < TB_N; q++) if (!strcmp(h, TBN[q])) {
            if (t->n != 2) die("wrong number of operands in", h);
            Syn *s = syn(S_GLOBTAB, 1); s->x = (uint32_t)q; s->k[0] = compile(t->it[1]); return s;
        }
    }
    if (IS(h, "PathP")) return compile_lazy_n(S_PATHP, t, 1, 3);
    if (IS(h, "@")) { Syn *s = compile_n(S_PAPP, t, 1, t->n - 1); s->x = t->n == 5; return s; }
    if (IS(h, "~")) return compile_n(S_NEG, t, 1, 1);
    if (IS(h, "/\\")) { Syn *s = compile_n(S_BIN, t, 1, 2); s->x = B_MEET; return s; }
    if (IS(h, "\\/")) { Syn *s = compile_n(S_BIN, t, 1, 2); s->x = B_JOIN; return s; }
    if (IS(h, "&")) { Syn *s = compile_n(S_BIN, t, 1, 2); s->x = B_FAND; return s; }
    if (IS(h, "|")) { Syn *s = compile_n(S_BIN, t, 1, 2); s->x = B_FOR; return s; }
    if (IS(h, "=")) {
        Syn *s = compile_n(S_FEQ, t, 1, 1);
        if (is(t->it[2], "0")) s->x = 0; else if (is(t->it[2], "1")) s->x = 1; else die("(= r e) needs e = 0 or 1", NULL);
        return s;
    }
    if (IS(h, "faceof")) return compile_n(S_FACEOF, t, 1, 1);
    if (IS(h, "%fneg")) { arity(t, 1); return compile_n(S_FNEG, t, 1, 1); }
    if (IS(h, "sys0")) return syn(S_SYS0, 0);
    if (IS(h, "sys+")) return compile_n(S_SYSCONS, t, 1, 3);
    if (IS(h, "sysmap")) return compile_n(S_SYSMAP, t, 1, 2);
    if (IS(h, "comp")) return compile_n(S_COMP, t, 1, 3);
    if (IS(h, "Glue")) return compile_n(S_GLUE, t, 1, 2);
    if (IS(h, "%hcompU")) { arity(t, 2); Syn *s = compile_n(S_GLUE, t, 1, 2); s->x = 1; return s; }
    if (IS(h, "%unglueU")) { arity(t, 2); Syn *s = compile_n(S_UNGLUE, t, 1, 2); s->x = 1; return s; }
    if (IS(h, "glue")) {
        /* the base is held unbuilt: where the system is total the glue value is its face's element
         * and the base is never read (in a composition the base is a further composition) */
        arity(t, 2); Syn *s = syn(S_GLUEV, 2); s->k[0] = compile(t->it[1]); s->k[1] = compile_lazy(t->it[2]); return s;
    }
    if (IS(h, "unglue")) return compile_n(S_UNGLUE, t, 1, 2);
    if (IS(h, "%data")) {
        int d = data_lookup(t->it[1]->s);
        if (d < 0 || t->n - 2 != DATA[d].nparams) die("bad %data", t->it[1]->s);
        Syn *s = compile_n(S_DATA, t, 2, t->n - 2); s->id = d; return s;
    }
    if (IS(h, "%con")) {
        int c = con_lookup(t->it[1]->s);
        if (c < 0 || t->n - 2 != CON[c].nfields + CON[c].ndims) die("bad %con", t->it[1]->s);
        Syn *s = compile_n(S_CON, t, 2, t->n - 2); s->id = c; return s;
    }
    if (IS(h, "hcompD")) return compile_n(S_HCOMPD, t, 1, 2);
    if (IS(h, "%hcompF")) { arity(t, 3); return compile_n(S_HCOMPD, t, 1, 3); }
    if (IS(h, "%record")) {
        /* a copattern: its fields held unbuilt, each built when its projection reads it */
        int d = data_lookup(t->it[1]->s);
        if (d < 0 || !DATA[d].co || t->n - 2 != DATA[d].nflds) die("bad %record", t->it[1]->s);
        Syn *s2 = compile_lazy_n(S_REC, t, 2, t->n - 2); s2->id = d; return s2;
    }
    if (IS(h, "%transD")) {
        int d = data_lookup(t->it[1]->s);
        if (d < 0 || t->n - 2 != DATA[d].nparams + 2) die("bad %transD", t->it[1]->s);
        Syn *s = compile_n(S_TRANSD, t, 2, t->n - 2); s->id = d; return s;
    }
    if (IS(h, "%case")) {
        if (t->n != 4 && t->n != 5) die("bad %case", NULL);
        return compile_case(t);
    }
    die("unknown form", h);
    return NULL;
}
/* compile a closed top-level term into a frame of its own */
static bool top_lazy;    /* a definition's body: built when first used */
/* a desugared term compiled at the top level (the caller holds the arena mark) */
static Syn *compile_top_ds(Node *d, int *nslots, int **uses) {
    nbinds = 0; nlevels = 1; bind_epoch++;
    GROW(levels, caplevels, 1);
    levels[0] = (Level){ 0 };
    Syn *s = top_lazy ? compile_lazy(d) : compile(d);
    *nslots = levels[0].nslots; *uses = levels[0].uses;
    if (!*uses) *uses = calloc(1, sizeof(int));
    nlevels = 0;
    return s;
}
static Syn *compile_top(Node *t, int *nslots, int **uses) {
    ArMark m = ar_mark();   /* the desugared term and the binders made for it live until compiled */
    Syn *s = compile_top_ds(ds(t), nslots, uses);
    ar_release(m);
    return s;
}
/* a term posed once (a reading, a check): compiled, built, and its compiled code freed. The net
 * holds no pointer into a top-level body (cells hold its records and case tables, which are
 * interned and kept, and numbers' payloads by count), so after the build it is garbage: the
 * checker's readings compiled every definition's quoted code again each round and kept it
 * (CUBICAL_KEEPTOP keeps it, to compare) */
static Port build_top(Node *t) {
    int ns; int *u; Syn *s = compile_top(t, &ns, &u);
    Port r = top_build(s, ns, u);
    static int keep = -1; if (keep < 0) keep = getenv("CUBICAL_KEEPTOP") != NULL;
    if (!keep) { syn_free(s); free(u); }
    return r;
}

/* ---------------- reading back ---------------- */
typedef struct { Dim d; char name[16]; } DimShow;
static DimShow *dshow; static size_t capdshow; static int ndshow, dcount; static long vcount;
static char dimbuf[8][24]; static int dimbuf_i;
/* the source names of dimensions the checker made for binders (a map from dimension to name) */
typedef struct { Dim d; int nm; } DimNm;
static DimNm *DNM; static size_t capdnm, ndnm;
static pthread_mutex_t dnm_mu = PTHREAD_MUTEX_INITIALIZER;
static size_t dnm_slot(Dim d, size_t cap) { return (size_t)((d * 0x9E3779B97F4A7C15ull) >> 20) & (cap - 1); }
void dim_named(Dim d, int nm) {
    pthread_mutex_lock(&dnm_mu);
    if (2 * (ndnm + 1) > capdnm) {
        size_t nc = capdnm ? capdnm * 2 : 1024;
        DimNm *nt = calloc(nc, sizeof(DimNm));
        for (size_t i = 0; i < capdnm; i++) if (DNM[i].d) { size_t j = dnm_slot(DNM[i].d, nc); while (nt[j].d) j = (j + 1) & (nc - 1); nt[j] = DNM[i]; }
        free(DNM); DNM = nt; capdnm = nc;
    }
    size_t j = dnm_slot(d, capdnm);
    while (DNM[j].d && DNM[j].d != d) j = (j + 1) & (capdnm - 1);
    if (!DNM[j].d) ndnm++;
    DNM[j] = (DimNm){ d, nm };
    pthread_mutex_unlock(&dnm_mu);
}
static int dim_name_index(Dim d) {
    if (!capdnm) return -1;
    size_t j = dnm_slot(d, capdnm);
    while (DNM[j].d) { if (DNM[j].d == d) return DNM[j].nm; j = (j + 1) & (capdnm - 1); }
    return -1;
}
static const char *dim_name(Dim d) {
    for (int i = ndshow - 1; i >= 0; i--) if (dshow[i].d == d) return dshow[i].name;
    { int k = dim_name_index(d); if (k >= 0) return srcname_of(k); }
    char *b = dimbuf[dimbuf_i++ & 7]; snprintf(b, 24, "#%llu", (unsigned long long)d); return b;
}
static Cell *observe(Port prov) {
    Cell *r = cell(O_ROOT, 1);
    link(prov, P(r, 0));
    if (!net_observe(r, -1)) return NULL;
    return PC(net_peek(r->port[0]));
}
static long *unfolding; static size_t capunfold; static int nunfold;
static void rb(FILE *o, Port prov);
static Port apply_fresh(Port f, long *name) {
    *name = fresh_var("x", vcount++);
    Cell *v = cell(V_NVAR, 2); v->a = (uint64_t)*name;
    Cell *u = cell(V_UNK, 1); link(P(u, 0), P(v, 1));
    Cell *a = cell(O_APP, 3);
    link(f, P(a, 0)); link(P(v, 0), P(a, 1));
    return P(a, 2);
}
/* Reading a value back runs on an explicit stack of tasks, so the depth of a
 * value is bounded by memory, not by the native stack. */
/* a field is read from its cell when its turn comes: until then its wire may
 * still be rewritten by the reduction of the fields before it */
enum { TK_PORT, TK_FIELD, TK_STR, TK_BINDER, TK_POPDIM, TK_POPUNFOLD, TK_DEPTH };
typedef struct { int kind; Port p; char *s; } Task;
static Task *tasks; static size_t captasks; static long ntasks;
static void push_task(int kind, Port p, char *s) { GROW(tasks, captasks, (size_t)ntasks + 1); tasks[ntasks++] = (Task){ kind, p, s }; }
static void push_str(const char *s) { push_task(TK_STR, 0, strdup(s)); }
static void push_port(Port p) { push_task(TK_PORT, p, NULL); }
static void push_field(Cell *c, int k) { push_task(TK_FIELD, P(c, k), NULL); }
static void push_binder(Cell *c, int k) { push_task(TK_BINDER, P(c, k), NULL); }
/* "(h f1 f2 ... )" over ports from..to-1 */
static void push_fields_to(FILE *o, const char *h, Cell *c, int from, int to) {
    fprintf(o, "(%s", h);
    push_str(")");
    for (int k = to - 1; k >= from; k--) { push_field(c, k); push_str(" "); }
}
static void push_fields(FILE *o, const char *h, Cell *c, int from) { push_fields_to(o, h, c, from, c->n); }
static char *dnf_string(const Dnf *d, bool face) {
    char *buf; size_t len; FILE *m = open_memstream(&buf, &len);
    dnf_print(m, d, face, dim_name); fclose(m);
    return buf;
}
static int obs_depth;
long rb_nodes;   /* cells of the value read back: its size, reported beside the steps */
static void rb_one(FILE *o, Port prov) {
    Cell *c = observe(prov);
    if (!c) { fputs("<pending>", o); return; }
    /* the size counts the value's cells, not the references and unbuilt subterms it is read through */
    if (c->kind != V_REF && c->kind != V_THUNK && c->kind != V_DEFER) rb_nodes++;
    long nm;
    if (obs_depth > 0 && (c->kind == V_NVAR || c->kind == V_NAPP || c->kind == V_NPAPP) && (c->ty & TY_COFIX)) {
        /* (observe n t): a stuck call unfolded, its fields read one level less deep */
        Cell *u = cell(O_UNFOLD, 2);
        link(P(c, 0), P(u, 0));
        push_task(TK_DEPTH, (Port)obs_depth, NULL);
        obs_depth--;
        push_port(P(u, 1));
        return;
    }
    switch (c->kind) {
    case V_LAM: {
        Port b = apply_fresh(P(c, 0), &nm);
        fprintf(o, "(lam %s ", name_of(nm));
        push_str(")"); push_port(b);
        return;
    }
    case V_PI: case V_SIGMA:
        fprintf(o, "(%s ", c->kind == V_PI ? "Pi" : "Sigma");
        push_binder(c, 2); push_field(c, 1);
        return;
    case V_PAIR: push_fields(o, "pair", c, 1); return;
    case V_U: {
        /* U at lzero reads as U: the level is observed (through references to it) before it is
         * compared, so the reading does not depend on how the level is held */
        extern Port ref_copy(Cell *c);
        Cell *lc = observe(c->port[1]);
        while (lc && lc->kind == V_REF) lc = observe(ref_copy(lc));
        if (lc && lc->kind == V_CON && !strcmp(CON[lc->a].name, "lzero")) { fputs("U", o); return; }
        fputs("(U ", o); push_str(")");
        if (lc) push_port(P(lc, 0)); else push_str("<pending>");
        return;
    }
    case V_PATHP: push_fields(o, "PathP", c, 1); return;
    case V_PLAM: {
        GROW(dshow, capdshow, (size_t)ndshow + 1);
        DimShow *d = &dshow[ndshow++]; d->d = (Dim)c->a; snprintf(d->name, sizeof d->name, "i%d", dcount++);
        fprintf(o, "(plam %s ", d->name);
        push_task(TK_POPDIM, 0, NULL); push_str(")"); push_field(c, 1);
        return;
    }
    case V_IV: dnf_print(o, c->p, false, dim_name); return;
    case V_FACE: dnf_print(o, c->p, true, dim_name); return;
    case V_SYS: {
        Dnf *cl = c->p;
        fputs("(sys", o);
        push_str(")");
        for (int k = c->n - 1; k >= 1; k--) {
            char *buf; size_t len; FILE *m = open_memstream(&buf, &len);
            fputs(" (", m); clause_print(m, cl->c[k - 1], dim_name); fputc(' ', m); fclose(m);
            push_str(")"); push_field(c, k); push_task(TK_STR, 0, buf);
        }
        return;
    }
    case V_GLUE: push_fields(o, c->x ? "%hcompU" : "Glue", c, 1); return;
    case V_GLUEV: push_fields(o, "glue", c, 1); return;
    case V_NVAR: if (c->x) fputs(srcname_of((int)c->x - 1), o); else fputs(name_of((long)c->a), o); return;
    case V_NAPP:
        fputc('(', o);
        push_str(")"); push_field(c, 2); push_str(" "); push_field(c, 1);
        return;
    case V_NFST: push_fields_to(o, "fst", c, 1, 2); return;
    case V_NPROJ: push_fields_to(o, FLD[c->a].name, c, 1, 2); return;
    case V_REC: {
        /* a copattern: (record (f t)...) */
        DataInfo *D = &DATA[c->a];
        fputs("(record", o);
        push_str(")");
        for (int k = c->n - 2; k >= 0; k--) {
            char b[256]; snprintf(b, sizeof b, " (%s ", k < D->nflds ? FLD[D->flds[k]].name : "?");
            push_str(")"); push_field(c, k + 1); push_str(b);
        }
        return;
    }
    case V_NSND: push_fields_to(o, "snd", c, 1, 2); return;
    case V_NPAPP: {
        fputs("(@ ", o);
        push_str(")");
        if (c->x) { push_field(c, 3); push_str(" "); push_field(c, 2); push_str(" "); }
        char *r = dnf_string(c->p, false);
        push_task(TK_STR, 0, r); push_str(" "); push_field(c, 1);
        return;
    }
    case V_NCOMP: push_fields_to(o, "comp", c, 1, 4); return;
    case V_NUNGLUE: push_fields_to(o, c->x ? "%unglueU" : "unglue", c, 1, 3); return;
    case V_THUNK: { extern Port thunk_build(Cell *c); push_port(thunk_build(c)); return; }
    case V_DEFER: { extern Port unbuilt_build(Cell *c); push_port(unbuilt_build(c)); return; }
    case V_UNK: fputs("?", o); return;
    case V_DATA: if (c->n == 1) fputs(DATA[c->a].name, o); else push_fields(o, DATA[c->a].name, c, 1); return;
    case V_CON: if (c->n == 1) fputs(CON[c->a].name, o); else push_fields(o, CON[c->a].name, c, 1); return;
    case V_HCOMP: push_fields_to(o, "hcompD", c, 1, 3); return;
    case V_TRANSH: { extern Port transh_expand(Cell *c); push_port(transh_expand(c)); return; }
    case V_CASEF: fputs("%casef", o); return;
    case V_WORD: { WN w = { c->a, c->p }; fputc('#', o); wn_print(o, &w); return; }
    case V_WTY: fputs("Word", o); return;
    case V_PTY:
        if (c->a == PT_BYTES) { fputs("(Bytes ", o); push_str(")"); push_field(c, 1); return; }
        fputs(c->a == PT_I64 ? "Int64" : c->a == PT_U32 ? "UInt32" : c->a == PT_F64 ? "Float64" : c->a == PT_U64 ? "UInt64" : "Float32", o); return;
    case V_PVAL: {
        /* a primitive value: #i64:n, #u32:n, #f64:x */
        if (c->x == PT_I64) fprintf(o, "#i64:%lld", (long long)(int64_t)c->a);
        else if (c->x == PT_U32) fprintf(o, "#u32:%u", (unsigned)c->a);
        else if (c->x == PT_U64) fprintf(o, "#u64:%llu", (unsigned long long)c->a);
        else if (c->x == PT_F32) { float f; uint32_t b32 = (uint32_t)c->a; memcpy(&f, &b32, 4); fprintf(o, "#f32:%.9g", (double)f); }   /* a Float32 is its 32 bits (f32ofbits) */
        else { double d; uint64_t bits = c->a; memcpy(&d, &bits, 8); fprintf(o, "#f64:%.17g", d); }
        return;
    }
    case V_BYTES: {
        Bytes *b = c->p; fprintf(o, "#bytes:%zu:", b ? b->n : 0);
        for (size_t i = 0; b && i < b->n && i < 32; i++) fprintf(o, "%02x", b->b[i]);
        if (b && b->n > 32) fputs("…", o);
        return;
    }
    case V_WPLUS: {
        /* k successors of x: (%wsucc x), or (%wadd x #k) */
        WN w = { c->a, c->p };
        if (!c->p && c->a == 1) { fputs("(%wsucc ", o); push_str(")"); push_field(c, 1); return; }
        char *buf; size_t len; FILE *m = open_memstream(&buf, &len); fputs(" #", m); wn_print(m, &w); fputc(')', m); fclose(m);
        fputs("(%wadd ", o); push_task(TK_STR, 0, buf); push_field(c, 1);
        return;
    }
    case V_NWOP: {
        static const char *nm[] = { "%weq", "%wlt", "%wadd", "%wmax", "%wsub", "%wmul", "%wdiv", "%wmod", "%wand", "%wor", "%wxor", "%wshl", "%wshr" };
        push_fields_to(o, nm[c->x - O_WEQ], c, 1, 3);
        return;
    }
    case V_NWELIM: push_fields_to(o, "%welim", c, 1, 4); return;
    case V_NPRIM: {
        /* a primitive operation stuck on a neutral: the operation at its arguments */
        static const char *PO1[] = { "%i64add", "%i64sub", "%i64mul", "%i64eq", "%i64lt", "%u32add", "%u32sub", "%u32mul", "%u32eq", "%u32lt", "%f64add", "%f64sub", "%f64mul", "%f64div", "%f64eq", "%f64lt",
            "%u64add", "%u64sub", "%u64mul", "%u64eq", "%u64lt", "%u64div", "%u64mod", "%u64and", "%u64or", "%u64xor", "%u64shl", "%u64shr",
            "%i64div", "%i64mod", "%i64and", "%i64or", "%i64xor", "%i64shl", "%i64shr",
            "%i64ofw", "%wofi64", "%u32ofw", "%wofu32", "%f64ofw", "%f64ofi64", "%i64off64", "%f64bits", "%f64ofbits", "%f32bits", "%f32ofbits", "%u64ofw", "%wofu64" };
        int cls = (int)(c->x >> 8), sb = (int)(c->x & 255); char nm[32]; const char *name;
        if (cls <= 2) name = sb < PO_N ? PO1[sb] : "%op";
        else if (cls == 3 || cls == 4) { snprintf(nm, sizeof nm, "%%%s%d", cls == 3 ? "load" : "store", sb * 8); name = nm; }
        else name = cls == 5 ? "%blen" : "%bytes";
        push_fields_to(o, name, c, 1, c->n);
        return;
    }
    case V_WSUSP: { extern Port wsusp_force(Cell *c); push_port(wsusp_force(c)); return; }
    case V_REF: { extern Port ref_copy(Cell *c); push_port(ref_copy(c)); return; }
    case V_SUSP:
        /* an unforced induction hypothesis */
        fputs("(rec ", o);
        push_str(")"); push_field(c, 1);
        return;
    case V_NCASE: {
        /* (case x ((C v...) body)...), or (elim P x ...): each branch built on fresh variables */
        CaseTab *tb = c->p;
        int ne = tb->nenv, N = DATA[tb->d].ncons;
        Port ec[ne + 1][N + 1];
        for (int e = 0; e < ne; e++) port_split(c->port[2 + (c->x ? 1 : 0) + e], N, ec[e]);
        fputs(c->x ? "(elim " : "(case ", o);
        push_str(")");
        for (int k = N - 1; k >= 0; k--) {
            ConInfo *ci = &CON[DATA[tb->d].cons[k]];
            int nf = ci->nfields, nd = ci->ndims, nr = (tb->mask >> k & 1) ? ci->nrec : 0, np = nf + nr + nd;
            Port prm[np + 1], env[ne + 1];
            char pat[1024]; int at = snprintf(pat, sizeof pat, " ((%s", ci->name);
            for (int j = 0; j < np; j++) {
                if (j >= nf + nr) {
                    Dim dd = fresh_dim();
                    Cell *iv = cell(V_IV, 1); iv->p = iv_atom(dd); prm[j] = P(iv, 0);
                    at += snprintf(pat + at, sizeof pat - (size_t)at, " %s", dim_name(dd));
                } else {
                    long nm = fresh_var(j < nf ? "x" : "ih", vcount++);
                    Cell *v = cell(V_NVAR, 2); v->a = (uint64_t)nm;
                    Cell *u = cell(V_UNK, 1); link(P(u, 0), P(v, 1));
                    prm[j] = P(v, 0);
                    if (j < nf) at += snprintf(pat + at, sizeof pat - (size_t)at, " %s", name_of(nm));
                }
                if (at >= (int)sizeof pat) at = (int)sizeof pat - 1;
            }
            snprintf(pat + at, sizeof pat - (size_t)at, ") ");
            for (int e = 0; e < ne; e++) env[e] = ec[e][k];
            /* held until its turn: earlier readings run the net, and an operation's result must have
             * a wire to arrive on */
            Cell *hold = cell(O_ROOT, 1); link(record_build_n(tb->br[k], prm, env), P(hold, 0));
            push_str(")"); push_field(hold, 0); push_str(pat);
        }
        push_field(c, 1);
        if (c->x) { push_str(" "); push_field(c, 2); }
        return;
    }
    case V_NTRANS:
        /* (%transD D ψ lines... x) */
        fprintf(o, "(%%transD %s", DATA[c->a].name);
        push_str(")"); push_field(c, 1); push_str(" ");
        for (int k = c->n - 2; k >= 2; k--) { push_field(c, k); push_str(" "); }
        push_task(TK_STR, 0, dnf_string(c->p, true)); push_str(" ");
        return;
    case V_GLOBAL: {
        /* a definition reads back as its value; within its own value it is its name */
        long g = (long)c->a;
        for (int i = 0; i < nunfold; i++) if (unfolding[i] == g) { fputs(name_of(g), o); return; }
        Port v = global_copy(g, NULL);
        if (!v) { fputs(name_of(g), o); return; }
        GROW(unfolding, capunfold, (size_t)nunfold + 1);
        unfolding[nunfold++] = g;
        push_task(TK_POPUNFOLD, 0, NULL); push_port(v);
        return;
    }
    }
    fputs("<?>", o);
}
static void rb(FILE *o, Port prov) {
    long base = ntasks;
    push_port(prov);
    while (ntasks > base) {
        Task t = tasks[--ntasks];
        switch (t.kind) {
        case TK_STR: fputs(t.s, o); free(t.s); break;
        case TK_POPDIM: ndshow--; break;
        case TK_POPUNFOLD: nunfold--; break;
        case TK_DEPTH: obs_depth = (int)t.p; break;
        case TK_FIELD: rb_one(o, PC(t.p)->port[PI(t.p)]); break;
        case TK_BINDER: {
            long nm;
            Port b = apply_fresh(PC(t.p)->port[PI(t.p)], &nm);
            fprintf(o, " %s ", name_of(nm));
            push_str(")"); push_port(b);
            break;
        }
        case TK_PORT: rb_one(o, t.p); break;
        }
    }
}
static char *read_port(Port prov) {
    char *buf; size_t len;
    FILE *o = open_memstream(&buf, &len);
    ndshow = 0; dcount = 0; vcount = 0; rb_nodes = 0;
    rb(o, prov);
    fclose(o);
    return buf;
}
/* ---------------- printing a code ----------------
 * A typed reading (quote in lib/check.cub) is a code of lib/code.cub: first-order data, with
 * bound variables and dimensions as de Bruijn indices. It is printed with names made in the
 * order the binders are printed, x0 x1 ... for variables (ih for an elimination's hypotheses)
 * and i0 i1 ... for dimensions, as a reading names them. */
extern Port ref_copy(Cell *c);
extern Port thunk_build(Cell *v);
static Cell *obs_val(Port p) {
    for (;;) {
        Cell *c = observe(p);
        if (!c) return NULL;
        if (c->kind == V_REF) { p = ref_copy(c); continue; }
        if (c->kind == V_THUNK) { p = thunk_build(c); continue; }
        if (c->kind == V_DEFER) { extern Port unbuilt_build(Cell *c); p = unbuilt_build(c); continue; }
        if (c->kind == V_GLOBAL) { Port v = global_copy((long)c->a, c->key); if (!v) return c; p = v; continue; }
        return c;
    }
}
static const char *con_of(Cell *c) { return c && c->kind == V_CON ? CON[c->a].name : ""; }
static long word_val(Port p) { Cell *c = obs_val(p); return c && c->kind == V_WORD && !c->p ? (long)c->a : -1; }
static char **cnm; static size_t capcnm; static int ncnm, cvc, cdc;
static void cnm_push(const char *pfx, int k) { GROW(cnm, capcnm, (size_t)ncnm + 1); char b[32]; snprintf(b, sizeof b, "%s%d", pfx, k); cnm[ncnm++] = strdup(b); }
static void cnm_pop(int k) { while (k-- > 0) free(cnm[--ncnm]); }
/* the elements of a list (Lst) */
static int list_ports(Port p, Port **out) {
    int n = 0, cap = 8; Port *xs = malloc((size_t)cap * sizeof(Port));
    for (;;) {
        Cell *c = obs_val(p);
        if (!c || strcmp(con_of(c), "lcons")) break;
        if (n == cap) { cap *= 2; xs = realloc(xs, (size_t)cap * sizeof(Port)); }
        xs[n++] = c->port[1]; p = c->port[2];
    }
    *out = xs; return n;
}
static void pcode(FILE *o, Port p);
/* a right-nested chain of one binary constructor, printed n-ary */
static void pchain(FILE *o, const char *h, const char *op, Cell *c) {
    fprintf(o, "(%s", op);
    for (;;) {
        fputc(' ', o); pcode(o, c->port[1]);
        Cell *r = obs_val(c->port[2]);
        if (r && !strcmp(con_of(r), h)) { c = r; continue; }
        fputc(' ', o); pcode(o, P(r, 0));
        break;
    }
    fputc(')', o);
}
static void pfields(FILE *o, Port *xs, int n) { for (int k = 0; k < n; k++) { fputc(' ', o); pcode(o, xs[k]); } }
static void pbranches(FILE *o, int d, Port bs) {
    Port *br; int nb = list_ports(bs, &br);
    int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
    for (int k = 0; k < nb; k++) {
        ConInfo *ci = D >= 0 && k < DATA[D].ncons ? &CON[DATA[D].cons[k]] : NULL;
        int nf = ci ? ci->nfields : 0, nr = ci ? ci->nrec : 0, nd = ci ? ci->ndims : 0, pushed = 0;
        fprintf(o, " ((%s", ci ? ci->name : "?");
        Port b = br[k];
        int recf[64], nrf = 0;   /* the recursive fields' positions in the name stack */
        for (int j = 0; j < nf + nr + nd; j++) {
            Cell *c = obs_val(b);
            if (j < nf) { cnm_push("x", cvc++); fprintf(o, " %s", cnm[ncnm - 1]); if (ci && (ci->recmask >> j & 1) && nrf < 64) recf[nrf++] = ncnm - 1; }
            else if (j < nf + nr) {
                /* a hypothesis: the elimination of its recursive field, written (rec x) */
                int q = j - nf; char b2[64]; snprintf(b2, sizeof b2, "(rec %s)", q < nrf ? cnm[recf[q]] : "?");
                GROW(cnm, capcnm, (size_t)ncnm + 1); cnm[ncnm++] = strdup(b2); cvc++;
            }
            else { cnm_push("i", cdc++); fprintf(o, " %s", cnm[ncnm - 1]); }
            pushed++;
            b = c->port[1];
        }
        fputs(") ", o); pcode(o, b); fputc(')', o);
        cnm_pop(pushed);
    }
    free(br);
}
static void pcode(FILE *o, Port p) {
    Cell *c = obs_val(p);
    const char *h = con_of(c);
    if (!*h) { fputs("<?>", o); return; }
    Port *xs; int n;
    if (!strcmp(h, "cvar")) { long k = word_val(c->port[1]); fputs(k >= 0 && k < ncnm ? cnm[ncnm - 1 - k] : "<unbound>", o); }
    else if (!strcmp(h, "cfree")) rb(o, c->port[1]);
    else if (!strcmp(h, "cforce")) {
        /* a hypothesis used is named (rec x) */
        Cell *v = obs_val(c->port[1]);
        if (!strcmp(con_of(v), "cvar")) { long k = word_val(v->port[1]); fputs(k >= 0 && k < ncnm ? cnm[ncnm - 1 - k] : "<unbound>", o); }
        else { fputs("(force ", o); pcode(o, P(v, 0)); fputc(')', o); }
    }
    else if (!strcmp(h, "cglob")) { long k = word_val(c->port[1]); fputs(k >= 0 && k < nchecked ? CHECKED[k] : "<global>", o); }
    else if (!strcmp(h, "clam") || !strcmp(h, "cdlam")) {
        bool dim = h[1] == 'd'; cnm_push(dim ? "i" : "x", dim ? cdc++ : cvc++);
        fprintf(o, "(lam %s ", cnm[ncnm - 1]); pcode(o, c->port[1]); fputc(')', o); cnm_pop(1);
    }
    else if (!strcmp(h, "cplam") || !strcmp(h, "cforall")) {
        cnm_push("i", cdc++);
        fprintf(o, "(%s %s ", h[1] == 'p' ? "plam" : "forall", cnm[ncnm - 1]); pcode(o, c->port[1]); fputc(')', o); cnm_pop(1);
    }
    else if (!strcmp(h, "cpi")) {
        /* (cpi m A B): read back as the Π (its multiplicity is for #usage) */
        fputs("(Pi ", o); pcode(o, c->port[2]);
        cnm_push("x", cvc++); fprintf(o, " %s ", cnm[ncnm - 1]); pcode(o, c->port[3]); fputc(')', o); cnm_pop(1);
    }
    else if (!strcmp(h, "csigma")) {
        fputs("(Sigma ", o); pcode(o, c->port[1]);
        cnm_push("x", cvc++); fprintf(o, " %s ", cnm[ncnm - 1]); pcode(o, c->port[2]); fputc(')', o); cnm_pop(1);
    }
    else if (!strcmp(h, "clet")) {
        fputs("(let ", o); cnm_push("x", cvc++); fprintf(o, "%s ", cnm[ncnm - 1]); cnm_pop(1);
        pcode(o, c->port[1]); cnm_push("x", cvc - 1); fputc(' ', o); pcode(o, c->port[2]); fputc(')', o); cnm_pop(1);
    }
    else if (!strcmp(h, "capp")) { fputc('(', o); pcode(o, c->port[1]); fputc(' ', o); pcode(o, c->port[2]); fputc(')', o); }
    else if (!strcmp(h, "cuniv")) {
        Cell *l = obs_val(c->port[1]);
        if (!strcmp(con_of(l), "clzero")) fputs("U", o); else { fputs("(U ", o); pcode(o, P(l, 0)); fputc(')', o); }
    }
    else if (!strcmp(h, "cmeet")) pchain(o, h, "/\\", c);
    else if (!strcmp(h, "cjoin")) pchain(o, h, "\\/", c);
    else if (!strcmp(h, "cfand")) pchain(o, h, "&", c);
    else if (!strcmp(h, "cfor")) pchain(o, h, "|", c);
    else if (!strcmp(h, "ceq0") || !strcmp(h, "ceq1")) { fputs("(= ", o); pcode(o, c->port[1]); fprintf(o, " %c)", h[3]); }
    else if (!strcmp(h, "csys0") || !strcmp(h, "csysc")) {
        fputs("(sys", o);
        while (!strcmp(con_of(c), "csysc")) {
            fputs(" (", o); pcode(o, c->port[1]); fputc(' ', o); pcode(o, c->port[2]); fputc(')', o);
            c = obs_val(c->port[3]);
        }
        fputc(')', o);
    }
    else if (!strcmp(h, "cdata")) {
        long d = word_val(c->port[1]); int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
        n = list_ports(c->port[2], &xs);
        if (n) fputc('(', o);
        fputs(D >= 0 ? DATA[D].name : "?", o); pfields(o, xs, n); if (n) fputc(')', o); free(xs);
    }
    else if (!strcmp(h, "ccon")) {
        long d = word_val(c->port[1]), k = word_val(c->port[2]); int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
        Port *ds; n = list_ports(c->port[3], &xs); int m = list_ports(c->port[4], &ds);
        if (n + m) fputc('(', o);
        fputs(D >= 0 && k >= 0 && k < DATA[D].ncons ? CON[DATA[D].cons[k]].name : "?", o);
        pfields(o, xs, n); pfields(o, ds, m); if (n + m) fputc(')', o); free(xs); free(ds);
    }
    else if (!strcmp(h, "celim")) {
        long d = word_val(c->port[1]);
        fputs("(elim ", o); pcode(o, c->port[2]); fputc(' ', o); pcode(o, c->port[4]); pbranches(o, (int)d, c->port[3]); fputc(')', o);
    }
    else if (!strcmp(h, "ccase")) {
        long d = word_val(c->port[1]);
        fputs("(case ", o); pcode(o, c->port[3]); pbranches(o, (int)d, c->port[2]); fputc(')', o);
    }
    else if (!strcmp(h, "cproj")) {
        long d = word_val(c->port[1]), k = word_val(c->port[2]); int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
        fprintf(o, "(%s ", D >= 0 && k >= 0 && k < DATA[D].nflds ? FLD[DATA[D].flds[k]].name : "?"); pcode(o, c->port[3]); fputc(')', o);
    }
    else if (!strcmp(h, "crec")) {
        long d = word_val(c->port[1]); int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
        n = list_ports(c->port[2], &xs);
        fputs("(record", o);
        for (int k = 0; k < n; k++) { fprintf(o, " (%s ", D >= 0 && k < DATA[D].nflds ? FLD[DATA[D].flds[k]].name : "?"); pcode(o, xs[k]); fputc(')', o); }
        fputc(')', o); free(xs);
    }
    else if (!strcmp(h, "ctrans")) {
        long d = word_val(c->port[1]); int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
        fprintf(o, "(%%transD %s ", D >= 0 ? DATA[D].name : "?"); pcode(o, c->port[2]);
        n = list_ports(c->port[3], &xs); pfields(o, xs, n); free(xs);
        fputc(' ', o); pcode(o, c->port[4]); fputc(')', o);
    }
    else if (!strcmp(h, "cwlit")) { Cell *w = obs_val(c->port[1]); if (w && w->kind == V_WORD) { WN x = { w->a, w->p }; fputc('#', o); wn_print(o, &x); } else fputs("<?>", o); }
    else if (!strcmp(h, "cplit")) {
        /* as the value prints */
        long k = word_val(c->port[1]); Cell *w = obs_val(c->port[2]); uint64_t b = w && w->kind == V_WORD && !w->p ? w->a : 0;
        if (k == PT_I64) fprintf(o, "#i64:%lld", (long long)(int64_t)b);
        else if (k == PT_U32) fprintf(o, "#u32:%u", (unsigned)b);
        else if (k == PT_U64) fprintf(o, "#u64:%llu", (unsigned long long)b);
        else if (k == PT_F32) { float f; uint32_t b32 = (uint32_t)b; memcpy(&f, &b32, 4); fprintf(o, "#f32:%.9g", (double)f); }
        else { double d; memcpy(&d, &b, 8); fprintf(o, "#f64:%.17g", d); }
    }
    else if (!strcmp(h, "cpty")) { long k = word_val(c->port[1]); if (k == PT_BYTES) { fputs("(%bytesty ", o); pcode(o, c->port[2]); fputc(')', o); } else fprintf(o, "(%%pty #%ld)", k); }
    else if (!strcmp(h, "cwop")) {
        static const char *op[] = { "%weq", "%wlt", "%wadd", "%wmax", "%wsub", "%wmul", "%wdiv", "%wmod", "%wand", "%wor", "%wxor", "%wshl", "%wshr" };
        long k = word_val(c->port[1]);
        fprintf(o, "(%s ", k >= 0 && k < 13 ? op[k] : "?"); pcode(o, c->port[2]); fputc(' ', o); pcode(o, c->port[3]); fputc(')', o);
    }
    else {
        /* the rest print as their head and fields */
        static const struct { const char *c, *s; } F[] = {
            { "cpair", "pair" }, { "cfst", "fst" }, { "csnd", "snd" }, { "cpathp", "PathP" }, { "cpapp", "@" },
            { "cneg", "~" }, { "ccomp", "comp" }, { "cgluet", "Glue" }, { "cglue", "glue" }, { "cunglue", "unglue" }, { "chcompu", "%hcompU" }, { "cunglueu", "%unglueU" },
            { "cann", "the" }, { "chcomp", "hcompD" }, { "cforce", "force" }, { "cthunk", "delay" }, { "clsuc", "lsuc" }, { "clmax", "lmax" },
            { "cwsucc", "%wsucc" }, { "cwelim", "%welim" },
            { "ci0", "0" }, { "ci1", "1" }, { "cftt", "tt" }, { "cfff", "ff" }, { "clevel", "Level" }, { "clzero", "lzero" },
            { "cword", "Word" }, { "cclock", "Clock" }, { "ctick", "Tick" }, { "cclock0", "κ0" }, { "ctick0", "⋄" }, { NULL, NULL } };
        int f = 0; while (F[f].c && strcmp(F[f].c, h)) f++;
        if (!F[f].c) { fprintf(o, "<%s>", h); return; }
        if (c->n == 1) { fputs(F[f].s, o); return; }
        fprintf(o, "(%s", F[f].s);
        if (!strcmp(h, "cann")) { fputc(' ', o); pcode(o, c->port[2]); fputc(' ', o); pcode(o, c->port[1]); }
        else for (int k = 1; k < (strcmp(h, "cwelim") ? c->n : 4); k++) { fputc(' ', o); pcode(o, c->port[k]); }   /* (cwelim's motive is an Opt, not printed) */
        fputc(')', o);
    }
}
/* an (Opt Code): the printed code, or NULL for none */
static char *read_code(Port prov) {
    Cell *c = obs_val(prov);
    if (!c || strcmp(con_of(c), "some")) return NULL;
    char *buf; size_t len; FILE *o = open_memstream(&buf, &len);
    ndshow = 0; dcount = 0; vcount = 0; ncnm = 0; cvc = 0; cdc = 0;
    pcode(o, c->port[1]);
    fclose(o);
    return buf;
}
/* the type a reading is taken at: a checked definition's, or the declared type of (the T t) */
static Node *reading_type(Node *t) {
    if (!t->list) { int i = checked_index(t->s); return i >= 0 && global_lookup(t->s) == table_global(TB_VAL, i) ? nl2("%gty", word(i)) : NULL; }
    if (t->n == 3 && is(t->it[0], "the")) return t->it[1];
    return NULL;
}
/* A reading is taken over the complete normalization of what was posed
 * (5.13 concerns a normal form): pending pairs are reduced before and after,
 * so the counts do not depend on the schedule. */
/* t61 (2026-11-12): needs-work (goal work-object). Every evaluator result read back here (checks,
 * the elaborator's solving rounds via %chk-def / %infer-ok, usages, readings) lives only for this
 * run. Each is to be kept beyond it in canonical form: the query identified by its canonical key
 * (references as content keys, t60; the checker identified by its own definitions' keys, t42), the
 * result stored with references as keys and rendered through the current bindings when read, the
 * same mechanism as the store's definitions. The store's certificates are then such results; the
 * per-file cache (t41) goes. Read 2026-11-12: the query built here (build_top) is keyed in
 * src/net.c by synkey, but that key's leaves are run-local: a definition by its global index
 * (key_global), a λ or thunk by the address of its compiled record (K_LAM over t->rec), and the
 * codes passed name checked definitions by their index (cglob i). So the evaluator's key of a query
 * means nothing in another run. Two key schemes exist: the evaluator's (structural, run-local) and
 * the store's (canonical forms, content keys). The end state is one: every key canonical, compiled
 * records keyed by their content (t60). The first step can be taken here: the query's key formed as
 * the store forms a definition's (its canonical form, references as content keys, plus the
 * checker's identity), and the result stored canonically.
 * Read 2026-11-12, for the elaborator's case (src/syntax.c x_checked, the round loop): the right unit
 * is the whole elaboration of a declaration, its input the declaration's surface forms (sig, body)
 * with every name resolved to the key it denotes, its output the solved core forms T and t. The
 * final forms then go through `checked` as now, so the certificate check (key_def, store_checked)
 * still decides acceptance: a remembered elaboration can only change which term is checked, never
 * whether a term is accepted. What is missing is a faithful canonical serialization: canon (src/
 * store.c) is a key, not a serialization. It drops the type written on a λ's binder (bnd_ty, kept on
 * the name node), instance flags and binder names, and prints a λ's binders so their count cannot be
 * read back. The store needs one canonical form that is both the key and readable back (references
 * as keys, binders nameless but counted, binder types and flags kept), rendered through the current
 * bindings when read; with it canon is that form's hash and nothing is duplicated. */
static char *readback(Node *t, long *steps) {
    net_drain();
    long s0 = stat_steps;
    char *buf = read_port(build_top(t));
    net_drain();
    if (steps) *steps = stat_steps - s0;
    return buf;
}
/* a reading at a type T: the normal form at T (quote), whose length includes the quoting; a value
 * with no code is read without its type */
static char *readback_at(Node *T, Node *t, long *steps) {
    net_drain();
    long s0 = stat_steps;
    char *buf = read_code(build_top(nl3("%%quote-top", T, t)));
    net_drain();
    if (!buf) buf = read_port(build_top(t));
    net_drain();
    if (steps) *steps = stat_steps - s0;
    return buf;
}

/* modules: a file is loaded once; a running key hashes the machine's build and every file loaded so
 * far, in order; a file whose key has a marker in the cache directory passed all its checks
 * before, and its checks are not rerun */
static bool trusted;
static const char *cur_dir;
/* a checked definition whose check the elaborator already ran: its result and length */
static const char *forced_result; static long forced_steps;
/* why a judgment failed: its normal form (no v why A B), lib/check.cub Res */
/* the slots of a failure that are types (read at their universe); the others are values whose
 * type the failure does not carry, read without one */
static bool slot_is_type(int why, int k) {
    switch (why) {
    case 0: case 21: return true;
    case 1: case 2: case 3: case 4: case 20: case 23: case 27: case 30: case 32: case 34: return k == 0;
    default: return false;
    }
}
static char *read_code(Port prov);
static Cell *obs_val(Port p);
static const char *con_of(Cell *c);
/* results in the language of the file being run: a surface file's in the surface syntax */
static bool out_surface;
static char *surface_str(Node *t);
static void out_term(Node *t) {
    if (!out_surface) { print_node(stdout, t); return; }
    char *s = surface_str(t); fputs(s, stdout); free(s);
}
/* a read-back term as a source term: the readback writes Π and Σ as (Pi A x B) */
/* suc (suc ... zero), two or more, as the decimal symbol the printer writes it as: found here, by a
 * walk, so the printer's recursion never descends a numeral (a value is as deep as it is large) */
static Node *numeral_of(Node *t) {
    long k = 0; Node *x = t;
    while (x->list && x->n == 2 && !x->it[0]->list && is(x->it[0], "suc")) { k++; x = x->it[1]; }
    if (k < 2 || x->list || !is(x, "zero")) return NULL;
    char b[32]; snprintf(b, sizeof b, "%ld", k);
    return nsym(b);
}
static Node *rb_src(Node *t) {
    if (!t->list || t->n == 0) return t;
    if (con_lookup("suc") >= 0) { Node *m = numeral_of(t); if (m) return m; }
    /* each list is copied, its sublists' copies made at once (so its swap can be done now) and
     * filled later from a work stack: no recursion down a deep value */
    typedef struct { Node *t, *r; } W;
    W *st = NULL; size_t cap = 0; int d = 0;
    Node *root = nlist(t->n);
    GROW(st, cap, 1); st[d++] = (W){ t, root };
    bool nats = con_lookup("suc") >= 0;
    while (d) {
        W w = st[--d];
        for (int i = 0; i < w.t->n; i++) {
            Node *c = w.t->it[i];
            if (!c->list || c->n == 0) w.r->it[i] = c;
            else if (nats && (w.r->it[i] = numeral_of(c))) continue;
            else { Node *rc = nlist(c->n); w.r->it[i] = rc; GROW(st, cap, (size_t)d + 1); st[d++] = (W){ c, rc }; }
        }
        if (w.t->n == 4 && !w.t->it[0]->list && (is(w.t->it[0], "Pi") || is(w.t->it[0], "Sigma")) && !w.t->it[2]->list) { Node *a = w.r->it[1]; w.r->it[1] = w.r->it[2]; w.r->it[2] = a; }
    }
    free(st);
    return root;
}
/* a read-back term (text) */
static void out_str(const char *r) {
    if (!out_surface) { fputs(r, stdout); return; }
    const char *saved = src; src = r;
    Node *n = read_node();
    src = saved;
    if (!n) { fputs(r, stdout); return; }
    out_term(rb_src(n));
}
/* q: the judgment that failed, run again to read its type slots at their universe */
/* the position of the declaration being processed (file:line, set by the surface parser; item 30) */
char decl_pos[512];
static void explain(const char *r, Node *q) {
    if (decl_pos[0]) printf("   in: %s\n", decl_pos);
    /* reason, then what its two values are */
    static const char *M[][3] = {
        /* 0 */ { "type mismatch", "expected", "actual" },
        { "applied, but not a function", "its type", NULL },
        { "projected, but not a pair", "its type", NULL },
        { "applied to an interval, but not a path", "its type", NULL },
        { "not a type", "its type", NULL },
        { "no inferable type here: annotate it with (the T t)", NULL, NULL },
        { "not an interval (a dimension, 0, 1, ~, /\\, \\/)", NULL, NULL },
        { "wrong endpoint at 0", "expected", "actual" },
        { "wrong endpoint at 1", "expected", "actual" },
        { "system branches disagree where both faces hold", "one", "other" },
        /* 10 */ { "the tube disagrees with the base at 0", "base", "tube" },
        { "a face of the tube depends on the composition's dimension", NULL, NULL },
        { "not a face: (= r 0), (= r 1), &, |, tt, ff, forall", NULL, NULL },
        { "a system (sys ...) was expected", NULL, NULL },
        { "the parameters do not match the data type's telescope", NULL, NULL },
        { "the fields and dimensions do not match the constructor's", NULL, NULL },
        { "the elimination has the wrong number of branches", NULL, NULL },
        { "the branch disagrees with the elimination of the constructor's boundary", "expected", "branch" },
        { "the glue system's face differs from the Glue type's", NULL, NULL },
        { "glue's system disagrees with its base under a branch's face", "base", "branch" },
        /* 20 */ { "eliminated, but not of that data type", "its type", NULL },
        { "the motive is not a family of types over the data type", "motive's type", "data type" },
        { "a line (plam i ...) was expected", NULL, NULL },
        { "a type over levels (in U omega) has no type and no level", "its type", NULL },
        { "the field type is not strictly positive (the type itself at its own parameters, or not mentioning it)", NULL, NULL },
        { "the boundary uses a constructor not declared before this one", NULL, NULL },
        { "the data type's universe level depends on a parameter", NULL, NULL },
        { "ungluded, but not of a Glue type", "its type", NULL },
        /* 28 */ { "a corecursive call is not guarded: a definition of the group may stand only as the head of a field's term (applied to arguments not mentioning the group), or there under a λ, a line, a pair, a constructor, a case or system branch, or a let's body", NULL, NULL },
        { "a record (copattern) stands only as the body of a definition, after its λs and lines", NULL, NULL },
        { "projected by a field, but not of that codata type", "its type", NULL },
        { "the field type is not strictly positive in the codata type (the type itself at any parameters not mentioning it, under Π codomains and Σ components, or not mentioning it)", NULL, NULL },
        /* 32 */ { "an implicit argument is not determined by the other arguments' types or the type expected", "its type", NULL },
        /* 33 */ { "applied to a tick, but it uses a variable bound after the tick (a later value used now)", NULL, NULL },
        /* 34 */ { "a tick here is not a tick variable (ticks are bound by λ only)", "its type", NULL },
        /* 35 */ { "clocks and ticks are not types in a universe (only the domain of a Π)", NULL, NULL },
        /* 36 */ { "a formal composition in a type without path constructors must cover every closed point (its tube's face has a complement)", "the type", NULL },
        /* 37 */ { "a binder at multiplicity 0 (erased) is used", NULL, NULL },
        /* 38 */ { "a linear binder (multiplicity 1) is not used", NULL, NULL },
        /* 39 */ { "a linear binder (multiplicity 1) is used more than once", NULL, NULL },
        /* 40 */ { "a constructor of a linear type (a field at multiplicity 1) is applied outside a primitive", NULL, NULL } };
    const char *saved = src; src = r;
    Node *n = read_node();
    src = saved;
    if (!n || !n->list || n->n != 5 || !is(n->it[0], "refused")) { printf("   reason: %s\n", r); return; }
    int why = n->it[2]->list ? -1 : atoi(n->it[2]->s + 1);
    bool known = why >= 0 && why < (int)(sizeof M / sizeof *M);
    if (!is(n->it[1], "opq")) {
        /* the location is the term's shape as a code: shown as source when it reads as one */
        Node *at = n->it[1], *sfc = NULL;
        if (at->list && at->n >= 1 && !at->it[0]->list && at->it[0]->s[0] == 'c') { ncn = 0; sfc = cn(at); }
        printf("   at: "); out_term(sfc ? sfc : rb_src(at)); printf("\n");
    }
    printf("   reason: %s\n", known ? M[why][0] : "rejected");
    if (why == 28) {
        /* the field whose term is not guarded: (no opq 28 d k), d the codata type, k the field */
        long d = n->it[3]->list ? -1 : atol(n->it[3]->s + 1), k = n->it[4]->list ? -1 : atol(n->it[4]->s + 1);
        int D = d >= 0 && d < nchkdata ? CHKDATA[d] : -1;
        printf("   field: %s\n", D >= 0 && k >= 0 && k < DATA[D].nflds ? FLD[DATA[D].flds[k]].name : "?");
        return;
    }
    char *typed[2] = { NULL, NULL };
    if (q && known && (slot_is_type(why, 0) || slot_is_type(why, 1))) {
        net_drain();
        Cell *c = obs_val(build_top(q));
        if (c && !strcmp(con_of(c), "refused"))
            for (int k = 0; k < 2; k++) if (slot_is_type(why, k)) {
                Cell *a = cell(O_APP, 3);
                link(build_top(nsym("%%quote-ty")), P(a, 0)); link(c->port[3 + k], P(a, 1));
                typed[k] = read_code(P(a, 2));
            }
        net_drain();
    }
    for (int k = 0; k < 2; k++) {
        Node *v = n->it[3 + k];
        if (is(v, "opq")) { free(typed[k]); continue; }
        printf("   %s: ", known && M[why][1 + k] ? M[why][1 + k] : (k ? "B" : "A"));
        if (typed[k]) { out_str(typed[k]); free(typed[k]); } else out_term(rb_src(v));
        printf("\n");
    }
}

/* ---------------- the driver ---------------- */
static int failures, passes;
static void toplevel(Node *f);
/* top-level forms written by the system (a data declaration's definitions) */
static void emit(const char *text) {
    if (getenv("CUBICAL_SHOW_EMIT")) fprintf(stderr, "%s\n", text);
    const char *saved = src; bool sv = x_surface;
    src = text; x_surface = false;
    Node *f;
    while ((f = read_node())) toplevel(f);
    src = saved; x_surface = sv;
}
static char **watch_name; static Cell **watch_root; static size_t capwn, capwr; static int nwatch;
static void load_file(const char *path, bool must);
static const char *cache_dir(void);
static void drivers_run(Node *t);
/* ---------------- signature entries, copattern definitions ---------------- */
/* an entry of the checker's signature: its value's global, its type's global */
static int sig_add(const char *name, long g, long tyg, bool hidden) {
    /* a checked name declared again rebinds the references of every module parsed after it,
     * silently (E28): until modules give namespaces, it is an error, except under #fail
     * (CUBICAL_REDECLARE=1 allows it, for experiments) */
    if (!hidden && name[0] != '%' && checked_index(name) >= 0 && !getenv("CUBICAL_REDECLARE")) die("a checked definition is declared again (rename it; the earlier one is the library's):", name);
    table_set(TB_VAL, nchecked, g);
    table_set(TB_TY, nchecked, tyg);
    GROW(CHECKED, capchecked, (size_t)nchecked + 1);
    GROW(CHKHIDE, capchkhide, (size_t)nchecked + 1);
    CHECKED[nchecked] = intern(name); CHKHIDE[nchecked] = hidden;
    GROW(CHKPOS, capchkpos, (size_t)nchecked + 1); CHKPOS[nchecked] = decl_pos[0] ? strdup(decl_pos) : NULL;
    if (g >= 0) G[g].chk = nchecked;
    return nchecked++;
}
#include "store.c"
/* the SHA-256 of a record's body as rec_eq compares it: every node's tag, operand count, slot, x,
 * id and word, and the records and tables it names by pointer (those keep their pointers while
 * a dropped record names them) */
static void syn_digest(Sha256 *h, const Syn *t) {
    if (!t) { unsigned char z = 0xFF; sha256_update(h, &z, 1); return; }
    struct { int tag, n, slot; uint32_t x; long id; const void *rec, *tab; uint32_t bn; } f;
    memset(&f, 0, sizeof f);
    f.tag = t->tag; f.n = t->n; f.slot = t->slot; f.x = t->x; f.id = t->id; f.rec = t->rec; f.tab = t->tab;
    f.bn = t->big ? t->big->n : 0;
    sha256_update(h, &f, sizeof f);
    if (t->big) sha256_update(h, t->big->l, (size_t)t->big->n * sizeof(uint64_t));
    for (int i = 0; i < t->n; i++) syn_digest(h, t->k[i]);
}
static void rec_digest(const Record *r, unsigned char out[32]) {
    Sha256 h; sha256_init(&h);
    syn_digest(&h, r->body);
    sha256_final(&h, out);
}
/* an unchecked definition (a library printer, the specializer) has no key: the libraries loaded
 * before the file stand for it (an unchecked definition of the file itself: the file's text too) */
/* a copattern definition: λs and lines over a record (desugared) */
static bool copattern_body(Node *b) {
    for (;;) {
        if (b->list && b->n == 4 && is(b->it[0], "%guess")) { b = b->it[3]; continue; }
        if (b->list && b->n == 3 && (is(b->it[0], "lam") || is(b->it[0], "plam"))) { b = b->it[2]; continue; }
        break;
    }
    return b->list && b->n >= 2 && is(b->it[0], "%record");
}
static bool plain_def;   /* a copattern definition's body: defined as it is */
/* a copattern definition's name, before its body (which may refer to it) */
static long cofix_declare(const char *name, long tyg) {
    long g = global_lookup(name);
    if (g >= 0 && G[g].kind == G_INPUT && !G[g].res->port[0]) G[g].kind = G_COFIX;
    else g = add_global(name, G_COFIX);
    G[g].tyg = tyg;
    return g;
}
/* its body, a definition of its own, and its value: the stuck head, which projection unfolds */
static void cofix_body(long g, Node *t) {
    char *bn = malloc(strlen(G[g].name) + 16); sprintf(bn, "%%cobody-%s", G[g].name);
    Node *d = nlist(3); d->it[0] = nsym("def"); d->it[1] = nsym(bn); d->it[2] = t;
    bool saved = plain_def; plain_def = true; toplevel(d); plain_def = saved;
    G[g].unf = global_lookup(bn); free(bn);
    link(cofix_head(g, G[g].tyg), P(G[g].res, 0));
    for (int i = 0; i < G[g].npark; i++) net_push(G[g].park_o[i], G[g].park_v[i]);
    G[g].npark = 0;
}
/* a rejected definition: no name refers to it any more */
static void cofix_hide(long g, int ix) {
    CHKHIDE[ix] = true;
    char *nm = malloc(strlen(G[g].name) + 16); sprintf(nm, "%%rejected-%s", G[g].name);
    G[g].name = nm;
}
/* copattern definitions checked together (one, or a mutual group): each type is checked to be a
 * type, each body against its type, and every call of the group in the bodies is guarded (the
 * checker's chk-codef). Each is declared, at its type, before any body is read */
static void checked_group(Node **fs, int n, bool expect) {
    long *gs = calloc((size_t)n + 1, sizeof(long)); int *ix = calloc((size_t)n + 1, sizeof(int));
    Node **cT = calloc((size_t)n + 1, sizeof(Node *));
    for (int i = 0; i < n; i++) {
        Node *f = fs[i];
        if (!f->list || f->n != 4 || f->it[1]->list) die("(checked name T t)", NULL);
        if (!copattern_body(ds(f->it[3]))) die("a definition of a corecursive group is λs and lines over a record (record (f t)...):", f->it[1]->s);
        cT[i] = nl2("%raw", aclosed(ds(f->it[2])));
    }
    for (int i = 0; i < n; i++) {
        char ty[32]; snprintf(ty, sizeof ty, "%%sty%d", nchecked);
        Node *td = nlist(3); td->it[0] = nsym("def"); td->it[1] = nsym(ty); td->it[2] = nl2("%%sig-type", cT[i]);
        toplevel(td);
        long tyg = global_lookup(ty);
        gs[i] = cofix_declare(fs[i]->it[1]->s, tyg);
        ix[i] = sig_add(fs[i]->it[1]->s, gs[i], tyg, false);
        /* the body as written (%gcode): the image translates a copattern definition from it, its
         * value being its stuck head (18.9's codata as classes) */
        chksrc_set(ix[i], ds(fs[i]->it[3])); chkty_set(ix[i], ds(fs[i]->it[2]));
    }
    for (int i = 0; i < n; i++) cofix_body(gs[i], fs[i]->it[3]);
    /* the group's keys (F34); a group the store holds as checked is not checked again */
    SKey *ks = calloc((size_t)n + 1, sizeof(SKey));
    { Node **Ts = calloc((size_t)n + 1, sizeof(Node *)), **ts = calloc((size_t)n + 1, sizeof(Node *));
      for (int i = 0; i < n; i++) { Ts[i] = ds(fs[i]->it[2]); ts[i] = ds(fs[i]->it[3]); }
      key_group(Ts, ts, n, ix[0], ks); free(Ts); free(ts); }
    for (int i = 0; i < n; i++) { ckey_set(ix[i], ks[i]); if (i == 0) deps_take(&CDEPS, &capcdeps, ix[0]); else { if ((size_t)ix[i] >= capcdeps) { size_t o = capcdeps; GROW(CDEPS, capcdeps, (size_t)ix[i] + 1); memset(CDEPS + o, 0, (capcdeps - o) * sizeof(Deps)); } CDEPS[ix[i]].d = malloc(sizeof(int)); CDEPS[ix[i]].d[0] = ix[0]; CDEPS[ix[i]].n = 1; } }
    bool stored = expect && !trusted;
    for (int i = 0; i < n && stored; i++) if (!store_checked(ks[i])) stored = false;
    bool all = true;
    if (stored) {
        for (int i = 0; i < n; i++) { passes++; printf("ok: checks %s   [store]\n", fs[i]->it[1]->s); names_bind(fs[i]->it[1]->s, ks[i]); }
    } else if (!trusted) {
        Node **ws = calloc((size_t)n + 1, sizeof(Node *));
        for (int i = 0; i < n; i++) ws[i] = word(ix[i]);
        Node *grp = nl2("%raw", alist(ws, n)); free(ws);
        long total = 0;
        char **rs = calloc((size_t)n + 1, sizeof(char *)); Node **qs = calloc((size_t)n + 1, sizeof(Node *));
        /* every body guarded before any is checked: checking one may unfold another */
        int unguarded = -1; char *ug = NULL; Node *ugq = NULL;
        for (int i = 0; i < n && unguarded < 0; i++) {
            Node *q = nl3("%%codef-ok", grp, nl2("%raw", aclosed(ds(fs[i]->it[3]))));
            long steps = 0;
            char *r = readback(q, &steps);
            total += steps;
            if (strcmp(r, "ok")) { unguarded = i; ug = r; ugq = q; } else free(r);
        }
        for (int i = 0; i < n; i++) {
            if (unguarded >= 0) { rs[i] = strdup(i == unguarded ? ug : "unguarded"); qs[i] = ugq; if (i == unguarded) free(ug); continue; }
            qs[i] = nl4("%%chk-codef", grp, cT[i], nl2("%raw", aclosed(ds(fs[i]->it[3]))));
            long steps = 0;
            rs[i] = readback(qs[i], &steps);
            bool acc = !strcmp(rs[i], "ok");
            total += steps;
            if (!acc) all = false;
            if (expect) {
                if (acc) passes++; else failures++;
                printf("%s: checks %s   [%ld steps]\n", acc ? "ok" : "FAIL", fs[i]->it[1]->s, steps);
                if (!acc) explain(rs[i], qs[i]);
            }
        }
        if (unguarded >= 0) {
            all = false;
            if (expect) { failures++; printf("FAIL: checks %s\n", fs[unguarded]->it[1]->s); explain(rs[unguarded], qs[unguarded]); }
        }
        if (!expect) {
            if (!all) passes++; else failures++;
            printf("%s: rejected %s   [%ld steps]\n", !all ? "ok" : "FAIL", fs[0]->it[1]->s, total);
            if (getenv("CUBICAL_WHY")) for (int i = 0; i < n; i++) if (strcmp(rs[i], "ok") && strcmp(rs[i], "unguarded")) explain(rs[i], qs[i]);
        }
        for (int i = 0; i < n; i++) free(rs[i]);
        free(rs); free(qs);
    } else if (!expect) all = false;
    if (!all) for (int i = 0; i < n; i++) cofix_hide(gs[i], ix[i]);
    else if (!stored && expect) {
        int saved = kgroup_base; kgroup_base = ix[0];
        for (int i = 0; i < n; i++) {
            SBuf c = { 0 }, y = { 0 }; canon(&c, ds(fs[i]->it[3])); canon(&y, ds(fs[i]->it[2]));
            store_put(fs[i]->it[1]->s, ks[i], c.b, y.b, 0, trusted ? NULL : "");
            free(c.b); free(y.b);
        }
        kgroup_base = saved;
    }
    free(ks); free(gs); free(ix); free(cT);
}
/* a codata declaration as a telescope: its parameters, then its fields' telescope */
static Node *atel_cofields(Node *dd, int i) {
    if (i >= dd->n) return kapp("tz", K("lnil"), NULL);
    Node *fl = dd->it[i];
    Chain C = { 0 }; NV A = ann(ds(fl->it[1]), &C);
    Node *ty = afresh("at");
    apush(fl->it[0]->s, ty);
    Node *rest = atel_cofields(dd, i + 1);
    nasc--;
    return chain_close(&C, nl3("app", kapp("tx", A.node, word(srcname(fl->it[0]->s))), nl3("lam", fl->it[0], nl3("lam", ty, rest))));
}
static Node *atel_coparams(Node *dd, int d, int k) {
    Node *hd = dd->it[1];
    if (k < DATA[d].nparams) {
        Node *p = hd->it[k + 1];
        if (!p->list || p->n != 2) die("a checked codata type needs its parameters' types:", DATA[d].name);
        Chain C = { 0 }; NV A = ann(ds(p->it[1]), &C);
        Node *ty = afresh("at");
        apush(p->it[0]->s, ty);
        Node *rest = atel_coparams(dd, d, k + 1);
        nasc--;
        return chain_close(&C, nl3("app", kapp("tx", A.node, word(srcname(p->it[0]->s))), nl3("lam", p->it[0], nl3("lam", ty, rest))));
    }
    return kapp("tz", atel_cofields(dd, 2), NULL);
}

/* ---------------- the compiled-form cache ----------------
 * A module whose checks are cached (load_file) replays its recorded core forms; compiling them was
 * most of a cached load. So a module's compiled forms are kept too, beside its recorded forms,
 * in the order its definitions are made: the Syn, records and case tables of each plain
 * definition, with its slot count, uses, parameter count, strictness and n-ary record. Globals,
 * constructors and data types are referred to by index, which the replay reproduces. A module
 * read from this store compiles nothing; on any mismatch (an entry that is not the definition
 * being made) the rest of the module compiles as before. */
static FILE *cc_out;                                   /* recording */
static const unsigned char *cc_in, *cc_end; static bool cc_bad;   /* replaying */
static void cc_w(const void *p, size_t n) { fwrite(p, 1, n, cc_out); }
static void cc_wu8(unsigned v) { unsigned char b = (unsigned char)v; cc_w(&b, 1); }
static void cc_wi32(int32_t v) { cc_w(&v, 4); }
static void cc_wu64(uint64_t v) { cc_w(&v, 8); }
static void cc_wstr(const char *x) { uint32_t n = (uint32_t)strlen(x); cc_w(&n, 4); cc_w(x, n); }
static bool cc_r(void *p, size_t n) { if (cc_bad || (size_t)(cc_end - cc_in) < n) { cc_bad = true; return false; } memcpy(p, cc_in, n); cc_in += n; return true; }
static unsigned cc_ru8(void) { unsigned char b = 0; cc_r(&b, 1); return b; }
static int32_t cc_ri32(void) { int32_t v = 0; cc_r(&v, 4); return v; }
static uint64_t cc_ru64(void) { uint64_t v = 0; cc_r(&v, 8); return v; }
static char *cc_rstr(void) { uint32_t n = 0; if (!cc_r(&n, 4) || (size_t)(cc_end - cc_in) < n) { cc_bad = true; return NULL; } char *x = malloc(n + 1); memcpy(x, cc_in, n); x[n] = 0; cc_in += n; return x; }
static void cc_wrec(const Record *r);
static Record *cc_rrec(void);
static void cc_wsyn(const Syn *t) {
    if (!t) { cc_wu8(0xFF); return; }
    cc_wu8((unsigned)t->tag); cc_wi32(t->n); cc_wi32(t->slot); cc_wi32((int32_t)t->x); cc_wu64((uint64_t)t->id);
    if (t->tag == S_WORD && t->big) { cc_wu8(1); char *b; size_t l; FILE *m = open_memstream(&b, &l); WN w = { (uint64_t)t->id, t->big }; wn_print(m, &w); fclose(m); cc_wstr(b); free(b); }
    else cc_wu8(0);
    if (t->rec) { cc_wu8(1); cc_wrec(t->rec); } else cc_wu8(0);
    if (t->tab) {
        CaseTab *tb = t->tab; int N = DATA[tb->d].ncons;
        cc_wu8(1); cc_wi32(tb->d); cc_wi32(tb->nenv); cc_wu64(tb->mask);
        for (int i = 0; i < N; i++) cc_wrec(tb->br[i]);
        if (tb->motive) { cc_wu8(1); cc_wrec(tb->motive); } else cc_wu8(0);
    } else cc_wu8(0);
    for (int i = 0; i < t->n; i++) cc_wsyn(t->k[i]);
}
static Syn *cc_rsyn(void) {
    unsigned tag = cc_ru8();
    if (cc_bad || tag == 0xFF) return NULL;
    int32_t n = cc_ri32(); if (cc_bad || n < 0 || n > 100000) { cc_bad = true; return NULL; }
    Syn *t = syn((int)tag, n);
    t->slot = cc_ri32(); t->x = (uint32_t)cc_ri32(); t->id = (long)cc_ru64();
    if (cc_ru8()) { char *b = cc_rstr(); if (b) { WN w = wn_parse(b); t->id = (long)w.s; t->big = w.b; free(b); } }
    if (cc_ru8()) t->rec = cc_rrec();
    if (cc_ru8()) {
        CaseTab *tb = calloc(1, sizeof(CaseTab));
        tb->d = cc_ri32(); tb->nenv = cc_ri32(); tb->mask = cc_ru64();
        if (cc_bad || tb->d < 0 || tb->d >= ndata) { cc_bad = true; free(tb); return t; }
        int N = DATA[tb->d].ncons;
        tb->br = calloc((size_t)N + 1, sizeof(Record *));
        for (int i = 0; i < N && !cc_bad; i++) tb->br[i] = cc_rrec();
        if (cc_ru8()) tb->motive = cc_rrec();
        if (!cc_bad) t->tab = tab_intern(tb);
    }
    for (int i = 0; i < n && !cc_bad; i++) t->k[i] = cc_rsyn();
    return t;
}
static void cc_wrec(const Record *r) {
    cc_wi32(r->nparam); cc_wi32(r->nslots); cc_wi32(r->nenv);
    for (int i = 0; i < r->nenv; i++) cc_wi32(r->env_slot[i]);
    for (int i = 0; i < r->nslots; i++) cc_wi32(r->uses[i]);
    cc_wsyn(r->body);
}
static Record *cc_rrec(void) {
    Record *r = calloc(1, sizeof(Record));
    r->nparam = cc_ri32(); r->nslots = cc_ri32(); r->nenv = cc_ri32();
    if (cc_bad || r->nslots < 0 || r->nenv < 0 || r->nslots > 1000000 || r->nenv > 1000000) { cc_bad = true; free(r); return NULL; }
    r->env_slot = calloc((size_t)r->nenv + 1, sizeof(int)); r->uses = calloc((size_t)r->nslots + 1, sizeof(int));
    for (int i = 0; i < r->nenv; i++) r->env_slot[i] = cc_ri32();
    for (int i = 0; i < r->nslots; i++) r->uses[i] = cc_ri32();
    r->body = cc_rsyn(); r->where = prof_cur;
    if (cc_bad) return NULL;
    return rec_intern(r);
}
#define CC_DEF 0xD1
static void cc_wdef(long g) {
    cc_wu8(CC_DEF); cc_wstr(G[g].name);
    cc_wi32(G[g].nslots); for (int i = 0; i < G[g].nslots; i++) cc_wi32(G[g].uses[i]);
    cc_wsyn(G[g].body);
    cc_wi32(G[g].nparams); cc_wu64(G[g].strict);
    if (G[g].nary) { cc_wu8(1); cc_wrec(G[g].nary); } else cc_wu8(0);
}
/* whether the next entry is the definition of this name (the entry is left in place) */
static bool cc_next_is(const char *name) {
    if (!cc_in || cc_bad || cc_in >= cc_end) return false;
    const unsigned char *at = cc_in;
    bool ok = cc_ru8() == CC_DEF;
    char *nm = ok ? cc_rstr() : NULL;
    ok = ok && nm && !strcmp(nm, name);
    free(nm);
    cc_in = at; cc_bad = false;
    return ok;
}
static bool cc_rdef(long g) {
    cc_ru8(); free(cc_rstr());
    int32_t ns = cc_ri32(); if (cc_bad || ns < 0 || ns > 1000000) { cc_bad = true; return false; }
    int *u = calloc((size_t)ns + 1, sizeof(int)); for (int i = 0; i < ns; i++) u[i] = cc_ri32();
    Syn *b = cc_rsyn();
    int32_t np = cc_ri32(); uint64_t st = cc_ru64();
    Record *nary = cc_ru8() ? cc_rrec() : NULL;
    if (cc_bad || !b) return false;
    G[g].body = b; G[g].nslots = ns; G[g].uses = u; G[g].nparams = np; G[g].strict = st; G[g].nary = nary;
    return true;
}
static void toplevel_in(Node *f);
/* a top-level form; once the outermost one is handled, compiled code nothing reaches is swept */
static void toplevel(Node *f) {
    toplevel_depth++;
    toplevel_in(f);
    toplevel_depth--;
    static int every = -1; if (every < 0) every = getenv("CUBICAL_SWEEPALL") != NULL;   /* a sweep after every form: to test */
    if (!toplevel_depth) records_sweep(every);
}
static void toplevel_in(Node *f) {
    if (!f->list || f->n == 0 || f->it[0]->list) die("top-level form expected", NULL);
    const char *h = f->it[0]->s;
    if (!strcmp(h, "import")) {
        /* (import "file"): loaded once, relative to the importing file */
        if (f->n != 2 || f->it[1]->list) die("(import \"file\")", NULL);
        const char *nm = f->it[1]->s;
        char *q = strdup(nm[0] == '"' ? nm + 1 : nm);
        size_t ln = strlen(q); if (ln && q[ln - 1] == '"') q[ln - 1] = 0;
        char *full = q;
        if (q[0] != '/' && cur_dir) { full = malloc(strlen(cur_dir) + strlen(q) + 2); sprintf(full, "%s/%s", cur_dir, q); }
        load_file(full, true);
        return;
    }
    /* a module whose checks are cached: its definitions are made, its judgments and tests not rerun */
    if (trusted && (!strcmp(h, "check") || !strcmp(h, "check-fail") || !strcmp(h, "checked-data-fail") || !strcmp(h, "checked-fail")
        || !strcmp(h, "checked-codata-fail") || !strcmp(h, "mutual-fail") || !strcmp(h, "same") || !strcmp(h, "show") || !strcmp(h, "same-watch") || !strcmp(h, "observe") || !strcmp(h, "usage") || !strcmp(h, "trusted") || !strcmp(h, "layout") || !strcmp(h, "fo") || !strcmp(h, "key") || !strcmp(h, "names") || !strcmp(h, "history") || !strcmp(h, "gc") || !strcmp(h, "safe") || !strcmp(h, "native") || !strcmp(h, "native-c") || !strcmp(h, "native-at") || !strcmp(h, "run") || !strcmp(h, "type") || !strcmp(h, "scope") || !strcmp(h, "where"))) return;
    if (!strcmp(h, "def")) {
        if (f->n != 3) die("(def name term)", NULL);
        if (!chk_salt[0] && !f->it[1]->list && !strcmp(f->it[1]->s, "chk-def")) chk_salt_set();   /* the checker (%%chk-def) is defined here: its name (F34) */
        if (cc_next_is(f->it[1]->s)) {
            /* its compiled form is cached: made without desugaring or compiling */
            long g = global_lookup(f->it[1]->s);
            bool fill_decl = g >= 0 && G[g].kind == G_INPUT && !G[g].res->port[0];
            if (fill_decl) G[g].kind = G_DEF; else { decl_fresh(f->it[1]->s); g = add_global(f->it[1]->s, G_DEF); }
            prof_enter(f->it[1]->s);
            if (cc_rdef(g)) {
                prof_cur = 0;
                link(top_build(G[g].body, G[g].nslots, G[g].uses), P(G[g].res, 0));
                if (!strcmp(G[g].name, "%case-hcomp")) case_hcomp_global = g;
                if (fill_decl) { for (int i = 0; i < G[g].npark; i++) net_push(G[g].park_o[i], G[g].park_v[i]); G[g].npark = 0; }
                return;
            }
            /* a bad entry: the module compiles from here on, this definition included */
            cc_in = NULL; cc_bad = true; prof_cur = 0;
            if (!fill_decl) { G[g].kind = G_INPUT; }   /* made again below by the ordinary path (same index: the name is known) */
        }
        ArMark dm = ar_mark();
        Node *dsb = ds(f->it[2]);   /* desugared once: the copattern test, the strictness analysis, the body and its n-ary form read it */
        if (!plain_def && copattern_body(dsb)) {
            /* a copattern definition: its value is its stuck head */
            ar_release(dm);
            decl_fresh(f->it[1]->s);
            long g = cofix_declare(f->it[1]->s, -1);
            cofix_body(g, f->it[2]);
            return;
        }
        /* a name declared earlier (forward reference, mutual recursion) is filled */
        long g = global_lookup(f->it[1]->s);
        bool fill_decl = g >= 0 && G[g].kind == G_INPUT && !G[g].res->port[0];
        if (fill_decl) G[g].kind = G_DEF; else { decl_fresh(f->it[1]->s); g = add_global(f->it[1]->s, G_DEF); }
        int ns; int *u;
        prof_enter(f->it[1]->s);
        {
            /* the parameters its body always reads */
            Node *b = dsb; int np = 0; const char *pn[64];
            G[g].nparams = 0; G[g].strict = 0;
            while (b->list && b->n == 3 && is(b->it[0], "lam") && np < 64) { pn[np++] = param_name(b->it[1])->s; b = b->it[2]; }
            uint64_t m = 0;
            rd_bound.n = 0;
            for (int q = 0; q < np; q++) names_add(&rd_bound, pn[q]);
            for (int q = 0; q < np; q++) {
                bool shadowed = false;
                for (int r = q + 1; r < np; r++) if (!strcmp(pn[r], pn[q])) shadowed = true;
                if (!shadowed && reads(b, pn[q])) m |= 1ull << q;
            }
            rd_bound.n = 0;
            G[g].nparams = np; G[g].strict = m;
        }
        top_lazy = true;
        Syn *s = compile_top_ds(dsb, &ns, &u);
        top_lazy = false;
        G[g].body = s; G[g].nslots = ns; G[g].uses = u;
        {
            /* the body with all its parameters at once */
            Node *b = dsb; int np = 0;
            Node *pn[256];
            while (b->list && b->n == 3 && is(b->it[0], "lam") && np < 256) { pn[np++] = param_name(b->it[1]); b = b->it[2]; }
            if (np >= 1) {
                nbinds = 0; nlevels = 1; GROW(levels, caplevels, 1); levels[0] = (Level){ 0 }; bind_epoch++;
                Names none = { 0 };
                G[g].nary = compile_branch(&none, pn, np, b);
                nlevels = 0;
            }
        }
        ar_release(dm);
        if (cc_out) cc_wdef(g);
        prof_cur = 0;
        link(top_build(s, ns, u), P(G[g].res, 0));
        if (!strcmp(G[g].name, "%case-hcomp")) case_hcomp_global = g;
        if (fill_decl) { for (int i = 0; i < G[g].npark; i++) net_push(G[g].park_o[i], G[g].park_v[i]); G[g].npark = 0; }
    } else if (!strcmp(h, "checked") || !strcmp(h, "checked-fail") || !strcmp(h, "check") || !strcmp(h, "check-fail")) {
        /* the checker (lib/check.cub), a term, run on the codes of T and t
         * against the signature of the definitions checked so far */
        bool define = !strcmp(h, "checked") || !strcmp(h, "checked-fail");
        bool expect = strcmp(h, "check-fail") != 0 && strcmp(h, "checked-fail") != 0;
        int at = define ? 2 : 1;
        if (f->n != at + 2) die(define ? "(checked name T t)" : "(check T t)", NULL);
        /* a copattern definition, which may call itself */
        ArMark cm = ar_mark();
        bool cop = define && copattern_body(ds(f->it[3]));
        ar_release(cm);
        if (cop) { checked_group(&f, 1, expect); return; }
        /* what the check makes in the arena (the desugared type and term, their closed copies, the
         * key's text, the usages read back) is the check's alone: the tables take their own copies
         * (perm_copy), names are interned, the definition's compiled code is the heap's. Released at
         * the end (CUBICAL_NOCHKMARK keeps it, to compare) */
        static int nochk = -1; if (nochk < 0) nochk = getenv("CUBICAL_NOCHKMARK") != NULL;
        cm = ar_mark();
        Node *T = f->it[at], *t = f->it[at + 1];
        Node *dT = ds(T), *dt = ds(t);
        /* its codebase key (F34): a definition the store holds as checked is not checked again */
        char key[65]; char *kcode = NULL, *ktype = NULL;
        key_def(dT, dt, key, &kcode, &ktype);
        bool stored = define && expect && !trusted && (forced_result ? forced_steps < 0 : store_checked(key));
        Node *cT = nl2("%raw", aclosed(dT)), *ct = trusted || stored ? NULL : nl2("%raw", aclosed(dt));
        long steps = 0;
        char *r = trusted || stored ? strdup("ok") : forced_result ? strdup(forced_result) : readback(nl3("%%chk-def", cT, ct), &steps), *tr = strdup("ok");
        if (forced_result) steps = forced_steps;
        bool accepted = !strcmp(r, tr), ok = accepted == expect;
        if (ok) passes++; else failures++;
        if (!trusted) {
            printf("%s: %s ", ok ? "ok" : "FAIL", expect ? "checks" : "rejected");
            if (define) printf("%s", f->it[1]->s); else out_term(t);
            if (stored) printf("   [store]\n"); else printf("   [%ld steps]\n", steps);
        }
        if (!accepted && (expect || getenv("CUBICAL_WHY"))) explain(r, nl3("%%chk-def", cT, ct));
        free(r); free(tr);
        if (define && accepted && expect) {
            Node *d = nlist(3); d->it[0] = nsym("def"); d->it[1] = f->it[1]; d->it[2] = t;
            toplevel(d);
            /* the checker's tables: the definition and its type, each a global, so a lookup
             * is a reference */
            char ty[32]; snprintf(ty, sizeof ty, "%%sty%d", nchecked);
            Node *td = nlist(3); td->it[0] = nsym("def"); td->it[1] = nsym(ty); td->it[2] = nl2("%%sig-type", cT);
            toplevel(td);
            int ck = sig_add(f->it[1]->s, global_lookup(f->it[1]->s), global_lookup(ty), false);
            chksrc_set(ck, dt); chkty_set(ck, dT);
            gsteps_set(ck, stored ? store_steps(key) : steps);
            ckey_set(ck, key); deps_take(&CDEPS, &capcdeps, ck);
            /* the parameters' computed usages (B11): asked of the checker (or read from the store),
             * kept, and recorded with the module's forms so a cached load has them */
            if (!trusted) {
                const char *nm[256]; unsigned char u[256]; int n = 0;
                char *su = stored ? store_read(key, "usage") : NULL;
                if (su) {
                    for (char *p = su; *p && n < 256; ) {
                        char *sp = strchr(p, ' '); if (!sp) break; *sp = 0;
                        char *e = strchr(sp + 1, '\n'); if (e) *e = 0;
                        nm[n] = intern(p); u[n] = (unsigned char)atoi(sp + 1); n++;
                        p = e ? e + 1 : sp + 1 + strlen(sp + 1);
                    }
                    free(su);
                } else {
                    long us = 0; char *r2 = readback(nl3("%%usages", cT, ct ? ct : nl2("%raw", aclosed(dt))), &us);
                    const char *saved = src; src = r2; Node *l = read_node(); src = saved;
                    for (Node *x = l; x && x->list && x->n == 3 && is(x->it[0], "lcons") && n < 256; x = x->it[2]) {
                        Node *pr = x->it[1];
                        if (!pr->list || pr->n != 3 || pr->it[1]->list || pr->it[2]->list || pr->it[1]->s[0] != '#' || pr->it[2]->s[0] != '#') break;
                        nm[n] = srcname_of(atoi(pr->it[1]->s + 1)); u[n] = (unsigned char)atoi(pr->it[2]->s + 1); n++;
                    }
                    free(r2);
                }
                usage_set(ck, n, nm, u);
                { SBuf ub = { 0 }; for (int i = 0; i < n; i++) sb_fmt(&ub, "%s %d\n", nm[i], u[i]); store_put(f->it[1]->s, key, kcode, ktype, steps, ub.b ? ub.b : ""); free(ub.b); }
                Node *rec = nlist(2 + 2 * n); rec->it[0] = nsym("usages"); rec->it[1] = f->it[1];
                for (int i = 0; i < n; i++) { char b[32]; rec->it[2 + 2 * i] = nsym(nm[i]); snprintf(b, sizeof b, "#%d", u[i]); rec->it[3 + 2 * i] = nsym(b); }
                x_rec_extra(rec);
            } else store_put(f->it[1]->s, key, kcode, ktype, 0, NULL);
        }
        free(kcode); free(ktype);
        if (!nochk) ar_release(cm);
        mem_report(define ? f->it[1]->s : "(check)");
    } else if (!strcmp(h, "usages")) {
        /* a cached module's record of a checked definition's parameter usages */
        int ck = checked_index(f->it[1]->s); int n = (f->n - 2) / 2;
        const char *nm[256]; unsigned char u[256]; if (n > 256) n = 256;
        for (int i = 0; i < n; i++) { nm[i] = f->it[2 + 2 * i]->s; u[i] = (unsigned char)atoi(f->it[3 + 2 * i]->s + 1); }
        if (ck >= 0) usage_set(ck, n, nm, u);
    } else if (!strcmp(h, "key")) {
        if (f->n != 2 || f->it[1]->list) die("(key name)", NULL);
        store_key_cmd(f->it[1]->s);
    } else if (!strcmp(h, "names")) {
        if (f->n != 2 || f->it[1]->list) die("(names key)", NULL);
        store_names_cmd(f->it[1]->s);
    } else if (!strcmp(h, "history")) {
        if (f->n != 2 || f->it[1]->list) die("(history name)", NULL);
        store_history_cmd(f->it[1]->s);
    } else if (!strcmp(h, "unbind")) {
        if (f->n != 2 || f->it[1]->list) die("(unbind name)", NULL);
        store_unbind_cmd(f->it[1]->s);
    } else if (!strcmp(h, "retract")) {
        /* #retract name (item 30, the editor protocol): the checked definition is hidden, so the
         * name may be declared again; what was checked against it keeps the definition it saw (a
         * global is referenced by its index, the name finds the newest) */
        if (f->n != 2 || f->it[1]->list) die("(retract name)", NULL);
        int ck = checked_index(f->it[1]->s);
        if (ck >= 0) {
            CHKHIDE[ck] = true; printf("retracted %s", f->it[1]->s);
            /* a record: its projections too, and its name for R { … } */
            char **fn = NULL; int nf = rec_retract(f->it[1]->s, &fn);
            for (int k = 0; k < nf; k++) { int cf = checked_index(fn[k]); if (cf >= 0) { CHKHIDE[cf] = true; printf(" .%s", fn[k]); } }
            printf("\n");
            return;
        }
        /* a data, codata or record type: its name, constructors and fields renamed out of reach
         * (lookups are by name, the newest first; what was checked against it holds it by its
         * index, so it stays as it was checked, and a declaration of the name again is a new type).
         * The reply names what went */
        int d = data_lookup(f->it[1]->s);
        if (d < 0) { printf("retract %s: not a checked definition or a data type\n", f->it[1]->s); return; }
        static int nretr;
        char nb[512];
        printf("retracted data %s", DATA[d].name);
        snprintf(nb, sizeof nb, "%%retracted%d:%s", nretr, DATA[d].name); DATA[d].name = strdup(nb);
        for (int k = 0; k < DATA[d].ncons; k++) {
            int c = DATA[d].cons[k]; printf(" %s", CON[c].name);
            snprintf(nb, sizeof nb, "%%retracted%d:%s", nretr, CON[c].name);
            ford_rename(CON[c].name, nb);
            CON[c].name = (char *)intern(nb);
        }
        for (int k = 0; k < DATA[d].nflds; k++) {
            int fl = DATA[d].flds[k]; printf(" .%s", FLD[fl].name);
            snprintf(nb, sizeof nb, "%%retracted%d:%s", nretr, FLD[fl].name); FLD[fl].name = strdup(nb);
        }
        conh_built = -1; nretr++;
        printf("\n");
    } else if (!strcmp(h, "scope")) {
        /* #scope (item 30): the names a declaration may use, on one line `scope: a b …` — checked
         * definitions not retracted, data types, their constructors (the newest of each name) */
        printf("scope:");
        for (int i = 0; i < nchecked; i++) if (!CHKHIDE[i] && CHECKED[i][0] != '%' && checked_index(CHECKED[i]) == i) printf(" %s", CHECKED[i]);
        for (int d = 0; d < ndata; d++) if (DATA[d].name[0] != '%' && data_lookup(DATA[d].name) == d && checked_index(DATA[d].name) < 0) {
            printf(" %s", DATA[d].name);
            for (int k = 0; k < DATA[d].ncons; k++) { int c = DATA[d].cons[k]; if (CON[c].name[0] != '%' && con_lookup(CON[c].name) == c) printf(" %s", CON[c].name); }
        }
        printf("\n");
    } else if (!strcmp(h, "where")) {
        /* #where name (item 30): `where: name file:line` for a checked definition, a data type or a
         * constructor (its data type's declaration); `where: name ?` when not known */
        if (f->n != 2 || f->it[1]->list) die("(where name)", NULL);
        const char *nm = f->it[1]->s, *pos = NULL;
        int ck = checked_index(nm);
        if (ck >= 0 && (size_t)ck < capchkpos) pos = CHKPOS[ck];
        if (!pos) { int d = data_lookup(nm); if (d >= 0 && (size_t)d < capdatapos) pos = DATAPOS[d]; }
        if (!pos) { int c = con_lookup(nm); if (c >= 0 && (size_t)CON[c].data < capdatapos) pos = DATAPOS[CON[c].data]; }
        printf("where: %s %s\n", nm, pos ? pos : "?");
    } else if (!strcmp(h, "gc")) {
        store_gc_cmd();
    } else if (!strcmp(h, "safe")) {
        if (f->n != 2 || f->it[1]->list) die("(safe name)", NULL);
        store_safe_cmd(f->it[1]->s);
    } else if (!strcmp(h, "type")) {
        /* #type e (item 30): e's type as the checker infers it, one line `type: e : T` on stdout (the
         * editor protocol's hover reads it); a term with no inferable type gets the reason */
        if (f->n != 2 && f->n != 3) die("(type term [shown-as])", NULL);
        Node *ct = nl2("%raw", aclosed(ds(f->it[1])));
        long st = 0; char *r = readback(nl2("%%infer-ok", ct), &st);
        printf("type: "); out_term(f->it[f->n - 1]);   /* (a data type's name stands for its type former written as a function) */
        if (strcmp(r, "ok")) { printf(" : ?\n"); explain(r, NULL); free(r); return; }
        free(r);
        net_drain();
        char *ty = read_code(build_top(nl2("%%type-of", ct)));
        net_drain();
        printf(" : "); if (ty) out_str(ty); else printf("?"); printf("\n");
        free(ty);
    } else if (!strcmp(h, "usage")) {
        /* #usage f: the parameters' computed usages */
        int ck = checked_index(f->it[1]->s);
        if (ck < 0 || (size_t)ck >= capgusage) { printf("usage %s: not a checked definition\n", f->it[1]->s); return; }
        printf("usage %s:", f->it[1]->s);
        for (int i = 0; i < GUSAGE[ck].n; i++) printf(" %s %s", GUSAGE[ck].nm[i], GUSAGE[ck].u[i] == 0 ? "0" : GUSAGE[ck].u[i] == 1 ? "1" : "ω");
        printf("\n");
    } else if (!strcmp(h, "primitive") || !strcmp(h, "primitive-tick") || !strcmp(h, "foreign")) {
        /* (primitive name T t): a rule of the type theory as a constant: T is checked to be a type,
         * t (its computation) is trusted. (primitive-tick name T t): its value is its stuck call,
         * which unfolds (t applied to the arguments) only when its last parameter is given ⋄ */
        if ((f->n != 4 && !(f->n == 3 && !strcmp(h, "primitive"))) || f->it[1]->list) die("(primitive name T [t])", NULL);
        const char *nm = f->it[1]->s;
        trusted_add(nm, !strcmp(h, "foreign") ? "foreign" : f->n == 3 ? "primitive, no computation" : !strcmp(h, "primitive-tick") ? "primitive, tick" : "primitive");
        Node *cT = nl2("%raw", aclosed(ds(f->it[2])));
        char key[65]; char *ktype = NULL; key_named("primitive", nm, ds(f->it[2]), key, &ktype);
        if (!trusted && store_checked(key)) { passes++; printf("ok: checks %s   [store]\n", nm); }
        else if (!trusted) {
            long steps = 0;
            char *r = readback(nl2("%%chk-type", cT), &steps);
            bool acc = !strcmp(r, "ok");
            if (acc) passes++; else failures++;
            printf("%s: checks %s   [%ld steps]\n", acc ? "ok" : "FAIL", nm, steps);
            if (!acc) { explain(r, nl2("%%chk-type", cT)); free(r); free(ktype); return; }
            free(r);
        }
        char ty[32]; snprintf(ty, sizeof ty, "%%sty%d", nchecked);
        Node *td = nlist(3); td->it[0] = nsym("def"); td->it[1] = nsym(ty); td->it[2] = nl2("%%sig-type", cT);
        toplevel(td);
        long tyg = global_lookup(ty), g;
        if (f->n == 3) {
            /* no computation: a constant of its type, its neutral carrying the type */
            g = add_global(nm, G_AXIOM);
            G[g].tyg = tyg;
        } else if (!strcmp(h, "primitive-tick")) {
            g = cofix_declare(nm, tyg);
            int np = 0; for (Node *b = ds(f->it[3]); b->list && b->n == 3 && (is(b->it[0], "lam") || is(b->it[0], "plam")); b = b->it[2]) np++;
            if (np < 1) die("primitive-tick: its computation takes its parameters, the last the tick:", nm);
            G[g].tickar = np;
            cofix_body(g, f->it[3]);
        } else {
            Node *d = nlist(3); d->it[0] = nsym("def"); d->it[1] = f->it[1]; d->it[2] = f->it[3];
            toplevel(d);
            g = global_lookup(nm);
        }
        int pk = sig_add(nm, g, tyg, false);
        /* the parameters' usages (B11, 18.1): a primitive's body is not elaborated, so its usages are
         * read off the body itself: a parameter absent from it is 0 (erased at every call: the
         * lengths and bound proofs of the byte layer's accesses); one read once, as an argument of
         * the machine's operation, is 1; anything else ω */
        if (f->n == 4) {
            const char *nms[64]; unsigned char us[64]; int np = 0;
            Node *b = ds(f->it[3]);
            for (; b->list && b->n == 3 && is(b->it[0], "lam") && np < 64; b = b->it[2]) nms[np++] = b->it[1]->s;
            for (int i = 0; i < np; i++) { int c = 0, direct = 0; prim_uses(b, nms[i], 0, &c, &direct); us[i] = c == 0 ? 0 : (c == 1 && direct == 1) ? 1 : 2; }
            usage_set(pk, np, nms, us);
        }
        ckey_set(pk, key); deps_take(&CDEPS, &capcdeps, pk); store_put(nm, key, "", ktype, 0, NULL); free(ktype);
    } else if (!strcmp(h, "checked-data") || !strcmp(h, "checked-data-fail") || !strcmp(h, "check-data")) {
        bool expect = strcmp(h, "checked-data-fail") != 0;
        /* (checked-data (data ...)): declare the data type and check it with the
         * checker: parameter and field types are types, fields strictly positive,
         * boundaries elements of the type made of earlier constructors.
         * (check-data D): check a data type already declared (the prelude's) */
        Node *dd; int d;
        if (!strcmp(h, "check-data")) {
            if (f->n != 2 || f->it[1]->list) die("(check-data D)", NULL);
            d = data_lookup(f->it[1]->s);
            if (d < 0) die("check-data: no such data type:", f->it[1]->s);
            if (chk_index(d) >= 0) die("check-data: already checked:", f->it[1]->s);
            dd = DATA[d].decl;
        } else {
            if (f->n != 2 || !f->it[1]->list || !is(f->it[1]->it[0], "data")) die("(checked-data[-fail] (data ...))", NULL);
            dd = f->it[1];
            toplevel(dd);
            d = ndata - 1;
        }
        GROW(CHKDATA, capchkdata, (size_t)nchkdata + 1);
        CHKDATA[nchkdata++] = d;
        Node *ent = nl2("%raw", kapp("dent", atel_params(dd, d, 0), K("lzero")));
        int di = nchkdata - 1;
        /* the tables while it is checked: its declaration at level lzero, and the machine's data
         * type, constructors, elimination and case for the evaluator */
        char den[32], dtn[32]; snprintf(den, sizeof den, "%%dentp%d", di); snprintf(dtn, sizeof dtn, "%%dtab%d", di);
        Node *pd = nlist(3); pd->it[0] = nsym("def"); pd->it[1] = nsym(den); pd->it[2] = ent;
        toplevel(pd);
        Node **cs = calloc((size_t)DATA[d].ncons + 1, sizeof(Node *));
        for (int ci = 0; ci < DATA[d].ncons; ci++) cs[ci] = nsym(CON[DATA[d].cons[ci]].name);
        char en[256], cn[256], trn[256]; snprintf(en, sizeof en, "%%elim-%s", DATA[d].name); snprintf(cn, sizeof cn, "%%case-%s", DATA[d].name);
        snprintf(trn, sizeof trn, "%%transf-%s", DATA[d].name);
        Node *td = nlist(3); td->it[0] = nsym("def"); td->it[1] = nsym(dtn);
        { Node *l = K("lnil"); for (int i = DATA[d].ncons - 1; i >= 0; i--) l = kapp("lcons", cs[i], l); td->it[2] = nl3("app", nl3("app", kapp("dtab", nsym(DATA[d].name), l), nsym(en)), nsym(cn)); }
        free(cs);
        toplevel(td);
        table_set(TB_DENT, di, global_lookup(den));
        table_set(TB_DTAB, di, global_lookup(dtn));
        table_set(TB_TRANS, di, global_lookup(trn));
        long steps = 0;
        char key[65]; char *kcode = NULL; key_decl(dd, key, &kcode);
        bool stored = expect && !trusted && store_checked(key);
        char *r = trusted || stored ? strdup("ok") : readback(nl3("%%chk-data", word(di), ent), &steps), *tr = strdup("ok");
        bool accepted = !strcmp(r, tr), ok = accepted == expect;
        if (ok) passes++; else failures++;
        if (!trusted) { printf("%s: %s data %s   ", ok ? "ok" : "FAIL", expect ? "checks" : "rejected", DATA[d].name); if (stored) printf("[store]\n"); else printf("[%ld steps]\n", steps); }
        if (!accepted && (expect || getenv("CUBICAL_WHY"))) explain(r, nl3("%%chk-data", word(di), ent));
        free(r); free(tr);
        if (!accepted) { nchkdata--; free(kcode); return; }
        dkey_set(di, key); deps_take(&DDEPS, &capddeps, di); store_put(DATA[d].name, key, kcode, "", steps, NULL); free(kcode);
        char dent[32]; snprintf(dent, sizeof dent, "%%dent%d", di);
        Node *ed = nlist(3); ed->it[0] = nsym("def"); ed->it[1] = nsym(dent); ed->it[2] = nl3("%%data-entry", word(di), ent);
        toplevel(ed);
        table_set(TB_DENT, di, global_lookup(dent));
    } else if (!strcmp(h, "mutual") || !strcmp(h, "mutual-fail")) {
        /* (mutual (checked a T t)...): copattern definitions calling each other */
        if (f->n < 2) die("(mutual (checked name T t)...)", NULL);
        for (int i = 1; i < f->n; i++) if (!f->it[i]->list || f->it[i]->n < 1 || !is(f->it[i]->it[0], "checked")) die("(mutual (checked name T t)...)", NULL);
        checked_group(f->it + 1, f->n - 1, !strcmp(h, "mutual"));
    } else if (!strcmp(h, "observe")) {
        if (f->n != 3 || f->it[1]->list) die("(observe n t)", NULL);
        long steps;
        obs_depth = atoi(f->it[1]->s);
        char *r = readback(f->it[2], &steps);
        obs_depth = 0;
        out_term(f); printf("\n  = "); out_str(r); printf("   [%ld steps]\n", steps);
        free(r);
    } else if (!strcmp(h, "checked-codata") || !strcmp(h, "checked-codata-fail")) {
        /* (checked-codata (codata ...)): declare a codata type and check it: parameter and field
         * types are types, each field's type strictly positive in the type itself */
        bool expect = !strcmp(h, "checked-codata");
        if (f->n != 2 || !f->it[1]->list || !is(f->it[1]->it[0], "codata")) die("(checked-codata[-fail] (codata ...))", NULL);
        Node *dd = f->it[1];
        toplevel(dd);
        int d = ndata - 1;
        GROW(CHKDATA, capchkdata, (size_t)nchkdata + 1);
        CHKDATA[nchkdata++] = d;
        int di = nchkdata - 1;
        Node *ent = nl2("%raw", kapp("dcoent", atel_coparams(dd, d, 0), K("lzero")));
        const char *D = DATA[d].name;
        char den[32], dtn[32], mk[256], cp[256];
        snprintf(den, sizeof den, "%%dentp%d", di); snprintf(dtn, sizeof dtn, "%%dtab%d", di);
        snprintf(mk, sizeof mk, "%%mkrec-%s", D); snprintf(cp, sizeof cp, "%%comp-%s", D);
        Node *pd = nlist(3); pd->it[0] = nsym("def"); pd->it[1] = nsym(den); pd->it[2] = ent;
        toplevel(pd);
        /* the evaluator's table: the type former, the projections, the record */
        Node *td = nlist(3); td->it[0] = nsym("def"); td->it[1] = nsym(dtn);
        { Node *l = K("lnil"); for (int i = DATA[d].nflds - 1; i >= 0; i--) l = kapp("lcons", nsym(FLD[DATA[d].flds[i]].name), l);
          td->it[2] = nl3("app", nl3("app", kapp("dtab", nsym(D), l), nsym(mk)), nsym(D)); }
        toplevel(td);
        table_set(TB_DENT, di, global_lookup(den));
        table_set(TB_DTAB, di, global_lookup(dtn));
        table_set(TB_TRANS, di, global_lookup(cp));
        long steps = 0;
        char key[65]; char *kcode = NULL; key_decl(dd, key, &kcode);
        bool stored = expect && !trusted && store_checked(key);
        char *r = trusted || stored ? strdup("ok") : readback(nl3("%%chk-codata", word(di), ent), &steps);
        bool accepted = !strcmp(r, "ok"), ok = accepted == expect;
        if (ok) passes++; else failures++;
        if (!trusted) { printf("%s: %s codata %s   ", ok ? "ok" : "FAIL", expect ? "checks" : "rejected", D); if (stored) printf("[store]\n"); else printf("[%ld steps]\n", steps); }
        if (!accepted && (expect || getenv("CUBICAL_WHY"))) explain(r, nl3("%%chk-codata", word(di), ent));
        free(r);
        if (!accepted) { nchkdata--; free(kcode); return; }
        dkey_set(di, key); deps_take(&DDEPS, &capddeps, di); store_put(D, key, kcode, "", steps, NULL); free(kcode);
        char dent[32]; snprintf(dent, sizeof dent, "%%dent%d", di);
        Node *ed = nlist(3); ed->it[0] = nsym("def"); ed->it[1] = nsym(dent); ed->it[2] = nl3("%%codata-entry", word(di), ent);
        toplevel(ed);
        table_set(TB_DENT, di, global_lookup(dent));
        /* the generated composition and record, in the signature (no name refers to them) so a
         * stuck call of either is read as a code */
        sig_add(cp, global_lookup(cp), global_lookup(cp), true);
        sig_add(mk, global_lookup(mk), global_lookup(mk), true);
    } else if (!strcmp(h, "codata")) {
        /* (codata D (f A)...) or (codata (D (z T)...) (f A)...): a coinductive record type. A
         * field's type may mention the parameters, the earlier fields and D itself. Generates the
         * type former D, each field's projection f, each field's type as a function of the
         * parameters and a value (%fty-D-f), the record as a function (%mkrec-D), and the
         * composition %comp-D: a copattern definition composing field by field, each along its
         * type at the earlier fields' fillers */
        if (f->n < 2) die("(codata D (f A)...)", NULL);
        Node *hd = f->it[1];
        const char *D = hd->list ? hd->it[0]->s : hd->s;
        int np = hd->list ? hd->n - 1 : 0, nf = f->n - 2;
        const char **pn = calloc((size_t)np + 1, sizeof(char *));
        for (int k = 0; k < np; k++) pn[k] = param_name(hd->it[k + 1])->s;
        decl_fresh(D);
        GROW(DATA, capdata, (size_t)ndata + 1);
        int d = ndata++;
        GROW(DATAPOS, capdatapos, (size_t)ndata); DATAPOS[d] = decl_pos[0] ? strdup(decl_pos) : NULL;
        DATA[d] = (DataInfo){ strdup(D), np, 0, calloc(1, sizeof(int)), -1, -1, f, 1, nf, calloc((size_t)nf + 1, sizeof(int)) };
        Node **fn = calloc((size_t)nf + 1, sizeof(Node *)), **ft = calloc((size_t)nf + 1, sizeof(Node *));
        for (int k = 0; k < nf; k++) {
            Node *it = f->it[k + 2];
            if (!it->list || it->n != 2 || it->it[0]->list) die("a field of a codata type is (f A) in", D);
            fn[k] = it->it[0]; ft[k] = it->it[1];
            GROW(FLD, capfld, (size_t)nfld + 1);
            FLD[nfld] = (FldInfo){ strdup(fn[k]->s), d, k, -1, bnd_mult(fn[k]) };
            DATA[d].flds[k] = nfld++;
        }
        char *buf; size_t len; FILE *m;
#define OPEN() (m = open_memstream(&buf, &len))
#define EMIT() do { fclose(m); emit(buf); free(buf); } while (0)
        OPEN();
        if (np) { fprintf(m, "(def %s (lam", D); for (int k = 0; k < np; k++) fprintf(m, " %s", pn[k]); fprintf(m, " (%%data %s", D); for (int k = 0; k < np; k++) fprintf(m, " %s", pn[k]); fprintf(m, ")))"); }
        else fprintf(m, "(def %s (%%data %s))", D, D);
        EMIT();
        for (int k = 0; k < nf; k++) { OPEN(); fprintf(m, "(def %s (lam %%x (%s %%x)))", fn[k]->s, fn[k]->s); EMIT(); }
        for (int k = 0; k < nf; k++) {
            OPEN();
            fprintf(m, "(def %%fty-%s-%s (lam", D, fn[k]->s);
            for (int q = 0; q < np; q++) fprintf(m, " %s", pn[q]);
            fputs(" %n ", m);
            for (int j = 0; j < k; j++) fprintf(m, "(let %s (%s %%n) ", fn[j]->s, fn[j]->s);
            print_node(m, ft[k]);
            for (int j = 0; j < k; j++) fputc(')', m);
            fputs("))", m);
            EMIT();
            char nm[256]; snprintf(nm, sizeof nm, "%%fty-%s-%s", D, fn[k]->s);
            FLD[DATA[d].flds[k]].fty = global_lookup(nm);
        }
        OPEN();
        fprintf(m, "(def %%mkrec-%s ", D);
        if (nf) { fputs("(lam", m); for (int k = 0; k < nf; k++) fprintf(m, " %%f%d", k); fputc(' ', m); }
        fprintf(m, "(record %s", D);
        for (int k = 0; k < nf; k++) fprintf(m, " (%s %%f%d)", fn[k]->s, k);
        fputs(nf ? ")))" : "))", m);
        EMIT();
        /* comp^i D(z i) [φ ↦ u] a0, field by field: field k composed along its type with the
         * parameters at i and each earlier field j at its filler c_j at i */
        OPEN();
        fprintf(m, "(def %%comp-%s (lam", D);
        for (int q = 0; q < np; q++) fprintf(m, " %%z%d", q);
        fprintf(m, " %%u %%a0 (record %s", D);
        for (int k = 0; k < nf; k++) {
            fprintf(m, " (%s ", fn[k]->s);
            for (int j = 0; j < k; j++) {
                /* c_j i = comp^j (type of j at i∧j) [φ ↦ u (i∧j), (i = 0) ↦ a0] a0, at field j */
                fprintf(m, "(let %%c%d (plam %%i (comp (plam %%j ", j);
                for (int q = 0; q < np; q++) fprintf(m, "(let %s (@ %%z%d (/\\ %%i %%j)) ", pn[q], q);
                for (int r = 0; r < j; r++) fprintf(m, "(let %s (@ %%c%d (/\\ %%i %%j)) ", fn[r]->s, r);
                print_node(m, ft[j]);
                for (int q = 0; q < np + j; q++) fputc(')', m);
                fprintf(m, ") (plam %%j (sys+ (= %%i 0) (%s %%a0) (%s (@ %%u (/\\ %%i %%j))))) (%s %%a0))) ", fn[j]->s, fn[j]->s, fn[j]->s);
            }
            fputs("(comp (plam %i ", m);
            for (int q = 0; q < np; q++) fprintf(m, "(let %s (@ %%z%d %%i) ", pn[q], q);
            for (int r = 0; r < k; r++) fprintf(m, "(let %s (@ %%c%d %%i) ", fn[r]->s, r);
            print_node(m, ft[k]);
            for (int q = 0; q < np + k; q++) fputc(')', m);
            fprintf(m, ") (plam %%i (%s (@ %%u %%i))) (%s %%a0))", fn[k]->s, fn[k]->s);
            for (int j = 0; j < k; j++) fputc(')', m);
            fputc(')', m);
        }
        fputs(")))", m);
        EMIT();
        { char nm[256]; snprintf(nm, sizeof nm, "%%comp-%s", D); DATA[d].comp = global_lookup(nm); }
#undef OPEN
#undef EMIT
        free(fn); free(ft); free(pn);
    } else if (!strcmp(h, "declare")) {
        /* a name already declared and not yet given is declared once */
        for (int i = 1; i < f->n; i++) {
            long g = global_lookup(f->it[i]->s);
            if (g >= 0 && G[g].kind == G_INPUT && !G[g].res->port[0]) continue;
            add_global(f->it[i]->s, G_INPUT);
        }
    } else if (!strcmp(h, "data")) {
/* t22 (2026-11-12): needs-work. For every data declaration this generates, as untyped source text,
 * the type's composition (%comp-D), its homogeneous composition, and transport on each constructor
 * and on formal compositions. That text is never checked: it is trusted as CCHM's rules for the
 * declared type, written once in C per declaration rather than as the single schematic rule CCHM
 * gives for inductive and higher inductive types. Show it is exactly that rule, or replace the
 * per-declaration generation by the schematic rule. */
        /* (data D ctor...) or (data (D z...) ctor...); ctor = (c field... [(dims i...) (φ e)...]),
         * field = x or (x A). Generates the type former D, the constructors, each path
         * constructor's boundary %bdy-c, and from CHM's equations the composition
         * %comp-D = hcomp after transport, transport on each constructor %trans-c
         * and on hcomp %trans-hcomp-D. */
        if (f->n < 2) die("(data D ctor...)", NULL);
        if (f->n - 2 > 64) die("more than 64 constructors in", f->it[1]->list ? f->it[1]->it[0]->s : f->it[1]->s);
        Node *hd = f->it[1];
        const char *D = hd->list ? hd->it[0]->s : hd->s;
        int np = hd->list ? hd->n - 1 : 0, nc = f->n - 2;
        const char **pn = calloc((size_t)np + 1, sizeof(char *));
        for (int k = 0; k < np; k++) pn[k] = param_name(hd->it[k + 1])->s;
        decl_fresh(D);
        GROW(DATA, capdata, (size_t)ndata + 1);
        int d = ndata++;
        GROW(DATAPOS, capdatapos, (size_t)ndata); DATAPOS[d] = decl_pos[0] ? strdup(decl_pos) : NULL;
        DATA[d] = (DataInfo){ strdup(D), np, nc, calloc((size_t)nc + 1, sizeof(int)), -1, -1, f, 0, 0, NULL, 0 };
        /* the trailing parameters named %ix… are indices (an indexed family by fording, E28) */
        for (int k = np - 1; k >= 0 && !strncmp(pn[k], "%ix", 3); k--) DATA[d].nidx++;
        typedef struct { Node *cd; int nf, nd, nb; Node **fn, **ft, **dn, **bf, **be; } CD;
        CD *cs = calloc((size_t)nc + 1, sizeof(CD));
        for (int ci = 0; ci < nc; ci++) {
            Node *cd = f->it[ci + 2];
            if (!cd->list || cd->n < 1 || cd->it[0]->list) die("constructor must be (C field...)", NULL);
            CD *x = &cs[ci]; x->cd = cd;
            x->fn = calloc((size_t)cd->n, sizeof(Node *)); x->ft = calloc((size_t)cd->n, sizeof(Node *));
            x->dn = calloc((size_t)cd->n, sizeof(Node *)); x->bf = calloc((size_t)cd->n, sizeof(Node *)); x->be = calloc((size_t)cd->n, sizeof(Node *));
            int i = 1;
            for (; i < cd->n; i++) {
                Node *it = cd->it[i];
                if (it->list && it->n >= 1 && is(it->it[0], "dims")) break;
                if (it->list) { if (it->n != 2 || it->it[0]->list) die("field must be x or (x A) in", cd->it[0]->s); x->fn[x->nf] = it->it[0]; x->ft[x->nf] = it->it[1]; }
                else x->fn[x->nf] = it;
                x->nf++;
            }
            if (i < cd->n) {
                Node *dl = cd->it[i++];
                for (int k = 1; k < dl->n; k++) x->dn[x->nd++] = dl->it[k];
                for (; i < cd->n; i++) {
                    Node *br = cd->it[i];
                    if (!br->list || br->n != 2) die("boundary must be (φ e) in", cd->it[0]->s);
                    x->bf[x->nb] = br->it[0]; x->be[x->nb] = br->it[1]; x->nb++;
                }
            }
            if (x->nb && !x->nd) die("a boundary needs dimensions in", cd->it[0]->s);
            GROW(CON, capcon, (size_t)ncon + 1);
            decl_fresh(cd->it[0]->s);
            DATA[d].cons[ci] = ncon;
            /* a field of type D itself, or of a path line into D (at any dimension), is recursive:
             * its branch may take its induction hypothesis (18.17) */
            uint64_t rm = 0; int nr = 0; unsigned char rd[64] = { 0 }; const char *rsh[64] = { 0 };
            if (x->nf > 64) die("more than 64 fields in", cd->it[0]->s);
            for (int k = 0; k < x->nf; k++) {
                int depth = rec_depth(x->ft[k], D);
                if (depth >= 0) { rm |= 1ull << k; nr++; rd[k] = (unsigned char)depth; }
                char sh[256]; int sn = 0;
                /* a field recursive under Π codomains or Σ components (18.18): its shape, and the
                 * hypothesis the net builds from it */
                if (rec_shape(x->ft[k], D, sh, &sn)) { sh[sn] = 0; rsh[k] = strdup(sh); if (depth < 0) { rm |= 1ull << k; nr++; } }
            }
            CON[ncon] = (ConInfo){ intern(cd->it[0]->s), d, ci, x->nf, x->nd, -1, -1, nr, rm };
            memcpy(CON[ncon].rdim, rd, sizeof rd);
            memcpy(CON[ncon].rsh, rsh, sizeof rsh);
            for (int k = 0; k < x->nf; k++) CON[ncon].fmult[k] = (unsigned char)bnd_mult(x->fn[k]);   /* (B11) */
            ncon++;
        }
        char *buf; size_t len; FILE *m;
#define OPEN() (m = open_memstream(&buf, &len))
#define EMIT() do { fclose(m); emit(buf); free(buf); } while (0)
#define PN(x) print_node(m, (x))
        /* the type former */
        OPEN();
        if (np) { fprintf(m, "(def %s (lam", D); for (int k = 0; k < np; k++) fprintf(m, " %s", pn[k]); fprintf(m, " (%%data %s", D); for (int k = 0; k < np; k++) fprintf(m, " %s", pn[k]); fprintf(m, ")))"); }
        else fprintf(m, "(def %s (%%data %s))", D, D);
        EMIT();
        for (int ci = 0; ci < nc; ci++) {
            CD *x = &cs[ci]; int c = DATA[d].cons[ci]; const char *cn = CON[c].name;
            /* fields then dimensions, as written */
            char *args; size_t alen; FILE *am = open_memstream(&args, &alen);
            for (int k = 0; k < x->nf; k++) { fputc(' ', am); print_node(am, x->fn[k]); }
            for (int k = 0; k < x->nd; k++) { fputc(' ', am); print_node(am, x->dn[k]); }
            fclose(am);
            if (x->nb) {
                OPEN(); fprintf(m, "(def %%bdy-%s (lam%s (sys", cn, args);
                for (int k = 0; k < x->nb; k++) { fputs(" (", m); PN(x->bf[k]); fputc(' ', m); PN(x->be[k]); fputc(')', m); }
                fprintf(m, ")))"); EMIT();
                char nm[256]; snprintf(nm, sizeof nm, "%%bdy-%s", cn);
                CON[c].bdy = global_lookup(nm);
            }
            OPEN();
            if (x->nf + x->nd) fprintf(m, "(def %s (lam%s (%%con %s%s)))", cn, args, cn, args);
            else fprintf(m, "(def %s (%%con %s))", cn, cn);
            EMIT();
            free(args);
        }
        /* the eliminators as functions (used by the evaluator over codes, whose branches
         * take every recursive field's hypothesis) */
        uint64_t allih = 0;
        for (int ci = 0; ci < nc; ci++) if (CON[DATA[d].cons[ci]].nrec) allih |= 1ull << ci;
        OPEN();
        fprintf(m, "(def %%elim-%s (lam %%P", D);
        for (int k = 0; k < nc; k++) fprintf(m, " %%b%d", k);
        fprintf(m, " %%x (%%case (%%branches %s %llu 0", D, (unsigned long long)allih);
        for (int k = 0; k < nc; k++) fprintf(m, " %%b%d", k);
        fprintf(m, ") %%P (lam %%g %%g) %%x)))");
        EMIT();
        OPEN();
        fprintf(m, "(def %%case-%s (lam", D);
        for (int k = 0; k < nc; k++) fprintf(m, " %%b%d", k);
        fprintf(m, " %%x (%%case (%%branches %s %llu 0", D, (unsigned long long)allih);
        for (int k = 0; k < nc; k++) fprintf(m, " %%b%d", k);
        fprintf(m, ") (lam %%g %%g) %%x)))");
        EMIT();
        /* transport as a function (used by the evaluator over codes) */
        OPEN();
        fprintf(m, "(def %%transf-%s (lam %%psi", D);
        for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
        fprintf(m, " %%x (%%transD %s %%psi", D);
        for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
        fprintf(m, " %%x)))");
        EMIT();
        /* a data type without path constructors whose fields have types: composition goes into
         * the fields when the cap and every side of the tube are one constructor c (CCHM): each
         * field composed along its type at the earlier fields' fillers, its sides the sides'
         * fields. F params u a, applied by the machine once the sides are seen to be c */
        bool fieldwise = nc > 0 && strcmp(D, "Tick") && strcmp(D, "Clock");
        for (int ci = 0; ci < nc; ci++) {
            if (cs[ci].nd) fieldwise = false;
            for (int k = 0; k < cs[ci].nf; k++) if (!cs[ci].ft[k]) fieldwise = false;
        }
        if (fieldwise) {
            OPEN();
            fprintf(m, "(def %%hcompf-%s (lam", D);
            for (int k = 0; k < np; k++) fprintf(m, " %s", pn[k]);
            fprintf(m, " %%u %%a (case %%a");
            for (int ci = 0; ci < nc; ci++) {
                CD *x = &cs[ci]; const char *cn = CON[DATA[d].cons[ci]].name;
                fprintf(m, " ((%s", cn);
                for (int k = 0; k < x->nf; k++) fprintf(m, " %%f%d", k);
                fputs(") ", m);
                if (!x->nf) { fprintf(m, "%s)", cn); continue; }
                for (int k = 0; k < x->nf; k++) {
                    /* field k's filler at r: along its type at the earlier fillers (at r ∧ j), from
                     * the cap's field, its sides the sides' field k */
                    fprintf(m, "(let %%F%d (plam %%r (comp (plam %%j ", k);
                    for (int q = 0; q < k; q++) { fprintf(m, "(let "); PN(x->fn[q]); fprintf(m, " (@ %%F%d (/\\ %%r %%j)) ", q); }
                    PN(x->ft[k]);
                    for (int q = 0; q < k; q++) fputc(')', m);
                    fprintf(m, ") (sys+ (= %%r 0) (plam %%j %%f%d) (sysmap (lam %%w (plam %%j (case (@ %%w (/\\ %%r %%j))", k);
                    /* the side's field k (the sides are c: the other branches are not reached) */
                    for (int cj = 0; cj < nc; cj++) {
                        fprintf(m, " ((%s", CON[DATA[d].cons[cj]].name);
                        for (int q = 0; q < cs[cj].nf; q++) fprintf(m, " %%g%d", q);
                        if (cj == ci) fprintf(m, ") %%g%d)", k); else fprintf(m, ") (@ %%w %%j))");
                    }
                    fprintf(m, "))) %%u)) %%f%d)) ", k);
                }
                fprintf(m, "(%s", cn);
                for (int k = 0; k < x->nf; k++) fprintf(m, " (@ %%F%d 1)", k);
                fputc(')', m);
                for (int k = 0; k < x->nf; k++) fputc(')', m);
                fputc(')', m);
            }
            fputs(")))", m);
            EMIT();
        }
        /* the formal hcomp, with the fieldwise composition at the end parameters when there is one */
        #define HCOMP_OPEN() fputs(fieldwise ? "(%hcompF " : "(hcompD ", m)
        #define HCOMP_CLOSE() do { if (fieldwise && !np) fprintf(m, " %%hcompf-%s", D); \
            else if (fieldwise) { fprintf(m, " (%%hcompf-%s", D); for (int k = 0; k < np; k++) fprintf(m, " (@ %%z%d 1)", k); fputc(')', m); } \
            fputc(')', m); } while (0)
        /* composition: hcomp after transport (CHM), generalized (Cubical Agda's gcomp): the tube
         * gets the complement of its face, on which the composite is the transported cap. On a
         * closed point the tube is then total, so a composition never leaves a formal hcomp behind
         * where it has nothing to compose, and a composition with an empty tube is transport. A
         * closed type transports as the identity. */
        OPEN();
        if (!strcmp(D, "Tick") || !strcmp(D, "Clock")) {
            /* a tick or a clock does not vary along a line: composition is the identity (a later
             * value is composed pointwise, at the same tick) */
            fprintf(m, "(def %%comp-%s (lam", D);
            for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
            fprintf(m, " %%u %%a0 %%a0))");
        }
        else if (!np) {
            fprintf(m, "(def %%comp-%s (lam %%u %%a0 ", D);
            HCOMP_OPEN(); fprintf(m, "(sys+ (%%fneg (faceof %%u)) (plam %%i %%a0) %%u) %%a0"); HCOMP_CLOSE();
            fputs("))", m);
        }
        else {
            fprintf(m, "(def %%comp-%s (lam", D);
            for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
            fprintf(m, " %%u %%a0 (let %%a1 (%%transD %s ff", D);
            for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
            fprintf(m, " %%a0) ");
            HCOMP_OPEN();
            fprintf(m, "(sys+ (%%fneg (faceof %%u)) (plam %%i %%a1) (sysmap (lam %%w (plam %%i (%%transD %s (= %%i 1)", D);
            for (int k = 0; k < np; k++) fprintf(m, " (plam %%j (@ %%z%d (\\/ %%i %%j)))", k);
            fprintf(m, " (@ %%w %%i)))) %%u)) %%a1");
            HCOMP_CLOSE();
            fputs(")))", m);
        }
        EMIT();
        { char nm[256]; snprintf(nm, sizeof nm, "%%comp-%s", D); DATA[d].comp = global_lookup(nm); }
        /* transport on hcomp: into its tube and cap */
        OPEN();
        fprintf(m, "(def %%trans-hcomp-%s (lam", D);
        for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
        if (!np) { fprintf(m, " %%psi %%u %%u0 "); HCOMP_OPEN(); fprintf(m, "%%u %%u0"); HCOMP_CLOSE(); fputs("))", m); }
        else {
            fprintf(m, " %%psi %%u %%u0 ");
            HCOMP_OPEN();
            fprintf(m, "(sysmap (lam %%w (plam %%j (%%transD %s %%psi", D);
            for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
            fprintf(m, " (@ %%w %%j)))) %%u) (%%transD %s %%psi", D);
            for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
            fprintf(m, " %%u0)");
            HCOMP_CLOSE();
            fputs("))", m);
        }
        EMIT();
        #undef HCOMP_OPEN
        #undef HCOMP_CLOSE
        { char nm[256]; snprintf(nm, sizeof nm, "%%trans-hcomp-%s", D); DATA[d].transh = global_lookup(nm); }
        /* transport on each constructor */
        for (int ci = 0; ci < nc; ci++) {
            CD *x = &cs[ci]; int c = DATA[d].cons[ci]; const char *cn = CON[c].name;
            OPEN();
            fprintf(m, "(def %%trans-%s (lam", cn);
            for (int k = 0; k < np; k++) fprintf(m, " %%z%d", k);
            fprintf(m, " %%psi");
            for (int k = 0; k < x->nf; k++) { fputc(' ', m); PN(x->fn[k]); }
            for (int k = 0; k < x->nd; k++) { fputc(' ', m); PN(x->dn[k]); }
            fputc(' ', m);
            if (!np) {
                /* a closed type: the identity */
                if (x->nf + x->nd) { fprintf(m, "(%s", cn); for (int k = 0; k < x->nf; k++) { fputc(' ', m); PN(x->fn[k]); } for (int k = 0; k < x->nd; k++) { fputc(' ', m); PN(x->dn[k]); } fputc(')', m); }
                else fputs(cn, m);
                fputs("))", m);
            } else {
                /* θ_m = transFill^i of field m along its type, with the parameters and the earlier
                 * fields at i ∧ j; a field of type D itself is transported by D's transport */
                for (int k = 0; k < x->nf; k++) {
                    if (!x->ft[k]) die("a parameterized data type needs its fields' types:", cn);
                    Node *A = x->ft[k];
                    bool rec = (!A->list && !strcmp(A->s, D)) || (A->list && A->n >= 1 && !A->it[0]->list && !strcmp(A->it[0]->s, D));
                    fprintf(m, "(let %%t%d (plam %%i ", k);
                    if (rec) {
                        fprintf(m, "(%%transD %s (| %%psi (= %%i 0))", D);
                        for (int q = 0; q < np; q++) fprintf(m, " (plam %%j (@ %%z%d (/\\ %%i %%j)))", q);
                        fputc(' ', m); PN(x->fn[k]); fputc(')', m);
                    } else {
                        fprintf(m, "(comp (plam %%j");
                        for (int q = 0; q < np; q++) fprintf(m, " (let %s (@ %%z%d (/\\ %%i %%j))", pn[q], q);
                        for (int q = 0; q < k; q++) { fputs(" (let ", m); PN(x->fn[q]); fprintf(m, " (@ %%t%d (/\\ %%i %%j))", q); }
                        fputc(' ', m); PN(A);
                        for (int q = 0; q < np + k; q++) fputc(')', m);
                        fprintf(m, ") (sys ((| %%psi (= %%i 0)) (plam %%j "); PN(x->fn[k]); fputs("))) ", m); PN(x->fn[k]); fputc(')', m);
                    }
                    fputs(") ", m);
                }
                /* the constructor on the transported fields */
                char *head; size_t hlen; FILE *hm = open_memstream(&head, &hlen);
                if (x->nf + x->nd) { fprintf(hm, "(%s", cn); for (int k = 0; k < x->nf; k++) fprintf(hm, " (@ %%t%d 1)", k); for (int k = 0; k < x->nd; k++) { fputc(' ', hm); print_node(hm, x->dn[k]); } fputc(')', hm); }
                else fputs(cn, hm);
                fclose(hm);
                if (!x->nb) fputs(head, m);
                else {
                    /* a path constructor: corrected at its boundary by the squeezed boundary,
                     * hcomp^i [φ(r) ↦ α(1-i), ψ ↦ c v r] (c (trans v) r) */
                    fputs("(hcompD (sys+ ", m);
                    if (x->nb > 1) fputs("(|", m);
                    for (int k = 0; k < x->nb; k++) { fputc(' ', m); PN(x->bf[k]); }
                    if (x->nb > 1) fputc(')', m);
                    fprintf(m, " (plam %%i (%%transD %s (| %%psi (= (~ %%i) 1))", D);
                    for (int q = 0; q < np; q++) fprintf(m, " (plam %%j (@ %%z%d (\\/ (~ %%i) %%j)))", q);
                    fputc(' ', m);
                    for (int k = 0; k < x->nf; k++) { fputs("(let ", m); PN(x->fn[k]); fprintf(m, " (@ %%t%d (~ %%i)) ", k); }
                    fputs("(sys", m);
                    for (int k = 0; k < x->nb; k++) { fputs(" (", m); PN(x->bf[k]); fputc(' ', m); PN(x->be[k]); fputc(')', m); }
                    fputc(')', m);
                    for (int k = 0; k < x->nf; k++) fputc(')', m);
                    fprintf(m, ")) (sys (%%psi (plam %%i (%s", cn);
                    for (int k = 0; k < x->nf; k++) { fputc(' ', m); PN(x->fn[k]); }
                    for (int k = 0; k < x->nd; k++) { fputc(' ', m); PN(x->dn[k]); }
                    fprintf(m, "))))) %s)", head);
                }
                free(head);
                for (int k = 0; k < x->nf; k++) fputc(')', m);
                fputs("))", m);
            }
            EMIT();
            char nm[256]; snprintf(nm, sizeof nm, "%%trans-%s", cn);
            CON[c].trans = global_lookup(nm);
        }
#undef OPEN
#undef EMIT
#undef PN
        for (int ci = 0; ci < nc; ci++) { free(cs[ci].fn); free(cs[ci].ft); free(cs[ci].dn); free(cs[ci].bf); free(cs[ci].be); }
        free(cs); free(pn);
    } else if (!strcmp(h, "run")) {
        /* (run e): e a Step W A (lib/world.cub), walked by the driver loop (src/drivers.c, D26) */
        if (f->n != 2) die("(run step)", NULL);
        drivers_run(f->it[1]);
    } else if (!strcmp(h, "trusted")) {
        /* #trusted: the postulates and primitives in scope */
        printf("trusted:");
        for (int i = 0; i < ntrusted; i++) printf("%s %s (%s)", i ? "," : "", TRUSTED[i].name, TRUSTED[i].how);
        if (!ntrusted) printf(" nothing");
        printf("\n");
    } else if (!strcmp(h, "axiom")) {
        /* name, or (name T): a constant of type T; its neutral carries T */
        for (int i = 1; i < f->n; i++) {
            Node *e = f->it[i];
            trusted_add(e->list ? e->it[0]->s : e->s, "postulate");
            if (!e->list) { add_global(e->s, G_AXIOM); continue; }
            if (e->n != 2 || e->it[0]->list) die("(axiom name... (name T)...)", NULL);
            char *tn = malloc(strlen(e->it[0]->s) + 8); sprintf(tn, "%%type-%s", e->it[0]->s);
            Node *d = nlist(3); d->it[0] = nsym("def"); d->it[1] = nsym(tn); d->it[2] = e->it[1];
            toplevel(d);
            long tg = global_lookup(tn);
            long g = add_global(e->it[0]->s, G_AXIOM);
            G[g].tyg = tg;
            free(tn);
        }
    } else if (!strcmp(h, "input")) {
        for (int i = 1; i < f->n; i++) add_global(f->it[i]->s, G_INPUT);
    } else if (!strcmp(h, "supply")) {
        if (f->n != 3) die("(supply name term)", NULL);
        long g = global_lookup(f->it[1]->s);
        if (g < 0 || G[g].kind != G_INPUT || G[g].res->port[0]) die("not an unsupplied input:", f->it[1]->s);
        link(build_top(f->it[2]), P(G[g].res, 0));
        for (int i = 0; i < G[g].npark; i++) net_push(G[g].park_o[i], G[g].park_v[i]);
        G[g].npark = 0;
    } else if (!strcmp(h, "watch")) {
        if (f->n != 3) die("(watch name term)", NULL);
        GROW(watch_name, capwn, (size_t)nwatch + 1); GROW(watch_root, capwr, (size_t)nwatch + 1);
        watch_name[nwatch] = strdup(f->it[1]->s);
        watch_root[nwatch] = cell(O_ROOT, 1);
        link(build_top(f->it[2]), P(watch_root[nwatch], 0));
        nwatch++;
    } else if (!strcmp(h, "same-watch")) {
        if (f->n != 3) die("(same-watch name term)", NULL);
        int w = -1;
        for (int i = 0; i < nwatch; i++) if (!strcmp(watch_name[i], f->it[1]->s)) w = i;
        if (w < 0) die("no such watch", f->it[1]->s);
        char *buf; size_t len;
        FILE *o = open_memstream(&buf, &len);
        ndshow = 0; dcount = 0; vcount = 0;
        /* the watch keeps its value */
        rb(o, held_copy(watch_root[w]));
        fclose(o);
        char *b = readback(f->it[2], NULL);
        if (strcmp(buf, b)) { failures++; printf("FAIL: watch %s\n  left:  %s\n  right: %s\n", f->it[1]->s, buf, b); }
        else { passes++; printf("ok: watch %s\n", f->it[1]->s); }
        free(buf); free(b);
    } else if (!strcmp(h, "show")) {
        long steps, h0 = stat_hits, k0 = stat_keys;
        /* CUBICAL_PROFILE=show: the profile counts only from the last #show on */
        if (prof_on && getenv("CUBICAL_PROFILE") && !strcmp(getenv("CUBICAL_PROFILE"), "show")) {
            extern long prof_odup[65536], prof_oera[65536], prof_rdup[256], prof_rera[256];
            memset(prof_count, 0, (size_t)prof_n * sizeof(long)); memset(prof_dup, 0, (size_t)prof_n * sizeof(long));
            memset(prof_dupk, 0, sizeof prof_dupk); memset(prof_erak, 0, sizeof prof_erak); memset(prof_con, 0, sizeof prof_con); memset(prof_glob, 0, sizeof prof_glob);
            memset(prof_odup, 0, sizeof prof_odup); memset(prof_oera, 0, sizeof prof_oera); memset(prof_rdup, 0, sizeof prof_rdup); memset(prof_rera, 0, sizeof prof_rera);
        }
        const char *tp = getenv("CUBICAL_TRACE");
        if (tp) { net_drain(); trace_out = fopen(tp, "a"); }
        Node *T = reading_type(f->it[1]);
        char *r = T ? readback_at(T, f->it[1], &steps) : readback(f->it[1], &steps);
        if (trace_out) { fprintf(trace_out, "-- %ld steps\n", steps); fclose(trace_out); trace_out = NULL; }
        /* length: every interaction; the size of the value (cells read back), so that the work can be
         * read against the result; and the values found already shared (memo hits), the values shared */
        out_term(f->it[1]); printf("\n  = "); out_str(r); printf("   [%ld steps", steps);
        if (rb_nodes) printf(", %ld cells", rb_nodes);
        if (stat_hits > h0 || stat_keys > k0) printf(", %ld hits, %ld shared", stat_hits - h0, stat_keys - k0);
        printf("]\n");
        free(r);
    } else if (!strcmp(h, "same")) {
        if (f->n != 3) die("(same t u)", NULL);
        /* at a type when either side has one: both at it */
        Node *T = reading_type(f->it[1]); if (!T) T = reading_type(f->it[2]);
        char *a = T ? readback_at(T, f->it[1], NULL) : readback(f->it[1], NULL);
        char *b = T ? readback_at(T, f->it[2], NULL) : readback(f->it[2], NULL);
        if (strcmp(a, b)) {
            failures++;
            printf("FAIL: "); out_term(f->it[1]); printf("\n  left:  "); out_str(a); printf("\n  right: "); out_str(b); printf("\n");
        } else { passes++; printf("ok: "); out_term(f->it[1]); printf("\n"); }
        free(a); free(b);
    } else die("unknown top-level form", h);
}
/* ---- #jit (plan 5.5): binding a compiled definition's bytes ----
 * The type of each parameter and of the result, read off the checked type (desugared): a word, a
 * primitive type, Bytes n, a pair, or a data type applied to its parameters; anything else does not
 * cross and the definition is not bound (it keeps running on the net) */
static JT *jt_new(int k) { JT *t = calloc(1, sizeof(JT)); t->k = k; t->d = -1; return t; }
static JT *jt_of(Node *ty, const char **pn, JT **pv, int np);
static JT *jt_app(Node *h, Node **args, int na, const char **pn, JT **pv, int np) {
    if (h->list) return jt_new(JT_NO);
    for (int i = 0; i < np; i++) if (na == 0 && !strcmp(h->s, pn[i])) return pv[i];
    if (na == 0) {
        if (!strcmp(h->s, "Word")) return jt_new(JT_WORD);
        static const struct { const char *n; int pt; } P[] = { { "Int64", PT_I64 }, { "UInt32", PT_U32 }, { "Float64", PT_F64 }, { "Float32", PT_F32 }, { "UInt64", PT_U64 }, { NULL, 0 } };
        for (int i = 0; P[i].n; i++) if (!strcmp(h->s, P[i].n)) { JT *t = jt_new(JT_PRIM); t->pt = P[i].pt; return t; }
    }
    if (!strcmp(h->s, "Bytes") || !strcmp(h->s, "%bytesty")) return jt_new(JT_BYTES);
    /* a Nat crosses as the count of its successors, a word (lib/x86.cub's encoding) */
    if (na == 0 && !strcmp(h->s, "Nat") && data_lookup("Nat") >= 0) { JT *t = jt_new(JT_NAT); t->d = data_lookup("Nat"); return t; }
    int d = data_lookup(h->s);
    if (d < 0 || DATA[d].nparams != na || DATA[d].co) return jt_new(JT_NO);
    JT *t = jt_new(JT_DATA); t->d = d; t->np = na; t->ps = calloc((size_t)na + 1, sizeof(JT *));
    for (int i = 0; i < na; i++) { t->ps[i] = jt_of(args[i], pn, pv, np); if (t->ps[i]->k == JT_NO) return jt_new(JT_NO); }
    return t;
}
static JT *jt_of(Node *ty, const char **pn, JT **pv, int np) {
    if (!ty->list) return jt_app(ty, NULL, 0, pn, pv, np);
    if (!ty->n) return jt_new(JT_NO);
    Node *h = ty->it[0];
    if (!h->list && !strcmp(h->s, "Sigma")) {
        Node *A = NULL, *B = NULL;
        if (ty->n == 4) { A = ty->it[2]; B = ty->it[3]; }
        else if (ty->n == 3 && ty->it[2]->list && ty->it[2]->n == 3 && is(ty->it[2]->it[0], "lam")) { A = ty->it[1]; B = ty->it[2]->it[2]; }
        if (!A) return jt_new(JT_NO);
        JT *t = jt_new(JT_PAIR); t->a = jt_of(A, pn, pv, np); t->b = jt_of(B, pn, pv, np);
        return t->a->k == JT_NO || t->b->k == JT_NO ? jt_new(JT_NO) : t;
    }
    /* an application: (app f a) nested, or (f a …) */
    Node *args[16]; int na = 0; Node *f = ty;
    while (f->list && f->n == 3 && is(f->it[0], "app") && na < 16) { args[na++] = f->it[2]; f = f->it[1]; }
    if (na) { for (int i = 0; i < na / 2; i++) { Node *s = args[i]; args[i] = args[na - 1 - i]; args[na - 1 - i] = s; } return jt_app(f, args, na, pn, pv, np); }
    if (ty->n - 1 > 16) return jt_new(JT_NO);
    return jt_app(h, ty->it + 1, ty->n - 1, pn, pv, np);
}
/* field j of constructor c of t's data type: its declared type at t's parameters, found once */
JT *jt_field(JT *t, int c, int j) {
    if (!t->fl) t->fl = calloc((size_t)DATA[t->d].ncons + 1, sizeof(JT **));
    if (!t->fl[c]) {
        Node *dd = DATA[t->d].decl, *hd = dd->it[1], *cd = dd->it[2 + c];
        int np = hd->list ? hd->n - 1 : 0, nf = cd->list ? cd->n - 1 : 0;
        const char *pn[64]; for (int k = 0; k < np && k < 64; k++) pn[k] = param_name(hd->it[k + 1])->s;
        t->fl[c] = calloc((size_t)nf + 1, sizeof(JT *));
        for (int k = 0; k < nf; k++) {
            Node *f = cd->it[k + 1];
            t->fl[c][k] = f->list && f->n == 2 ? jt_of(f->it[1], pn, t->ps, np) : jt_new(JT_NO);
        }
    }
    return t->fl[c][j];
}
/* every type reachable from t crosses (a data type's fields, all its constructors, each once) */
static bool jt_crosses(JT *t, int depth) {
    if (depth > 64) return true;
    switch (t->k) {
    case JT_NO: return false;
    case JT_PAIR: return jt_crosses(t->a, depth + 1) && jt_crosses(t->b, depth + 1);
    case JT_DATA:
        for (int c = 0; c < DATA[t->d].ncons; c++) {
            ConInfo *k = &CON[DATA[t->d].cons[c]];
            if (k->ndims) return false;
            for (int j = 0; j < k->nfields; j++) { JT *f = jt_field(t, c, j); if (f != t && !(f->k == JT_DATA && f->d == t->d) && !jt_crosses(f, depth + 1)) return false; if (f->k == JT_NO) return false; }
        }
        return true;
    default: return true;
    }
}
/* a first-order type whose values hold no byte strings (a #build-loop state: copied each round by
 * a definition that rebuilds its cells; a byte string would be shared, not copied) */
static bool jt_plain(JT *t, int depth) {
    if (depth > 64) return true;
    switch (t->k) {
    case JT_NO: case JT_BYTES: return false;
    case JT_PAIR: return jt_plain(t->a, depth + 1) && jt_plain(t->b, depth + 1);
    case JT_DATA:
        for (int c = 0; c < DATA[t->d].ncons; c++) {
            ConInfo *k = &CON[DATA[t->d].cons[c]];
            if (k->ndims) return false;
            for (int j = 0; j < k->nfields; j++) { JT *f = jt_field(t, c, j); if (f->k == JT_NO) return false; if (f != t && !(f->k == JT_DATA && f->d == t->d) && !jt_plain(f, depth + 1)) return false; }
        }
        return true;
    default: return true;
    }
}
static bool checked_type_plain(int ck) { return (size_t)ck < capchkty && CHKTY[ck] && jt_plain(jt_of(CHKTY[ck], NULL, NULL, 0), 0); }
/* does the code (its S-expression readback) under its leading λs read parameter i of n? depth counts
 * the binders entered since: λ, let's body, a path abstraction, ∀, a dimension λ, Π and Σ codomains */
static bool code_reads(Node *c, long target, long depth) {
    if (!c->list || !c->n) return false;
    const char *h = c->it[0]->list ? "" : c->it[0]->s;
    if (!strcmp(h, "cvar") && c->n == 2) { long k = !c->it[1]->list && c->it[1]->s[0] == '#' ? atol(c->it[1]->s + 1) : -1; return k == target + depth; }
    if ((!strcmp(h, "clam") || !strcmp(h, "cplam") || !strcmp(h, "cforall") || !strcmp(h, "cdlam")) && c->n == 2) return code_reads(c->it[1], target, depth + 1);
    if (!strcmp(h, "clet") && c->n == 3) return code_reads(c->it[1], target, depth) || code_reads(c->it[2], target, depth + 1);
    if ((!strcmp(h, "cpi") && c->n == 4) || (!strcmp(h, "csigma") && c->n == 3)) { int a = c->n - 2; return code_reads(c->it[a], target, depth) || code_reads(c->it[a + 1], target, depth + 1); }
    for (int i = 1; i < c->n; i++) if (code_reads(c->it[i], target, depth)) return true;
    return false;
}
/* t6 (2026-11-12): wrong, against the paper (§9.3, "native substitution"): the evaluator may replace
 * a definition by native code only once a target-correctness witness relating the code's execution
 * to the definition exists. Here the bytes are bound after the only check made, that the
 * specialized term converts with the definition (lib/x86.cub jit-plan); the translation from that
 * term to bytes (x-push, x-lift, x-erase, the templates, this boundary's marshaling) carries no
 * relation, so a wrong translation would answer wrongly with nothing to catch it. */
/* the bytes into executable memory, an entry over them, and name rebound to (%native k g a…): the
 * call on closed data runs the bytes, anything else (and an overflow) runs g, the definition */
static int jit_bind(const char *name, int ck, const unsigned char *b, size_t len, int arity, const bool *used) {
    if ((size_t)ck >= capchkty || !CHKTY[ck]) return -1;
    Node *doms[NATIVE_MAXW], *cod; int n = pi_parts(CHKTY[ck], doms, NATIVE_MAXW, &cod);
    if (n != arity || n < 1) return -2;
    JT **jt = calloc((size_t)n + 1, sizeof(JT *));
    /* a parameter the code never reads (a proof, erased) crosses as nothing, whatever its type */
    for (int i = 0; i <= n; i++) {
        if (i < n && !used[i]) { jt[i] = jt_new(JT_ERASED); continue; }
        jt[i] = jt_of(i < n ? doms[i] : cod, NULL, NULL, 0); if (!jt_crosses(jt[i], 0)) return -3 - i;
    }
    void *m = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (m == MAP_FAILED) return -1;
    memcpy(m, b, len);
    if (mprotect(m, len, PROT_READ | PROT_EXEC) != 0) return -1;
    unsigned char kind[NATIVE_MAXW];
    for (int i = 0; i < n; i++) kind[i] = jt[i]->k == JT_WORD ? 4 : jt[i]->k == JT_PRIM ? 5 : jt[i]->k == JT_ERASED ? 9 : 8;   /* 8: converted by type (jit_in); 9: not read */
    GROW(FFI, capffi, (size_t)nffi + 1);
    char sym[300]; snprintf(sym, sizeof sym, "jit:%s", name);
    FFI[nffi] = (Ffi){ strdup(sym), m, n, { 0 }, 4, jt };
    memcpy(FFI[nffi].kind, kind, (size_t)n);
    int k = nffi++;
    long g = global_lookup(name);
    char *buf; size_t bl; FILE *ms = open_memstream(&buf, &bl);
    fprintf(ms, "(def %s (lam", name); for (int i = 0; i < n; i++) fprintf(ms, " %%a%d", i);
    fprintf(ms, " (%%native #%d #%ld", k, g); for (int i = 0; i < n; i++) fprintf(ms, " %%a%d", i);
    fprintf(ms, ")))"); fclose(ms); emit(buf); free(buf);
    return k;
}
#include "syntax.c"
#include "drivers.c"
static char *slurp(const char *path) {
    FILE *f = fopen(path, "rb"); if (!f) die("cannot open", path);
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1); size_t got = fread(b, 1, (size_t)n, f); b[got] = 0; fclose(f);
    return b;
}
static uint64_t run_key[2] = { 0xcbf29ce484222325ull, 0x84222325cbf29ce4ull };
static char **loaded; static size_t caploaded; static int nloaded;
static void key_mix(const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        run_key[0] = (run_key[0] ^ (unsigned char)b[i]) * 0x100000001b3ull;
        run_key[1] = (run_key[1] ^ (unsigned char)b[i]) * 0x1000193ull + (run_key[1] >> 29);
    }
}
static const char *cache_dir(void) { const char *d = getenv("CUBICAL_CACHE"); return d ? d : ".cubical-cache"; }
static void load_file(const char *path, bool must) {
    char *rp = realpath(path, NULL);
    if (!rp) die("cannot open", path);
    for (int i = 0; i < nloaded; i++) if (!strcmp(loaded[i], rp)) { free(rp); return; }
    GROW(loaded, caploaded, (size_t)nloaded + 1); loaded[nloaded++] = rp;
    char *text = slurp(rp);
    size_t pl = strlen(rp);
    if (pl > 4 && !strcmp(rp + pl - 4, ".cub")) {
/* t41 (2026-11-12): two incremental mechanisms overlap. This per-file cache replays a file's
 * elaborated and compiled forms when the hash of every text loaded so far is unchanged, so editing
 * a comment in any earlier file invalidates every later file. The store (src/store.c) already
 * identifies each checked definition by the content of its canonical form, independent of names,
 * files and positions, and skips checking a definition it holds a certificate for. Measured on the
 * library: cold 64 s; store warm, this cache off 29 s; both warm 4.7 s. With this cache warm the
 * run also reports 1,799 passed instead of 1,907, so its reporting differs from a full run's. One
 * mechanism keyed by content at the granularity of a definition, covering elaboration and
 * compilation as well as checking, would subsume both.
 * 2026-11-12, what each per-definition step depends on (read in src/syntax.c and here):
 * - Elaboration of a declaration (syntax.c, the round loop near x_solve_expr and the
 *   definition's): its text, the scope (which global, constructor, field or fixity each name
 *   resolves to) and the types of the definitions it references. Implicit arguments are holes,
 *   solved by running the checker (%chk-def / %infer-ok answer "elab" solutions) up to 400 rounds.
 *   The store's key is a function of the elaborated output, so it can only be consulted once
 *   every hole is solved, so a warm store still pays every solving round (inferred to be most of
 *   the 29 s, not measured).
 * - Compilation (compile_top) depends on the core term and the referenced globals.
 * - Checking (%chk-def) depends on the core term's key and the checker.
 * Observed in src/net.c the same day: the evaluator already keys every term by its structure and
 * keeps one value per live key, but a reference to a definition is keyed by its index in the
 * current run (key_global), and an entry lives only while something holds it. The trusted replay
 * below (trusted = true: nothing in the file is checked) is a second trust root beside the store's
 * certificates. */
        /* the surface language: its imports first, then the running key, then its declarations */
        const char *saved_dir = cur_dir;
        char *dir = strdup(rp); char *sl = strrchr(dir, '/'); if (sl) *sl = 0;
        cur_dir = dir;
        load_surface(text, rp, true);
        /* the libraries before this file (item 17's specialization keys): its own text mixed after */
        cur_file = ++file_serial;
        key_mix(text, strlen(text)); lib_mix(text);
        char key[40]; snprintf(key, sizeof key, "%016llx%016llx", (unsigned long long)run_key[0], (unsigned long long)run_key[1]);
        char *marker = malloc(strlen(cache_dir()) + 48); sprintf(marker, "%s/%s", cache_dir(), key);
        bool saved_trusted = trusted;
        char *cached = NULL;
        { FILE *mf = getenv("CUBICAL_NOCACHE") ? NULL : fopen(marker, "r"); trusted = mf != NULL; if (mf) fclose(mf); }
        if (trusted) cached = slurp(marker);
        /* this module's recorded forms: read (cached) or recorded (checked now) */
        FILE *s_rec = xrec; bool s_recbad = xrec_bad; Node **s_rep = xrep; int s_nrep = nxrep, s_irep = ixrep; bool s_repbad = xrep_bad;
        FILE *s_ccout = cc_out; const unsigned char *s_ccin = cc_in, *s_ccend = cc_end; bool s_ccbad = cc_bad;
        char *recbuf = NULL; size_t reclen = 0, cclen = 0; char *ccbuf = NULL; unsigned char *ccblob = NULL;
        char *ccpath = malloc(strlen(marker) + 4); sprintf(ccpath, "%s.c", marker);   /* the compiled forms, beside the recorded ones */
        xrec = NULL; xrec_bad = false; xrep = NULL; nxrep = ixrep = 0; xrep_bad = false;
        cc_out = NULL; cc_in = cc_end = NULL; cc_bad = false;
        if (cached) {
            const char *saved = src; src = strchr(cached, '\n'); src = src ? src + 1 : cached + strlen(cached);
            Node *f; size_t cap = 0; xrep = NULL;
            while ((f = read_node())) { GROW(xrep, cap, (size_t)nxrep + 1); xrep[nxrep++] = f; }
            src = saved;
            if (!xrep) { xrep = malloc(sizeof(Node *)); }   /* no forms recorded: every form a mismatch */
            FILE *cf = fopen(ccpath, "rb");
            if (cf) {
                fseek(cf, 0, SEEK_END); long n = ftell(cf); fseek(cf, 0, SEEK_SET);
                if (n > 0) { ccblob = malloc((size_t)n); if (fread(ccblob, 1, (size_t)n, cf) == (size_t)n) { cc_in = ccblob; cc_end = ccblob + n; } }
                fclose(cf);
            }
        } else if (!getenv("CUBICAL_NOCACHE")) { xrec = open_memstream(&recbuf, &reclen); cc_out = open_memstream(&ccbuf, &cclen); }
        int f0 = failures;
        bool saved_os = out_surface; out_surface = !getenv("CUBICAL_CORE_OUT");
        load_surface(text, rp, false);
        out_surface = saved_os;
        if (xrec) fclose(xrec);
        if (cc_out) fclose(cc_out);
        if (trusted) {
            printf("cached: %s\n", path);
            if (xrep_bad || ixrep != nxrep) printf("warning: the cache of %s does not match its forms; elaborated forms used where they differ\n", path);
            if (cc_bad) printf("warning: the compiled forms of %s do not match; compiled again where they differ\n", path);
        } else if (failures == f0 && !getenv("CUBICAL_NOCACHE") && !xrec_bad) {
            mkdir(cache_dir(), 0777);
            FILE *mk = fopen(marker, "w"); if (mk) { fputs(rp, mk); fputc('\n', mk); if (recbuf) fputs(recbuf, mk); fclose(mk); }
            if (ccbuf) { FILE *cf = fopen(ccpath, "wb"); if (cf) { fwrite(ccbuf, 1, cclen, cf); fclose(cf); } }
        }
        free(recbuf); free(ccbuf); free(ccblob); free(ccpath); free(cached); free(xrep);
        xrec = s_rec; xrec_bad = s_recbad; xrep = s_rep; nxrep = s_nrep; ixrep = s_irep; xrep_bad = s_repbad;
        cc_out = s_ccout; cc_in = s_ccin; cc_end = s_ccend; cc_bad = s_ccbad;
        trusted = saved_trusted; cur_dir = saved_dir;
        free(marker);
        return;
    }
    die("refused: a file of core forms is outside the type system; only .cub files load:", path);
    /* its imports first (they extend the running key before this file does) */
    {
        const char *saved = src; src = text;
        Node *f; Node **fs = NULL; size_t capfs = 0; int nf = 0;
        while ((f = read_node())) { GROW(fs, capfs, (size_t)nf + 1); fs[nf++] = f; }
        src = saved;
        const char *saved_dir = cur_dir;
        char *dir = strdup(rp); char *sl = strrchr(dir, '/'); if (sl) *sl = 0;
        cur_dir = dir;
        for (int i = 0; i < nf; i++) if (fs[i]->list && fs[i]->n >= 1 && is(fs[i]->it[0], "import")) toplevel(fs[i]);
        key_mix(text, strlen(text)); lib_mix(text);
        char key[40]; snprintf(key, sizeof key, "%016llx%016llx", (unsigned long long)run_key[0], (unsigned long long)run_key[1]);
        char *marker = malloc(strlen(cache_dir()) + 48); sprintf(marker, "%s/%s", cache_dir(), key);
        bool saved_trusted = trusted;
        { FILE *mf = getenv("CUBICAL_NOCACHE") ? NULL : fopen(marker, "r"); trusted = mf != NULL; if (mf) fclose(mf); }
        int f0 = failures;
        for (int i = 0; i < nf; i++) if (!(fs[i]->list && fs[i]->n >= 1 && is(fs[i]->it[0], "import"))) toplevel(fs[i]);
        if (trusted) printf("cached: %s\n", path);
        else if (failures == f0 && !getenv("CUBICAL_NOCACHE")) {
            mkdir(cache_dir(), 0777);
            FILE *mk = fopen(marker, "w"); if (mk) { fputs(rp, mk); fputc('\n', mk); fclose(mk); }
        }
        trusted = saved_trusted;
        cur_dir = saved_dir;
        free(marker); free(fs);
    }
    (void)must;
}
static void prof_print(void);
static void prof_signal(int sig) { (void)sig; fflush(stdout); prof_print(); _Exit(3); }
/* ---------------- the machine as a library (build/libcubical.a, src/libcubical.h) ----------------
 * A C program linked with libcubical.a loads the libraries it needs with cubical_init, then calls
 * cubical_call_w(name, args, n, &out): the definition applied to the word arguments, read to its normal
 * form by the net (synchronous), the result a word in out. Returns 0, or -1 when the result is not
 * a word (its text is then in cubical_last_text) */
static char *last_text;
const char *cubical_last_text(void) { return last_text ? last_text : ""; }
int cubical_init(int nfiles, const char *const *files) {
    static bool started;
    if (!started) { net_start(0); { const char *b = __DATE__ " " __TIME__; key_mix(b, strlen(b)); } prof_enter("(top level)"); started = true; }
    for (int i = 0; i < nfiles; i++) load_file(files[i], true);
    return failures ? -1 : 0;
}
int cubical_call_w(const char *name, const uint64_t *args, int n, uint64_t *out) {
    char *txt; size_t tl; FILE *m = open_memstream(&txt, &tl);
    if (n) { fputc('(', m); fputs(name, m); for (int i = 0; i < n; i++) fprintf(m, " #%llu", (unsigned long long)args[i]); fputc(')', m); }
    else fputs(name, m);
    fclose(m);
    const char *saved = src; src = txt;
    Node *t = read_node();
    src = saved;
    char *r = readback(t, NULL);
    free(txt);
    free(last_text); last_text = r;
    if (r[0] == '#' && r[1] >= '0' && r[1] <= '9') { *out = strtoull(r + 1, NULL, 10); return 0; }
    if (!strcmp(r, "true")) { *out = 1; return 0; }
    if (!strcmp(r, "false")) { *out = 0; return 0; }
    return -1;
}
/* --serve (item 30, the editor protocol's first form): after the files, requests from the standard
 * input, each a block of surface declarations ended by a line holding a single '.', processed as a
 * file would be (checks, readings and errors printed as usual); a fatal error ends the request, not
 * the machine; each reply ends with a line holding '.'. A definition is declared once: an editor
 * sends new declarations and readings, and restarts the machine to redefine (there is no retraction).
 * Run with CUBICAL_WORKERS=0 unless the requests are independent of each other's errors */
/* t62 (2026-11-12): needs-work (goal work-object). Serve mode is how agents and people use the work
 * continuously. Missing: the work given to a term as a value (the declarations and their bindings,
 * each declaration's code), so that views are terms of the language applied to it, and a reading
 * that evaluates a view. Replacing a binding is #retract then a new declaration (t56, closed). */
static void serve(void) {
    static char *buf; static size_t cap;
    size_t len = 0; char line[65536];
    fflush(stdout);
    for (;;) {
        if (!fgets(line, sizeof line, stdin)) break;
        if (!strcmp(line, ".\n") || !strcmp(line, ".")) {
            if (len) {
                buf[len] = 0;
                /* (verdicts in surface syntax, as for a file) */
                bool saved_os = out_surface; out_surface = !getenv("CUBICAL_CORE_OUT");
                jmp_buf jb; if (setjmp(jb) == 0) { die_jb = &jb; load_surface(buf, "<serve>", false); die_jb = NULL; }
                else { die_jb = NULL; toplevel_depth = 0; printf("error: %s\n", die_msg); }   /* (left from inside a form: sweeps resume) */
                out_surface = saved_os;
                net_drain();
            }
            len = 0;
            puts("."); fflush(stdout);
            continue;
        }
        size_t l = strlen(line);
        if (len + l + 2 > cap) { cap = (len + l + 2) * 2; buf = realloc(buf, cap); }
        memcpy(buf + len, line, l); len += l;
    }
}
static void memend_report(void) {
    if (!getenv("CUBICAL_MEMEND")) return;
    /* what a sweep now would keep whole, keep named only, free (bytes of record bodies, shallow) */
    sw_mark(true);
    long nb[3] = { 0 }, nn[3] = { 0 }, ngone = 0;
    for (size_t i = 0; i < RTcap; i++) if (RT[i]) {
        Record *r = RT[i]; int c = ps_has(&sw_liver, r) ? 0 : ps_has(&sw_namer, r) ? 1 : 2;
        nn[c]++; nb[c] += syn_bytes_shallow(r->body); if (r->gone) ngone++;
    }
    sw_unmark(true);
    long gb = 0;
    for (long g = 0; g < nG; g++) if (G[g].kind == G_DEF) gb += syn_bytes_shallow(G[g].body);
    fprintf(stderr, "memend: definitions' top bodies %ld MB; records built from %ld (%ld MB), named only %ld (%ld MB, %ld dropped), unreached %ld (%ld MB); %ld sweeps: %ld records freed, %ld dropped, %ld given back, %ld tables freed\n",
            gb >> 20, nn[0], nb[0] >> 20, nn[1], nb[1] >> 20, ngone, nn[2], nb[2] >> 20, stat_sweeps, stat_swept_recs, stat_dropped_recs, stat_rec_revived, stat_swept_tabs);
}
#ifdef CUBICAL_LIB
int cubical_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    /* the parser and the printer recurse once per nesting level, and a program printed as a byte
     * list is nested once per byte: the stack is raised to 1 GB (or the hard limit) and the
     * machine started again under it, once */
    struct rlimit rl;
    if (!getenv("CUBICAL_STACKED") && getrlimit(RLIMIT_STACK, &rl) == 0) {
        rlim_t want = (rlim_t)1 << 30;
        if (rl.rlim_max != RLIM_INFINITY && rl.rlim_max < want) want = rl.rlim_max;
        if (rl.rlim_cur != RLIM_INFINITY && rl.rlim_cur < want) {
            rl.rlim_cur = want;
            if (setrlimit(RLIMIT_STACK, &rl) == 0) { setenv("CUBICAL_STACKED", "1", 1); execv("/proc/self/exe", argv); }
        }
    }
    const char *w = getenv("CUBICAL_WORKERS");
    prof_on = getenv("CUBICAL_PROFILE") != NULL;
    if (prof_on) signal(SIGTERM, prof_signal);
    prof_enter("(top level)");
    net_start(w ? atoi(w) : 0);
    /* the machine's build is part of every module's key */
    { const char *b = __DATE__ " " __TIME__; key_mix(b, strlen(b)); }
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--print")) {
        /* printing a file: everything else the run prints is silenced */
        fflush(stdout); int so = dup(1); conv_out = fdopen(so, "w");
        FILE *nul = fopen("/dev/null", "w"); dup2(fileno(nul), 1);
    }
    for (int i = 1; i < argc; i++) {
        /* --print f: f's core forms in the surface language; --roundtrip f: each printed form parsed
         * back and compared, while f is loaded */
        if (!strcmp(argv[i], "--print") && i + 1 < argc) { convert_file(argv[++i], false); fflush(stdout); _Exit(0); }
        if (!strcmp(argv[i], "--roundtrip") && i + 1 < argc) { convert_file(argv[++i], true); continue; }
        if (!strcmp(argv[i], "--serve")) continue;
        load_file(argv[i], true);
    }
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--serve")) serve();
    /* steps performed so far: a reading only when the workload has normalized (show) */
    memend_report();
    printf("%d passed, %d failed; %ld interaction steps performed; %ld memo hits, %ld shared keys; %ld cells live; %ld payloads live\n", passes, failures, stat_steps, stat_hits, stat_keys, stat_cells, stat_payloads);
    prof_print();
    extern void memo_dump(void); memo_dump();
    return failures ? 1 : 0;
}
static void prof_print(void) {
    if (prof_on) {
        /* the definitions whose λs and thunks were entered most */
        int *ix = malloc((size_t)prof_n * sizeof(int)), *ix2 = malloc((size_t)prof_n * sizeof(int));
        for (int i = 0; i < prof_n; i++) ix[i] = i;
        for (int i = 0; i < prof_n; i++) for (int j = i + 1; j < prof_n; j++) if (prof_count[ix[j]] > prof_count[ix[i]]) { int t = ix[i]; ix[i] = ix[j]; ix[j] = t; }
        for (int i = 0; i < prof_n && i < 40; i++) fprintf(stderr, "%12ld  %s\n", prof_count[ix[i]], prof_name[ix[i]]);
        { extern long stat_dnf_allocs, stat_cl_allocs, stat_cl_lits, stat_dnf_clauses; fprintf(stderr, "formulas: %ld Dnf allocations (%ld clauses), %ld Clause allocations (%ld literals); %ld payloads live\n", stat_dnf_allocs, stat_dnf_clauses, stat_cl_allocs, stat_cl_lits, stat_payloads); }
        stat_gather_kinds();
        { extern const char *kind_name(int); for (int k = 0; k < K_END; k++) if (stat_kind[k]) fprintf(stderr, "kind %3d %s: %ld\n", k, kind_name(k), stat_kind[k]); }
        { extern const char *kind_name(int); extern long prof_pair[4][256]; static const char *pn[4] = { "SUBST", "ERA", "DUP", "PAPPP" };
          for (int q = 0; q < 4; q++) for (int k = 0; k < 256; k++) if (prof_pair[q][k]) fprintf(stderr, "kind %s·%s: %ld\n", pn[q], kind_name(k), prof_pair[q][k]); }
        { extern long prof_reads, prof_reads_rep, prof_reads_sub, prof_reads_sub_rep, prof_reads_sub_boxes;
          fprintf(stderr, "kind reads-subst-boxes: %ld\n", prof_reads_sub_boxes);
          { extern long prof_sub_held, prof_sub_dropped; fprintf(stderr, "kind subst-held: %ld\nkind subst-dropped: %ld\n", prof_sub_held, prof_sub_dropped); }
          extern long prof_fd_cut, prof_fd_dropped, prof_fd_why[7], prof_fd_pending; { extern long prof_fd_var, prof_fd_prin[512]; fprintf(stderr, "kind fd-var: %ld\n", prof_fd_var); for (int k = 0; k < 512; k++) if (prof_fd_prin[k]) fprintf(stderr, "kind fd-prin %d: %ld\n", k, prof_fd_prin[k]); } fprintf(stderr, "kind fd-cut: %ld\nkind fd-dropped: %ld\nkind fd-unknown root/unbuilt/pending/budget/tries/taint: %ld %ld %ld %ld %ld %ld\nkind fd-pending-bounded: %ld\n", prof_fd_cut, prof_fd_dropped, prof_fd_why[1], prof_fd_why[2], prof_fd_why[3], prof_fd_why[4], prof_fd_why[5], prof_fd_why[6], prof_fd_pending); fprintf(stderr, "kind reads-plain: %ld\nkind reads-plain-repeat: %ld\nkind reads-subst: %ld\nkind reads-subst-repeat: %ld\n", prof_reads, prof_reads_rep, prof_reads_sub, prof_reads_sub_rep); }
        for (int k = 0; k < 64; k++) if (prof_dupk[k] || prof_erak[k]) fprintf(stderr, "value kind %3d: dup %ld era %ld\n", k, prof_dupk[k], prof_erak[k]);
        for (int i = 0; i < prof_n; i++) ix2[i] = i;
        for (int i = 0; i < prof_n; i++) for (int j = i + 1; j < prof_n; j++) if (prof_dup[ix2[j]] > prof_dup[ix2[i]]) { int t = ix2[i]; ix2[i] = ix2[j]; ix2[j] = t; }
        for (int i = 0; i < prof_n && i < 25; i++) fprintf(stderr, "dup %12ld  %s\n", prof_dup[ix2[i]], prof_name[ix2[i]]);
        free(ix2);
        {
            extern long prof_odup[65536], prof_oera[65536], prof_rdup[256], prof_rera[256];
            for (int w = 0; w < 2; w++) {
                long *rec = w ? prof_oera : prof_odup, *rul = w ? prof_rera : prof_rdup;
                int *o = malloc(65536 * sizeof(int)); int no = 0;
                for (int i = 0; i < 65536; i++) if (rec[i]) o[no++] = i;
                for (int i = 0; i < no; i++) for (int j = i + 1; j < no; j++) if (rec[o[j]] > rec[o[i]]) { int t = o[i]; o[i] = o[j]; o[j] = t; }
                for (int i = 0; i < no && i < 25; i++) fprintf(stderr, "%s from %10ld  %s\n", w ? "era" : "dup", rec[o[i]], o[i] == 0 ? "(top or unknown)" : (o[i] - 1 < prof_n ? prof_name[o[i] - 1] : "?"));
                extern const char *kind_name(int); for (int k = 0; k < 256; k++) if (rul[k] > 100) fprintf(stderr, "%s from %10ld  rule kind %d %s\n", w ? "era" : "dup", rul[k], k, kind_name(k));
                free(o);
            }
        }
        for (int c = 0; c < ncon && c < 4096; c++) if (prof_con[c] > 50000) fprintf(stderr, "con dup %10ld %s\n", prof_con[c], CON[c].name);
        for (long g = 0; g < 65536; g++) if (prof_glob[g] > 50000) fprintf(stderr, "glob dup %10ld %s\n", prof_glob[g], name_of(g));
        free(ix);
    }
}
