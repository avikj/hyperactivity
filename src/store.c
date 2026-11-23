/* store.c — codebase keys and the store (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.4 “F34 — The store as the codebase: two keys”). Included by lang.c before toplevel.
 *
 * A checked definition's key is the SHA-256 of the canonical form of its type and its term:
 * the elaborated core forms with every binder nameless (@k, the distance to its binder), every
 * checked definition it mentions replaced by that definition's key (G<key>), every checked data
 * type by its key (D<key>), a constructor or field by its type's key and its index (C<key>.i,
 * F<key>.i), hole solutions in place of their holes, and binder multiplicities (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.1 “B11 — Multiplicities (static side)”) kept. Not
 * in it: names, files, positions, the machine's build. A data type's key is the canonical form of
 * its declaration (its own name `self`); a postulate's or primitive's its name and its type (a
 * postulate is its name: two postulates of one type are two assumptions). A corecursive group's
 * members are keyed by the group's canonical form (members R<i>) and their index.
 *
 * The store, CUBICAL_STORE (default .cubical-store; off under CUBICAL_NOCACHE unless it is
 * set): under <key>/, `code` and `type` (the canonical forms), `check` (one line per checker
 * that accepted it: `ok <steps> <checker>`, the checker named by the hash of the library texts
 * loaded up to the one defining %%chk-def), `usage` (the parameter usages). A definition whose
 * key has a `check` line by the current checker is not checked again. `names`: a log of
 * `name key time`, appended when a name is first bound or bound to a new key (`-` unbinds). */
#include "sha256.c"
#include <dirent.h>
#include <time.h>
int unlink(const char *); int rmdir(const char *); int getpid(void);

typedef struct { char *b; size_t n, cap; } SBuf;
static void sb_putn(SBuf *o, const char *s, size_t l) {
    if (o->n + l + 1 > o->cap) { o->cap = (o->n + l + 1) * 2 + 256; o->b = realloc(o->b, o->cap); }
    memcpy(o->b + o->n, s, l); o->n += l; o->b[o->n] = 0;
}
static void sb_put(SBuf *o, const char *s) { sb_putn(o, s, strlen(s)); }
static void sb_fmt(SBuf *o, const char *fmt, ...) { char b[160]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap); sb_put(o, b); }

typedef char SKey[65];
static SKey *CKEY; static size_t capckey;    /* by checked index; "" none yet */
static SKey *DKEY; static size_t capdkey;    /* by checked data index */
static char chk_salt[65];                   /* the checker's name, "" before %%chk-def is defined */
static const char *ckey_of(int ck) { return ck >= 0 && (size_t)ck < capckey && CKEY[ck][0] ? CKEY[ck] : "?"; }
static const char *dkey_of(int di) { return di >= 0 && (size_t)di < capdkey && DKEY[di][0] ? DKEY[di] : "?"; }
/* The number a code names a checked definition or a checked data type by (cglob n, cdata d ..., and
 * the tables %gval, %gty, %dtab, %dent, %dtrans, %gsteps read with it). It was the place in this
 * machine's tables, the order things were loaded in, so a program that takes the number out of a
 * code literal (step-d in lib/elf.cub) and every term that held one had a key that what was loaded
 * before changed (2026-11-22). The number is the first 60 bits of the key: the same on every
 * machine. Before a key is known (the members of a recursive group, while it is checked) the
 * number is 2^62 + the place, and stays readable afterwards. w: 0 definitions, 1 data types */
#define PUB_PROV ((uint64_t)1 << 62)
typedef struct { uint64_t k; int v; } PubE;
static PubE *PUBM[2]; static size_t PUBCAP[2], PUBN[2];
static uint64_t *PUBOF[2]; static size_t PUBOFCAP[2];
static int pub_get(int w, uint64_t n) {
    if (n >= PUB_PROV) return n - PUB_PROV < 0x7fffffff ? (int)(n - PUB_PROV) : -1;
    if (!PUBCAP[w]) return -1;
    for (size_t h = (size_t)(n * 0x9E3779B97F4A7C15ULL) & (PUBCAP[w] - 1); ; h = (h + 1) & (PUBCAP[w] - 1)) {
        if (PUBM[w][h].v < 0) return -1;
        if (PUBM[w][h].k == n) return PUBM[w][h].v;
    }
}
static void pub_put(int w, uint64_t n, int v) {
    if ((PUBN[w] + 1) * 2 > PUBCAP[w]) {
        size_t oc = PUBCAP[w], nc = oc ? oc * 2 : 1024; PubE *old = PUBM[w];
        PUBM[w] = malloc(nc * sizeof(PubE)); for (size_t i = 0; i < nc; i++) PUBM[w][i].v = -1;
        PUBCAP[w] = nc; PUBN[w] = 0;
        for (size_t i = 0; i < oc; i++) if (old[i].v >= 0) pub_put(w, old[i].k, old[i].v);
        free(old);
    }
    size_t h = (size_t)(n * 0x9E3779B97F4A7C15ULL) & (PUBCAP[w] - 1);
    while (PUBM[w][h].v >= 0) h = (h + 1) & (PUBCAP[w] - 1);
    PUBM[w][h].k = n; PUBM[w][h].v = v; PUBN[w]++;
}
static uint64_t pub_of(int w, int i) { return i >= 0 && (size_t)i < PUBOFCAP[w] && PUBOF[w][i] ? PUBOF[w][i] : PUB_PROV + (uint64_t)i; }
static void pub_set(int w, int i, const char *key) {
    if (i < 0 || strlen(key) < 15) return;
    char b[16]; memcpy(b, key, 15); b[15] = 0; uint64_t n = strtoull(b, NULL, 16); if (!n) n = 1;
    if ((size_t)i >= PUBOFCAP[w]) { size_t o = PUBOFCAP[w]; GROW(PUBOF[w], PUBOFCAP[w], (size_t)i + 1); memset(PUBOF[w] + o, 0, (PUBOFCAP[w] - o) * sizeof(uint64_t)); }
    PUBOF[w][i] = n;
    if (pub_get(w, n) < 0) pub_put(w, n, i);   /* two places holding one key are one object: the first stands for it */
}
static void ckey_set(int ck, const char *k) { if ((size_t)ck >= capckey) { size_t o = capckey; GROW(CKEY, capckey, (size_t)ck + 1); memset(CKEY + o, 0, (capckey - o) * sizeof(SKey)); } snprintf(CKEY[ck], sizeof(SKey), "%s", k); pub_set(0, ck, k); }
static void dkey_set(int di, const char *k) { if ((size_t)di >= capdkey) { size_t o = capdkey; GROW(DKEY, capdkey, (size_t)di + 1); memset(DKEY + o, 0, (capdkey - o) * sizeof(SKey)); } snprintf(DKEY[di], sizeof(SKey), "%s", k); pub_set(1, di, k); }
static void key_hash(const char *s, size_t n, char out[65]) { Sha256 h; unsigned char d[32]; sha256_init(&h); sha256_update(&h, s, n); sha256_final(&h, d); sha256_hex(d, out); }

/* ---- the canonical form ---- */
/* what a form mentions, gathered as it is canonicalized: checked indices (k) and checked data
 * indices (-(di+1)); kept per definition for #safe (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C20) */
typedef struct { int *d; int n; } Deps;
static Deps cur_deps; static Deps *CDEPS; static size_t capcdeps; static Deps *DDEPS; static size_t capddeps;
static void dep_add(int k) { for (int i = 0; i < cur_deps.n; i++) if (cur_deps.d[i] == k) return; cur_deps.d = realloc(cur_deps.d, (size_t)(cur_deps.n + 1) * sizeof(int)); cur_deps.d[cur_deps.n++] = k; }
static void deps_note(const char *name, const Deps *d);
static void deps_take(Deps **tab, size_t *cap, int i) { if ((size_t)i >= *cap) { size_t o = *cap; GROW(*tab, *cap, (size_t)i + 1); memset(*tab + o, 0, (*cap - o) * sizeof(Deps)); } free((*tab)[i].d); (*tab)[i] = cur_deps; deps_note(tab == &CDEPS ? CHECKED[i] : tab == &DDEPS && i < nchkdata ? DATA[CHKDATA[i]].name : NULL, &(*tab)[i]); cur_deps.d = NULL; cur_deps.n = 0; }
static const char **kscope; static size_t capkscope; static int nkscope;
static const char *kself;        /* a data declaration's own name */
static int kgroup_base = -1;     /* a group: its members are the checked indices from here */
static void kpush(const char *x) { GROW(kscope, capkscope, (size_t)nkscope + 1); kscope[nkscope++] = x; }
static void canon(SBuf *o, Node *t);
static bool kser_on;   /* a serialization, not a key: binder names kept (=name), for forms that are read back and checked by name (a branch's rec) */
static bool kfresh_on; static const char **kfresh; static size_t capkfresh; static int nkfresh;
static bool n_isfresh(const char *s) { size_t n = strlen(s); if (n < 3 || !isdigit((unsigned char)s[n - 1])) return false; size_t i = 1; while (i < n && (isalpha((unsigned char)s[i]) || s[i] == '_' || (unsigned char)s[i] >= 0x80)) i++; if (i == 1) return false; while (i < n && isdigit((unsigned char)s[i])) i++; return i == n; }
static void canon_sym(SBuf *o, const char *s);
/* a binder's name kept in a serialization (kser_on): a generated name renumbered as every other
 * occurrence of it is (kfresh), so a name the elaborator uses both as a branch's field and as a
 * binder inside it (a rebuilt scrutinee's let, which `rec` resolves by the field's name) stays one
 * name when read back (2026-11-12: x-case-chain failed warm, "rec of a name that is not a recursive
 * field of its branch", the pattern's %f5221 renumbered and the let's written raw) */
static void kser_name(SBuf *o, const char *s) {
    if (kfresh_on && s[0] == '%' && n_isfresh(s) && global_lookup(s) < 0) {
        int i; for (i = 0; i < nkfresh && strcmp(kfresh[i], s); i++) ;
        if (i == nkfresh) { GROW(kfresh, capkfresh, (size_t)nkfresh + 1); kfresh[nkfresh++] = s; }
        sb_fmt(o, "%%kf%d", i); return;
    }
    sb_put(o, s);
}
static void canon_sym(SBuf *o, const char *s) {
    for (int i = nkscope - 1; i >= 0; i--) if (!strcmp(kscope[i], s)) { sb_fmt(o, "@%d", nkscope - 1 - i); return; }
    if (kself && !strcmp(s, kself)) { sb_put(o, "self"); return; }
    int ck = checked_index(s);
    if (ck >= 0) { dep_add(ck); if (kgroup_base >= 0 && ck >= kgroup_base) sb_fmt(o, "R%d", ck - kgroup_base); else { sb_put(o, "G"); sb_put(o, ckey_of(ck)); } return; }
    int d = data_lookup(s);
    if (d >= 0) { int di = chk_index(d); if (di >= 0) { dep_add(-(di + 1)); sb_put(o, "D"); sb_put(o, dkey_of(di)); return; } }
    int c = con_lookup(s);
    if (c >= 0) {
        int dd = CON[c].data;
        if (kself && !strcmp(DATA[dd].name, kself)) { sb_fmt(o, "Cself.%d", CON[c].index); return; }
        int di = chk_index(dd); if (di >= 0) { dep_add(-(di + 1)); sb_fmt(o, "C%s.%d", dkey_of(di), CON[c].index); return; }
    }
    int fl = fld_lookup(s);
    if (fl >= 0) {
        int dd = FLD[fl].data;
        if (kself && !strcmp(DATA[dd].name, kself)) { sb_fmt(o, "Fself.%d", FLD[fl].index); return; }
        int di = chk_index(dd); if (di >= 0) { dep_add(-(di + 1)); sb_fmt(o, "F%s.%d", dkey_of(di), FLD[fl].index); return; }
    }
    /* under kfresh_on (an elaboration's key and output): a generated name (%, letters, digits: xfresh,
     * afresh), whose number depends on how much was elaborated before it in the run, is numbered by
     * its first occurrence here, so the key does not depend on that (2026-11-12) */
    kser_name(o, s);
}
static void canon_binder(SBuf *o, Node *x) {
    /* a binder: its multiplicity when not the default */
    int m = bnd_mult(x);
    if (m != 2) sb_fmt(o, ":%d", m);
}
/* a λ-like form's binder, readable back (decanon): %b, its multiplicity (:0, :1), ! for an instance
 * binder; with the type written on it, (%b... T), T in the scope of the earlier binders */
static void canon_lbinder(SBuf *o, Node *x) {
    if (x->list) { sb_put(o, "(%bl "); canon(o, x); sb_put(o, ")"); return; }
    Node *T = bnd_ty(x);
    if (T) sb_put(o, "(");
    sb_put(o, "%b"); canon_binder(o, x); if (bnd_inst(x)) sb_put(o, "!");
    if (kser_on) { sb_put(o, "="); kser_name(o, x->s); }
    if (T) { sb_put(o, " "); canon(o, T); sb_put(o, ")"); }
}
static void canon(SBuf *o, Node *t) {
    if (!t->list) { canon_sym(o, t->s); return; }
    if (t->n == 0) { sb_put(o, "()"); return; }
    Node *h = t->it[0];
    if (!h->list) {
        const char *k = h->s;
        if (!strcmp(k, "%guess") && t->n == 4) { canon(o, t->it[3]); return; }
        /* %rebuilt is kept: the compiler reads it (a scrutinee the elaborator rebuilt), so a form
         * read back without it would compile differently (2026-11-12) */
        if (!strcmp(k, "%spine") && t->n == 2) { canon(o, t->it[1]); return; }
        /* a code literal (`code X`, src/lang.c to_code) names a checked definition or a checked data
         * type by its number in this machine's tables, which is the order they were loaded in: in a
         * key the number is written as that one's key, so one definition has one key whatever was
         * loaded before it (2026-11-22: ce-demo2-c in lib/x86.cub had one key on the machine that
         * wrote it and another on a machine that took its root, #2261 and #2412) */
        if (!kser_on && !strcmp(k, "app") && t->n == 3 && !t->it[1]->list && !t->it[2]->list && t->it[2]->s[0] == '#' && !strncmp(t->it[1]->s, "%%c", 3)) {
            const char *c = t->it[1]->s + 2, *key = NULL; uint64_t pn = strtoull(t->it[2]->s + 1, NULL, 10); int n = pn >= PUB_PROV && pn - PUB_PROV < 0x7fffffff ? (int)(pn - PUB_PROV) : -1; char tag = 0;   /* a number of the key is the same everywhere and stays; one of the place is written as the key when it has one */
            if (!strcmp(c, "cglob")) { tag = 'G'; key = ckey_of(n); }
            else if (!strcmp(c, "ccon") || !strcmp(c, "cdata") || !strcmp(c, "cproj") || !strcmp(c, "crec") || !strcmp(c, "celim") || !strcmp(c, "ccase")) { tag = 'D'; key = dkey_of(n); }
            if (tag && key && key[0] != '?') { sb_put(o, "(app "); canon_sym(o, t->it[1]->s); sb_fmt(o, " %c%s)", tag, key); return; }
        }
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && t->n >= 3) {
            sb_put(o, "("); sb_put(o, k);
            int nb = t->n - 2;
            for (int i = 1; i <= nb; i++) { sb_put(o, " "); canon_lbinder(o, t->it[i]); kpush(t->it[i]->list ? "" : t->it[i]->s); }
            sb_put(o, " ");
            canon(o, t->it[t->n - 1]);
            nkscope -= nb;
            sb_put(o, ")");
            return;
        }
        if (!strcmp(k, "let") && t->n == 4 && !t->it[1]->list) {
            sb_put(o, "(let "); if (kser_on) { sb_put(o, "="); kser_name(o, t->it[1]->s); sb_put(o, " "); } canon(o, t->it[2]); sb_put(o, " ");
            kpush(t->it[1]->s); canon(o, t->it[3]); nkscope--;
            sb_put(o, ")");
            return;
        }
    }
    sb_put(o, "(");
    for (int i = 0; i < t->n; i++) { if (i) sb_put(o, " "); canon(o, t->it[i]); }
    sb_put(o, ")");
}
/* a data or codata declaration: (data D c...) / (data (D p...) c...); a constructor's fields bind in
 * turn, then its dimensions; a codata field's type sees the earlier fields */
static void canon_decl(SBuf *o, Node *dd) {
    Node *hd = dd->it[1];
    const char *saved_self = kself; int base = nkscope;
    kself = hd->list ? hd->it[0]->s : hd->s;
    sb_put(o, "("); sb_put(o, dd->it[0]->s);
    if (hd->list) for (int k = 1; k < hd->n; k++) {
        Node *p = hd->it[k];
        sb_put(o, " [");
        if (p->list && p->n == 2) { canon_binder(o, p->it[0]); canon(o, p->it[1]); kpush(p->it[0]->s); }
        else { canon_binder(o, p); kpush(p->list ? "" : p->s); }
        sb_put(o, "]");
    }
    bool codata = !strcmp(dd->it[0]->s, "codata");
    for (int ci = 2; ci < dd->n; ci++) {
        Node *cd = dd->it[ci];
        if (!cd->list || cd->n < 1) { sb_put(o, " ?"); continue; }
        if (codata) {
            /* (f A): a field; the later fields' types see it */
            sb_put(o, " ("); canon_binder(o, cd->it[0]); if (cd->n > 1) canon(o, cd->it[1]); sb_put(o, ")");
            kpush(cd->it[0]->s);
            continue;
        }
        int n0 = nkscope;
        sb_put(o, " (");
        int i = 1;
        for (; i < cd->n; i++) {
            Node *it = cd->it[i];
            if (it->list && it->n >= 1 && is(it->it[0], "dims")) break;
            if (i > 1) sb_put(o, " ");
            if (it->list && it->n == 2) { canon_binder(o, it->it[0]); sb_put(o, "["); canon(o, it->it[1]); sb_put(o, "]"); kpush(it->it[0]->s); }
            else { canon_binder(o, it); sb_put(o, "[]"); kpush(it->list ? "" : it->s); }
        }
        if (i < cd->n) {
            Node *dl = cd->it[i++];
            sb_fmt(o, " dims:%d", dl->n - 1);
            for (int k = 1; k < dl->n; k++) kpush(dl->it[k]->list ? "" : dl->it[k]->s);
            for (; i < cd->n; i++) { sb_put(o, " "); canon(o, cd->it[i]); }
        }
        sb_put(o, ")");
        nkscope = n0;
    }
    nkscope = base;
    sb_put(o, ")");
    kself = saved_self;
}

/* a declaration as it is kept beside its key: (decl KIND NAME (B [T])... (c CNAME item...)...), a
 * parameter or field a binder with its type, dims and the terms after them as they stand */
static void ser_item(SBuf *o, Node *p) {
    sb_put(o, " (");
    if (p->list && p->n == 2) { canon_lbinder(o, p->it[0]); sb_put(o, " "); canon(o, p->it[1]); kpush(p->it[0]->list ? "" : p->it[0]->s); }
    else { canon_lbinder(o, p); kpush(p->list ? "" : p->s); }
    sb_put(o, ")");
}
static void ser_decl(SBuf *o, Node *dd) {
    Node *hd = dd->it[1];
    const char *saved_self = kself; int base = nkscope; bool sk = kser_on, sf = kfresh_on;
    kser_on = true; kfresh_on = true; nkfresh = 0;
    kself = hd->list ? hd->it[0]->s : hd->s;
    sb_put(o, "(decl "); sb_put(o, dd->it[0]->s); sb_put(o, " "); sb_put(o, kself); sb_put(o, " (params");
    if (hd->list) for (int k = 1; k < hd->n; k++) ser_item(o, hd->it[k]);
    sb_put(o, ")");
    bool codata = !strcmp(dd->it[0]->s, "codata");
    for (int ci = 2; ci < dd->n; ci++) {
        Node *cd = dd->it[ci];
        if (!cd->list || cd->n < 1) continue;
        if (codata) { sb_put(o, " (f "); sb_put(o, cd->it[0]->s); if (cd->n > 1) { sb_put(o, " "); canon(o, cd->it[1]); } sb_put(o, ")"); kpush(cd->it[0]->s); continue; }
        int n0 = nkscope;
        sb_put(o, " (c "); sb_put(o, cd->it[0]->s);
        int i = 1;
        for (; i < cd->n; i++) { Node *it = cd->it[i]; if (it->list && it->n >= 1 && is(it->it[0], "dims")) break; ser_item(o, it); }
        if (i < cd->n) {
            Node *dl = cd->it[i++];
            sb_put(o, " (dims");
            for (int k = 1; k < dl->n; k++) { sb_put(o, " "); sb_put(o, dl->it[k]->list ? "_" : dl->it[k]->s); kpush(dl->it[k]->list ? "" : dl->it[k]->s); }
            sb_put(o, ")");
            for (; i < cd->n; i++) { sb_put(o, " (t "); canon(o, cd->it[i]); sb_put(o, ")"); }
        }
        sb_put(o, ")");
        nkscope = n0;
    }
    sb_put(o, ")");
    nkscope = base; kself = saved_self; kser_on = sk; kfresh_on = sf;
}
/* ---- reading a canonical form back (2026-11-12): the inverse of canon for the forms canon
 * keeps whole. Bound variables get fresh names; G<key>, D<key>, C<key>.i, F<key>.i are rendered
 * through the current bindings (the unhidden checked definition, the checked data type with that
 * key); any other symbol is itself. NULL when a key is bound to nothing now. ---- */
static const char **dscope; static size_t capdscope; static int ndscope; static long dfresh;
static bool dc_bad;
static int data_of_dkey(const char *k) { for (int i = 0; i < nchkdata; i++) if (!strcmp(dkey_of(i), k)) return CHKDATA[i]; return -1; }
static const char *lk_self; static char **lk_cons; static int lk_ncons; static char **lk_flds; static int lk_nflds;   /* a declaration being read back (src/syntax.c, load_decl): its own name and its constructors' and fields' */
static const char *dc_sym(const char *s) {
    if (lk_self) {
        if (!strcmp(s, "self")) return lk_self;
        if (!strncmp(s, "Cself.", 6)) { int ix = atoi(s + 6); if (ix >= 0 && ix < lk_ncons) return lk_cons[ix]; dc_bad = true; return s; }
        if (!strncmp(s, "Fself.", 6)) { int ix = atoi(s + 6); if (ix >= 0 && ix < lk_nflds) return lk_flds[ix]; dc_bad = true; return s; }
    }
    if (s[0] == '@' && s[1] >= '0' && s[1] <= '9') { int k = atoi(s + 1); if (k < ndscope) return dscope[ndscope - 1 - k]; dc_bad = true; return s; }
    size_t n = strlen(s);
    if (s[0] == 'G' && n == 65) { for (int i = nchecked - 1; i >= 0; i--) if (!CHKHIDE[i] && !strcmp(ckey_of(i), s + 1)) return CHECKED[i]; dc_bad = true; return s; }
    if (s[0] == 'D' && n == 65) { int d = data_of_dkey(s + 1); if (d >= 0) return DATA[d].name; dc_bad = true; return s; }
    if ((s[0] == 'C' || s[0] == 'F') && n > 66 && s[65] == '.') {
        char k[65]; memcpy(k, s + 1, 64); k[64] = 0; int d = data_of_dkey(k), ix = atoi(s + 66);
        if (d >= 0 && s[0] == 'C') for (int c = 0; c < ncon; c++) if (CON[c].data == d && CON[c].index == ix) return CON[c].name;
        if (d >= 0 && s[0] == 'F') for (int f = 0; f < nfld; f++) if (FLD[f].data == d && FLD[f].index == ix) return FLD[f].name;
        dc_bad = true; return s;
    }
    return s;
}
static Node *dc_node(Node *t);
static Node *dc_binder(Node *b) {
    /* %b[:m][!] or (%b[:m][!] T) or (%bl X) */
    char nm[32]; snprintf(nm, sizeof nm, "%%d%ld", dfresh++);
    if (b->list && b->n == 2 && is(b->it[0], "%bl")) { GROW(dscope, capdscope, (size_t)ndscope + 1); dscope[ndscope++] = ""; return dc_node(b->it[1]); }
    Node *h = b->list ? (b->n >= 1 ? b->it[0] : NULL) : b;
    if (!h || h->list || strncmp(h->s, "%b", 2)) { dc_bad = true; return b; }
    const char *eq = strchr(h->s + 2, '=');
    Node *x = nsym(eq ? eq + 1 : nm);
    if (b->list && b->n == 2) bnd_set(x, dc_node(b->it[1]));
    const char *f = h->s + 2;
    if (f[0] == ':') { bnd_set_mult(x, f[1] - '0'); f += 2; }
    if (f[0] == '!') bnd_set_inst(x);
    GROW(dscope, capdscope, (size_t)ndscope + 1); dscope[ndscope++] = x->s;
    return x;
}
static Node *dc_node(Node *t) {
    if (!t->list) return nsym(dc_sym(t->s));
    if (t->n == 0) return t;
    Node *h = t->it[0];
    if (!h->list && (!strcmp(h->s, "lam") || !strcmp(h->s, "plam") || !strcmp(h->s, "forall")) && t->n >= 3) {
        Node *r = nlist(t->n); r->it[0] = h; int base = ndscope;
        for (int i = 1; i < t->n - 1; i++) r->it[i] = dc_binder(t->it[i]);
        r->it[t->n - 1] = dc_node(t->it[t->n - 1]); ndscope = base; return r;
    }
    if (!h->list && !strcmp(h->s, "let") && (t->n == 3 || (t->n == 4 && !t->it[1]->list && t->it[1]->s[0] == '='))) {
        char nm[32]; snprintf(nm, sizeof nm, "%%d%ld", dfresh++);
        int o = t->n - 3;
        Node *r = nlist(4); r->it[0] = h; r->it[1] = nsym(o ? t->it[1]->s + 1 : nm); r->it[2] = dc_node(t->it[1 + o]);
        GROW(dscope, capdscope, (size_t)ndscope + 1); dscope[ndscope++] = r->it[1]->s;
        r->it[3] = dc_node(t->it[2 + o]); ndscope--; return r;
    }
    Node *r = nlist(t->n); for (int i = 0; i < t->n; i++) r->it[i] = dc_node(t->it[i]);
    return r;
}
static Node *decanon(const char *text) {
    const char *saved = src; src = text; Node *n = read_node(); src = saved;
    if (!n) return NULL;
    dc_bad = false; ndscope = 0; Node *r = dc_node(n);
    return dc_bad ? NULL : r;
}

/* the checker's name: the hash of the library texts loaded up to the one defining %%chk-def
 * together with the machine's own binary (/proc/self/exe; its build date and time when that
 * cannot be read). A definition's identity key stays build-independent; its certificate does
 * not: a change to the rules in net.c (conversion, composition, a primitive) with check.cub
 * unchanged must recheck, so a rebuilt machine rechecks the library once and then hits. */
static Sha256 lib_hash; static bool lib_hash_on;
static void lib_mix(const char *text) { if (!lib_hash_on) { sha256_init(&lib_hash); lib_hash_on = true; } sha256_update(&lib_hash, text, strlen(text)); }
static void chk_salt_set(void) {
    Sha256 h = lib_hash; unsigned char d[32]; if (!lib_hash_on) sha256_init(&h);
    FILE *f = fopen("/proc/self/exe", "rb"); bool mixed = false;
    if (f) { char buf[65536]; size_t n; while ((n = fread(buf, 1, sizeof buf, f)) > 0) { sha256_update(&h, buf, n); mixed = true; } fclose(f); }
    if (!mixed) { const char *bt = __DATE__ " " __TIME__; sha256_update(&h, bt, strlen(bt)); }
    sha256_final(&h, d); sha256_hex(d, chk_salt);
}

/* ---- the store ---- */
static const char *store_dir(void) {
    static const char *d; static bool looked;
    if (!looked) { looked = true; d = getenv("CUBICAL_STORE"); if (!d && !getenv("CUBICAL_NOCACHE")) d = ".cubical-store"; if (d && !*d) d = NULL; }
    return d;
}
/* the source (2026-11-16, Avik: "there will be no file system (asap)"): the written code and documents
 * as objects every session shares, store/ at the repository's root: each piece of text
 * (<hash>/text), each module's list of pieces (<hash>/module), and the pointers modules/<path>,
 * written/<name>, module-of/<name>, names-of/<path> (lib/host.cub reads them as h-source). The rest of the store above
 * is the machine's own, kept per machine. */
static void root_init(void);
static char *repo_root;
static const char *source_dir(void) {
    static char *d;
    if (!d) { root_init(); d = malloc(strlen(repo_root) + 7); sprintf(d, "%s/store", repo_root); }
    return d;
}
static char *path_in(const char *d, const char *key, const char *file) {
    if (!d) return NULL;
    char *p = malloc(strlen(d) + strlen(key) + strlen(file) + 4); sprintf(p, "%s/%s%s%s", d, key, *file ? "/" : "", file);
    return p;
}
static char *read_in(const char *d, const char *key, const char *file) {
    char *p = path_in(d, key, file); if (!p) return NULL;
    FILE *f = fopen(p, "rb"); free(p); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1); size_t got = fread(b, 1, (size_t)n, f); b[got] = 0; fclose(f);
    return b;
}
static void write_in(const char *d, const char *key, const char *file, const char *text, bool append);
static char *store_path(const char *key, const char *file) { return path_in(store_dir(), key, file); }
static char *store_read(const char *key, const char *file) { return read_in(store_dir(), key, file); }
static void store_write(const char *key, const char *file, const char *text, bool append) { write_in(store_dir(), key, file, text, append); }
static char *src_path(const char *key, const char *file) { return path_in(source_dir(), key, file); }
static char *src_read(const char *key, const char *file) { return read_in(source_dir(), key, file); }
static void src_write(const char *key, const char *file, const char *text) { write_in(source_dir(), key, file, text, false); }
static void write_in(const char *d, const char *key, const char *file, const char *text, bool append) {
    char *p = path_in(d, key, file); if (!p) return;
    char *dir = path_in(d, key, "");
    mkdir(d, 0777); mkdir(dir, 0777); free(dir);
    if (append) { FILE *f = fopen(p, "a"); free(p); if (!f) return; fputs(text, f); fclose(f); return; }
    /* a whole entry is written beside it and renamed over it (atomic on POSIX): a machine killed while
     * writing, or another machine reading the same store, never sees a truncated entry */
    /* (named shortly, in the same directory: an entry's own name may take a file name's whole length) */
    char *t = malloc(strlen(d) + strlen(key) + 40); sprintf(t, "%s/%s%s.%ld.tmp", d, key, *file ? "/" : "", (long)getpid());
    FILE *f = fopen(t, "w"); if (!f) { free(t); free(p); return; }
    bool ok = fputs(text, f) >= 0; ok = (fclose(f) == 0) && ok;
    if (!ok || rename(t, p) != 0) unlink(t);
    free(t); free(p);
}
/* what is kept beside a key for every machine (2026-11-22): a checked definition's forms as
 * elaborated, its implicit mask, whether it computes, a data type's declaration, a definition's
 * text. They are objects of the pool, under the key, written once: a machine that never checked
 * the definition takes it by key from them (src/syntax.c, load_key) and checks it itself */
static char *kobj_read(const char *key, const char *file) { return src_read(key, file); }
static void kobj_write(const char *key, const char *file, const char *text) {
    char *p = src_path(key, file); if (!p) return;
    FILE *f = fopen(p, "rb"); if (f) fclose(f); else src_write(key, file, text);
    free(p);
}
/* a `check` line by the current checker */
static bool store_checked(const char *key) {
    if (!chk_salt[0]) return false;
    char *c = store_read(key, "check"); if (!c) return false;
    bool hit = false;
    for (char *l = c; *l && !hit; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        char *sp = strrchr(l, ' ');
        if (sp && !strcmp(sp + 1, chk_salt) && !strncmp(l, "ok ", 3)) hit = true;
        l = e ? e + 1 : l + strlen(l);
    }
    free(c);
    return hit;
}
/* the steps on the key's ok line for the current checker (the latest such line), 0 when none */
static long store_steps(const char *key) {
    if (!chk_salt[0]) return 0;
    char *c = store_read(key, "check"); if (!c) return 0;
    long steps = 0;
    for (char *l = c; *l; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        char *sp = strrchr(l, ' ');
        if (sp && !strcmp(sp + 1, chk_salt) && !strncmp(l, "ok ", 3)) steps = atol(l + 3);
        l = e ? e + 1 : l + strlen(l);
    }
    free(c);
    return steps;
}
/* the names log: name key time; read once, the latest key of each name kept */
typedef struct { char *name; char *key; } NameBind;
static NameBind *NB; static size_t capnb; static int nnb; static bool nb_read;
/* a name's place in NB found by hashing it: the log is a line per binding ever made, so finding each
 * line's name by a scan of NB was quadratic in the log (4.4 billion instructions a load, 2026-11-18) */
static int *NBH; static size_t capnbh;
static size_t nb_hash(const char *s) { size_t h = 1469598103934665603ull; for (; *s; s++) h = (h ^ (unsigned char)*s) * 1099511628211ull; return h; }
static void nbh_put(int i) { size_t m = capnbh - 1, j = nb_hash(NB[i].name) & m; while (NBH[j] >= 0) j = (j + 1) & m; NBH[j] = i; }
static int nb_find(const char *name) {
    if (!capnbh) return -1;
    size_t m = capnbh - 1, j = nb_hash(name) & m;
    for (; NBH[j] >= 0; j = (j + 1) & m) if (!strcmp(NB[NBH[j]].name, name)) return NBH[j];
    return -1;
}
static int nb_add(const char *name) {
    if ((size_t)(nnb + 1) * 2 > capnbh) {
        size_t nc = capnbh ? capnbh * 2 : 1024; free(NBH); NBH = malloc(nc * sizeof(int)); capnbh = nc;
        for (size_t j = 0; j < nc; j++) NBH[j] = -1;
        for (int i = 0; i < nnb; i++) nbh_put(i);
    }
    GROW(NB, capnb, (size_t)nnb + 1); NB[nnb].name = strdup(name); NB[nnb].key = NULL; nbh_put(nnb);
    return nnb++;
}
static void names_load(void) {
    if (nb_read) return;
    nb_read = true;
    char *t = store_read("names", ""); if (!t) return;
    for (char *l = t; *l; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        char *s1 = strchr(l, ' ');
        if (s1) {
            *s1 = 0; char *s2 = strchr(s1 + 1, ' '); if (s2) *s2 = 0;
            int i = nb_find(l); if (i < 0) i = nb_add(l);
            free(NB[i].key); NB[i].key = strdup(s1 + 1);
        }
        l = e ? e + 1 : l + strlen(l);
    }
    free(t);
}
/* whether the file being read is part of the work (src/syntax.c, w_held): a scratch file's definitions
 * are checked like any, but bind no name and enter no name's history */
static bool bind_work = true;
/* a text that is no module of the work whose definitions are kept in the pool all the same (their
 * forms, mask and declaration beside their keys): one declaration being written on its own, which
 * is labelled once it has checked (src/lang.c, --keep) */
static bool keep_free;
/* the module being read, when it is one of the work (src/syntax.c): a name another module holds
 * (module-of/NAME names that one) is not bound from this one, so what a name's label is does not
 * depend on which module was read last; a name this module comes to hold is bound when it is
 * claimed (w_file) */
static const char *bind_rel;
static char *idx_read(const char *dir, const char *hx);
static void names_bind(const char *name, const char *key) {
    if (!store_dir() || !bind_work) return;
    /* bound/<name in hex>: the name's key now, read by name with no log to scan */
    SBuf hx = {0}; for (const unsigned char *c = (const unsigned char *)name; *c; c++) sb_fmt(&hx, "%02x", *c);
    if (bind_rel && hx.b && hx.n <= 250) { char *mo = idx_read("module-of", hx.b); if (mo) mo[strcspn(mo, "\n")] = 0;
        if (mo && strcmp(mo, bind_rel)) { free(mo); free(hx.b); return; } free(mo); }
    if (hx.b && hx.n <= 250) { char *cur = store_read("bound", hx.b); if (!cur || strcmp(cur, key)) store_write("bound", hx.b, key, false); free(cur); }
    free(hx.b);
    names_load();
    int i = nb_find(name);
    if (i >= 0 && NB[i].key && !strcmp(NB[i].key, key)) return;
    if (i < 0) i = nb_add(name);
    free(NB[i].key); NB[i].key = strdup(key);
    char line[512]; snprintf(line, sizeof line, "%s %s %ld\n", name, key, (long)time(NULL));
    mkdir(store_dir(), 0777);
    char *p = store_path("names", ""); FILE *f = fopen(p, "a"); free(p);
    if (f) { fputs(line, f); fclose(f); }
}
/* deps/<name in hex>: the names a definition mentions, one per line, as the checker found them;
 * users/<name in hex>: a line for each name whose definition has mentioned it (a reader keeps a
 * line only while that name's deps/ still holds the name, so a change of mind is no stale use) */
static void deps_note(const char *name, const Deps *d) {
    if (!name || name[0] == '%' || !store_dir()) return;
    SBuf hx = {0}; for (const unsigned char *c = (const unsigned char *)name; *c; c++) sb_fmt(&hx, "%02x", *c);
    if (!hx.b || hx.n > 250) { free(hx.b); return; }
    SBuf o = {0}; sb_put(&o, "");
    for (int j = 0; j < d->n; j++) {
        int k = d->d[j]; const char *dn = k >= 0 ? CHECKED[k] : -k - 1 < nchkdata ? DATA[CHKDATA[-k - 1]].name : NULL;
        if (dn && dn[0] != '%' && strcmp(dn, name)) { sb_put(&o, dn); sb_put(&o, "\n"); }
    }
    char *cur = store_read("deps", hx.b);
    if (!cur || strcmp(cur, o.b)) {
        store_write("deps", hx.b, o.b, false);
        SBuf line = {0}; sb_put(&line, name); sb_put(&line, "\n");
        for (char *l = o.b; *l; ) {
            char *e = strchr(l, '\n'); *e = 0;
            SBuf dh = {0}; for (const unsigned char *c = (const unsigned char *)l; *c; c++) sb_fmt(&dh, "%02x", *c);
            if (dh.b && dh.n <= 250) store_write("users", dh.b, line.b, true);
            free(dh.b); *e = '\n'; l = e + 1;
        }
        free(line.b);
    }
    free(cur); free(o.b); free(hx.b);
}
/* a refusal by the current checker, as a `no` line: the checker's answer (refused at code why ...),
 * as it was read back (2026-11-12: refusals were not recorded, so what fails, and why, was known
 * only by rerunning). The answer names definitions by name, so after a rename its explanation can
 * show the old name; the verdict is the key's. NULL when none. */
static char *store_refused(const char *key) {
    if (!chk_salt[0]) return NULL;
    char *c = store_read(key, "check"); if (!c) return NULL;
    char *hit = NULL; size_t sl = strlen(chk_salt);
    for (char *l = c; *l && !hit; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        if (!strncmp(l, "no ", 3)) {
            char *a = strchr(l + 3, ' ');
            if (a && !strncmp(a + 1, chk_salt, sl) && a[1 + sl] == ' ') hit = strdup(a + 2 + sl);
        }
        l = e ? e + 1 : l + strlen(l);
    }
    free(c);
    return hit;
}
static void store_refuse(const char *key, long steps, const char *r) {
    if (!store_dir() || !chk_salt[0] || store_checked(key)) return;
    char *old = store_refused(key); if (old) { free(old); return; }
    size_t n = strlen(r) + 200; char *l = malloc(n);
    snprintf(l, n, "no %ld %s %s\n", steps, chk_salt, r);
    for (char *p = l; p[1]; p++) if (*p == '\n') *p = ' ';
    store_write(key, "check", l, true); free(l);
}
/* a definition's entry: written when it is checked (or replayed as checked) */
static void store_put(const char *name, const char *key, const char *code, const char *type, long steps, const char *usage) {
    if (!store_dir()) return;
    if (code) store_write(key, "code", code, false);
    if (type) store_write(key, "type", type, false);
    if (usage) store_write(key, "usage", usage, false);
    if (chk_salt[0] && !store_checked(key)) { char l[160]; snprintf(l, sizeof l, "ok %ld %s\n", steps, chk_salt); store_write(key, "check", l, true); }
    if (name) names_bind(name, key);
}

/* a data type's declaration kept beside its key, and each of its constructors' and fields' names
 * pointed at the key (conof/NAME), so a name of either finds the type that holds it */
static void store_decl(const char *key, Node *dd, int d) {
    if (!store_dir()) return;
    char *have = kobj_read(key, "decl");
    if (!have) { SBuf s = { 0 }; int sd = cur_deps.n; ser_decl(&s, dd); cur_deps.n = sd; if (s.b && (bind_work || keep_free)) kobj_write(key, "decl", s.b); free(s.b); }
    free(have);
    if (!bind_work) return;
    for (int pass = 0; pass < 2; pass++) {
        int n = pass == 0 ? DATA[d].ncons : nfld;
        for (int i = 0; i < n; i++) {
            const char *nm = pass == 0 ? CON[DATA[d].cons[i]].name : FLD[i].data == d ? FLD[i].name : NULL;
            if (!nm) continue;
            SBuf hx = { 0 }; for (const unsigned char *c = (const unsigned char *)nm; *c; c++) sb_fmt(&hx, "%02x", *c);
            if (hx.b && hx.n <= 250) { char *cur = store_read("conof", hx.b); if (!cur || strcmp(cur, key)) store_write("conof", hx.b, key, false); free(cur); }
            free(hx.b);
        }
    }
}
/* A data type's key includes its declared name: two declarations of one content (List in
 * lib/prelude.cub, Lst in lib/code.cub) are two types to the checker (data types are generative), so
 * they are two keys, as Unison's unique types carry a salt (2026-11-12). */
/* ---- keys of the kinds of entry ---- */
/* a checked definition: its type and term; the canonical forms are returned for the store */
static void key_def(Node *T, Node *t, char out[65], char **code, char **type) {
    cur_deps.n = 0;
    /* generated names numbered by first occurrence (kfresh_on): a definition whose elaborated form
     * binds a generated name in a pattern (calc chains, clauses) otherwise had a different key in
     * every run whose earlier elaborations differed, and its certificate never hit (2026-11-12,
     * lib/obs.cub's o-run-comp, 0.6M steps rechecked on every warm run) */
    kfresh_on = true; nkfresh = 0;
    SBuf b = { 0 }; canon(&b, T); char *ty = b.b;
    SBuf c = { 0 }; if (t) canon(&c, t); char *cd = c.b ? c.b : strdup("");
    kfresh_on = false;
    SBuf w = { 0 }; sb_put(&w, "def\ntype "); sb_put(&w, ty); sb_put(&w, "\nterm "); sb_put(&w, cd); sb_put(&w, "\n");
    key_hash(w.b, w.n, out); free(w.b);
    if (type) *type = ty; else free(ty);
    if (code) *code = cd; else free(cd);
}
static void key_named(const char *kind, const char *name, Node *T, char out[65], char **type) {
    cur_deps.n = 0;
    kfresh_on = true; nkfresh = 0;   /* as key_def: generated names by first occurrence */
    SBuf b = { 0 }; if (T) canon(&b, T); char *ty = b.b ? b.b : strdup("");
    SBuf w = { 0 }; sb_put(&w, kind); sb_put(&w, " "); sb_put(&w, name); sb_put(&w, "\ntype "); sb_put(&w, ty); sb_put(&w, "\n");
    key_hash(w.b, w.n, out); free(w.b); kfresh_on = false;
    if (type) *type = ty; else free(ty);
}
static void key_decl(Node *dd, char out[65], char **code) {
    cur_deps.n = 0;
    kfresh_on = true; nkfresh = 0;   /* as key_def: generated names by first occurrence */
    SBuf b = { 0 }; canon_decl(&b, dd);
    /* a data type is generative: two declarations of one content are two types to the checker (List
     * and Lst in lib/prelude.cub and lib/code.cub), so the name it is declared under is part of its
     * identity, as Unison's unique types carry a salt (2026-11-12; without it the store conflated
     * them and a form read back named the wrong one) */
    Node *hd = dd->it[1]; const char *dn = hd->list ? hd->it[0]->s : hd->s;
    SBuf w = { 0 }; sb_put(&w, "data "); sb_put(&w, dn); sb_put(&w, "\n"); sb_put(&w, b.b); sb_put(&w, "\n");
    key_hash(w.b, w.n, out); free(w.b); kfresh_on = false;
    if (code) *code = b.b; else free(b.b);
}
/* a group: the canonical forms of every type and body, members R<i>; a member's key the group's
 * key and its index */
static void key_group(Node **Ts, Node **ts, int n, int base, SKey *out) {
    cur_deps.n = 0;
    SBuf w = { 0 }; sb_fmt(&w, "group %d\n", n);
    int saved = kgroup_base; kgroup_base = base; kfresh_on = true; nkfresh = 0;
    for (int i = 0; i < n; i++) { sb_put(&w, "type "); canon(&w, Ts[i]); sb_put(&w, "\nterm "); canon(&w, ts[i]); sb_put(&w, "\n"); }
    kgroup_base = saved; kfresh_on = false;
    char gk[65]; key_hash(w.b, w.n, gk); free(w.b);
    for (int i = 0; i < n; i++) { char m[96]; snprintf(m, sizeof m, "member %d of %s\n", i, gk); key_hash(m, strlen(m), out[i]); }
}

/* ---- #safe f (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C20): every trusted definition (postulate, primitive, foreign) reachable from f
 * through what its type and term mention, in order of discovery ---- */
static const char *trusted_how(const char *name) { for (int i = 0; i < ntrusted; i++) if (!strcmp(TRUSTED[i].name, name)) return TRUSTED[i].how; return NULL; }
static int *safe_seen; static int nsafe_seen, capsafe_seen; static int nsafe_found;
static void safe_walk(int k) {
    for (int i = 0; i < nsafe_seen; i++) if (safe_seen[i] == k) return;
    if (nsafe_seen == capsafe_seen) { capsafe_seen = capsafe_seen ? capsafe_seen * 2 : 64; safe_seen = realloc(safe_seen, (size_t)capsafe_seen * sizeof(int)); }
    safe_seen[nsafe_seen++] = k;
    if (k >= 0) {
        const char *how = trusted_how(CHECKED[k]);
        if (how) { printf("%s %s (%s)", nsafe_found ? "," : "", CHECKED[k], how); nsafe_found++; }
        if ((size_t)k < capcdeps) for (int i = 0; i < CDEPS[k].n; i++) safe_walk(CDEPS[k].d[i]);
    } else {
        int di = -k - 1;
        if ((size_t)di < capddeps) for (int i = 0; i < DDEPS[di].n; i++) safe_walk(DDEPS[di].d[i]);
    }
}
static void store_safe_cmd(const char *name) {
    int ck = checked_index(name), d = data_lookup(name), di = d < 0 ? -1 : chk_index(d);
    if (ck < 0 && di < 0) { printf("safe %s: not a checked definition\n", name); return; }
    printf("safe %s:", name);
    nsafe_seen = 0; nsafe_found = 0;
    safe_walk(ck >= 0 ? ck : -(di + 1));
    printf("%s\n", nsafe_found ? "" : " nothing assumed");
}

/* ---- the commands ---- */
static void store_key_cmd(const char *name) {
    int ck = checked_index(name);
    if (ck >= 0) { printf("key %s = %s\n", name, ckey_of(ck)); return; }
    int d = data_lookup(name), di = d < 0 ? -1 : chk_index(d);
    if (di >= 0) { printf("key %s = %s\n", name, dkey_of(di)); return; }
    printf("key %s: not a checked definition\n", name);
}
static void store_names_cmd(const char *key) {
    if (!store_dir()) { printf("names: no store\n"); return; }
    char *t = store_read("names", ""); size_t kl = strlen(key); int n = 0;
    printf("names %s:", key);
    for (char *l = t ? t : ""; *l; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        char *s1 = strchr(l, ' ');
        if (s1 && !strncmp(s1 + 1, key, kl)) { *s1 = 0; printf(" %s", l); n++; }
        l = e ? e + 1 : l + strlen(l);
    }
    printf("%s\n", n ? "" : " (none)"); free(t);
}
static void store_history_cmd(const char *name) {
    if (!store_dir()) { printf("history: no store\n"); return; }
    char *t = store_read("names", ""); size_t nl = strlen(name); int n = 0;
    printf("history %s:", name);
    for (char *l = t ? t : ""; *l; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        if (!strncmp(l, name, nl) && l[nl] == ' ') { char *k = l + nl + 1, *sp = strchr(k, ' '); if (sp) *sp = 0; printf(" %s", k); n++; }
        l = e ? e + 1 : l + strlen(l);
    }
    printf("%s\n", n ? "" : " (none)"); free(t);
}
static void store_unbind_cmd(const char *name) { names_bind(name, "-"); printf("unbound %s\n", name); }
/* #gc: the entries reachable from the names' current keys (through the keys their code and type
 * mention) stay; the rest are removed */
static char **gcmark; static size_t capgcmark; static int ngcmark;
static bool gc_marked(const char *k) { for (int i = 0; i < ngcmark; i++) if (!strcmp(gcmark[i], k)) return true; return false; }
static void gc_mark(const char *k) {
    if (strlen(k) != 64 || gc_marked(k)) return;
    GROW(gcmark, capgcmark, (size_t)ngcmark + 1); gcmark[ngcmark++] = strdup(k);
    static const char *files[] = { "code", "type" };
    for (int f = 0; f < 2; f++) {
        char *t = store_read(k, files[f]); if (!t) continue;
        for (char *p = t; *p; p++) if ((*p == 'G' || *p == 'D' || *p == 'C' || *p == 'F') && (p == t || !isalnum((unsigned char)p[-1]))) {
            size_t n = 0; while (isxdigit((unsigned char)p[1 + n]) && n < 64) n++;
            if (n == 64) { char kk[65]; memcpy(kk, p + 1, 64); kk[64] = 0; gc_mark(kk); }
        }
        free(t);
    }
}
static void store_gc_cmd(void) {
    if (!store_dir()) { printf("gc: no store\n"); return; }
    names_load();
    for (int i = 0; i < ngcmark; i++) free(gcmark[i]);
    ngcmark = 0;
    for (int i = 0; i < nnb; i++) if (NB[i].key && strcmp(NB[i].key, "-")) gc_mark(NB[i].key);
    DIR *d = opendir(store_dir()); int kept = 0, removed = 0;
    if (d) {
        struct dirent *e;
        while ((e = readdir(d))) {
            if (strlen(e->d_name) != 64) continue;
            if (gc_marked(e->d_name)) { kept++; continue; }
            static const char *files[] = { "code", "type", "check", "usage" };
            for (int f = 0; f < 4; f++) { char *p = store_path(e->d_name, files[f]); unlink(p); free(p); }
            char *dir = store_path(e->d_name, ""); rmdir(dir); free(dir);
            removed++;
        }
        closedir(d);
    }
    printf("gc: %d entries kept, %d removed\n", kept, removed);
}
