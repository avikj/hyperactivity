/* store.c — codebase keys and the store (plan F34). Included by lang.c before toplevel.
 *
 * A checked definition's key is the SHA-256 of the canonical form of its type and its term:
 * the elaborated core forms with every binder nameless (@k, the distance to its binder), every
 * checked definition it mentions replaced by that definition's key (G<key>), every checked data
 * type by its key (D<key>), a constructor or field by its type's key and its index (C<key>.i,
 * F<key>.i), hole solutions in place of their holes, and binder multiplicities (B11) kept. Not
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
int unlink(const char *); int rmdir(const char *);

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
static void ckey_set(int ck, const char *k) { if ((size_t)ck >= capckey) { size_t o = capckey; GROW(CKEY, capckey, (size_t)ck + 1); memset(CKEY + o, 0, (capckey - o) * sizeof(SKey)); } snprintf(CKEY[ck], sizeof(SKey), "%s", k); }
static void dkey_set(int di, const char *k) { if ((size_t)di >= capdkey) { size_t o = capdkey; GROW(DKEY, capdkey, (size_t)di + 1); memset(DKEY + o, 0, (capdkey - o) * sizeof(SKey)); } snprintf(DKEY[di], sizeof(SKey), "%s", k); }
static void key_hash(const char *s, size_t n, char out[65]) { Sha256 h; unsigned char d[32]; sha256_init(&h); sha256_update(&h, s, n); sha256_final(&h, d); sha256_hex(d, out); }

/* ---- the canonical form ---- */
/* what a form mentions, gathered as it is canonicalized: checked indices (k) and checked data
 * indices (-(di+1)); kept per definition for #safe (C20) */
typedef struct { int *d; int n; } Deps;
static Deps cur_deps; static Deps *CDEPS; static size_t capcdeps; static Deps *DDEPS; static size_t capddeps;
static void dep_add(int k) { for (int i = 0; i < cur_deps.n; i++) if (cur_deps.d[i] == k) return; cur_deps.d = realloc(cur_deps.d, (size_t)(cur_deps.n + 1) * sizeof(int)); cur_deps.d[cur_deps.n++] = k; }
static void deps_take(Deps **tab, size_t *cap, int i) { if ((size_t)i >= *cap) { size_t o = *cap; GROW(*tab, *cap, (size_t)i + 1); memset(*tab + o, 0, (*cap - o) * sizeof(Deps)); } free((*tab)[i].d); (*tab)[i] = cur_deps; cur_deps.d = NULL; cur_deps.n = 0; }
static const char **kscope; static size_t capkscope; static int nkscope;
static const char *kself;        /* a data declaration's own name */
static int kgroup_base = -1;     /* a group: its members are the checked indices from here */
static void kpush(const char *x) { GROW(kscope, capkscope, (size_t)nkscope + 1); kscope[nkscope++] = x; }
static void canon(SBuf *o, Node *t);
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
    sb_put(o, s);
}
static void canon_binder(SBuf *o, Node *x) {
    /* a binder: its multiplicity when not the default */
    int m = bnd_mult(x);
    if (m != 2) sb_fmt(o, ":%d", m);
}
static void canon(SBuf *o, Node *t) {
    if (!t->list) { canon_sym(o, t->s); return; }
    if (t->n == 0) { sb_put(o, "()"); return; }
    Node *h = t->it[0];
    if (!h->list) {
        const char *k = h->s;
        if (!strcmp(k, "%guess") && t->n == 4) { canon(o, t->it[3]); return; }
        if ((!strcmp(k, "%spine") || !strcmp(k, "%rebuilt")) && t->n == 2) { canon(o, t->it[1]); return; }
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && t->n >= 3) {
            sb_put(o, "("); sb_put(o, k);
            int nb = t->n - 2;
            for (int i = 1; i <= nb; i++) { if (!t->it[i]->list) canon_binder(o, t->it[i]); sb_put(o, " "); }
            for (int i = 1; i <= nb; i++) kpush(t->it[i]->list ? "" : t->it[i]->s);
            canon(o, t->it[t->n - 1]);
            nkscope -= nb;
            sb_put(o, ")");
            return;
        }
        if (!strcmp(k, "let") && t->n == 4 && !t->it[1]->list) {
            sb_put(o, "(let "); canon(o, t->it[2]); sb_put(o, " ");
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

/* the checker's name: the hash of the library texts loaded up to the one defining %%chk-def
 * together with the machine's own binary (/proc/self/exe; its build date and time when that
 * cannot be read). A definition's identity key stays build-independent; its certificate does
 * not: a change to the rules in net.c (conversion, composition, a primitive) with check.cub
 * unchanged must recheck, so a rebuilt machine rechecks the library once and then hits. */
/* t42 (2026-11-12): the checker's identity here is the hash of the texts of the files loaded up to
 * the checker's definition (comments and unrelated definitions included) plus the machine binary,
 * not the content keys of the definitions the checker consists of. So a comment edited in the
 * bootstrap files invalidates every certificate in the store. */
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
static char *store_path(const char *key, const char *file) {
    const char *d = store_dir(); if (!d) return NULL;
    char *p = malloc(strlen(d) + strlen(key) + strlen(file) + 4); sprintf(p, "%s/%s%s%s", d, key, *file ? "/" : "", file);
    return p;
}
static char *store_read(const char *key, const char *file) {
    char *p = store_path(key, file); if (!p) return NULL;
    FILE *f = fopen(p, "rb"); free(p); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1); size_t got = fread(b, 1, (size_t)n, f); b[got] = 0; fclose(f);
    return b;
}
static void store_write(const char *key, const char *file, const char *text, bool append) {
    char *p = store_path(key, file); if (!p) return;
    char *dir = store_path(key, "");
    mkdir(store_dir(), 0777); mkdir(dir, 0777); free(dir);
    FILE *f = fopen(p, append ? "a" : "w"); free(p); if (!f) return;
    fputs(text, f); fclose(f);
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
static void names_load(void) {
    if (nb_read) return;
    nb_read = true;
    char *t = store_read("names", ""); if (!t) return;
    for (char *l = t; *l; ) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        char *s1 = strchr(l, ' ');
        if (s1) {
            *s1 = 0; char *s2 = strchr(s1 + 1, ' '); if (s2) *s2 = 0;
            int i; for (i = 0; i < nnb && strcmp(NB[i].name, l); i++) ;
            if (i == nnb) { GROW(NB, capnb, (size_t)nnb + 1); NB[nnb].name = strdup(l); NB[nnb].key = NULL; nnb++; }
            free(NB[i].key); NB[i].key = strdup(s1 + 1);
        }
        l = e ? e + 1 : l + strlen(l);
    }
    free(t);
}
static void names_bind(const char *name, const char *key) {
    if (!store_dir()) return;
    names_load();
    int i; for (i = 0; i < nnb && strcmp(NB[i].name, name); i++) ;
    if (i < nnb && NB[i].key && !strcmp(NB[i].key, key)) return;
    if (i == nnb) { GROW(NB, capnb, (size_t)nnb + 1); NB[nnb].name = strdup(name); NB[nnb].key = NULL; nnb++; }
    free(NB[i].key); NB[i].key = strdup(key);
    char line[512]; snprintf(line, sizeof line, "%s %s %ld\n", name, key, (long)time(NULL));
    mkdir(store_dir(), 0777);
    char *p = store_path("names", ""); FILE *f = fopen(p, "a"); free(p);
    if (f) { fputs(line, f); fclose(f); }
}
/* t43 (2026-11-12): the store records only acceptance (an ok line with steps per checker). A
 * refusal and its reason are not recorded, so what fails, and why, is known only by rerunning. */
/* a definition's entry: written when it is checked (or replayed as checked) */
static void store_put(const char *name, const char *key, const char *code, const char *type, long steps, const char *usage) {
    if (!store_dir()) return;
    if (code) store_write(key, "code", code, false);
    if (type) store_write(key, "type", type, false);
    if (usage) store_write(key, "usage", usage, false);
    if (chk_salt[0] && !store_checked(key)) { char l[160]; snprintf(l, sizeof l, "ok %ld %s\n", steps, chk_salt); store_write(key, "check", l, true); }
    if (name) names_bind(name, key);
}

/* ---- keys of the kinds of entry ---- */
/* a checked definition: its type and term; the canonical forms are returned for the store */
static void key_def(Node *T, Node *t, char out[65], char **code, char **type) {
    cur_deps.n = 0;
    SBuf b = { 0 }; canon(&b, T); char *ty = b.b;
    SBuf c = { 0 }; if (t) canon(&c, t); char *cd = c.b ? c.b : strdup("");
    SBuf w = { 0 }; sb_put(&w, "def\ntype "); sb_put(&w, ty); sb_put(&w, "\nterm "); sb_put(&w, cd); sb_put(&w, "\n");
    key_hash(w.b, w.n, out); free(w.b);
    if (type) *type = ty; else free(ty);
    if (code) *code = cd; else free(cd);
}
static void key_named(const char *kind, const char *name, Node *T, char out[65], char **type) {
    cur_deps.n = 0;
    SBuf b = { 0 }; if (T) canon(&b, T); char *ty = b.b ? b.b : strdup("");
    SBuf w = { 0 }; sb_put(&w, kind); sb_put(&w, " "); sb_put(&w, name); sb_put(&w, "\ntype "); sb_put(&w, ty); sb_put(&w, "\n");
    key_hash(w.b, w.n, out); free(w.b);
    if (type) *type = ty; else free(ty);
}
static void key_decl(Node *dd, char out[65], char **code) {
    cur_deps.n = 0;
    SBuf b = { 0 }; canon_decl(&b, dd);
    SBuf w = { 0 }; sb_put(&w, "data\n"); sb_put(&w, b.b); sb_put(&w, "\n");
    key_hash(w.b, w.n, out); free(w.b);
    if (code) *code = b.b; else free(b.b);
}
/* a group: the canonical forms of every type and body, members R<i>; a member's key the group's
 * key and its index */
static void key_group(Node **Ts, Node **ts, int n, int base, SKey *out) {
    cur_deps.n = 0;
    SBuf w = { 0 }; sb_fmt(&w, "group %d\n", n);
    int saved = kgroup_base; kgroup_base = base;
    for (int i = 0; i < n; i++) { sb_put(&w, "type "); canon(&w, Ts[i]); sb_put(&w, "\nterm "); canon(&w, ts[i]); sb_put(&w, "\n"); }
    kgroup_base = saved;
    char gk[65]; key_hash(w.b, w.n, gk); free(w.b);
    for (int i = 0; i < n; i++) { char m[96]; snprintf(m, sizeof m, "member %d of %s\n", i, gk); key_hash(m, strlen(m), out[i]); }
}

/* ---- #safe f (C20): every trusted definition (postulate, primitive, foreign) reachable from f
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
