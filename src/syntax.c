#include <setjmp.h>
/* syntax.c — the surface language: tokens, layout, parsing into the core forms, and printing the
 * core forms back in the surface language. Included by lang.c (it uses its Node and tables).
 *
 * The surface language is mathematical notation: application by juxtaposition, declared infix
 * operators, indentation for structure, λ x → e, (x : A) → B, (x : A) × B, a ≡ b, definitions by
 * equations. Every Unicode symbol has an ASCII spelling (→ ->, λ \, ↦ |->, ≡ ==, ∧ /\, ∨ \/, × **,
 * ℕ Nat, S¹ S1, ⊥ Empty, ⊤ Unit); the lexer reads both as one token. */

/* ---------------- tokens ---------------- */
enum { X_ID, X_NUM, X_STR, X_LP, X_RP, X_LB, X_RB, X_LC, X_RC, X_COMMA, X_SEMI, X_DOT, X_VO, X_VS, X_VC, X_EOF };
typedef struct { int k; char *s; int line, col; } Tok;
static Tok *xt; static int nxt, capxt;
static const char *x_file;
static void x_push(int k, const char *s, size_t n, int line, int col) {
    if (nxt == capxt) { capxt = capxt ? capxt * 2 : 1024; xt = realloc(xt, (size_t)capxt * sizeof(Tok)); }
    char *b = malloc(n + 1); memcpy(b, s, n); b[n] = 0;
    xt[nxt++] = (Tok){ k, b, line, col };
}
/* the one spelling of each symbol */
static const struct { const char *a, *u; } X_ALIAS[] = {
    { "->", "→" }, { "\\", "λ" }, { "|->", "↦" }, { "==", "≡" }, { "∧", "/\\" }, { "∨", "\\/" }, { "**", "×" },
    { "ℕ", "Nat" }, { "S¹", "S1" }, { "⊥", "Empty" }, { "⊤", "Unit" }, { "=/=", "≢" }, { "forall", "∀" }, { "|>", "▸" },
    { NULL, NULL } };
static const char *x_canon(const char *s) {
    for (int i = 0; X_ALIAS[i].a; i++) if (!strcmp(s, X_ALIAS[i].a)) return X_ALIAS[i].u;
    return s;
}
static bool x_delim(int c) { return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == ';' || c == '"'; }
static bool x_identstart(unsigned char c) { return isalpha(c) || c == '_' || c >= 0x80 || c == '%'; }
/* an error at a token. x_unreadable: the text could not be read (the lexer's, the layout's and the
 * parser's errors), which ends the load; an error of a declaration that was read (x_sem) is that
 * declaration's failure (p_decl) */
static bool x_unreadable, x_sem_now;
static void x_die(int line, int col, const char *m, const char *x) {
    x_unreadable = !x_sem_now; x_sem_now = false;
    /* under --serve, the request ends and the machine stays (the parse error as `file:line:col:`) */
    if (die_jb) { char p[600]; snprintf(p, sizeof p, "%s:%d:%d: %s", x_file ? x_file : "?", line, col, m); die(p, x); }
    fprintf(stderr, "%s:%d:%d: %s%s%s\n", x_file ? x_file : "?", line, col, m, x ? " " : "", x ? x : "");
    exit(1);
}
/* a word (a maximal run of non-blank, non-delimiter characters) as tokens: projections .f split off,
 * a leading ~ or λ split off */
static void x_word(const char *b, size_t n, int line, int col) {
    if (n > 1 && b[0] == '~' && b[1] != '~') { x_push(X_ID, "~", 1, line, col); x_word(b + 1, n - 1, line, col + 1); return; }
    if (n > 2 && (unsigned char)b[0] == 0xCE && (unsigned char)b[1] == 0xBB) { x_push(X_ID, "λ", 2, line, col); x_word(b + 2, n - 2, line, col + 1); return; }
    if (n > 1 && b[0] == '\\' && isalpha((unsigned char)b[1])) { x_push(X_ID, "λ", 2, line, col); x_word(b + 1, n - 1, line, col + 1); return; }
    if (n > 1 && b[0] == '.' && x_identstart((unsigned char)b[1])) {
        size_t e = 1; while (e < n && b[e] != '.') e++;
        x_push(X_DOT, b + 1, e - 1, line, col);
        if (e < n) x_word(b + e, n - e, line, col + (int)e);
        return;
    }
    bool num = true; for (size_t i = 0; i < n; i++) if (!isdigit((unsigned char)b[i])) num = false;
    if (num) { x_push(X_NUM, b, n, line, col); return; }
    /* s.f: a projection of an identifier */
    for (size_t i = 1; i + 1 < n; i++)
        if (b[i] == '.' && b[i - 1] != '.' && x_identstart((unsigned char)b[i + 1]) && b[0] != '#') {
            x_word(b, i, line, col); x_word(b + i, n - i, line, col + (int)i); return;
        }
    char *w = malloc(n + 1); memcpy(w, b, n); w[n] = 0;
    const char *c = x_canon(w);
    x_push(X_ID, c, strlen(c), line, col);
    free(w);
}
static void x_lex(const char *text) {
    nxt = 0;
    const char *p = text; int line = 1; const char *ls = text;
#define COL ((int)(p - ls) + 1)
    while (*p) {
        if (*p == '\n') { p++; line++; ls = p; continue; }
        if (isspace((unsigned char)*p)) { p++; continue; }
        if (p[0] == '-' && p[1] == '-') { while (*p && *p != '\n') p++; continue; }
        if (p[0] == '{' && p[1] == '-') {
            int depth = 0;
            while (*p) {
                if (p[0] == '{' && p[1] == '-') { depth++; p += 2; continue; }
                if (p[0] == '-' && p[1] == '}') { depth--; p += 2; if (!depth) break; continue; }
                if (*p == '\n') { line++; ls = p + 1; }
                p++;
            }
            continue;
        }
        int col = COL;
        if (*p == '"') {
            const char *b = ++p; while (*p && *p != '"' && *p != '\n') p++;
            if (*p != '"') x_die(line, col, "unclosed string", NULL);
            x_push(X_STR, b, (size_t)(p - b), line, col); p++; continue;
        }
        if (x_delim(*p)) {
            static const char D[] = "()[]{},;"; static const int K[] = { X_LP, X_RP, X_LB, X_RB, X_LC, X_RC, X_COMMA, X_SEMI };
            x_push(K[strchr(D, *p) - D], p, 1, line, col); p++; continue;
        }
        const char *b = p;
        while (*p && !isspace((unsigned char)*p) && !x_delim(*p)) p++;
        x_word(b, (size_t)(p - b), line, col);
    }
    x_push(X_EOF, "<end of file>", 13, line, COL);
#undef COL
}
/* ---------------- layout ----------------
 * A block opens after where, of, let, mutual, postulate (and the file is one): its column is that of
 * its first token. A line starting at the block's column starts a new item (X_VS); one starting left
 * of it closes the block (X_VC); one starting right of it continues the item. `in` closes a let's
 * block; a closing bracket closes the blocks opened inside it. */
static Tok *lt; static int nlt, caplt;
static void l_push(Tok t) { if (nlt == caplt) { caplt = caplt ? caplt * 2 : 1024; lt = realloc(lt, (size_t)caplt * sizeof(Tok)); } lt[nlt++] = t; }
static bool x_is(Tok *t, const char *s) { return t->k == X_ID && !strcmp(t->s, s); }
static bool x_opens(Tok *t) { return x_is(t, "where") || x_is(t, "of") || x_is(t, "let") || x_is(t, "mutual") || x_is(t, "postulate"); }
static void x_layout(void) {
    nlt = 0;
    /* the open blocks: column, bracket depth where opened, whether a let's; the lets closed by
     * indentation that still await their `in` (by bracket depth) */
    int stk[256], depthat[256], islet[256], ns = 0, depth = 0;
    int await[256], na = 0;
    int pending = 0, pending_let = 0, prevline = 0;
    for (int i = 0; i < nxt; i++) {
        Tok t = xt[i];
        Tok v = t; v.s = "";
#define CLOSE() do { v.k = X_VC; l_push(v); if (islet[ns - 1]) { if (na == 256) x_die(t.line, t.col, "too deep", NULL); await[na++] = depthat[ns - 1]; } ns--; } while (0)
        if (t.k == X_EOF) { while (ns > 0) { v.k = X_VC; l_push(v); ns--; } l_push(t); break; }
        if (i == 0 || pending) {
            if (ns == 256) x_die(t.line, t.col, "blocks nested too deeply", NULL);
            if (pending && ns > 0 && t.col <= stk[ns - 1] && t.line != prevline) {
                /* an empty block */
                v.k = X_VO; l_push(v); v.k = X_VC; l_push(v); pending = 0;
            } else {
                stk[ns] = t.col; depthat[ns] = depth; islet[ns] = pending_let; ns++;
                if (i) { v.k = X_VO; l_push(v); }
                pending = 0; prevline = t.line;
                goto emit;
            }
        }
        if (t.line != prevline) {
            while (ns > 1 && t.col < stk[ns - 1] && depthat[ns - 1] == depth) CLOSE();
            if (ns > 0 && t.col == stk[ns - 1] && depthat[ns - 1] == depth) { v.k = X_VS; l_push(v); }
            prevline = t.line;
        }
        if (x_is(&t, "in")) {
            if (na > 0 && await[na - 1] == depth) na--;
            else {
                bool found = false;
                for (int q = ns - 1; q >= 1 && depthat[q] == depth; q--) if (islet[q]) { found = true; break; }
                if (!found) x_die(t.line, t.col, "`in` without its let", NULL);
                while (!islet[ns - 1]) CLOSE();
                v.k = X_VC; l_push(v); ns--;
            }
        }
        if (t.k == X_RP || t.k == X_RB || t.k == X_RC) {
            while (ns > 1 && depthat[ns - 1] >= depth) CLOSE();
            while (na > 0 && await[na - 1] >= depth) na--;
            depth--;
        }
    emit:
        l_push(t);
        if (t.k == X_LP || t.k == X_LB || t.k == X_LC) depth++;
        if (x_opens(&t)) { pending = 1; pending_let = x_is(&t, "let"); }
#undef CLOSE
    }
}

/* ---------------- implicit arguments: each definition's implicit parameters (a mask) ---------------- */
typedef struct { char *name; uint64_t m; int len; uint64_t inst; } IMask;   /* inst: the implicit positions found by instance search (E28) */
static IMask *IM; static int nim, capim;
static IMask *imask_of(const char *s) { for (int i = nim - 1; i >= 0; i--) if (!strcmp(IM[i].name, s)) return IM[i].m ? &IM[i] : NULL; return NULL; }
static void imask_set(const char *s, uint64_t m, int len) { GROW(IM, capim, (size_t)nim + 1); IM[nim++] = (IMask){ strdup(s), m, len, 0 }; }
/* the instance positions of a signature: its {{..}} binders */
static uint64_t inst_bits(Node *sig) {
    uint64_t b = 0; int q = 0;
    for (Node *x = sig; x && q < 64; q++) {
        bool im = false;
        if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im = true; x = x->it[1]; }
        if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
        if (im && bnd_inst(x->it[1])) b |= 1ull << q;
        x = x->it[3];
    }
    return b;
}
static void imask_inst(const char *s, Node *sig) { uint64_t b = inst_bits(sig); if (b) for (int i = nim - 1; i >= 0; i--) if (!strcmp(IM[i].name, s)) { IM[i].inst = b; return; } }
/* instance declarations (E28): `instance` before a signature; each is a candidate for an instance
 * hole whose type its result type matches, the latest first */
typedef struct { char *name; Node *sig; } Inst;
static Inst *INST; static size_t capinst; static int ninst; static bool pending_instance;
static void inst_add(const char *name, Node *sig) { GROW(INST, capinst, (size_t)ninst + 1); INST[ninst++] = (Inst){ strdup(name), sig }; }

/* ---------------- operators ---------------- */
typedef struct { char *op; int prec, assoc; } Fix;   /* assoc: 0 none, 1 left, 2 right */
static Fix *FIX; static int nfix, capfix;
static void fix_set(const char *op, int prec, int assoc) {
    for (int i = 0; i < nfix; i++) if (!strcmp(FIX[i].op, op)) { FIX[i].prec = prec; FIX[i].assoc = assoc; return; }
    GROW(FIX, capfix, (size_t)nfix + 1); FIX[nfix++] = (Fix){ strdup(op), prec, assoc };
}
/* the builtin operators: → (0, right), × (2, right), ≡ ≡[] (4), = (5, faces), \/ (6), /\ (7), @ (30, left) */
static Fix *fix_of(Tok *t) {
    static Fix B[] = { { "→", 0, 2 }, { "×", 2, 2 }, { "≡", 4, 0 }, { "\\/", 6, 1 }, { "/\\", 7, 1 }, { "@", 30, 1 } };
    if (t->k != X_ID) return NULL;
    for (size_t i = 0; i < sizeof B / sizeof *B; i++) if (!strcmp(B[i].op, t->s)) return &B[i];
    for (int i = nfix - 1; i >= 0; i--) if (!strcmp(FIX[i].op, t->s)) return &FIX[i];
    return NULL;
}
/* an operator's name as a definition: _+_ */
static char *op_name(const char *op) { char *r = malloc(strlen(op) + 3); sprintf(r, "_%s_", op); return r; }

/* ---------------- the parser ---------------- */
static int xp;                    /* position in lt */
static bool x_typed;              /* the checker is loaded: data fields are types, signatures check */
static bool x_untyped_file;       /* converting a file that was loaded before the checker */
static Tok *P0(void) { return &lt[xp]; }
static Tok *P1(void) { return &lt[xp + 1 < nlt ? xp + 1 : nlt - 1]; }
static Tok *P2(void) { return &lt[xp + 2 < nlt ? xp + 2 : nlt - 1]; }
/* the records declared: their field names in order, for R { f = a ; ... } and R { r | f = v } (E28) */
typedef struct { char *name; int nf; char **fn; } RecInfo;
static RecInfo *RECS; static size_t caprecs; static int nrecs;
static RecInfo *rec_of(const char *s) { for (int i = nrecs - 1; i >= 0; i--) if (!strcmp(RECS[i].name, s)) return &RECS[i]; return NULL; }
/* #retract of a record: its entry renamed out of reach; its field names (projections) returned */
static int rec_retract(const char *name, char ***fn) {
    RecInfo *R = rec_of(name);
    if (!R) return 0;
    char nb[512]; snprintf(nb, sizeof nb, "%%retracted:%s", R->name); R->name = strdup(nb);
    *fn = R->fn; return R->nf;
}

static Tok *NX(void) { Tok *t = &lt[xp]; if (t->k != X_EOF) xp++; return t; }
static void x_err(const char *m, Tok *t) { x_die(t->line, t->col, m, t->k == X_VS ? "(at a new line)" : t->k == X_VC ? "(at the end of a block)" : t->k == X_VO ? "(at a new block)" : t->s); }
static void x_sem(const char *m, Tok *t) { x_sem_now = true; x_err(m, t); }
static bool at(const char *s) { return x_is(P0(), s); }
/* a generalized variable (E28): its name and declared type */
typedef struct { const char *name; Node *type; } GVar;
static GVar *GVAR; static int ngvar; static size_t capgvar;
static void want(const char *s) { if (!at(s)) { char m[64]; snprintf(m, sizeof m, "expected %s, found", s); x_err(m, P0()); } NX(); }
static void wantk(int k, const char *what) { if (P0()->k != k) { char m[64]; snprintf(m, sizeof m, "expected %s, found", what); x_err(m, P0()); } NX(); }
static const char *RESERVED[] = { "=", ":", "→", "λ", "↦", "|", "of", "where", "let", "in", "if", "then", "else", "case", "return",
    "data", "codata", "record", "mutual", "postulate", "import", "infix", "infixl", "infixr", "by", "calc", "∀", "decreasing", "using", "▸", "primitive", ":0", ":1", "instance", "module", "open", "deriving", "foreign", "variable", NULL };
static bool reserved(const char *s) { for (int i = 0; RESERVED[i]; i++) if (!strcmp(RESERVED[i], s)) return true; return false; }
static int nfresh_x;
static Node *xfresh(const char *p) { char b[32]; snprintf(b, sizeof b, "%%%s%d", p, nfresh_x++); return nsym(b); }
static Node *lst(int n, ...) {
    va_list ap; va_start(ap, n);
    Node *x = nlist(n);
    for (int i = 0; i < n; i++) x->it[i] = va_arg(ap, Node *);
    va_end(ap);
    return x;
}
static Node *sym2(const char *h, Node *a) { return lst(2, nsym(h), a); }
static Node *sym3(const char *h, Node *a, Node *b) { return lst(3, nsym(h), a, b); }
static Node *sym4(const char *h, Node *a, Node *b, Node *c) { return lst(4, nsym(h), a, b, c); }
static Node *subst_sym(Node *t, const char *x, Node *e);
static Node *subst_multi(Node *t, const char **ps, Node **vs, int n);
static Node *syn_type(Node *a, int depth);
static Node *wordn(long n);
static Node *hole_or_sol(Node *key, int idx, bool *pend);
static Node *cn(Node *c);
/* parameterized modules (E28): `module M (A : Type) (e : Eq A) where` a block of definitions; each
 * is a definition M·f abstracted over the parameters (implicit ones {..} implicit), its references
 * to the module's other definitions applied to the parameters; `open M a b` defines each f as
 * M·f a b at the instantiated signature */
typedef struct { Node *name, *ty; bool imp; } MPar;
typedef struct { char *name; MPar *ps; int np; char **mem; Node **msig; bool *minst; int nmem; } Module;
static Module *MODS; static size_t capmods; static int nmods;
static Module *cur_mod;
static Module *mod_of(const char *s) { for (int i = nmods - 1; i >= 0; i--) if (!strcmp(MODS[i].name, s)) return &MODS[i]; return NULL; }
static char *mod_qual(Module *m, const char *f) { char *q = malloc(strlen(m->name) + strlen(f) + 4); sprintf(q, "%s·%s", m->name, f); return q; }
/* a member's name in a term: the qualified name at the parameters */
static Node *mod_ref(Module *m, int k) {
    Node *r = nlist(m->np + 1); r->it[0] = nsym(m->mem[k] + strlen(m->name) + 2);   /* placeholder, replaced below */
    r->it[0] = nsym(m->mem[k]);
    for (int i = 0; i < m->np; i++) r->it[i + 1] = m->ps[i].imp ? sym2("%imp", m->ps[i].name) : m->ps[i].name;
    return m->np ? r : r->it[0];
}
/* an application whose head became an application: one application */
static Node *mod_flat(Node *t) {
    if (!t->list || t->n == 0) return t;
    Node *r = nlist(t->n); for (int i = 0; i < t->n; i++) r->it[i] = mod_flat(t->it[i]);
    Node *h = r->it[0];
    if (h->list && h->n > 0 && !h->it[0]->list && h->it[0]->s[0] != '%' && !reserved(h->it[0]->s) && strchr(h->it[0]->s, 0xC2) && strstr(h->it[0]->s, "·")) {
        Node *f = nlist(h->n + r->n - 1);
        for (int i = 0; i < h->n; i++) f->it[i] = h->it[i];
        for (int i = 1; i < r->n; i++) f->it[h->n + i - 1] = r->it[i];
        return f;
    }
    return r;
}
static Node *mod_subst(Module *m, Node *t) { for (int k = 0; k < m->nmem; k++) t = subst_sym(t, m->mem[k] + strlen(m->name) + 2, mod_ref(m, k)); return mod_flat(t); }
static Node *mod_wrap_sig(Module *m, Node *sig) {
    for (int i = m->np - 1; i >= 0; i--) { Node *P = sym4("Pi", m->ps[i].name, m->ps[i].ty, sig); sig = m->ps[i].imp ? sym2("%impPi", P) : P; }
    return sig;
}
static void mod_member(Module *m, const char *q, Node *sig, bool inst) {
    m->mem = realloc(m->mem, (size_t)(m->nmem + 1) * sizeof(char *)); m->msig = realloc(m->msig, (size_t)(m->nmem + 1) * sizeof(Node *)); m->minst = realloc(m->minst, (size_t)(m->nmem + 1) * sizeof(bool));
    m->mem[m->nmem] = strdup(q); m->msig[m->nmem] = sig; m->minst[m->nmem] = inst; m->nmem++;
}

/* the primitives written as prefix functions: their number of operands (min, max) */
static const struct { const char *n; int lo, hi; } PRIM[] = {
    { "fst", 1, 1 }, { "snd", 1, 1 }, { "force", 1, 1 }, { "thunk", 1, 1 }, { "delay", 1, 1 }, { "rec", 1, 1 }, { "the", 2, 2 },
    { "code", 1, 1 }, { "PathP", 3, 3 }, { "faceof", 1, 1 }, { "sysmap", 2, 2 }, { "sys+", 3, 3 }, { "comp", 3, 3 }, { "Glue", 2, 2 },
    { "glue", 2, 2 }, { "unglue", 2, 2 }, { "hcompD", 2, 2 }, { "%view", 1, 1 }, { "%ieq", 2, 2 }, { "%feq", 2, 2 }, { "%clauses", 1, 1 },
    { "%dnf", 1, 1 }, { "%motive", 1, 1 }, { "%restrict", 2, 2 }, { "%abs", 2, 2 }, { "%branch", 3, 3 }, { "%clos", 2, 2 },
    { "%wsucc", 1, 1 }, { "%weq", 2, 2 }, { "%wlt", 2, 2 }, { "%wadd", 2, 2 }, { "%wmax", 2, 2 }, { "%wsub", 2, 2 }, { "%wmul", 2, 2 }, { "%wdiv", 2, 2 }, { "%wmod", 2, 2 }, { "%wand", 2, 2 }, { "%wor", 2, 2 }, { "%wxor", 2, 2 }, { "%wshl", 2, 2 }, { "%wshr", 2, 2 }, { "%welim", 3, 3 },
    { "%welimP", 4, 4 }, { "%box", 1, 1 }, { "%gval", 1, 1 }, { "%gty", 1, 1 }, { "%dtab", 1, 1 }, { "%dent", 1, 1 }, { "%dtrans", 1, 1 },
    { "%var", 1, 2 }, { "%dim", 0, 1 }, { "%@", 4, 4 }, { "%fand", 2, 2 }, { "%for", 2, 2 }, { "pair", 2, 2 }, { NULL, 0, 0 } };
static int prim_ix(const char *s) { for (int i = 0; PRIM[i].n; i++) if (!strcmp(PRIM[i].n, s)) return i; return -1; }

static Node *p_expr(int minprec);
static Node *p_face(void);
static bool atom_start(Tok *t) {
    switch (t->k) {
    case X_ID:
        if (x_is(t, "~") || x_is(t, "▸") || x_is(t, "λ") || x_is(t, "let") || x_is(t, "if") || x_is(t, "case") || x_is(t, "∀") || x_is(t, "calc")) return true;
        if (x_is(t, "record")) return P1()->k == X_LC;
        return !reserved(t->s) && !fix_of(t);
    case X_NUM: case X_STR: case X_LP: case X_LB: case X_LC: return true;
    default: return false;
    }
}
/* the binders of λ: x, _, (x y : A), (i : I), {x} */
typedef struct { Node *name; Node *ty; bool dim, imp; } Bnd;
/* a binder's colon: ":", or ":0" / ":1" with its multiplicity (B11); -1 when not a colon */
static int x_colon_mult(Tok *t) { return x_is(t, ":") ? 2 : x_is(t, ":0") ? 0 : x_is(t, ":1") ? 1 : -1; }
static int p_binders(Bnd *bs, int max, bool need_type) {
    int n = 0;
    for (;;) {
        Tok *t = P0();
        if (t->k == X_ID && !reserved(t->s) && !fix_of(t)) { if (n == max) x_err("too many binders", t); NX(); bs[n++] = (Bnd){ nsym(t->s), NULL, false, false }; continue; }
        if (t->k == X_LP || t->k == X_LC) {
            /* (x y : A); {x : A} implicit; {{x : A}} an instance argument (E28) */
            int save = xp; bool imp = t->k == X_LC; NX();
            bool inst = imp && P0()->k == X_LC; if (inst) NX();
            int k0 = n;
            while (P0()->k == X_ID && !reserved(P0()->s) && !fix_of(P0()) && x_colon_mult(P0()) < 0) { if (n == max) x_err("too many binders", P0()); bs[n++] = (Bnd){ nsym(NX()->s), NULL, false, imp }; }
            if (n > k0 && x_colon_mult(P0()) >= 0) {
                int m = x_colon_mult(NX()); Node *A = p_expr(0);
                wantk(imp ? X_RC : X_RP, imp ? "}" : ")"); if (inst) wantk(X_RC, "}}");
                for (int q = k0; q < n; q++) { bs[q].ty = A; bs[q].dim = is(A, "I"); if (m != 2 && !bs[q].dim) bnd_set_mult(bs[q].name, m); if (inst) bnd_set_inst(bs[q].name); }
                continue;
            }
            if (n > k0 && imp && !inst && P0()->k == X_RC) { NX(); continue; }
            n = k0; xp = save;
        }
        break;
    }
    if (need_type) for (int i = 0; i < n; i++) if (!bs[i].ty) x_err("a binder needs its type here:", P0());
    return n;
}
static Node *lam_of(Bnd *bs, int n, Node *body) {
    for (int i = n - 1; i >= 0; i--) {
        if (bs[i].ty && !bs[i].dim) bnd_set(bs[i].name, bs[i].ty);
        body = sym3(bs[i].dim ? "plam" : "lam", bs[i].name, body);
    }
    return body;
}
/* faces: φ ∨ ψ, φ ∧ ψ, r = 0, r = 1, (φ), ∀ i → φ, tt, ff */
static Node *p_iexpr(void);
static bool group_has_eq(void);
static Node *p_fatom(void) {
    if (P0()->k == X_LP && group_has_eq()) {
        int save = xp; NX();
        Node *f = p_face();
        if (P0()->k == X_RP && !x_is(P1(), "=")) { NX(); return f; }
        xp = save;
    }
    if (at("∀")) { NX(); Tok *i = NX(); want("→"); return sym3("forall", nsym(i->s), p_face()); }
    if (at("tt") || at("ff")) return nsym(NX()->s);
    Node *r = p_iexpr();
    if (!at("=")) return r;   /* a face given by an expression */
    NX();
    Tok *e = NX();
    const char *v = (e->k == X_NUM && (!strcmp(e->s, "0") || !strcmp(e->s, "1"))) ? e->s : x_is(e, "i0") ? "0" : x_is(e, "i1") ? "1" : NULL;
    if (!v) x_err("a face is r = 0 or r = 1, found", e);
    return sym3("=", r, nsym(v));
}
static Node *p_fconj(void) { Node *a = p_fatom(); while (at("/\\")) { NX(); a = sym3("&", a, p_fatom()); } return a; }
static Node *p_face(void) { Node *a = p_fconj(); while (at("\\/")) { NX(); a = sym3("|", a, p_fconj()); } return a; }
/* an interval: application terms joined by /\ \/ (no =) */
static Node *p_app(void);
static Node *p_iexpr(void) {
    Node *a = p_app();
    while (at("/\\") || at("\\/")) {
        /* a face's ∧ ∨ join faces, not intervals: stop where the next operand is a face */
        int save = xp; const char *op = NX()->s;
        Node *b = p_app();
        if (at("=")) { xp = save; break; }
        a = sym3(op, a, b);
    }
    return a;
}
/* does a bracketed group hold a face (an = at its top level)? */
static bool group_has_eq(void) {
    int d = 0;
    for (int i = xp + 1; i < nlt; i++) {
        int k = lt[i].k;
        if (k == X_LP || k == X_LB || k == X_LC) d++;
        else if (k == X_RP || k == X_RB || k == X_RC) { if (--d < 0) return false; }
        else if (d == 0 && x_is(&lt[i], "=")) return true;
        else if (d == 0 && (x_is(&lt[i], "let") || x_is(&lt[i], "↦") || k == X_COMMA || k == X_EOF)) return false;
    }
    return false;
}
/* a system: [ φ ↦ e , ... ] */
static Node *p_system(void) {
    wantk(X_LB, "[");
    Node **bs = NULL; size_t cap = 0; int n = 0;
    while (P0()->k != X_RB) {
        Node *f = p_face(); want("↦"); Node *e = p_expr(0);
        GROW(bs, cap, (size_t)n + 1); bs[n++] = lst(2, f, e);
        if (P0()->k == X_COMMA) NX(); else break;
    }
    wantk(X_RB, "]");
    Node *s = nlist(n + 1); s->it[0] = nsym("sys");
    for (int i = 0; i < n; i++) s->it[i + 1] = bs[i];
    free(bs);
    return s;
}
static Node *p_block_lets(Node *body_after);
static Node *p_case(void);
static Node *p_lambda(void) {
    NX();
    Bnd bs[64]; int n = p_binders(bs, 64, false);
    if (!n) x_err("λ needs a binder, found", P0());
    want("→");
    return lam_of(bs, n, p_expr(0));
}
/* an atom: a name, a number, (e), (a , b), [system], ~ r, λ, let, if, case */
static Node *p_atom(void) {
    Tok *t = P0();
    if (x_is(t, "~")) { NX(); return sym2("~", p_atom()); }
    if (x_is(t, "▸")) {
        /* ▸ κ A: an A a tick of the clock κ later, the Π over the ticks of κ */
        NX(); Node *k = p_atom(); Node *A = p_atom();
        return sym4("Pi", xfresh("α"), lst(2, nsym("Tick"), k), A);
    }
    if (x_is(t, "λ")) return p_lambda();
    if (x_is(t, "let")) return p_block_lets(NULL);
    if (x_is(t, "if")) { NX(); Node *c = p_expr(0); want("then"); Node *a = p_expr(0); want("else"); Node *b = p_expr(0); return sym4("if", c, a, b); }
    if (x_is(t, "case")) return p_case();
    if (x_is(t, "∀")) { NX(); Tok *i = NX(); want("→"); return sym3("forall", nsym(i->s), p_face()); }
    if (x_is(t, "record") && P1()->k == X_LC) {
        /* record { f = e ; ... }: a copattern's fields */
        NX(); NX();
        Node *fs[64]; int n = 0;
        while (P0()->k != X_RC) {
            Tok *f = NX(); if (f->k != X_ID) x_err("a field's name, found", f);
            want("="); if (n == 64) x_err("too many fields", f);
            fs[n++] = lst(2, nsym(f->s), p_expr(0));
            if (P0()->k == X_SEMI) NX(); else if (P0()->k != X_RC) x_err("expected ; or }, found", P0());
        }
        NX();
        Node *r = nlist(n + 1); r->it[0] = nsym("record");
        for (int i = 0; i < n; i++) r->it[i + 1] = fs[i];
        return r;
    }
    if (x_is(t, "calc")) {
        /* calc e0 ≡ e1 by p1 ≡ e2 by p2 ...: each step checked at its equation, composed by _∙_ */
        NX();
        Node *e0 = p_expr(5), *steps[256]; int n = 0;
        while (at("≡")) {
            NX(); Node *e1 = p_expr(5); want("by"); Node *pf = p_expr(5);
            if (n == 256) x_err("calc too long", P0());
            steps[n++] = sym3("the", lst(3, nsym("_≡_"), e0, e1), pf);
            e0 = e1;
        }
        if (!n) x_err("calc needs a step: ≡ e by proof, found", P0());
        Node *r = steps[n - 1];
        for (int i = n - 2; i >= 0; i--) r = lst(3, nsym("_∙_"), steps[i], r);
        return r;
    }
    if (t->k == X_NUM) { NX(); return sym2("%num", nsym(t->s)); }
    if (t->k == X_LB) return p_system();
    if (t->k == X_LP) {
        if (group_has_eq()) { NX(); Node *f = p_face(); wantk(X_RP, ")"); return f; }
        NX();
        if (P0()->k == X_RP) { NX(); return nsym("%absurd"); }
        /* an operator section name: (_+_) or (+) */
        if (fix_of(P0()) && P1()->k == X_RP) { Node *r = nsym(op_name(NX()->s)); NX(); return r; }
        Node *e = p_expr(0);
        if (P0()->k == X_COMMA) {
            Node *xs[256]; int n = 0; xs[n++] = e;
            while (P0()->k == X_COMMA) { NX(); if (n == 256) x_err("tuple too long", P0()); xs[n++] = p_expr(0); }
            wantk(X_RP, ")");
            Node *r = xs[n - 1];
            for (int i = n - 2; i >= 0; i--) r = sym3("pair", xs[i], r);
            return r;
        }
        wantk(X_RP, ")");
        return e;
    }
    if (t->k == X_LC && P1()->k == X_LC) { NX(); NX(); Node *e = p_expr(0); wantk(X_RC, "}"); wantk(X_RC, "}}"); Node *r = sym2("%imp", e); bnd_set_inst(r->it[0]); return r; }   /* an instance argument given: an implicit one, marked, for an instance position */
    if (t->k == X_LC) { NX(); Node *e = p_expr(0); wantk(X_RC, "}"); return sym2("%imp", e); }
    if (t->k == X_ID && !reserved(t->s) && rec_of(t->s) && P1()->k == X_LC && !(P2()->k == X_LC)) {
        /* R { f = a ; g = b }: a value of the record, its fields by name, in any order, each once;
         * R { r | f = v }: r with the fields given replaced (the others projected from r) */
        int save = xp; RecInfo *R = rec_of(t->s); NX(); NX();
        Node *base = NULL; bool named = P0()->k == X_ID && x_is(P1(), "=");
        if (!named) {
            base = p_expr(0);
            if (!at("|")) { xp = save; goto plain; }
            NX();
        }
        Node *vals[64] = { 0 };
        while (P0()->k != X_RC) {
            Tok *f = NX(); if (f->k != X_ID) x_err("a field's name, found", f);
            int k; for (k = 0; k < R->nf && strcmp(R->fn[k], f->s); k++) ;
            if (k == R->nf) x_sem("not a field of the record:", f);
            if (vals[k]) x_sem("a field given twice:", f);
            want("="); vals[k] = p_expr(0);
            if (P0()->k == X_SEMI) NX(); else if (P0()->k != X_RC) x_err("expected ; or }, found", P0());
        }
        NX();
        for (int k = 0; k < R->nf; k++) if (!vals[k]) {
            if (!base) { char m[256]; snprintf(m, sizeof m, "%s { ... }: the field %s is not given", R->name, R->fn[k]); x_err(m, P0()); }
            vals[k] = lst(2, nsym(R->fn[k]), base);
        }
        Node *r = vals[R->nf - 1];
        for (int k = R->nf - 2; k >= 0; k--) r = sym3("pair", vals[k], r);
        return r;
    }
    plain:
    if (t->k == X_ID && !reserved(t->s)) {
        NX();
        if (!strcmp(t->s, "i0")) return nsym("0");
        if (!strcmp(t->s, "i1")) return nsym("1");
        if (!strcmp(t->s, "Type")) return nsym("U");
        if (!strcmp(t->s, "_")) return nsym("%hole");
        return nsym(t->s);
    }
    x_err("unexpected", t);
    return NULL;
}
/* a postfix projection .f */
static Node *p_post(void) {
    Node *a = p_atom();
    while (P0()->k == X_DOT) a = lst(2, nsym(NX()->s), a);
    return a;
}
/* application: a head and its operands; a primitive takes its operands into its own form */
static Node *p_app(void) {
    Node *h = p_post();
    Node *args[256]; int n = 0;
    while (atom_start(P0()) && !x_is(P0(), "λ") && !x_is(P0(), "let") && !x_is(P0(), "if") && !x_is(P0(), "case") && !x_is(P0(), "calc")) {
        if (n == 256) x_err("too many operands", P0());
        args[n++] = p_post();
    }
    /* a λ, let, if or case as the last operand */
    if (x_is(P0(), "λ") || x_is(P0(), "let") || x_is(P0(), "if") || x_is(P0(), "case") || x_is(P0(), "calc")) { if (n == 256) x_err("too many operands", P0()); args[n++] = p_atom(); }
    int pi = !h->list ? prim_ix(h->s) : -1;
    if (!n) return pi >= 0 && PRIM[pi].lo == 0 ? lst(1, h) : h;
    if (!h->list && !strcmp(h->s, "U")) {
        /* Type ℓ */
        Node *l = args[0];
        if (l->list && l->n == 2 && is(l->it[0], "%num")) l = l->it[1];   /* a level as a number */
        Node *u = sym2("U", l); Node *r = u;
        for (int i = 1; i < n; i++) { Node *a = lst(2, r, args[i]); r = a; }
        return r;
    }
    if (pi >= 0) {
        int k = n < PRIM[pi].hi ? n : PRIM[pi].hi;
        if (k < PRIM[pi].lo) x_err("too few operands for", P0());
        const char *nm = !strcmp(h->s, "%@") ? "@" : !strcmp(h->s, "%fand") ? "&" : !strcmp(h->s, "%for") ? "|" : h->s;
        Node *r = nlist(k + 1); r->it[0] = nsym(nm);
        for (int i = 0; i < k; i++) r->it[i + 1] = args[i];
        if (k == n) return r;
        Node *a = nlist(n - k + 1); a->it[0] = r;
        for (int i = k; i < n; i++) a->it[i - k + 1] = args[i];
        return a;
    }
    Node *r = nlist(n + 1); r->it[0] = h;
    for (int i = 0; i < n; i++) r->it[i + 1] = args[i];
    return r;
}
static Node *mk_pi(const char *k, Node *x, Node *A, Node *B) { return sym4(k, x, A, B); }
/* a dependent arrow or product: (x y : A) → B, {x : A} → B, (x : A) × B */
static Node *p_telescope_type(void) {
    int save = xp;
    Bnd bs[64]; int n = p_binders(bs, 64, false);
    bool typed = n > 0; for (int i = 0; i < n; i++) if (!bs[i].ty) typed = false;
    if (typed && (at("→") || at("×"))) {
        const char *k = at("→") ? "Pi" : "Sigma"; NX();
        Node *B = p_expr(k[0] == 'P' ? 0 : 2);
        for (int i = n - 1; i >= 0; i--) {
            B = mk_pi(k, bs[i].name, bs[i].ty, B);
            if (bs[i].imp) B = sym2("%impPi", B);
        }
        return B;
    }
    xp = save;
    return NULL;
}
static Node *p_expr(int minprec) {
    Node *lhs = NULL;
    if (P0()->k == X_LP || P0()->k == X_LC) lhs = p_telescope_type();
    if (!lhs) lhs = p_app();
    for (;;) {
        Tok *t = P0();
        if (x_is(t, "≡") && P1()->k == X_LB) {
            /* a ≡[ i ↦ A ] b */
            if (4 < minprec) break;
            NX(); NX();
            Tok *i = NX(); want("↦"); Node *A = p_expr(0); wantk(X_RB, "]");
            Node *b = p_expr(5);
            lhs = sym4("PathP", sym3("plam", nsym(i->s), A), lhs, b);
            continue;
        }
        Fix *f = fix_of(t);
        if (!f || f->prec < minprec) break;
        NX();
        Node *rhs = p_expr(f->assoc == 2 ? f->prec : f->prec + 1);
        if (!strcmp(f->op, "→")) lhs = mk_pi("Pi", xfresh("_"), lhs, rhs);
        else if (!strcmp(f->op, "×")) lhs = mk_pi("Sigma", xfresh("_"), lhs, rhs);
        else if (!strcmp(f->op, "@")) lhs = sym3("@", lhs, rhs);
        else if (!strcmp(f->op, "/\\") || !strcmp(f->op, "\\/")) lhs = sym3(f->op, lhs, rhs);
        else lhs = lst(3, nsym(op_name(f->op)), lhs, rhs);
        if (f->assoc == 0) { Fix *g = fix_of(P0()); if (g && g->prec == f->prec && g->assoc == 0) x_err("a non-associative operator chained:", P0()); }
    }
    return lhs;
}
/* case e [return P] of alternatives (pattern → body) */
static Node *p_pattern_head(void);
static Node *p_case(void) {
    want("case");
    Node *e = p_expr(0);
    Node *P = NULL;
    if (at("return")) { NX(); P = p_expr(0); }
    want("of");
    wantk(X_VO, "a block of alternatives");
    /* case e of D: a type without constructors, no alternatives */
    if (P0()->k == X_ID && data_lookup(P0()->s) >= 0 && DATA[data_lookup(P0()->s)].ncons == 0 && !DATA[data_lookup(P0()->s)].co && (P1()->k == X_VC || P1()->k == X_VS)) {
        Node *D = nsym(NX()->s);
        if (P0()->k == X_VS) x_err("a case of a type without constructors has no alternatives, found", P1());
        NX();
        return P ? lst(4, nsym("elim"), P, e, D) : lst(3, nsym("case"), e, D);
    }
    Node **alts = NULL; size_t cap = 0; int n = 0;
    while (P0()->k != X_VC) {
        Node *pat = p_pattern_head();
        want("→");
        Node *b = p_expr(0);
        GROW(alts, cap, (size_t)n + 1); alts[n++] = lst(2, pat, b);
        if (P0()->k == X_VS || P0()->k == X_SEMI) NX(); else if (P0()->k != X_VC) x_err("expected a new alternative, found", P0());
    }
    NX();
    Node *r = nlist(n + (P ? 3 : 2)); int k = 0;
    r->it[k++] = nsym(P ? "elim" : "case");
    if (P) r->it[k++] = P;
    r->it[k++] = e;
    for (int i = 0; i < n; i++) r->it[k++] = alts[i];
    free(alts);
    return r;
}
/* a case alternative's pattern: c x y, _, x */
static Node *p_pattern_head(void) {
    Tok *t = NX();
    if (t->k != X_ID) x_err("a pattern starts with a constructor or a variable, found", t);
    if (!strcmp(t->s, "_")) return nsym("_");
    int c = con_lookup(t->s);
    if (c < 0 || (CON[c].nfields + CON[c].ndims > 0 && at("→"))) {
        /* a variable: the scrutinee, named (a constructor's name with no fields, of a constructor
         * that has fields, is a variable) */
        if (atom_start(P0()) && !at("→")) x_err("not a constructor:", t);
        return nsym(t->s);
    }
    Node *xs[64]; int n = 0;
    xs[n++] = nsym(t->s);
    while (P0()->k == X_ID && !at("→")) { if (n == 64) x_err("too many fields", P0()); Tok *f = NX(); xs[n++] = nsym(!strcmp(f->s, "_") ? "_" : f->s); }
    Node *r = nlist(n);
    for (int i = 0; i < n; i++) r->it[i] = xs[i];
    return r;
}
/* let x = e; f x y = e; ... in body (after: the body of a where, given) */
static Node *p_block_lets(Node *after) {
    NX();   /* let, or where */
    wantk(X_VO, "a block of definitions");
    Node *names[256], *vals[256]; int n = 0;
    while (P0()->k != X_VC) {
        Tok *t = NX();
        if (t->k != X_ID || reserved(t->s)) x_err("a let defines a name, found", t);
        Bnd bs[64]; int nb = p_binders(bs, 64, false);
        Node *T = NULL;
        if (at(":")) { NX(); T = p_expr(0); if (P0()->k == X_VS) NX(); Tok *u = NX(); if (!x_is(u, t->s)) x_err("a signature is followed by its definition, found", u); nb = p_binders(bs, 64, false); }
        want("=");
        Node *v = lam_of(bs, nb, p_expr(0));
        if (T) v = sym3("the", T, v);
        if (n == 256) x_err("too many definitions in one let", t);
        names[n] = nsym(t->s); vals[n++] = v;
        if (P0()->k == X_VS || P0()->k == X_SEMI) NX(); else if (P0()->k != X_VC) x_err("expected a new definition, found", P0());
    }
    NX();
    Node *body = after;
    if (!body) { want("in"); body = p_expr(0); }
    for (int i = n - 1; i >= 0; i--) body = sym4("let", names[i], vals[i], body);
    return body;
}

/* ---------------- numerals and intervals ----------------
 * A numeral n is an interval endpoint where an interval stands (an operand of @, ~, ∧, ∨, a face,
 * a constructor's dimension), else the natural number suc (... zero). */
static Node *numeral(const char *s) {
    long n = atol(s);
    if (con_lookup("zero") < 0 || con_lookup("suc") < 0) die("a numeral needs the natural numbers (zero, suc):", s);
    Node *r = nsym("zero");
    for (long i = 0; i < n; i++) r = lst(2, nsym("suc"), r);
    return r;
}
static Node *resolve(Node *t, bool iv) {
    if (!t->list) return t;
    if (t->n == 2 && is(t->it[0], "%num")) {
        if (iv) { if (!strcmp(t->it[1]->s, "0") || !strcmp(t->it[1]->s, "1")) return t->it[1]; die("an interval is 0, 1, a dimension, ~, /\\ or \\/; found", t->it[1]->s); }
        return numeral(t->it[1]->s);
    }
    if (t->n == 0) return t;
    Node *h = t->it[0];
    if (!h->list && (is(h, "U") || is(h, "Type")) && t->n == 2 && t->it[1]->list && t->it[1]->n == 2 && is(t->it[1]->it[0], "%num")) return sym2("U", t->it[1]->it[1]);   /* a level as a number */
    Node *r = nlist(t->n); r->it[0] = h->list ? resolve(h, false) : h;
    int c = !h->list ? con_lookup(h->s) : -1;
    for (int i = 1; i < t->n; i++) {
        bool v = false;
        if (!h->list) {
            const char *k = h->s;
            if (!strcmp(k, "@")) v = i >= 2 && i <= 2;
            else if (!strcmp(k, "~") || !strcmp(k, "/\\") || !strcmp(k, "\\/")) v = true;
            else if (!strcmp(k, "=")) v = i == 1;
            else if (c >= 0) v = i - 1 >= CON[c].nfields;
        }
        r->it[i] = resolve(t->it[i], v);
    }
    return r;
}

/* ---------------- names in terms ---------------- */
/* t with the free name x replaced by the name y (y is fresh or not bound inside t) */
static Node *x_rename(Node *t, const char *x, const char *y) {
    if (!t->list) return !strcmp(t->s, x) ? nsym(y) : t;
    if (t->n == 0 || t->it[0]->list) { Node *r = nlist(t->n); for (int i = 0; i < t->n; i++) r->it[i] = x_rename(t->it[i], x, y); return r; }
    const char *k = t->it[0]->s;
    if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && t->n == 3 && !t->it[1]->list && !strcmp(t->it[1]->s, x)) return t;
    if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma")) && t->n == 4 && !t->it[1]->list && !strcmp(t->it[1]->s, x))
        return sym4(k, t->it[1], x_rename(t->it[2], x, y), t->it[3]);
    if (!strcmp(k, "let") && t->n == 4 && !strcmp(t->it[1]->s, x)) return sym4("let", t->it[1], x_rename(t->it[2], x, y), t->it[3]);
    Node *r = nlist(t->n); r->it[0] = !strcmp(k, x) ? nsym(y) : t->it[0];
    bool cs = !strcmp(k, "case") || !strcmp(k, "elim");
    int from = !strcmp(k, "elim") ? 3 : 2;
    for (int i = 1; i < t->n; i++) {
        Node *a = t->it[i];
        if (cs && i >= from && a->list && a->n == 2) {
            /* an alternative: its pattern binds */
            Node *pat = a->it[0]; bool binds = false;
            if (pat->list) { for (int q = 1; q < pat->n; q++) if (!strcmp(pat->it[q]->s, x)) binds = true; }
            else if (!strcmp(pat->s, x)) binds = true;
            r->it[i] = binds ? a : lst(2, pat, x_rename(a->it[1], x, y));
            continue;
        }
        r->it[i] = (!strcmp(k, "=") && i == 2) ? a : x_rename(a, x, y);
    }
    return r;
}

/* ---------------- definitions by clauses (untyped) ----------------
 * Clauses f p1 ... pn = e, tried in order. Each column whose patterns are all the same variable
 * keeps its name; a split column is cased on, constructor by constructor: the clauses with that
 * constructor (its field patterns as new columns) and those with a variable there (renamed to the
 * scrutinee); the constructors no clause names share one default alternative. */
typedef struct { Node **p; Node *rhs; int id; } Row;   /* id: the clause it came from (-1: made by the compiler) */
/* the clauses a match reached (a clause no case reaches is refused: written twice, or after one
 * that covers it) */
static bool *row_used; static int row_n;
static void rows_track(int n) { row_used = calloc((size_t)n + 1, sizeof(bool)); row_n = n; }
static void rows_check(const char *f) {
    for (int i = 0; i < row_n; i++) if (!row_used[i]) { char m[96]; snprintf(m, sizeof m, "clause %d is reached by no case (written twice, or after one covering it), in", i + 1); die(m, f); }
    free(row_used); row_used = NULL; row_n = 0;
}
static void row_reached(const Row *r) { if (row_used && r->id >= 0 && r->id < row_n) row_used[r->id] = true; }
static bool pat_wild(Node *p) { return !p->list && (!strcmp(p->s, "%hole") || !strcmp(p->s, "_")); }
/* a pattern's constructor: (c p...), or a constructor without fields (a name of a constructor with
 * fields, alone, is a variable) */
typedef struct { char *con; int nidx, nfields; char **fields; Node **idx; } Ford;   /* nfields: the ordinary fields; idx: the index expressions over them */
static Ford *FORD; static size_t capford; static int nford;
static bool ford_head;   /* an application's head being elaborated (not a bare constructor to pad) */
static int data_nidx; static char *data_ixnames[16]; static Node *data_ixtypes[16];
/* a retracted constructor's entry renamed with it (#retract of its data type) */
static void ford_rename(const char *old, const char *nw) { for (int i = 0; i < nford; i++) if (!strcmp(FORD[i].con, old)) FORD[i].con = strdup(nw); }
static Ford *ford_of(const char *con) { for (int i = nford - 1; i >= 0; i--) if (!strcmp(FORD[i].con, con)) return &FORD[i]; return NULL; }
static int pat_con(Node *p) {
    if (!p->list) { int c = con_lookup(p->s); Ford *fd = c >= 0 ? ford_of(p->s) : NULL; return c >= 0 && CON[c].nfields + CON[c].ndims == (fd ? fd->nidx : 0) ? c : -1; }
    return p->it[0]->list ? -1 : con_lookup(p->it[0]->s);
}
static Node *pat_norm(Node *p) {
    /* numerals as constructors */
    if (p->list && p->n == 2 && is(p->it[0], "%num")) return numeral(p->it[1]->s);
    if (p->list) { Node *r = nlist(p->n); for (int i = 0; i < p->n; i++) r->it[i] = i ? pat_norm(p->it[i]) : p->it[i]; return r; }
    return p;
}
static Node *cc_match(Node **vars, int nv, Row *rows, int nr, const char *fname);
static Node *cc_match(Node **vars, int nv, Row *rows, int nr, const char *fname) {
    if (!nr) die("the clauses do not cover every case of", fname);
    /* the first row matches when all its patterns are variables */
    int j = -1;
    for (int q = 0; q < nv; q++) if (!pat_wild(rows[0].p[q]) && pat_con(rows[0].p[q]) >= 0) { j = q; break; }
    if (j >= 0) {
        /* split the leftmost column any clause matches by constructor: a recursion on an earlier
         * parameter must re-split the later ones it changes, so they are split inside it */
        for (int q = 0; q < j; q++) {
            bool any = false;
            for (int r = 0; r < nr && !any; r++) if (!pat_wild(rows[r].p[q]) && pat_con(rows[r].p[q]) >= 0) any = true;
            if (any) { j = q; break; }
        }
    }
    if (j < 0) {
        Node *e = rows[0].rhs; row_reached(&rows[0]);
        for (int q = 0; q < nv; q++) if (!pat_wild(rows[0].p[q]) && strcmp(rows[0].p[q]->s, vars[q]->s)) e = x_rename(e, rows[0].p[q]->s, vars[q]->s);
        return e;
    }
    int jr = 0; while (jr < nr && (pat_wild(rows[jr].p[j]) || pat_con(rows[jr].p[j]) < 0)) jr++;
    int d = CON[pat_con(rows[jr].p[j])].data;
    int N = DATA[d].ncons;
    Node **alts = calloc((size_t)N + 2, sizeof(Node *)); int na = 0;
    bool need_default = false;
    for (int ci = 0; ci < N; ci++) {
        int c = DATA[d].cons[ci];
        int nf = CON[c].nfields + CON[c].ndims;
        bool named = false;
        for (int r = 0; r < nr; r++) if (pat_con(rows[r].p[j]) == c) named = true;
        if (!named) { need_default = true; continue; }
        /* the field names: the first row's variables there, else fresh */
        Node **fv = calloc((size_t)nf + 1, sizeof(Node *));
        for (int r = 0; r < nr && nf; r++) if (pat_con(rows[r].p[j]) == c) {
            Node *p = rows[r].p[j];
            for (int q = 0; q < nf; q++) if (!fv[q] && p->list && q + 1 < p->n && !p->it[q + 1]->list && !pat_wild(p->it[q + 1]) && con_lookup(p->it[q + 1]->s) < 0) {
                bool dup = false; for (int z = 0; z < q; z++) if (fv[z] && !strcmp(fv[z]->s, p->it[q + 1]->s)) dup = true;
                bool clash = false; for (int z = 0; z < nv; z++) if (z != j && !strcmp(vars[z]->s, p->it[q + 1]->s)) clash = true;
                if (!dup && !clash) fv[q] = p->it[q + 1];
            }
            break;
        }
        for (int q = 0; q < nf; q++) if (!fv[q]) fv[q] = xfresh("f");
        /* the rows for c: its fields as columns in place of column j */
        int nv2 = nv - 1 + nf;
        Node **v2 = calloc((size_t)nv2 + 1, sizeof(Node *));
        for (int q = 0; q < j; q++) v2[q] = vars[q];
        for (int q = 0; q < nf; q++) v2[j + q] = fv[q];
        for (int q = j + 1; q < nv; q++) v2[q - 1 + nf] = vars[q];
        Row *r2 = calloc((size_t)nr + 1, sizeof(Row)); int n2 = 0;
        for (int r = 0; r < nr; r++) {
            Node *p = rows[r].p[j];
            int pc = pat_con(p);
            if (!pat_wild(p) && pc >= 0 && pc != c) continue;
            Row x; x.p = calloc((size_t)nv2 + 1, sizeof(Node *)); x.rhs = rows[r].rhs; x.id = rows[r].id;
            for (int q = 0; q < j; q++) x.p[q] = rows[r].p[q];
            for (int q = j + 1; q < nv; q++) x.p[q - 1 + nf] = rows[r].p[q];
            if (pc == c) {
                int got = p->list ? p->n - 1 : 0;
                Ford *fd = ford_of(CON[c].name);
                if (fd && got == nf - fd->nidx) { Node *q2 = nlist(nf + 1); q2->it[0] = p->list ? p->it[0] : p; for (int q = 0; q < got; q++) q2->it[q + 1] = p->it[q + 1]; for (int q = got; q < nf; q++) q2->it[q + 1] = nsym("_"); p = q2; got = nf; }
                if (got != nf) die("a pattern has the wrong number of fields for", CON[c].name);
                for (int q = 0; q < nf; q++) x.p[j + q] = p->it[q + 1];
            } else {
                for (int q = 0; q < nf; q++) x.p[j + q] = nsym("_");
                if (!pat_wild(p) && strcmp(p->s, vars[j]->s)) x.rhs = x_rename(x.rhs, p->s, vars[j]->s);
            }
            r2[n2++] = x;
        }
        Node *body = cc_match(v2, nv2, r2, n2, fname);
        Node *pat = nlist(nf + 1); pat->it[0] = nsym(CON[c].name);
        for (int q = 0; q < nf; q++) pat->it[q + 1] = fv[q];
        alts[na++] = lst(2, pat, body);
    }
    if (need_default) {
        /* the rows with a variable in column j */
        Node **v2 = calloc((size_t)nv + 1, sizeof(Node *));
        for (int q = 0; q < nv; q++) v2[q] = vars[q];
        Row *r2 = calloc((size_t)nr + 1, sizeof(Row)); int n2 = 0;
        for (int r = 0; r < nr; r++) if (pat_wild(rows[r].p[j]) || pat_con(rows[r].p[j]) < 0) {
            Row x = rows[r]; x.p = calloc((size_t)nv + 1, sizeof(Node *));
            memcpy(x.p, rows[r].p, (size_t)nv * sizeof(Node *));
            if (!pat_wild(x.p[j]) && strcmp(x.p[j]->s, vars[j]->s)) x.rhs = x_rename(x.rhs, x.p[j]->s, vars[j]->s);
            x.p[j] = nsym("_");
            r2[n2++] = x;
        }
        alts[na++] = lst(2, nsym("_"), cc_match(v2, nv, r2, n2, fname));
    }
    Node *cs = nlist(na + 2); cs->it[0] = nsym("case"); cs->it[1] = vars[j];
    for (int i = 0; i < na; i++) cs->it[i + 2] = alts[i];
    free(alts);
    return cs;
}
/* a clause's left side as its function and patterns: f p..., a op b */
typedef struct { const char *f; Node **ps; int np; Node *field; } Lhs;
static Lhs lhs_of(Node *e) {
    Lhs l = { 0 };
    if (!e->list) { l.f = e->s; return l; }
    /* (fld (f args)): a copattern */
    if (e->n == 2 && !e->it[0]->list && fld_lookup(e->it[0]->s) >= 0 && e->it[1]->list) { l = lhs_of(e->it[1]); l.field = e->it[0]; return l; }
    if (e->n == 2 && !e->it[0]->list && fld_lookup(e->it[0]->s) >= 0 && !e->it[1]->list) { l.f = e->it[1]->s; l.field = e->it[0]; return l; }
    if (e->it[0]->list) die("a clause's left side is a name applied to patterns", NULL);
    l.f = e->it[0]->s; l.np = e->n - 1;
    l.ps = calloc((size_t)l.np + 1, sizeof(Node *));
    for (int i = 0; i < l.np; i++) l.ps[i] = pat_norm(e->it[i + 1]);
    return l;
}
static Node *clauses_untyped(const char *f, Lhs *ls, Node **rhs, int n) {
    int np = ls[0].np;
    for (int i = 1; i < n; i++) if (ls[i].np != np) die("clauses with different numbers of patterns:", f);
    Node **vars = calloc((size_t)np + 1, sizeof(Node *));
    for (int q = 0; q < np; q++) {
        /* a column whose patterns are one variable keeps its name */
        const char *nm = NULL; bool same = true;
        for (int i = 0; i < n; i++) {
            Node *p = ls[i].ps[q];
            if (pat_wild(p)) continue;
            if (pat_con(p) >= 0 || p->list) { same = false; break; }
            if (!nm) nm = p->s; else if (strcmp(nm, p->s)) same = false;
        }
        bool dup = false; for (int z = 0; z < q && nm; z++) if (!strcmp(vars[z]->s, nm)) dup = true;
        vars[q] = same && nm && !dup ? nsym(nm) : xfresh("a");
    }
    Row *rows = calloc((size_t)n + 1, sizeof(Row));
    for (int i = 0; i < n; i++) rows[i] = (Row){ ls[i].ps, rhs[i], i };
    bool *ru_saved = row_used; int rn_saved = row_n;   /* (a clause compiled inside another's) */
    rows_track(n);
    Node *body = cc_match(vars, np, rows, n, f);
    rows_check(f); row_used = ru_saved; row_n = rn_saved;
    for (int q = np - 1; q >= 0; q--) body = sym3("lam", vars[q], body);
    return body;
}

/* ---------------- declarations ---------------- */
static void toplevel(Node *f);
static void x_top(Node *f);
static Node *core(Node *t) { return resolve(t, false); }
static Node *resolve_keep(Node *t) { return t; }
typedef struct { int np; Node **pn, **pty; bool *imp, *rev; Node *R; int *ord; } TSig;   /* ord: when the parameters stand permuted (a recursion on a later one, clauses_typed), where the k-th argument of a call stands */
/* does the implicit argument a ({p}, or {{p}} marked for an instance position) stand at parameter q? */
static bool imp_at(Node *a, Node *pn) { return a->list && a->n == 2 && is(a->it[0], "%imp") && (!bnd_inst(a->it[0]) || bnd_inst(pn)); }
static TSig tsig;
static Node *autobind(Node *sig);
static void tele(Node *sig);
static void tele_more(Lhs *ls, int ncl);
static Node *clauses_typed(const char *f, Lhs *ls, Node **rhs, int n);
static Node *clauses_wf(const char *f, Lhs *ls, Node **rhs, int n, Node *m, Node *L);
static void x_checked(const char *name, Node *sig, Node *body, bool expect, uint64_t mask, int mlen);
static Node *copattern_clauses(const char *f, Lhs *ls, Node **rhs, int n, bool typed);
static Node *copattern_wrap(Node *b);
static void x_record(const char *R, Node **ps, int np, Node **fn, Node **ft, int nf, bool expect);

static void el_once(Node *sig, Node *body, Node **T, Node **t);
static void x_primitive(const char *nm, bool tick, Node *sig, Node *b, Tok *nt);
static void x_foreign(const char *nm, Tok *st, Node *sig, Tok *nt);
static Node *x_solve_expr(Node *e);
static bool has_hole(Node *t);
static Node *strip_marks(Node *t);
/* the names defined without a signature later in the file: a use before the definition declares it */
static char **xlater; static int nxlater, capxlater;
static void free_syms(Node *t, Names *bound, Names *out) {
    if (!t->list) { if (!names_has(bound, t->s)) names_add(out, t->s); return; }
    if (t->n == 0) return;
    if (!t->it[0]->list) {
        const char *k = t->it[0]->s;
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && t->n == 3) {
            int save = bound->n; names_add(bound, t->it[1]->s); free_syms(t->it[2], bound, out); bound->n = save; return;
        }
        if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma")) && t->n == 4) {
            free_syms(t->it[2], bound, out); int save = bound->n; names_add(bound, t->it[1]->s); free_syms(t->it[3], bound, out); bound->n = save; return;
        }
        if (!strcmp(k, "let") && t->n == 4) {
            free_syms(t->it[2], bound, out); int save = bound->n; names_add(bound, t->it[1]->s); free_syms(t->it[3], bound, out); bound->n = save; return;
        }
        if (!strcmp(k, "case") || !strcmp(k, "elim")) {
            int from = !strcmp(k, "elim") ? 3 : 2;
            for (int i = 1; i < from && i < t->n; i++) free_syms(t->it[i], bound, out);
            for (int i = from; i < t->n; i++) {
                Node *a = t->it[i]; if (!a->list || a->n != 2) continue;
                int save = bound->n;
                if (a->it[0]->list) for (int q = 1; q < a->it[0]->n; q++) names_add(bound, a->it[0]->it[q]->s);
                else names_add(bound, a->it[0]->s);
                free_syms(a->it[1], bound, out); bound->n = save;
            }
            return;
        }
    }
    for (int i = 0; i < t->n; i++) free_syms(t->it[i], bound, out);
}
static void declare_forward(Node *body) {
    Names b = { 0 }, out = { 0 };
    free_syms(body, &b, &out);
    for (int i = 0; i < out.n; i++)
        for (int k = 0; k < nxlater; k++) if (xlater[k] && !strcmp(xlater[k], out.v[i]) && global_lookup(out.v[i]) < 0) {
            x_top(sym2("declare", nsym(out.v[i])));
        }
    free(b.v); free(out.v);
}
/* an untyped definition is not elaborated, so a checked definition with leading implicit
 * parameters applied in it positionally takes the first explicit argument as the implicit one
 * (idw #3 binding A := #3): refused, with the remedy named */
static void no_ctor_name(const char *name);
static const char *imp_misuse(Node *t, Names *bound) {
    if (!t->list) return NULL;   /* a bare name: passed on, its arguments given wherever it is applied */
    if (t->n == 0) return NULL;
    Node *h = t->it[0];
    if (!h->list) {
        const char *k = h->s;
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "%lam") || !strcmp(k, "forall")) && t->n == 3 && !t->it[1]->list) {
            int save = bound->n; names_add(bound, t->it[1]->s); const char *r = imp_misuse(t->it[2], bound); bound->n = save; return r;
        }
        if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma") || !strcmp(k, "let")) && t->n == 4 && !t->it[1]->list) {
            const char *r = imp_misuse(t->it[2], bound); if (r) return r;
            int save = bound->n; names_add(bound, t->it[1]->s); r = imp_misuse(t->it[3], bound); bound->n = save; return r;
        }
        if (!strcmp(k, "case") || !strcmp(k, "elim")) {
            int from = !strcmp(k, "elim") ? 3 : 2;
            for (int i = 1; i < from && i < t->n; i++) { const char *r = imp_misuse(t->it[i], bound); if (r) return r; }
            for (int i = from; i < t->n; i++) {
                Node *a = t->it[i]; if (!a->list || a->n != 2) continue;
                int save = bound->n;
                if (a->it[0]->list) for (int q = 1; q < a->it[0]->n; q++) { if (!a->it[0]->it[q]->list) names_add(bound, a->it[0]->it[q]->s); }
                else names_add(bound, a->it[0]->s);
                const char *r = imp_misuse(a->it[1], bound); bound->n = save; if (r) return r;
            }
            return NULL;
        }
        if (!strcmp(k, "%imp") || !strcmp(k, "quote")) return NULL;
        if (!names_has(bound, k) && con_lookup(k) >= 0) { for (int i = 1; i < t->n; i++) { const char *r = imp_misuse(t->it[i], bound); if (r) return r; } return NULL; }
        IMask *m = !names_has(bound, k) ? imask_of(k) : NULL;
        if (m && (m->m & 1) && global_lookup(k) >= 0) {
            /* applied as a typed definition would be: exactly its explicit arguments (an untyped
             * definition passes every parameter in order, the implicit ones too) */
            int nimp = 0; for (int b = 0; b < m->len && b < 64; b++) if (m->m >> b & 1) nimp++;
            int nexp = m->len - nimp, na = t->n - 1;
            bool braces = t->n >= 2 && t->it[1]->list && t->it[1]->n == 2 && !t->it[1]->it[0]->list && !strcmp(t->it[1]->it[0]->s, "%imp");
            if (!braces && na == nexp && na < m->len) return k;
            for (int i = 1; i < t->n; i++) { const char *r = imp_misuse(t->it[i], bound); if (r) return r; }
            return NULL;
        }
    }
    for (int i = 0; i < t->n; i++) { const char *r = imp_misuse(t->it[i], bound); if (r) return r; }
    return NULL;
}
static void def_untyped(const char *name, Node *body) {
    no_ctor_name(name);
    if (x_typed) {
        Names b = { 0 }; const char *bad = imp_misuse(body, &b); free(b.v);
        if (bad) {
            char m[512]; snprintf(m, sizeof m, "%s: an untyped definition applies %s to its explicit arguments only, so the first would stand for an implicit parameter: give the implicit ones too (in order, or in braces: %s {…} …) or write the definition's signature, in", name, bad, bad);
            die(m, name);
        }
    }
    body = core(body);
    declare_forward(body);
    for (int k = 0; k < nxlater; k++) if (xlater[k] && !strcmp(xlater[k], name)) { xlater[k] = NULL; break; }
    x_top(sym3("def", nsym(name), body));
}
/* a data declaration: data D params = c fields | ...  A field is (x : A), (i : I) a dimension, a
 * system [ φ ↦ e , ... ] the boundary; a bare atom is the field's type (or, in an unchecked type,
 * its name). */
/* a constructor given by its type: its fields the Π telescope's binders, and a path type at the end
 * (a ≡ b, PathP (λ i → T) a b, a ≡[ i ↦ T ] b) a dimension with that boundary */
/* indexed families by fording (E28): `data D ps : (i : Idx) → Type where c : … → D ps e` declares
 * D with the indices as its last parameters (%ix0 …) and gives each constructor an equality field
 * per index, (%e0 :0 (%ix0 ≡ e)), at multiplicity 0 so it is erased. A constructor applied without
 * its equality fields gets refl at the declared index expressions (over its field arguments); a
 * pattern without them is padded with _. Matching does not insert transports: in a branch the
 * equality is a field (e : m ≡ e') to transport along by hand */
static Node *ctor_by_type(const char *c, Node *T, const char *D) {
    Node *items[128]; int n = 0;
    items[n++] = nsym(c);
    static int nfld1;
    while (T->list && T->n == 2 && is(T->it[0], "%impPi")) T = T->it[1];
    while (T->list && T->n == 4 && is(T->it[0], "Pi")) {
        if (n >= 120) die("too many fields in", c);
        Node *x = T->it[1];
        if (x->s[0] == '%') { char nm[32]; snprintf(nm, sizeof nm, "x%d", nfld1++); Node *y = nsym(nm); T = sym4("Pi", y, T->it[2], x_rename(T->it[3], x->s, y->s)); x = y; }
        items[n++] = lst(2, x, core(T->it[2]));
        T = T->it[3];
        while (T->list && T->n == 2 && is(T->it[0], "%impPi")) T = T->it[1];
    }
    Node *a = NULL, *b = NULL;
    if (T->list && T->n == 3 && !T->it[0]->list && !strcmp(T->it[0]->s, "_≡_")) { a = T->it[1]; b = T->it[2]; }
    else if (T->list && T->n == 4 && is(T->it[0], "PathP")) { a = T->it[2]; b = T->it[3]; }
    if (a) {
        char nm[32]; snprintf(nm, sizeof nm, "i%d", nfld1++);
        items[n++] = lst(2, nsym("dims"), nsym(nm));
        items[n++] = lst(2, sym3("=", nsym(nm), nsym("0")), core(a));
        items[n++] = lst(2, sym3("=", nsym(nm), nsym("1")), core(b));
    } else {
        Node *h = T->list ? T->it[0] : T;
        if (h->list || strcmp(h->s, D)) die("a constructor's type ends in its data type or a path in it:", c);
        if (data_nidx) {
            /* the last data_nidx arguments of the result are the index expressions: an equality
             * field each, and the constructor remembered for its applications and patterns */
            int na = T->list ? T->n - 1 : 0;
            if (na < data_nidx) die("an indexed constructor's type ends in the family at its indices:", c);
            Ford f = { strdup(c), data_nidx, n - 1, calloc((size_t)n, sizeof(char *)), calloc((size_t)data_nidx, sizeof(Node *)) };
            for (int q = 1; q < n; q++) f.fields[q - 1] = strdup(items[q]->it[0]->s);
            for (int q = 0; q < data_nidx; q++) {
                Node *e = T->it[na - data_nidx + q + 1];
                f.idx[q] = e;
                char nm[32]; snprintf(nm, sizeof nm, "%%e%d", nfld1++);
                Node *en = nsym(nm); bnd_set_mult(en, 0);
                /* PathP (λ m → Idx) %ixq e, as core (a data field's type takes no implicit arguments) */
                items[n++] = lst(2, en, core(lst(4, nsym("PathP"), sym3("plam", nsym("%m"), data_ixtypes[q]), nsym(data_ixnames[q]), e)));
            }
            GROW(FORD, capford, (size_t)nford + 1); FORD[nford++] = f;
        }
    }
    Node *r = nlist(n);
    for (int i = 0; i < n; i++) r->it[i] = items[i];
    return r;
}
static const char *data_name_parsing;
static Node *p_data_ctor(void) {
    Tok *c = NX();
    if (c->k != X_ID || reserved(c->s)) x_err("a constructor's name, found", c);
    if (at(":")) { NX(); return ctor_by_type(c->s, p_expr(0), data_name_parsing); }
    Node *items[128]; int n = 0;
    items[n++] = nsym(c->s);
    Node *dims[16]; int nd = 0; Node *bdy = NULL;
    while (atom_start(P0()) || P0()->k == X_LB) {
        if (n >= 120) x_err("too many fields", P0());
        if (P0()->k == X_LB) { bdy = p_system(); continue; }
        if (P0()->k == X_LP) {
            Bnd bs[64]; int save = xp; int nb = p_binders(bs, 64, false);
            if (nb && bs[0].ty) {
                for (int q = 0; q < nb; q++) {
                    if (bs[q].dim) { if (nd == 16) x_err("too many dimensions", P0()); dims[nd++] = bs[q].name; }
                    else items[n++] = lst(2, bs[q].name, core(bs[q].ty));
                }
                continue;
            }
            xp = save;
        }
        Node *a = p_post();
        if (!x_typed && !a->list) items[n++] = a;
        else { static int nfld0; char nm[32]; snprintf(nm, sizeof nm, "x%d", nfld0++); items[n++] = lst(2, nsym(nm), core(a)); }
    }
    if (nd) {
        Node *dl = nlist(nd + 1); dl->it[0] = nsym("dims");
        for (int q = 0; q < nd; q++) dl->it[q + 1] = dims[q];
        items[n++] = dl;
        if (bdy) for (int q = 1; q < bdy->n; q++) items[n++] = core(bdy->it[q]);
    } else if (bdy) x_err("a boundary needs dimensions (i : I)", P0());
    Node *r = nlist(n);
    for (int i = 0; i < n; i++) r->it[i] = items[i];
    return r;
}
static Node *p_data(void) {
    want("data");
    Tok *d = NX();
    if (d->k != X_ID) x_err("a data type's name, found", d);
    Node *ps[32]; int np = 0;
    while (!at("=") && !at("where") && !at(":") && P0()->k != X_VS && P0()->k != X_VC && P0()->k != X_EOF) {
        if (np == 32) x_err("too many parameters", P0());
        if (P0()->k == X_LP) {
            Bnd bs[16]; int nb = p_binders(bs, 16, true);
            for (int q = 0; q < nb; q++) ps[np++] = lst(2, bs[q].name, core(bs[q].ty));
            continue;
        }
        Tok *t = NX();
        if (t->k != X_ID) x_err("a parameter, found", t);
        ps[np++] = x_typed ? lst(2, nsym(t->s), nsym("U")) : nsym(t->s);
    }
    Node *cs[256]; int nc = 0;
    data_name_parsing = d->s;
    data_nidx = 0;
    if (at(":")) {
        /* data D ps : (i : Idx) → … → Type: the indices, as the last parameters %ix0 … (fording) */
        NX(); Node *ty = p_expr(0);
        while (ty->list && ty->n == 2 && is(ty->it[0], "%impPi")) ty = ty->it[1];
        while (ty->list && ty->n == 4 && is(ty->it[0], "Pi")) {
            if (data_nidx == 16 || np == 32) x_err("too many indices", P0());
            char nm[32]; snprintf(nm, sizeof nm, "%%ix%d", data_nidx);
            data_ixnames[data_nidx] = strdup(nm);
            Node *dom = ty->it[2];
            if (!ty->it[1]->list && ty->it[1]->s[0] != '%') dom = x_rename(dom, ty->it[1]->s, nm);
            data_ixtypes[data_nidx] = core(dom);
            ps[np++] = lst(2, nsym(nm), data_ixtypes[data_nidx]);
            ty = x_rename(ty->it[3], ty->it[1]->s, nm);
            data_nidx++;
        }
    }
    Node *head = nsym(d->s);
    if (np) { head = nlist(np + 1); head->it[0] = nsym(d->s); for (int i = 0; i < np; i++) head->it[i + 1] = ps[i]; }
    if (at("where")) {
        NX(); wantk(X_VO, "a block of constructors");
        while (P0()->k != X_VC) {
            if (nc == 256) x_err("too many constructors", P0());
            cs[nc++] = p_data_ctor();
            if (P0()->k == X_VS || P0()->k == X_SEMI) NX(); else if (P0()->k != X_VC) x_err("expected a new constructor, found", P0());
        }
        NX();
    } else if (at("=")) {
        NX();
        if (at("|")) NX();
        for (;;) {
            if (nc == 256) x_err("too many constructors", P0());
            cs[nc++] = p_data_ctor();
            if (at("|")) { NX(); continue; }
            break;
        }
    }
    Node *r = nlist(nc + 2); r->it[0] = nsym("data"); r->it[1] = head;
    for (int i = 0; i < nc; i++) r->it[i + 2] = cs[i];
    return r;
}
/* the end of a declaration */
static Node *sig_of(const char *n);
static void decl_end(void) {
    if (P0()->k == X_VS) { NX(); return; }
    if (P0()->k == X_EOF || P0()->k == X_VC) return;
    x_err("expected the end of the declaration, found", P0());
}
/* a clause's left side up to = (or : for a signature) */
static Node *p_lhs(void) { return p_expr(1); }
static bool x_fail_next;
/* a clause's right side, and its where block (definitions in scope of it) */
static Node *p_rhs(void) {
    Node *e = p_expr(0);
    if (at("where")) e = p_block_lets(e);
    return e;
}
static void x_test(const char *h, Node *a, Node *b) {
    Node *f = b ? sym3(h, a, b) : sym2(h, a);
    x_top(f);
}
static void p_decl(void);
extern char decl_pos[512];
static void p_decl_in(void);
static void decl_reset(void);
static const char *decl_name;   /* the declaration being handled, once its name is read */
static bool x_collecting;
/* A declaration. An error raised while it is handled (die, x_sem: in the clause compiler, the
 * elaborator, the forms it becomes) is its failure: a FAIL line with the error's message (ok:
 * rejected under #fail), the load going on at the next declaration. Text that cannot be read
 * (x_unreadable) ends the load as before. */
static void p_decl(void) {
    jmp_buf jb; jmp_buf *outer = die_jb;
    int depth0 = toplevel_depth, xp0 = xp;
    bool fail = x_fail_next, coll = x_collecting, inst = pending_instance; Module *mod = cur_mod;
    const char *name0 = decl_name;
    decl_name = NULL;
    if (setjmp(jb)) {
        die_jb = outer;
        if (x_unreadable) {
            if (outer) die(die_msg, NULL);
            fprintf(stderr, "%s\n", die_msg); exit(1);
        }
        toplevel_depth = depth0; x_collecting = coll; pending_instance = inst; cur_mod = mod; x_fail_next = false;
        decl_reset();
        char pos[512]; { int save = xp; xp = xp0; snprintf(pos, sizeof pos, "%s:%d", x_file ? x_file : "?", P0()->line); xp = save; }
        if (fail) passes++; else failures++;
        printf("%s: %s %s\n   reason: %s\n", fail ? "ok" : "FAIL", fail ? "rejected" : "checks", decl_name ? decl_name : pos, die_msg);
        if (!fail) printf("   in: %s\n", pos);
        fflush(stdout);
        /* on at the next declaration of this block */
        int d = 0; for (int i = xp0; i < xp && i < nlt; i++) { if (lt[i].k == X_VO) d++; else if (lt[i].k == X_VC) d--; }
        while (P0()->k != X_EOF && !(d == 0 && (P0()->k == X_VS || P0()->k == X_VC))) { if (P0()->k == X_VO) d++; else if (P0()->k == X_VC) d--; NX(); }
        if (P0()->k == X_VS) NX();
        decl_name = name0;
        return;
    }
    die_jb = &jb; x_unreadable = false;
    p_decl_in();
    die_jb = outer; decl_name = name0;
}
static void p_decl_in(void) {
    Tok *t = P0();
    snprintf(decl_pos, sizeof decl_pos, "%s:%d", x_file ? x_file : "?", t->line);
    bool fail = x_fail_next; x_fail_next = false;
    if (x_is(t, "#fail")) { NX(); x_fail_next = true; return; }
    if (x_is(t, "instance")) { NX(); pending_instance = true; x_fail_next = fail; p_decl(); pending_instance = false; return; }   /* the next definition is an instance (E28) */
    if (x_is(t, "deriving")) {
        /* deriving eq D [as name]: a decidable equality on the data type D, its code derived by
         * lib/derive.cub (derive-eq) over the data table, checked and defined at D → D → Bool as
         * name (D·eq by default) (plan F31) */
        NX(); Tok *what = NX(); Tok *dn = NX();
        if (!x_is(what, "eq") && !x_is(what, "Eq")) x_sem("deriving eq D is what can be derived; found", what);
        if (dn->k != X_ID || data_lookup(dn->s) < 0) x_sem("deriving: not a data type:", dn);
        const char *nm = NULL;
        if (x_is(P0(), "as")) { NX(); nm = NX()->s; }
        if (global_lookup("derive-eq") < 0) x_sem("deriving needs lib/derive.cub loaded, for", dn);
        int di = -1; for (int i = 0; i < nchkdata; i++) if (CHKDATA[i] == data_lookup(dn->s)) di = i;
        if (di < 0) x_sem("deriving: the data type is not checked:", dn);
        long steps = 0;
        char *r = readback(nl2("%%derive-eq", wordn(di)), &steps);
        const char *saved = src; src = r; Node *rn = read_node(); src = saved;
        Node *code = rn && rn->list && rn->n == 2 && is(rn->it[0], "some") ? rn->it[1] : NULL;
        ncn = 0; Node *term = code ? cn(code) : NULL;
        free(r);
        if (!term) x_sem("deriving eq: not derivable (a higher inductive type, or a field of a kind not covered):", dn);
        char *name = nm ? strdup(nm) : NULL;
        if (!name) { name = malloc(strlen(dn->s) + 8); sprintf(name, "%s·eq", dn->s); }
        Node *D = nsym(dn->s);
        Node *sig = sym4("Pi", nsym("%x"), D, sym4("Pi", nsym("%y"), D, nsym("Bool")));
        tsig.np = 0;
        x_checked(name, sig, term, !fail, 0, 0);
        decl_end(); return;
    }
    if (x_is(t, "module")) {
        NX(); Tok *mn = NX(); if (mn->k != X_ID) x_err("a module's name, found", mn);
        if (cur_mod) x_sem("a module inside a module:", mn);
        Bnd bs[64]; int nb = p_binders(bs, 64, true);
        want("where"); wantk(X_VO, "a block of definitions");
        GROW(MODS, capmods, (size_t)nmods + 1); Module *m = &MODS[nmods++];
        memset(m, 0, sizeof *m); m->name = strdup(mn->s); m->np = nb; m->ps = calloc((size_t)nb + 1, sizeof(MPar));
        for (int i = 0; i < nb; i++) m->ps[i] = (MPar){ bs[i].name, bs[i].ty, bs[i].imp };
        cur_mod = m;
        while (P0()->k != X_VC) { p_decl(); if (P0()->k == X_VS) NX(); }
        NX();
        cur_mod = NULL;
        decl_end(); return;
    }
    if (x_is(t, "open")) {
        /* open M a b: each member of M, at the arguments */
        NX(); Tok *mn = NX(); Module *m = mod_of(mn->s); if (!m) x_sem("not a module:", mn);
        Node *given[64]; int ng = 0;
        while (P0()->k != X_VS && P0()->k != X_VC && P0()->k != X_EOF && P0()->k != X_SEMI && ng < 64) given[ng++] = p_atom();
        /* the arguments at the parameters: an implicit one {a} where given, else a hole */
        Node *args[64]; int gi = 0;
        for (int i = 0; i < m->np; i++) {
            bool gimp = gi < ng && given[gi]->list && given[gi]->n == 2 && is(given[gi]->it[0], "%imp");
            if (m->ps[i].imp) args[i] = gimp ? given[gi++]->it[1] : nsym("%hole");
            else { if (gi >= ng || gimp) x_sem("open: the module's parameters are not all given, after", mn); args[i] = given[gi++]; }
        }
        if (gi != ng) x_sem("open: more arguments than the module's parameters, after", mn);
        /* an implicit parameter not given: the type of an explicit argument whose parameter's type
         * is that parameter (open Pair true: A is Bool) */
        for (int i = 0; i < m->np; i++) if (m->ps[i].imp && !args[i]->list && !strcmp(args[i]->s, "%hole"))
            for (int j = 0; j < m->np; j++) if (!m->ps[j].imp && !m->ps[j].ty->list && !strcmp(m->ps[j].ty->s, m->ps[i].name->s)) { Node *T = syn_type(args[j], 0); if (T) { args[i] = T; break; } }
        for (int k = 0; k < m->nmem; k++) {
            const char *nm = m->mem[k] + strlen(m->name) + 2;
            Node *sig = m->msig[k];
            const char *pn[64]; for (int i = 0; i < m->np; i++) pn[i] = m->ps[i].name->s;
            sig = subst_multi(sig, pn, args, m->np);
            Node *call = nlist(m->np + 1); call->it[0] = nsym(m->mem[k]);
            for (int i = 0; i < m->np; i++) call->it[i + 1] = m->ps[i].imp ? sym2("%imp", args[i]) : args[i];
            Lhs l = { nm, NULL, 0, NULL }; Node *rhs = m->np ? call : call->it[0];
            Node *sg = autobind(resolve_keep(sig)); tele(sg);
            uint64_t mask = 0; for (int q = 0; q < tsig.np && q < 64; q++) if (tsig.imp[q]) mask |= 1ull << q;
            if (m->minst[k]) pending_instance = true;
            x_checked(nm, sg, clauses_typed(nm, &l, &rhs, 1), true, mask, tsig.np);
            pending_instance = false;
        }
        decl_end(); return;
    }
    if (x_is(t, "import")) {
        NX(); Tok *f = NX();
        if (f->k != X_STR && f->k != X_ID) x_err("import needs a file, found", f);
        char *q = malloc(strlen(f->s) + 3); sprintf(q, "\"%s\"", f->s);
        x_top(sym2("import", nsym(q)));
        decl_end(); return;
    }
    if (x_is(t, "infix") || x_is(t, "infixl") || x_is(t, "infixr")) {
        int assoc = x_is(t, "infixl") ? 1 : x_is(t, "infixr") ? 2 : 0; NX();
        Tok *n = NX(); if (n->k != X_NUM) x_err("a fixity needs a precedence, found", n);
        while (P0()->k == X_ID) {
            const char *o = NX()->s;
            if (o[0] == '_' && strlen(o) > 2 && o[strlen(o) - 1] == '_') { char *b = strdup(o + 1); b[strlen(b) - 1] = 0; o = b; }
            fix_set(o, atoi(n->s), assoc);
        }
        decl_end(); return;
    }
    if (x_is(t, "data")) {
        Node *d = p_data();
        if (x_typed) x_top(sym2(fail ? "checked-data-fail" : "checked-data", d));
        else x_top(d);
        decl_end(); return;
    }
    if (x_is(t, "codata") || x_is(t, "record")) {
        /* codata D params where f : A ...  (or = { f : A ; ... }); record R params likewise: a Σ
         * type with its fields' projections */
        bool co = x_is(t, "codata"); NX();
        Tok *d = NX(); if (d->k != X_ID) x_err("a type's name, found", d);
        Node *ps[32]; int np = 0;
        while (!at("=") && !at("where") && P0()->k != X_VS && P0()->k != X_EOF) {
            if (np == 32) x_err("too many parameters", P0());
            if (P0()->k == X_LP) { Bnd bs[16]; int nb = p_binders(bs, 16, true); for (int q = 0; q < nb; q++) ps[np++] = lst(2, bs[q].name, bs[q].ty); continue; }
            Tok *p = NX(); ps[np++] = lst(2, nsym(p->s), nsym("U"));
        }
        Node *fn[64], *ft[64]; int nf = 0;
        bool block = at("where"); NX();
        if (block) wantk(X_VO, "a block of fields"); else wantk(X_LC, "{");
        while (P0()->k != (block ? X_VC : X_RC)) {
            Tok *f = NX(); if (f->k != X_ID) x_err("a field's name, found", f);
            int fm = x_colon_mult(P0()); if (fm < 0) want(":"); else NX();
            if (nf == 64) x_err("too many fields", f);
            fn[nf] = nsym(f->s); if (fm == 0 || fm == 1) bnd_set_mult(fn[nf], fm); ft[nf++] = p_expr(0);
            if (P0()->k == X_VS || P0()->k == X_SEMI) NX(); else if (P0()->k != (block ? X_VC : X_RC)) x_err("expected a new field, found", P0());
        }
        NX();
        if (co) {
            Node *hd = nsym(d->s);
            if (np) { hd = nlist(np + 1); hd->it[0] = nsym(d->s); for (int i = 0; i < np; i++) hd->it[i + 1] = lst(2, ps[i]->it[0], core(ps[i]->it[1])); }
            Node *cd = nlist(nf + 2); cd->it[0] = nsym("codata"); cd->it[1] = hd;
            for (int i = 0; i < nf; i++) cd->it[i + 2] = lst(2, fn[i], core(ft[i]));
            if (x_typed) x_top(sym2(fail ? "checked-codata-fail" : "checked-codata", cd)); else x_top(cd);
        } else x_record(d->s, ps, np, fn, ft, nf, !fail);
        decl_end(); return;
    }
    if (x_is(t, "mutual")) {
        /* definitions calling each other (copattern definitions): checked together */
        NX(); wantk(X_VO, "a block of definitions");
        Node *gs[64]; int ng = 0;
        while (P0()->k != X_VC) {
            Node *l = p_lhs(); if (l->list || !at(":")) x_err("a mutual block holds signatures and their clauses, found", P0());
            NX(); Node *sg = autobind(p_expr(0));
            if (P0()->k == X_VS) NX();
            Lhs ls[64]; Node *rs[64]; int n = 0;
            for (;;) {
                if (P0()->k != X_ID || strcmp(P0()->s, l->s)) break;
                Node *lh = p_lhs(); want("=");
                if (n == 64) x_err("too many clauses", P0());
                ls[n] = lhs_of(lh); rs[n++] = p_rhs();
                if (P0()->k == X_VS) NX(); else break;
            }
            if (!n) x_sem("a signature without clauses in a mutual block:", P0());
            tele(sg);
            Node *body = ls[0].field ? copattern_wrap(copattern_clauses(l->s, ls, rs, n, true)) : clauses_typed(l->s, ls, rs, n);
            Node *T, *bt; el_once(sg, body, &T, &bt);
            if (ng == 64) x_err("too many definitions in a mutual block", P0());
            no_ctor_name(l->s);
            gs[ng++] = lst(4, nsym("checked"), l, T, bt);
        }
        NX();
        Node *m = nlist(ng + 1); m->it[0] = nsym(fail ? "mutual-fail" : "mutual");
        for (int i = 0; i < ng; i++) m->it[i + 1] = gs[i];
        x_top(m);
        decl_end(); return;
    }
    if (x_is(t, "primitive")) {
        /* primitive [tick] name : T [= t]: a rule of the type theory as a constant. T is checked to
         * be a type; t, its computation, is trusted (core terms, every argument explicit); without
         * t it is a constant with no computation. tick: its value is its stuck call, unfolding
         * only when its last parameter is given ⋄ */
        NX();
        bool tick = x_is(P0(), "tick"); if (tick) NX();
        Tok *nt = NX(); if (nt->k != X_ID) x_err("a primitive names something, found", nt);
        const char *nm = strdup(nt->s);
        want(":"); Node *sig = p_expr(0);
        Node *b = NULL;
        if (at("=")) { NX(); b = core(p_expr(0)); }
        x_primitive(nm, tick, sig, b, nt);
        decl_end(); return;
    }
    if (x_is(t, "world") && P1()->k == X_STR) {
        /* world "name" [: World S St] = handler (D27): a named world, lib/proc.cub's Named S St — the
         * checked definitions world-name : Named S St = (id , handler) and world-name-id : Word = id,
         * id the name's FNV-1a hash (a registry is a list of them; find-world picks one by id). The
         * type is the handler's signature when the handler is a name, else the annotation */
        NX(); Tok *st = NX();
        Node *wt = NULL;
        if (at(":")) { NX(); wt = p_expr(0); }
        want("=");
        Node *h = p_expr(0);
        if (!wt && !h->list) wt = sig_of(h->s);
        if (!wt) x_sem("world: give the handler's type (world \"name\" : World S St = handler), at", st);
        /* the arguments of World in wt, flattened from a curried or a spine application */
        Node *args[8]; int na = 0; Node *w = wt;
        while (w->list && w->n == 2 && na < 8) { args[na++] = w->it[1]; w = w->it[0]; }
        if (w->list && w->n == 3 && !w->it[0]->list && !strcmp(w->it[0]->s, "World")) { args[na++] = w->it[2]; args[na++] = w->it[1]; w = w->it[0]; }
        if (w->list || strcmp(w->s, "World") || na != 2) x_sem("world: the handler's type is World S St, at", st);
        uint32_t hsh = 2166136261u; for (const unsigned char *q = (const unsigned char *)st->s; *q; q++) { hsh ^= *q; hsh *= 16777619u; }
        char idn[40]; snprintf(idn, sizeof idn, "#%u", hsh);
        char nm[256], nmi[256]; snprintf(nm, sizeof nm, "world-%s", st->s); snprintf(nmi, sizeof nmi, "world-%s-id", st->s);
        for (char *q = nm; *q; q++) if (!isalnum((unsigned char)*q) && *q != '-') *q = '-';
        for (char *q = nmi; *q; q++) if (!isalnum((unsigned char)*q) && *q != '-') *q = '-';
        tsig.np = 0; x_checked(strdup(nmi), nsym("Word"), nsym(idn), true, 0, 0);
        tsig.np = 0; x_checked(strdup(nm), lst(3, nsym("Named"), args[1], args[0]), sym3("pair", nsym(idn), h), true, 0, 0);
        decl_end(); return;
    }
    if (x_is(t, "foreign")) {
        /* foreign name "symbol" : T (D27): a C function of the running program as a primitive at T,
         * a Π over arguments of primitive types (Int64, UInt32, Float64, Word, Bytes n; an argument
         * at multiplicity 0 is not passed) to a primitive type or Word; trusted (#trusted: foreign) */
        NX(); Tok *nt = NX(); if (nt->k != X_ID) x_err("foreign names something, found", nt);
        const char *nm = strdup(nt->s);
        Tok *st = NX(); if (st->k != X_STR) x_err("foreign: the C symbol, a string, found", st);
        want(":"); Node *sig = p_expr(0);
        x_foreign(nm, st, sig, nt);
        decl_end(); return;
    }
    if (x_is(t, "variable")) {
        /* generalized variables (E28): `variable x y : T`; a signature mentioning x free gets {x : T}
         * (and the variables T mentions) as leading implicit parameters */
        NX();
        Node *ns[64]; int n = 0;
        while (P0()->k == X_ID && !reserved(P0()->s)) { if (n == 64) x_err("too many names", P0()); ns[n++] = nsym(NX()->s); }
        if (!n) x_err("variable names something, found", P0());
        want(":");
        Node *T = p_expr(0);
        for (int i = 0; i < n; i++) {
            if (ngvar >= 256) x_err("too many generalized variables (256)", P0());
            GROW(GVAR, capgvar, (size_t)ngvar + 1); GVAR[ngvar++] = (GVar){ ns[i]->s, T };
        }
        decl_end(); return;
    }
    if (x_is(t, "postulate")) {
        NX();
        wantk(X_VO, "a block of postulates");
        while (P0()->k != X_VC) {
            Node *ns[64]; int n = 0;
            while (P0()->k == X_ID && !reserved(P0()->s)) { if (n == 64) x_err("too many names", P0()); ns[n++] = nsym(NX()->s); }
            if (!n) x_err("a postulate names something, found", P0());
            Node *ax = nlist(n + 1); ax->it[0] = nsym("axiom");
            if (at(":")) { NX(); Node *T = core(p_expr(0)); for (int i = 0; i < n; i++) ax->it[i + 1] = lst(2, ns[i], T); }
            else for (int i = 0; i < n; i++) ax->it[i + 1] = ns[i];
            x_top(ax);
            if (P0()->k == X_VS || P0()->k == X_SEMI) NX(); else if (P0()->k != X_VC) x_err("expected a new postulate, found", P0());
        }
        NX();
        decl_end(); return;
    }
    if (x_is(t, "#build-loop")) {
        /* #build-loop init step copy copy-id "file" (lib/elf.cub build-loop-plan): a process that
         * runs forever from a state, as a standalone ELF whose memory is bounded: init : St,
         * step : St → Step W St, copy : St → St, copy-id : (s : St) → copy s ≡ s; St first-order
         * with no byte strings. Nothing is written unless the types are these, each specialized
         * code converts with its definition, and the program compiles */
        NX(); Tok *fn[4]; int ck[4];
        for (int i = 0; i < 4; i++) {
            fn[i] = NX(); if (fn[i]->k != X_ID) x_err("#build-loop init step copy copy-id \"file\": a checked definition expected, found", fn[i]);
            ck[i] = checked_index(fn[i]->s); if (ck[i] < 0) x_sem("#build-loop: not a checked definition:", fn[i]);
        }
        Tok *pt = NX(); if (pt->k != X_STR) x_err("#build-loop: the file to write, a string, expected after", fn[3]);
        if (global_lookup("build-loop-plan") < 0) x_sem("#build-loop needs lib/elf.cub loaded, for", fn[0]);
        if (!checked_type_plain(ck[0])) { failures++; printf("FAIL: build-loop %s: its type is not first-order data without byte strings\n", fn[0]->s); decl_end(); return; }
        long steps = 0;
        Node *call = nlist(5); call->it[0] = nsyml("%%build-loop-plan"); for (int i = 0; i < 4; i++) call->it[i + 1] = wordn(ck[i]);
        char *r = readback(call, &steps);
        const char *saved = src; src = r; Node *rn = read_node(); src = saved;
        Node *grp[3]; int ng = 0;
        for (Node *cur = rn; ng < 3; ) {
            if (ng == 2) { grp[ng++] = cur; break; }
            if (!cur || !cur->list || cur->n != 3 || !is(cur->it[0], "pair")) break;
            grp[ng++] = cur->it[1]; cur = cur->it[2];
        }
        free(r);
        if (ng != 3) x_sem("#build-loop: the plan did not read back, for", fn[1]);
        Node *v = grp[0];
        if (v->list && v->n >= 2 && !v->it[0]->list && !strcmp(v->it[0]->s, "not-fo")) {
            failures++; printf("FAIL: build-loop %s: not compilable: %s   [%ld steps]\n", fn[1]->s, v->it[1]->list ? "?" : v->it[1]->s, steps);
            decl_end(); return;
        }
        if (grp[1]->list || strcmp(grp[1]->s, "true")) {
            failures++; printf("FAIL: build-loop %s: the types are not init : St, step : St → Step W St, copy : St → St, copy-id : (s : St) → copy s ≡ s, or a specialized code does not convert with its definition   [%ld steps]\n", fn[1]->s, steps);
            decl_end(); return;
        }
        FILE *o = fopen(pt->s, "wb"); if (!o) x_sem("#build-loop: the file cannot be written:", pt);
        size_t len = 0;
        for (Node *cur = grp[2]; cur->list && cur->n == 3 && !cur->it[0]->list && !strcmp(cur->it[0]->s, "lcons"); cur = cur->it[2]) { fputc((int)atol(cur->it[1]->s + 1), o); len++; }
        fclose(o); chmod(pt->s, 0755);
        passes++; printf("ok: build-loop %s   [%s, %zu bytes, the checker's conversion, %ld steps]\n", fn[1]->s, pt->s, len, steps);
        decl_end(); return;
    }
    if (x_is(t, "#build")) {
        /* #build s "file" (PLAN 4.4 item 5): s : Step W A, a process as a tree of steps, written as a
         * standalone static ELF64 executable (lib/elf.cub build-plan): the specialized code is checked
         * by the checker's conversion against s, then encoded with the driver loop as machine code.
         * Nothing is written unless the program compiles and the check holds */
        NX(); Tok *fn = NX(); if (fn->k != X_ID) x_err("#build names a checked definition, found", fn);
        int ck = checked_index(fn->s); if (ck < 0) x_sem("#build: not a checked definition:", fn);
        Tok *pt = NX(); if (pt->k != X_STR) x_err("#build: the file to write, a string, expected after", fn);
        if (global_lookup("build-plan") < 0) x_sem("#build needs lib/elf.cub loaded, for", fn);
        long steps = 0;
        char *r = readback(nl2("%%build-plan", wordn(ck)), &steps);
        const char *saved = src; src = r; Node *rn = read_node(); src = saved;
        Node *grp[3]; int ng = 0;
        for (Node *cur = rn; ng < 3; ) {
            if (ng == 2) { grp[ng++] = cur; break; }
            if (!cur || !cur->list || cur->n != 3 || !is(cur->it[0], "pair")) break;
            grp[ng++] = cur->it[1]; cur = cur->it[2];
        }
        free(r);
        if (ng != 3) x_sem("#build: the plan did not read back, for", fn);
        Node *v = grp[0];
        if (v->list && v->n >= 2 && !v->it[0]->list && !strcmp(v->it[0]->s, "not-fo")) {
            failures++; printf("FAIL: build %s: not compilable: %s   [%ld steps]\n", fn->s, v->it[1]->list ? "?" : v->it[1]->s, steps);
            decl_end(); return;
        }
        if (grp[1]->list || strcmp(grp[1]->s, "true")) {
            failures++; printf("FAIL: build %s: the specialized code does not convert with %s   [%ld steps]\n", fn->s, fn->s, steps);
            decl_end(); return;
        }
        FILE *o = fopen(pt->s, "wb"); if (!o) x_sem("#build: the file cannot be written:", pt);
        size_t len = 0;
        for (Node *cur = grp[2]; cur->list && cur->n == 3 && !cur->it[0]->list && !strcmp(cur->it[0]->s, "lcons"); cur = cur->it[2]) { fputc((int)atol(cur->it[1]->s + 1), o); len++; }
        fclose(o); chmod(pt->s, 0755);
        passes++; printf("ok: build %s   [%s, %zu bytes, the checker's conversion, %ld steps]\n", fn->s, pt->s, len, steps);
        decl_end(); return;
    }
    if (x_is(t, "#jit")) {
        /* #jit f [using t1 t2 …] (plan 5.4, 5.5): f compiled to x86-64 and bound to the bytes. The
         * library computes the plan (lib/x86.cub jit-plan): the specialized code (rewritten along the
         * theorems given, each rewrite carrying its path), the checker's conversion of f with the code's
         * meaning, and the bytes. With theorems the path is checked by the checker as the definition
         * f·jit : f ≡ code. f is bound only when the program compiles and the check holds; on closed
         * data the bytes compute f, on anything else (and past 2^64 in a word operation) f's
         * definition does. Trusted: the encoder's templates (PLAN §13 item 3) and the boundary */
        NX(); Tok *fn = NX(); if (fn->k != X_ID) x_err("#jit names a checked definition, found", fn);
        int ck = checked_index(fn->s); if (ck < 0) x_sem("#jit: not a checked definition:", fn);
        long ths[64]; int nth = 0;
        if (x_is(P0(), "using") && P0()->line == fn->line) {
            NX();
            while (P0()->k == X_ID && P0()->line == fn->line) {
                Tok *tn = NX(); int tk = checked_index(tn->s); if (tk < 0) x_sem("#jit … using: not a checked theorem:", tn);
                if (nth < 64) ths[nth++] = tk;
            }
        }
        if (global_lookup("jit-plan") < 0) x_sem("#jit needs lib/x86.cub loaded, for", fn);
        for (int attempt = 0; attempt < 2; attempt++) {
            int nt = attempt ? 0 : nth;
            Node *rl = nsym("lnil"); for (int i = nt - 1; i >= 0; i--) rl = lst(3, nsym("lcons"), wordn(ths[i]), rl);
            long steps = 0;
            char *r = readback(nl3("%%jit-plan", wordn(ck), nl2("%%view-of", rl)), &steps);
            const char *saved = src; src = r; Node *rn = read_node(); src = saved;
            /* the plan's five parts: (pair a (pair b (pair c (pair d e)))) */
            Node *grp[5]; int ng = 0;
            for (Node *cur = rn; ng < 5; ) {
                if (ng == 4) { grp[ng++] = cur; break; }
                if (!cur || !cur->list || cur->n != 3 || !is(cur->it[0], "pair")) break;
                grp[ng++] = cur->it[1]; cur = cur->it[2];
            }
            free(r);
            if (ng != 5) x_sem("#jit: the plan did not read back, for", fn);
            Node *v = grp[0];
            if (v->list && v->n >= 2 && !v->it[0]->list && !strcmp(v->it[0]->s, "not-fo")) {
                printf("jit: %s stays on the net: %s   [%ld steps]\n", fn->s, v->it[1]->list ? "?" : v->it[1]->s, steps);
                decl_end(); return;
            }
            /* without theorems the code is f's normal form, and the check is the checker's conversion;
             * with them the code is only propositionally f, and the check is the path, below */
            if (!nt && (grp[1]->list || strcmp(grp[1]->s, "true"))) {
                failures++; printf("FAIL: jit %s: the specialized code does not convert with %s   [%ld steps]\n", fn->s, fn->s, steps);
                decl_end(); return;
            }
            const char *how = "the checker's conversion";
            if (nt) {
                ncn = 0; Node *term = cn(grp[2]); ncn = 0; Node *ct = cn(grp[3]);
                if (!term || !ct) { printf("jit: %s: the path along the theorems is not readable as a term; compiled without them\n", fn->s); continue; }
                char *jn = malloc(strlen(fn->s) + 8); sprintf(jn, "%s·jit", fn->s);
                tsig.np = 0;
                x_checked(jn, lst(3, nsym("_≡_"), nsym(fn->s), ct), term, true, 0, 0);
                if (checked_index(jn) < 0) { printf("FAIL: jit %s: the path along the theorems does not check\n", fn->s); failures++; decl_end(); return; }
                how = "the path checked as a definition";
            }
            /* the bytes, and the entry's arity (the code's leading λs) */
            size_t cap = 256, len = 0; unsigned char *bytes = malloc(cap);
            for (Node *cur = grp[4]; cur->list && cur->n == 3 && !cur->it[0]->list && !strcmp(cur->it[0]->s, "lcons"); cur = cur->it[2]) {
                if (len == cap) { cap *= 2; bytes = realloc(bytes, cap); }
                bytes[len++] = (unsigned char)atol(cur->it[1]->s + 1);
            }
            int arity = 0; Node *body = grp[3];
            for (; body->list && body->n == 2 && !body->it[0]->list && !strcmp(body->it[0]->s, "clam"); body = body->it[1]) arity++;
            bool used[NATIVE_MAXW] = { 0 };
            for (int i = 0; i < arity && i < NATIVE_MAXW; i++) used[i] = code_reads(body, arity - 1 - i, 0);
            int k = jit_bind(fn->s, ck, bytes, len, arity, used);
            free(bytes);
            if (k < 0) {
                if (k <= -3) printf("jit: %s stays on the net: its %s does not cross the boundary (a function, a dependent pair, a higher inductive type)\n", fn->s, k == -3 - arity ? "result" : "parameter");
                else printf("jit: %s stays on the net: %s\n", fn->s, k == -2 ? "no parameters, or its arity differs from its type's" : "executable memory refused");
                decl_end(); return;
            }
            passes++; printf("ok: jit %s   [%zu bytes, %s, %ld steps]\n", fn->s, len, how, steps);
            decl_end(); return;
        }
        decl_end(); return;
    }
    if (x_is(t, "#show")) { NX(); x_test("show", x_solve_expr(p_expr(0)), NULL); decl_end(); return; }
    if (x_is(t, "#type")) { NX(); x_top(sym2("type", x_solve_expr(p_expr(0)))); decl_end(); return; }
    if (x_is(t, "#usage")) { NX(); Tok *n = NX(); x_top(sym2("usage", nsym(n->s))); decl_end(); return; }
    if (x_is(t, "#trusted")) { NX(); x_top(lst(1, nsym("trusted"))); decl_end(); return; }
    /* the store (F34): #key d, #names K, #history name, #unbind name, #gc */
    if (x_is(t, "#key") || x_is(t, "#names") || x_is(t, "#history") || x_is(t, "#unbind") || x_is(t, "#safe") || x_is(t, "#retract")) { const char *c = NX()->s + 1; Tok *n = NX(); x_top(sym2(c, nsym(n->s))); decl_end(); return; }
    if (x_is(t, "#gc")) { NX(); x_top(lst(1, nsym("gc"))); decl_end(); return; }
    if (x_is(t, "#scope")) { NX(); x_top(lst(1, nsym("scope"))); decl_end(); return; }
    if (x_is(t, "#where")) { NX(); Tok *n = NX(); x_top(sym2("where", nsym(n->s))); decl_end(); return; }
    if (x_is(t, "#run")) { NX(); x_top(sym2("run", x_solve_expr(p_expr(0)))); decl_end(); return; }
    if (x_is(t, "#observe")) { NX(); Tok *n = NX(); x_test("observe", nsym(n->s), x_solve_expr(p_expr(0))); decl_end(); return; }
    if (x_is(t, "#same")) { NX(); Node *a = p_expr(0); want("="); Node *b = p_expr(0); x_test("same", x_solve_expr(a), x_solve_expr(b)); decl_end(); return; }
    if (x_is(t, "#check")) {
        NX(); Node *e = p_expr(0); want(":"); Node *T = p_expr(0);
        if (x_typed) { tsig.np = 0; x_checked(NULL, T, e, !fail, 0, 0); }
        else x_test(fail ? "check-fail" : "check", core(T), core(e));
        decl_end(); return;
    }
    if (x_is(t, "#check-data")) { NX(); x_test("check-data", nsym(NX()->s), NULL); decl_end(); return; }
    if (x_is(t, "#input")) { NX(); Node *ns = nlist(1); ns->it[0] = nsym("input"); while (P0()->k == X_ID) { Node *a = nlist(ns->n + 1); memcpy(a->it, ns->it, (size_t)ns->n * sizeof(Node *)); a->it[ns->n] = nsym(NX()->s); ns = a; } x_top(ns); decl_end(); return; }
    if (x_is(t, "#supply")) { NX(); Node *n = nsym(NX()->s); want("="); x_test("supply", n, core(p_expr(0))); decl_end(); return; }
    /* clauses (and a signature): every clause of one name, consecutive */
    Node *lhs = p_lhs();
    Node *sig = NULL;
    if (at(":")) {
        if (lhs->list) x_err("a signature names one thing, found", P0());
        if (!x_typed) x_sem("a signature, before the checker is loaded:", P0());
        NX(); sig = p_expr(0);
        if (P0()->k != X_VS) x_err("a signature is followed by its definition, found", P0());
        NX();
        lhs = p_lhs();
    }
    want("=");
    Node *rhs = p_rhs();
    Lhs l0 = lhs_of(lhs);
    decl_name = l0.f;
    Lhs *ls = malloc(sizeof(Lhs)); Node **rs = malloc(sizeof(Node *)); int n = 1;
    ls[0] = l0; rs[0] = rhs;
    for (;;) {
        int save = xp;
        if (P0()->k != X_VS) break;
        NX();
        if (!(P0()->k == X_LP || (P0()->k == X_ID && !reserved(P0()->s) && P0()->s[0] != '#'))) { xp = save; break; }
        Node *l2 = p_lhs();
        if (!at("=")) { xp = save; break; }
        Lhs l = lhs_of(l2);
        if (strcmp(l.f, l0.f)) { xp = save; break; }
        NX();
        ls = realloc(ls, (size_t)(n + 1) * sizeof(Lhs)); rs = realloc(rs, (size_t)(n + 1) * sizeof(Node *));
        ls[n] = l; rs[n] = p_rhs(); n++;
    }
    /* inside a module: the definition abstracted over the parameters, its name qualified, its
     * references to the module's members applied to them */
    if (cur_mod) {
        Module *m = cur_mod;
        if (!sig) x_sem("a module's definitions have signatures:", P0());
        if (l0.field) x_sem("a copattern definition inside a module is not supported:", P0());
        Node *osig = mod_subst(m, sig);
        char *q = mod_qual(m, l0.f);
        mod_member(m, q, osig, pending_instance);
        sig = mod_wrap_sig(m, osig);
        int ne = 0; for (int i = 0; i < m->np; i++) if (!m->ps[i].imp) ne++;
        l0.f = q;
        for (int i = 0; i < n; i++) {
            ls[i].f = q;
            Node **ps = calloc((size_t)(ls[i].np + ne + 1), sizeof(Node *)); int k = 0;
            for (int j = 0; j < m->np; j++) if (!m->ps[j].imp) ps[k++] = m->ps[j].name;
            for (int j = 0; j < ls[i].np; j++) ps[k++] = ls[i].ps[j];
            ls[i].ps = ps; ls[i].np = k;
            rs[i] = mod_subst(m, rs[i]);
        }
    }
    /* recursion along a measure: decreasing by m [using L] */
    Node *dec_m = NULL, *dec_l = NULL;
    {
        int save = xp;
        if (P0()->k == X_VS) {
            NX();
            if (x_is(P0(), "decreasing")) {
                NX(); if (!x_is(P0(), "by")) x_err("decreasing by a measure: expected by, found", P0());
                NX(); dec_m = p_expr(0);
                if (x_is(P0(), "using")) { NX(); dec_l = p_expr(0); }
                if (!sig) x_sem("decreasing by needs a signature (its parameters' names), for", P0());
            } else xp = save;
        }
    }
    if (l0.field) {
        /* copattern clauses: one per field, its patterns variables */
        Node *body = copattern_clauses(l0.f, ls, rs, n, sig != NULL);
        if (sig) {
            Node *sg = autobind(sig); tele(sg);
            uint64_t mask = 0; for (int q = 0; q < tsig.np && q < 64; q++) if (tsig.imp[q]) mask |= 1ull << q;
            x_checked(l0.f, sg, copattern_wrap(body), !fail, mask, tsig.np);
        } else def_untyped(l0.f, body);
        decl_end(); return;
    }
    if (sig) {
        Node *sg = autobind(resolve_keep(sig));
        tele(sg);
        uint64_t mask = 0; for (int q = 0; q < tsig.np && q < 64; q++) if (tsig.imp[q]) mask |= 1ull << q;
        for (int i = 0; i < n; i++) if (strcmp(ls[i].f, l0.f)) x_sem("a clause of another definition after a signature:", P0());
        tele_more(ls, n);
        Node *body = dec_m ? clauses_wf(l0.f, ls, rs, n, dec_m, dec_l) : clauses_typed(l0.f, ls, rs, n);
        x_checked(l0.f, sg, body, !fail, mask, tsig.np);
    } else def_untyped(l0.f, clauses_untyped(l0.f, ls, rs, n));
    decl_end();
}
/* a file in the surface language. Loading is reentrant (an import loads another file): each file
 * has its own tokens. skip_imports: its imports were loaded already (load_file loads them first). */
static void load_surface(const char *text, const char *path, bool imports_only) {
    Tok *s_xt = xt, *s_lt = lt; int s_nxt = nxt, s_capxt = capxt, s_nlt = nlt, s_caplt = caplt, s_xp = xp;
    char **s_later = xlater; int s_nlater = nxlater, s_caplater = capxlater;
    const char *s_file = x_file; bool s_typed = x_typed, s_fail = x_fail_next;
    xt = NULL; nxt = capxt = 0; lt = NULL; nlt = caplt = 0; xlater = NULL; nxlater = capxlater = 0;
    x_file = path; x_fail_next = false;
    x_lex(text); x_layout();
    for (int i = 0; i < nlt; i++) if ((i == 0 || lt[i - 1].k == X_VS) && lt[i].k == X_ID && !reserved(lt[i].s) && lt[i].s[0] != '#') {
        GROW(xlater, capxlater, (size_t)nxlater + 1); xlater[nxlater++] = lt[i].s;
    }
    xp = 0;
    /* the declarations of a file loaded once the checker is are checked */
    x_typed = global_lookup("chk-def") >= 0 && !x_untyped_file;
    while (P0()->k != X_EOF) {
        while (P0()->k == X_VS || P0()->k == X_VC) NX();
        if (P0()->k == X_EOF) break;
        bool imp = at("import");
        if (imp != imports_only) {
            /* skip this declaration: to the next item of the file's block */
            NX(); int d = 0;
            while (P0()->k != X_EOF && !(d == 0 && P0()->k == X_VS)) { if (P0()->k == X_VO) d++; else if (P0()->k == X_VC) d--; NX(); }
            continue;
        }
        p_decl();
    }
    xt = s_xt; nxt = s_nxt; capxt = s_capxt; lt = s_lt; nlt = s_nlt; caplt = s_caplt; xp = s_xp;
    xlater = s_later; nxlater = s_nlater; capxlater = s_caplater; x_file = s_file; x_typed = s_typed; x_fail_next = s_fail;
}

/* ---------------- printing the core forms in the surface language ----------------
 * pr(t, prec) is t's text at the context precedence prec (parenthesized when t binds more loosely),
 * on one line or broken over lines indented from the column it starts at. Precedences: 0 λ let if
 * case (they extend to the right), 0 →, 2 ×, operators by their fixity, 6 ∨, 7 ∧, 30 @, 100 an
 * application, 101 an operand. */
typedef struct { char *b; size_t n, cap; } Buf;
static void bput(Buf *o, const char *s) { size_t l = strlen(s); if (o->n + l + 1 > o->cap) { o->cap = (o->n + l + 1) * 2 + 256; o->b = realloc(o->b, o->cap); } memcpy(o->b + o->n, s, l + 1); o->n += l; }
static void bputc(Buf *o, char c) { char s[2] = { c, 0 }; bput(o, s); }
static void bnl(Buf *o, int col) { bputc(o, '\n'); for (int i = 1; i < col; i++) bputc(o, ' '); }
static int pr_width = 100;
static bool pr_typed;        /* a path λ prints as λ i → (the checker tells it apart), else λ (i : I) → */
static bool pr_nodrop;       /* the implicit arguments are dropped already */
static bool pr_keepimp;      /* a term read, not checked: its implicit arguments given in braces */
static bool pr_rawargs;      /* a term not elaborated: every argument as it stands */
static Buf pr_lost;          /* comments of subterms printed within a line */
static int vlen(const char *s) { int n = 0; for (; *s; s++) if (((unsigned char)*s & 0xC0) != 0x80) n++; return n; }
static bool has_nl(const char *s) { return strchr(s, '\n') != NULL; }
static void pr(Buf *o, Node *t, int prec, int col);
static char *pr_str(Node *t, int prec, int col) { Buf b = { 0 }; bput(&b, ""); pr(&b, t, prec, col); return b.b; }
static char *pr_flat(Node *t, int prec);
static bool e_head_kw(const char *s);
/* ((f a) b) as (f a b), a definition's head applied (nothing else changed) */
static Node *flat_apps(Node *t) {
    if (!t->list || t->n == 0) return t;
    if (!t->it[0]->list && is(t->it[0], "%hole")) return nsym("_");   /* an argument left to be found */
    if (!t->it[0]->list && (is(t->it[0], "case") || is(t->it[0], "elim") || is(t->it[0], "sys"))) {
        /* alternatives: their patterns as they are */
        int from = is(t->it[0], "elim") ? 3 : is(t->it[0], "case") ? 2 : 1;
        Node *r = nlist(t->n); r->it[0] = t->it[0];
        for (int i = 1; i < t->n; i++) {
            Node *a = t->it[i];
            if (i < from || !a->list || a->n != 2) r->it[i] = i < from ? flat_apps(a) : a;
            else r->it[i] = lst(2, is(t->it[0], "sys") ? flat_apps(a->it[0]) : a->it[0], flat_apps(a->it[1]));
        }
        return r;
    }
    Node *r = nlist(t->n);
    for (int i = 0; i < t->n; i++) r->it[i] = flat_apps(t->it[i]);
    Node *h = r->it[0];
    if (r->n >= 2 && h->list && h->n >= 2 && !h->it[0]->list && !e_head_kw(h->it[0]->s)) {
        Node *r2 = nlist(h->n + r->n - 1);
        for (int i = 0; i < h->n; i++) r2->it[i] = h->it[i];
        for (int i = 1; i < r->n; i++) r2->it[h->n + i - 1] = r->it[i];
        return r2;
    }
    return r;
}
/* a term (a test's, or a read-back value) in the surface syntax, on one line */
static char *surface_str(Node *t) {
    bool pt = pr_typed, pk = pr_keepimp, pn = pr_nodrop;
    pr_typed = true; pr_keepimp = true;
    jmp_buf jb; char *s; jmp_buf *outer = die_jb;   /* (--serve's, restored after) */
    if (setjmp(jb)) {
        die_jb = outer;
        /* a form the printer does not write: as read */
        if (getenv("CUB_DBGPR")) { fprintf(stderr, "[surface] could not print: "); print_node(stderr, t); fputc('\n', stderr); }
        size_t len; FILE *o = open_memstream(&s, &len); print_node(o, t); fclose(o);
    } else {
        die_jb = &jb;
        s = pr_flat(flat_apps(t), 0);
        die_jb = outer;
    }
    pr_typed = pt; pr_keepimp = pk; pr_nodrop = pn;
    return s;
}
static char *pr_flat(Node *t, int prec) { int w = pr_width; pr_width = 1 << 30; char *s = pr_str(t, prec, 1); pr_width = w; return s; }
static void lost(Node *t) { if (t->cmt) { bput(&pr_lost, t->cmt); t->cmt = NULL; } }
static bool face_has_eq(Node *t) {
    if (!t->list || t->n < 1 || t->it[0]->list) return false;
    if (is(t->it[0], "=")) return true;
    if (is(t->it[0], "&") || is(t->it[0], "|")) { for (int i = 1; i < t->n; i++) if (face_has_eq(t->it[i])) return true; }
    if (is(t->it[0], "forall")) return true;
    return false;
}
/* a face inside a face: φ ∧ ψ, φ ∨ ψ, r = 0, ∀ i → φ */
static void pr_face(Buf *o, Node *t, int prec, int col) {
    if (t->list && t->n >= 1 && !t->it[0]->list) {
        const char *h = t->it[0]->s;
        if (!strcmp(h, "=")) {
            char *r = pr_str(t->it[1], 101, col);
            if (prec > 8) bputc(o, '(');
            bput(o, r); bput(o, " = "); bput(o, t->it[2]->s);
            if (prec > 8) bputc(o, ')');
            return;
        }
        if (!strcmp(h, "&") || !strcmp(h, "|")) {
            int p = h[0] == '&' ? 7 : 6;
            if (prec > p) bputc(o, '(');
            for (int i = 1; i < t->n; i++) { if (i > 1) bput(o, p == 7 ? " /\\ " : " \\/ "); pr_face(o, t->it[i], i == 1 ? p : p + 1, col); }
            if (prec > p) bputc(o, ')');
            return;
        }
        if (!strcmp(h, "forall")) { if (prec > 0) bputc(o, '('); bput(o, "∀ "); bput(o, t->it[1]->s); bput(o, " → "); pr_face(o, t->it[2], 0, col); if (prec > 0) bputc(o, ')'); return; }
    }
    pr(o, t, 101, col);
}
static bool mentions_x(Node *t, const char *x) { return mentions_name(t, x); }
static Node *sig_of(const char *n);
static void sig_set(const char *n, Node *s);
static bool infer_looking(Node *t);
/* is implicit parameter q of h found from an explicit argument whose type the checker infers (it
 * occurs in that argument's domain)? A level always is (an undetermined one is the least) */
/* definitions that compute by cases on an argument: their arguments are not read back from a value of the type */
static char **COMPUTING; static int ncomputing, capcomputing;
static bool is_computing(const char *n) { for (int i = 0; i < ncomputing; i++) if (!strcmp(COMPUTING[i], n)) return true; return false; }
static bool rigid_mentions(Node *t, const char *x) {
    if (!t->list) return !strcmp(t->s, x);
    if (t->n == 0) return false;
    if (!t->it[0]->list) {
        const char *k = t->it[0]->s;
        if (is_computing(k)) return false;
        if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma")) && t->n == 4 && !t->it[1]->list)
            return rigid_mentions(t->it[2], x) || (strcmp(t->it[1]->s, x) && rigid_mentions(t->it[3], x));
        if (!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "let") || !strcmp(k, "forall")) return mentions_name(t, x);
    }
    for (int i = 0; i < t->n; i++) if (rigid_mentions(t->it[i], x)) return true;
    return false;
}
static bool determined(const char *h, int q, Node *app) {
    Node *x = sig_of(h); if (!x) return true;
    const char *nm = NULL; int k = 0;
    while (x) {
        bool im = false;
        if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im = true; x = x->it[1]; }
        if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) return !nm;
        if (k == q) { nm = x->it[1]->s; if (is(x->it[2], "Level")) return true; }
        else if (k > q && !im && k + 1 < app->n && nm && infer_looking(app->it[k + 1]) && rigid_mentions(x->it[2], nm)) return true;
        else if (k > q && im && nm && rigid_mentions(x->it[2], nm) && k + 1 < app->n && determined(h, k, app)) return true;
        x = x->it[3]; k++;
    }
    return false;
}
/* is parameter q of definition h an implicit one of a function type? */
static bool is_hoarg(const char *h, int q) {
    Node *x = sig_of(h); int k = 0;
    while (x) {
        bool im = false;
        if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im = true; x = x->it[1]; }
        if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) return false;
        if (k == q) { Node *A = x->it[2]; return im && A->list && A->n == 4 && is(A->it[0], "Pi"); }
        x = x->it[3]; k++;
    }
    return false;
}
/* an end whose type the checker infers: a variable, a definition applied, a projection, a path at a point */
static bool infer_looking(Node *t) {
    if (!t->list) return con_lookup(t->s) < 0 || (CON[con_lookup(t->s)].nfields == 0 && false);
    if (t->n == 0 || t->it[0]->list) return t->n > 0 && infer_looking(t->it[0]);
    const char *h = t->it[0]->s;
    if (!strcmp(h, "pair") || !strcmp(h, "lam") || !strcmp(h, "plam") || !strcmp(h, "%num") || !strcmp(h, "if") || !strcmp(h, "case") || !strcmp(h, "elim") || !strcmp(h, "let")) return false;
    if (con_lookup(h) >= 0) return false;
    return true;
}
static const char *keyw_name(const char *s) {
    /* names that would read as something else: written with a % prefix is not possible, so refuse */
    return s;
}
static void pr_sym(Buf *o, const char *s, bool iv) {
    if (!strcmp(s, "%cm")) { bput(o, "_"); return; }   /* a constant motive's binder */
    if (!strcmp(s, "0")) { bput(o, iv ? "0" : "i0"); return; }
    if (!strcmp(s, "1")) { bput(o, iv ? "1" : "i1"); return; }
    if (!strcmp(s, "U")) { bput(o, "Type"); return; }
    bput(o, keyw_name(s));
}
/* the binders of a chain of λs and lines */
static Node *pr_binders(Buf *o, Node *t) {
    bool first = true;
    while (t->list && t->n >= 3 && !t->it[0]->list && (is(t->it[0], "lam") || is(t->it[0], "plam"))) {
        bool dim = is(t->it[0], "plam");
        for (int i = 1; i < t->n - 1; i++) {
            if (!first) bputc(o, ' ');
            first = false;
            if (dim && (!pr_typed || pr_keepimp)) { bput(o, "("); bput(o, param_name(t->it[i])->s); bput(o, " : I)"); }
            else bput(o, param_name(t->it[i])->s);
        }
        t = t->it[t->n - 1];
    }
    return t;
}
static void pr_alts(Buf *o, Node **alts, int n, int col, bool flat) {
    for (int i = 0; i < n; i++) {
        Node *a = alts[i];
        if (flat) { if (i) bput(o, "; "); }
        else {
            bnl(o, col);
            if (a->cmt) { char *c = a->cmt; while (*c) { char *e = strchr(c, '\n'); bput(o, "-- "); size_t l = e ? (size_t)(e - c) : strlen(c); char *ln = strndup(c, l); bput(o, ln); free(ln); bnl(o, col); c = e ? e + 1 : c + l; } a->cmt = NULL; }
        }
        if (!a->list || a->n != 2) die("print: a malformed alternative", NULL);
        Node *p = a->it[0];
        if (p->list) { for (int q = 0; q < p->n; q++) { if (q) bputc(o, ' '); bput(o, p->it[q]->s); } }
        else bput(o, p->s);
        bput(o, " → ");
        int c2 = col + vlen(o->b + (strrchr(o->b, '\n') ? strrchr(o->b, '\n') - o->b + 1 : 0)) - (col - 1);
        (void)c2;
        /* the body: on this line, or the next, indented */
        size_t ls = strrchr(o->b, '\n') ? (size_t)(strrchr(o->b, '\n') - o->b + 1) : 0;
        int here = vlen(o->b + ls) + 1;
        /* a case as the body of an alternative that others follow on its line is parenthesized */
        Node *bd = a->it[1];
        bool opencase = bd->list && bd->n >= 1 && (is(bd->it[0], "case") || is(bd->it[0], "elim"));
        char *f = pr_flat(bd, opencase && i < n - 1 ? 1 : 0);
        if (opencase && !flat && here + vlen(f) <= pr_width) { free(f); f = pr_flat(bd, 1); }
        if (flat || here + vlen(f) <= pr_width) { if (!flat && has_nl(f)) { free(f); pr(o, a->it[1], 0, here); } else { bput(o, f); free(f); } }
        else { free(f); bnl(o, col + 2); pr(o, a->it[1], 0, col + 2); }
    }
}
static int cur_col(Buf *o) { size_t ls = strrchr(o->b, '\n') ? (size_t)(strrchr(o->b, '\n') - o->b + 1) : 0; return vlen(o->b + ls) + 1; }
static void pr(Buf *o, Node *t, int prec, int col) {
    lost(t);
    if (!t->list && pr_typed && !pr_rawargs && imask_of(t->s) && (imask_of(t->s)->m & 1)) {
        /* a definition with implicit parameters, as a value: λ over them, given in braces */
        IMask *im = imask_of(t->s); int k = 0;
        while (k < im->len && (im->m >> k & 1)) k++;
        if (prec > 0) bputc(o, '(');
        bput(o, "λ");
        for (int q = 0; q < k; q++) { char b[16]; snprintf(b, sizeof b, " x%d", q); bput(o, b); }
        bput(o, " → "); bput(o, t->s);
        for (int q = 0; q < k; q++) { char b[16]; snprintf(b, sizeof b, " {x%d}", q); bput(o, b); }
        if (prec > 0) bputc(o, ')');
        return;
    }
    if (!t->list) { pr_sym(o, t->s, false); return; }
    if (t->n == 0) die("print: an empty list", NULL);
    Node *h = t->it[0];
    if (!h->list && is(h, "suc") && t->n == 2 && con_lookup("suc") >= 0) {
        /* a numeral: suc (suc ... zero), two or more */
        long k = 0; Node *x = t;
        while (x->list && x->n == 2 && !x->it[0]->list && is(x->it[0], "suc")) { k++; x = x->it[1]; }
        if (k >= 2 && !x->list && is(x, "zero")) { char b[32]; snprintf(b, sizeof b, "%ld", k); bput(o, b); return; }
    }
    if (pr_typed && !pr_rawargs && !h->list && imask_of(h->s) && !pr_nodrop) {
        /* applied to fewer arguments than its implicit parameters before the next explicit one: λ
         * over the rest, all given in braces */
        IMask *im = imask_of(h->s); int a = t->n - 1;
        if (a < im->len && (im->m >> a & 1)) {
            int k = a; while (k < im->len && (im->m >> k & 1)) k++;
            Node *r = nlist(k + 1); r->it[0] = h;
            for (int q = 0; q < a; q++) r->it[q + 1] = (im->m >> q & 1) ? sym2("%imp", t->it[q + 1]) : t->it[q + 1];
            Node *xs[64]; int nx = 0;
            for (int q = a; q < k && nx < 64; q++) { char b[16]; snprintf(b, sizeof b, "x%d", nx); xs[nx] = nsym(b); r->it[q + 1] = sym2("%imp", xs[nx]); nx++; }
            if (prec > 0) bputc(o, '(');
            bput(o, "λ"); for (int q = 0; q < nx; q++) { bputc(o, ' '); bput(o, xs[q]->s); }
            bput(o, " → "); pr_nodrop = true; pr(o, r, 0, cur_col(o));
            if (prec > 0) bputc(o, ')');
            return;
        }
    }
    /* fits on the line: print it flat */
    if (pr_width < (1 << 29)) {
        char *f = pr_flat(t, prec);
        if (!has_nl(f) && col + vlen(f) <= pr_width) { bput(o, f); free(f); return; }
        free(f);
    }
    bool flat = pr_width >= (1 << 29);
    const char *k = h->list ? "" : h->s;
#define OPEN(p) do { if (prec > (p)) { bputc(o, '('); col++; } } while (0)
#define CLOSE(p) do { if (prec > (p)) bputc(o, ')'); } while (0)
    if (!strcmp(k, "lam") || !strcmp(k, "plam")) {
        OPEN(0);
        bput(o, "λ "); Node *b = pr_binders(o, t); bput(o, " →");
        if (flat) { bputc(o, ' '); pr(o, b, 0, col); }
        else { char *f = pr_flat(b, 0); if (cur_col(o) + 1 + vlen(f) <= pr_width) { bputc(o, ' '); bput(o, f); } else { bnl(o, col + 2); pr(o, b, 0, col + 2); } free(f); }
        CLOSE(0); return;
    }
    if (!strcmp(k, "forall") || !strcmp(k, "=") || !strcmp(k, "&") || !strcmp(k, "|")) {
        if (face_has_eq(t)) { bputc(o, '('); pr_face(o, t, 0, col + 1); bputc(o, ')'); return; }
        /* a face of face variables */
        if (!strcmp(k, "&") || !strcmp(k, "|")) {
            Node *r = t->it[1];
            for (int i = 2; i < t->n; i++) r = lst(3, nsym(k), r, t->it[i]);
            OPEN(99); bput(o, k[0] == '&' ? "%fand " : "%for "); pr(o, r->it[1], 101, col); bputc(o, ' '); pr(o, r->it[2], 101, col); CLOSE(99);
            return;
        }
    }
    if (!strcmp(k, "let")) {
        OPEN(0);
        bput(o, "let ");
        Node *x = t; bool first = true;
        int ic = col + 4;
        while (x->list && x->n == 4 && is(x->it[0], "let")) {
            if (!first) { if (flat) bput(o, " in let "); else bnl(o, ic); }
            first = false;
            bput(o, x->it[1]->s); bput(o, " = ");
            pr(o, x->it[2], 0, cur_col(o));
            x = x->it[3];
            if (flat) break;
        }
        if (flat) { bput(o, " in "); pr(o, x, 0, col); }
        else { bnl(o, col); bput(o, "in "); pr(o, x, 0, col + 3); }
        CLOSE(0); return;
    }
    if (!strcmp(k, "if") && t->n == 4) {
        OPEN(0);
        bput(o, "if "); pr(o, t->it[1], 0, col + 3);
        if (flat) { bput(o, " then "); pr(o, t->it[2], 0, col); bput(o, " else "); pr(o, t->it[3], 0, col); }
        else { bnl(o, col + 2); bput(o, "then "); pr(o, t->it[2], 0, col + 7); bnl(o, col + 2); bput(o, "else "); pr(o, t->it[3], 0, col + 7); }
        CLOSE(0); return;
    }
    if (!strcmp(k, "case") || !strcmp(k, "elim")) {
        bool el = !strcmp(k, "elim");
        OPEN(0);
        bput(o, "case "); pr(o, t->it[el ? 2 : 1], 1, col + 5);
        if (el) { bput(o, " return "); pr(o, t->it[1], 1, cur_col(o)); }
        bput(o, " of");
        int from = el ? 3 : 2;
        if (t->n - from == 1 && !t->it[from]->list) { /* (case e D): D has no constructors */ bputc(o, ' '); bput(o, t->it[from]->s); CLOSE(0); return; }
        if (flat) { bputc(o, ' '); pr_alts(o, t->it + from, t->n - from, col, true); }
        else pr_alts(o, t->it + from, t->n - from, col + 2, false);
        CLOSE(0); return;
    }
    if (!strcmp(k, "record")) {
        bput(o, "record {");
        for (int i = 1; i < t->n; i++) { if (i > 1) bput(o, " ;"); bputc(o, ' '); bput(o, t->it[i]->it[0]->s); bput(o, " = "); pr(o, t->it[i]->it[1], 0, cur_col(o)); }
        bput(o, " }"); return;
    }
    if (!strcmp(k, "%imp") && t->n == 2) { bputc(o, '{'); pr(o, t->it[1], 0, col + 1); bputc(o, '}'); return; }
    if (!strcmp(k, "%impPi") && t->n == 2) {
        /* {x y : A} → B */
        Node *P = t->it[1];
        OPEN(0);
        bput(o, "{"); bput(o, P->it[1]->s);
        Node *B = P->it[3];
        while (B->list && B->n == 2 && is(B->it[0], "%impPi") && node_eq(B->it[1]->it[2], P->it[2]) && !mentions_x(P->it[2], B->it[1]->it[1]->s)) { bputc(o, ' '); bput(o, B->it[1]->it[1]->s); B = B->it[1]->it[3]; }
        bput(o, " : "); pr(o, P->it[2], 0, cur_col(o)); bput(o, "}");
        /* binder groups follow each other without arrows */
        bool more = (B->list && B->n == 2 && is(B->it[0], "%impPi"));
        if (more) { bputc(o, ' '); pr(o, B, 0, cur_col(o)); CLOSE(0); return; }
        bput(o, " →");
        char *f = pr_flat(B, 0);
        if (flat || cur_col(o) + 1 + vlen(f) <= pr_width) { bputc(o, ' '); pr(o, B, 0, cur_col(o)); } else { bnl(o, col); pr(o, B, 0, col); }
        free(f);
        CLOSE(0); return;
    }
    if (!strcmp(k, "PathP") && t->n == 4 && pr_typed && !pr_keepimp && global_lookup("Path") >= 0 && t->it[1]->list && t->it[1]->n == 3 && is(t->it[1]->it[0], "plam")
        && !mentions_x(t->it[1]->it[2], t->it[1]->it[1]->s) && !infer_looking(t->it[2]) && !infer_looking(t->it[3])) {
        /* neither end tells the type: Path A a b */
        Node *r = lst(4, nsym("Path"), t->it[1]->it[2], t->it[2], t->it[3]);
        pr_nodrop = true; pr(o, r, prec, col); return;
    }
    if (!strcmp(k, "PathP") && t->n == 4 && pr_typed && !pr_keepimp && global_lookup("_≡_") >= 0 && t->it[1]->list && t->it[1]->n == 3 && is(t->it[1]->it[0], "plam")) {
        /* a ≡ b, a ≡[ i ↦ A ] b */
        Node *L = t->it[1];
        OPEN(4);
        pr(o, t->it[2], 5, col);
        if (mentions_x(L->it[2], L->it[1]->s)) { bput(o, " ≡[ "); bput(o, L->it[1]->s); bput(o, " ↦ "); pr(o, L->it[2], 0, cur_col(o)); bput(o, " ] "); }
        else if (!infer_looking(t->it[2]) && !infer_looking(t->it[3])) {
            /* neither end tells the type: Path A a b */
            o->n -= strlen(o->b + (o->n - (size_t)0)) ; /* (nothing printed yet but the left end) */
            (void)0;
            bput(o, " ≡ ");
        }
        else bput(o, " ≡ ");
        pr(o, t->it[3], 5, cur_col(o));
        CLOSE(4); return;
    }
    if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma"))) {
        bool pi = k[0] == 'P';
        Node *x, *A, *B;
        if (t->n == 4) { x = t->it[1]; A = t->it[2]; B = t->it[3]; }
        else if (t->n == 3 && t->it[1]->list && t->it[1]->n >= 1 && t->it[1]->it[0]->list) {
            /* a telescope */
            Node *tel = t->it[1], *r = t->it[2];
            for (int q = tel->n - 1; q >= 0; q--) r = sym4(k, tel->it[q]->it[0], tel->it[q]->it[1], r);
            pr(o, r, prec, col); return;
        }
        else { x = t->it[1]->it[0]; A = t->it[1]->it[1]; B = t->it[2]; }
        if (pi && !x->list && A->list && A->n == 2 && is(A->it[0], "Tick") && !mentions_x(B, x->s)) {
            /* later: ▸ κ B */
            OPEN(30); bput(o, "▸ "); pr(o, A->it[1], 31, cur_col(o)); bputc(o, ' '); pr(o, B, 31, cur_col(o)); CLOSE(30); return;
        }
        int p = pi ? 0 : 2;
        OPEN(p);
        if (mentions_x(B, x->s) || is(A, "I")) {
            bputc(o, '('); bput(o, x->s);
            /* binders of one type, together */
            while (B->list && B->n == 4 && is(B->it[0], k) && node_eq(B->it[2], A) && !mentions_x(A, B->it[1]->s) && mentions_x(B->it[3], B->it[1]->s)) { bputc(o, ' '); bput(o, B->it[1]->s); B = B->it[3]; }
            bput(o, " : "); pr(o, A, 0, cur_col(o)); bputc(o, ')');
        } else pr(o, A, p + 1, col);
        bput(o, pi ? " →" : " ×");
        char *f = pr_flat(B, p);
        if (flat || cur_col(o) + 1 + vlen(f) <= pr_width) { bputc(o, ' '); pr(o, B, p, cur_col(o)); }
        else { bnl(o, col); pr(o, B, p, col); }
        free(f);
        CLOSE(p); return;
    }
    if (!strcmp(k, "pair") && t->n == 3) {
        bputc(o, '(');
        Node *x = t; bool first = true;
        while (x->list && x->n == 3 && is(x->it[0], "pair")) {
            if (!first) { char *f = pr_flat(x->it[1], 0); if (flat || cur_col(o) + 2 + vlen(f) <= pr_width) bput(o, " , "); else { bnl(o, col); bput(o, ", "); } free(f); }
            first = false;
            pr(o, x->it[1], 0, cur_col(o));
            x = x->it[2];
        }
        { char *f = pr_flat(x, 0); if (flat || cur_col(o) + 3 + vlen(f) <= pr_width) bput(o, " , "); else { bnl(o, col); bput(o, ", "); } free(f); }
        pr(o, x, 0, cur_col(o));
        bputc(o, ')'); return;
    }
    if (!strcmp(k, "sys")) {
        bputc(o, '[');
        for (int i = 1; i < t->n; i++) {
            if (i > 1) { if (flat) bput(o, " ,"); else { bnl(o, col); bputc(o, ','); } }
            bputc(o, ' ');
            pr_face(o, t->it[i]->it[0], 0, cur_col(o)); bput(o, " ↦ ");
            pr(o, t->it[i]->it[1], 0, cur_col(o));
        }
        bput(o, t->n > 1 ? " ]" : "]"); return;
    }
    if ((!strcmp(k, "/\\") || !strcmp(k, "\\/")) && t->n >= 3) {
        int p = k[0] == '/' ? 7 : 6;
        OPEN(p);
        for (int i = 1; i < t->n; i++) {
            if (i > 1) { bputc(o, ' '); bput(o, k); bputc(o, ' '); }
            Node *a = t->it[i];
            if (!a->list && (!strcmp(a->s, "0") || !strcmp(a->s, "1"))) pr_sym(o, a->s, true); else pr(o, a, i == 1 ? p : p + 1, cur_col(o));
        }
        CLOSE(p); return;
    }
    if (!strcmp(k, "~") && t->n == 2) {
        OPEN(100);
        bput(o, "~ "); Node *a = t->it[1];
        if (!a->list) pr_sym(o, a->s, true); else pr(o, a, 101, cur_col(o));
        CLOSE(100); return;
    }
    if (!strcmp(k, "@") && t->n == 3 && pr_typed) {
        /* p r: the elaborator sees an interval */
        Node *a = t->it[2];
        Node *r = lst(2, t->it[1], a->list ? a : nsym(!strcmp(a->s, "0") ? "i0" : !strcmp(a->s, "1") ? "i1" : a->s));
        pr_nodrop = true; pr(o, r, prec, col); return;
    }
    if (!strcmp(k, "@") && t->n == 3) {
        OPEN(30);
        pr(o, t->it[1], 30, col); bput(o, " @ ");
        Node *a = t->it[2];
        if (!a->list) pr_sym(o, a->s, true); else pr(o, a, 31, cur_col(o));
        CLOSE(30); return;
    }
    if (!strcmp(k, "@") && t->n == 5) { Node *r = nlist(5); r->it[0] = nsym("%@"); for (int i = 1; i < 5; i++) r->it[i] = t->it[i]; t = r; h = r->it[0]; k = "%@"; }
    if (!strcmp(k, "U") && t->n == 1) { bput(o, "Type"); return; }
    if (!strcmp(k, "U") && t->n == 2 && !t->it[1]->list && isdigit((unsigned char)t->it[1]->s[0])) {
        /* a level written as a number */
        OPEN(30); bput(o, "Type "); bput(o, t->it[1]->s); CLOSE(30); return;
    }
    if (!strcmp(k, "U") && t->n == 2) { Node *r = lst(2, nsym("Type"), t->it[1]); t = r; h = r->it[0]; }
    /* an application (a primitive's operands included); a definition's implicit arguments left out */
    bool nodrop_here = pr_nodrop; pr_nodrop = false;
    if (pr_typed && !nodrop_here && !pr_rawargs && !h->list && imask_of(h->s) && t->n > 1) {
        IMask *im = imask_of(h->s);
        Node *keep[256]; int nk = 0;
        /* when no explicit argument tells the types, the implicit ones are given, in braces */
        bool told = false;
        if (pr_keepimp) goto braces;
        for (int i = 1; i < t->n; i++) if (!(i - 1 < im->len && (im->m >> (i - 1) & 1)) && infer_looking(t->it[i])) told = true;
        braces:
        /* an implicit argument of a function type (a motive, a family) is given; those before it
         * left to be found, as {_} */
        int lastho = -1;
        if (told) {
            Node *sg = sig_of(h->s); int q = 0;
            for (Node *x = sg; x && q < t->n - 1; q++) {
                bool im2 = false;
                if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im2 = true; x = x->it[1]; }
                if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
                Node *A = x->it[2];
                while (A->list && A->n == 2 && is(A->it[0], "%impPi")) A = A->it[1];
                if (im2 && A->list && A->n == 4 && is(A->it[0], "Pi")) lastho = q;
                x = x->it[3];
            }
        }
        if (told) for (int i = 1; i < t->n; i++) if (i - 1 < im->len && (im->m >> (i - 1) & 1) && !determined(h->s, i - 1, t) && i - 1 > lastho) lastho = i - 1;
        for (int i = 1; i < t->n && nk < 255; i++) {
            bool imp = i - 1 < im->len && (im->m >> (i - 1) & 1);
            if (!imp) keep[nk++] = t->it[i];
            else if (!told) keep[nk++] = sym2("%imp", t->it[i]);
            else if (i - 1 == lastho || (i - 1 < lastho && is_hoarg(h->s, i - 1)) || !determined(h->s, i - 1, t)) { keep[nk++] = sym2("%imp", t->it[i]); }
            else if (i - 1 < lastho) keep[nk++] = sym2("%imp", nsym("_"));
        }
        bool wrapped = false; for (int i = 0; i < nk; i++) if (keep[i]->list && keep[i]->n == 2 && is(keep[i]->it[0], "%imp")) wrapped = true;
        if (nk == t->n - 1 && !wrapped) goto app;
        if (!nk) { bput(o, h->s); return; }
        Node *r = nlist(nk + 1); r->it[0] = h; for (int i = 0; i < nk; i++) r->it[i + 1] = keep[i];
        pr_nodrop = true; char *f = pr_flat(r, prec);
        bool fits = col + vlen(f) <= pr_width || pr_width >= (1 << 29);
        if (fits) { bput(o, f); free(f); pr_nodrop = false; return; }
        free(f);
        /* broken over lines: the operands printed with their own implicit arguments dropped */
        pr_nodrop = false;
        t = r; h = r->it[0];
        OPEN(100);
        bput(o, h->s);
        int c0 = col;
        for (int i = 1; i < t->n; i++) { char *fa = pr_flat(t->it[i], 101); if (cur_col(o) + 1 + vlen(fa) <= pr_width) { bputc(o, ' '); bput(o, fa); } else { bnl(o, c0 + 2); pr(o, t->it[i], 101, c0 + 2); } free(fa); }
        CLOSE(100); return;
    }
    app:
    if (!h->list && t->n == 3 && strlen(h->s) > 2 && h->s[0] == '_' && h->s[strlen(h->s) - 1] == '_'
        && !(t->it[1]->list && t->it[1]->n == 2 && is(t->it[1]->it[0], "%imp")) && !(t->it[2]->list && t->it[2]->n == 2 && is(t->it[2]->it[0], "%imp"))) {
        /* a declared operator applied to its two operands: infix */
        char op[64]; size_t l = strlen(h->s) - 2; if (l < sizeof op) { memcpy(op, h->s + 1, l); op[l] = 0; } else op[0] = 0;
        Fix *fx = NULL; for (int i = 0; i < nfix; i++) if (!strcmp(FIX[i].op, op)) fx = &FIX[i];
        if (fx) {
            int p = fx->prec;
            OPEN(p);
            pr(o, t->it[1], fx->assoc == 1 ? p : p + 1, col);
            bputc(o, ' '); bput(o, op); bputc(o, ' ');
            pr(o, t->it[2], fx->assoc == 2 ? p : p + 1, cur_col(o));
            CLOSE(p);
            return;
        }
    }
    OPEN(100);
    {
        int c0 = cur_col(o);
        if (h->list) pr(o, h, 100, c0); else bput(o, h->s);
        for (int i = 1; i < t->n; i++) {
            Node *a = t->it[i];
            char *f;
            bool iv = !h->list && ((!strcmp(h->s, "%@") && i == 2));
            if (!a->list && (!strcmp(a->s, "0") || !strcmp(a->s, "1"))) { Buf b = { 0 }; bput(&b, ""); pr_sym(&b, a->s, iv); f = b.b; }
            else f = pr_flat(a, 101);
            if (flat || (!has_nl(f) && cur_col(o) + 1 + vlen(f) <= pr_width)) { bputc(o, ' '); bput(o, f); }
            else { bnl(o, c0 + 2); if (!a->list) bput(o, f); else pr(o, a, 101, c0 + 2); }
            free(f);
        }
    }
    CLOSE(100);
#undef OPEN
#undef CLOSE
}
/* comment lines */
static void pr_cmt(Buf *o, const char *c) {
    while (c && *c) {
        const char *e = strchr(c, '\n'); size_t l = e ? (size_t)(e - c) : strlen(c);
        bput(o, l ? "-- " : "--"); char *ln = strndup(c, l); bput(o, ln); free(ln); bputc(o, '\n');
        c = e ? e + 1 : c + l;
    }
}

/* ---------------- converting a file of core forms ---------------- */
static Node *x_rename_all(Node *t, const char *a, const char *b) {
    if (!t->list) return strcmp(t->s, a) ? t : nsym(b);
    Node *r = nlist(t->n); r->cmt = t->cmt;
    for (int i = 0; i < t->n; i++) r->it[i] = x_rename_all(t->it[i], a, b);
    return r;
}
/* (def f (lam params... (case v alts))): as clauses when every alternative is a constructor with
 * variable fields (or _), v is a parameter not used in the alternatives, and no name is a constructor */
static bool clause_form(Node *body, Node **ps, int *np, int *vi, Node **cs) {
    *np = 0;
    Node *b = body;
    while (b->list && b->n >= 3 && is(b->it[0], "lam")) { for (int i = 1; i < b->n - 1; i++) { if (*np == 64) return false; ps[(*np)++] = param_name(b->it[i]); } b = b->it[b->n - 1]; }
    if (!*np || !b->list || b->n < 3 || !is(b->it[0], "case") || b->it[1]->list) return false;
    *cs = b; *vi = -1;
    for (int i = 0; i < *np; i++) {
        if (con_lookup(ps[i]->s) >= 0 || reserved(ps[i]->s)) return false;
        for (int j = 0; j < i; j++) if (!strcmp(ps[i]->s, ps[j]->s)) return false;
        if (!strcmp(ps[i]->s, b->it[1]->s)) *vi = i;
    }
    if (*vi < 0) return false;
    bool any = false;
    for (int i = 2; i < b->n; i++) {
        Node *a = b->it[i];
        if (!a->list || a->n != 2) return false;
        if (mentions_name(a->it[1], b->it[1]->s)) return false;
        Node *p = a->it[0];
        if (!p->list) { if (strcmp(p->s, "_")) return false; continue; }
        if (p->n < 1 || p->it[0]->list || con_lookup(p->it[0]->s) < 0) return false;
        any = true;
        for (int q = 1; q < p->n; q++) {
            if (p->it[q]->list || con_lookup(p->it[q]->s) >= 0 || reserved(p->it[q]->s) || !strcmp(p->it[q]->s, "_")) return false;
            for (int r = 0; r < *np; r++) if (r != *vi && !strcmp(ps[r]->s, p->it[q]->s)) return false;
            for (int r = 1; r < q; r++) if (!strcmp(p->it[r]->s, p->it[q]->s)) return false;
        }
    }
    return any;
}
static void conv_def(Buf *o, Node *f) {
    const char *name = f->it[1]->s; Node *body = f->it[2];
    Node *ps[64]; int np, vi; Node *cs;
    if (clause_form(body, ps, &np, &vi, &cs)) {
        /* the default alternative last */
        for (int pass = 0; pass < 2; pass++)
        for (int i = 2; i < cs->n; i++) {
            Node *a = cs->it[i];
            if ((pass == 0) != (a->it[0]->list != 0)) continue;
            if (a->cmt) { pr_cmt(o, a->cmt); a->cmt = NULL; }
            bput(o, name);
            for (int q = 0; q < np; q++) {
                bputc(o, ' ');
                if (q != vi) { bput(o, ps[q]->s); continue; }
                Node *p = a->it[0];
                if (!p->list) bput(o, "_");
                else if (p->n == 1) bput(o, p->it[0]->s);
                else { bputc(o, '('); for (int r = 0; r < p->n; r++) { if (r) bputc(o, ' '); bput(o, p->it[r]->s); } bputc(o, ')'); }
            }
            bput(o, " =");
            char *fl = pr_flat(a->it[1], 0);
            if (cur_col(o) + 1 + vlen(fl) <= pr_width && !has_nl(fl)) { bputc(o, ' '); bput(o, fl); }
            else { bnl(o, 3); pr(o, a->it[1], 0, 3); }
            free(fl);
            bputc(o, '\n');
        }
        return;
    }
    bput(o, name);
    Node *b = body;
    while (b->list && b->n >= 3 && is(b->it[0], "lam")) { for (int i = 1; i < b->n - 1; i++) { bputc(o, ' '); bput(o, param_name(b->it[i])->s); } b = b->it[b->n - 1]; }
    bput(o, " =");
    char *fl = pr_flat(b, 0);
    if (cur_col(o) + 1 + vlen(fl) <= pr_width && !has_nl(fl)) { bputc(o, ' '); bput(o, fl); }
    else { bnl(o, 3); pr(o, b, 0, 3); }
    free(fl);
    bputc(o, '\n');
}
static void conv_data(Buf *o, Node *f) {
    bput(o, "data ");
    Node *hd = f->it[1];
    if (!hd->list) bput(o, hd->s);
    else {
        bput(o, hd->it[0]->s);
        for (int i = 1; i < hd->n; i++) {
            Node *p = hd->it[i];
            if (p->list) { bput(o, " ("); bput(o, p->it[0]->s); bput(o, " : "); pr(o, p->it[1], 0, cur_col(o)); bputc(o, ')'); }
            else { bputc(o, ' '); bput(o, p->s); }
        }
    }
    int start = (int)o->n;
    for (int i = 2; i < f->n; i++) {
        Node *c = f->it[i];
        bput(o, i == 2 ? " = " : " | ");
        bput(o, c->it[0]->s);
        int k = 1;
        for (; k < c->n; k++) {
            Node *x = c->it[k];
            if (x->list && x->n >= 1 && is(x->it[0], "dims")) break;
            if (!x->list) { bputc(o, ' '); bput(o, x->s); }
            else { bput(o, " ("); bput(o, x->it[0]->s); bput(o, " : "); pr(o, x->it[1], 0, cur_col(o)); bputc(o, ')'); }
        }
        if (k < c->n) {
            Node *dl = c->it[k++];
            for (int q = 1; q < dl->n; q++) { bput(o, " ("); bput(o, dl->it[q]->s); bput(o, " : I)"); }
            Node *sy = nlist(c->n - k + 1); sy->it[0] = nsym("sys");
            for (int q = k; q < c->n; q++) sy->it[q - k + 1] = c->it[q];
            bputc(o, ' '); pr(o, sy, 101, cur_col(o));
        }
    }
    /* too long for a line: one constructor per line */
    if (vlen(o->b + start) + 20 > pr_width) {
        Buf b = { 0 }; bput(&b, "");
        const char *s = o->b + start;
        for (const char *p = s; *p; p++) {
            if ((p == s && !strncmp(p, " = ", 3)) || !strncmp(p, " | ", 3)) {
                /* a separator at the top level of the declaration */
                int d = 0; for (const char *q = s; q < p; q++) { if (*q == '(' || *q == '[') d++; else if (*q == ')' || *q == ']') d--; }
                if (!d) { bput(&b, "\n  "); bputc(&b, p[1]); bputc(&b, ' '); p += 2; continue; }
            }
            bputc(&b, *p);
        }
        o->n = (size_t)start; o->b[start] = 0; bput(o, b.b); free(b.b);
    }
    bputc(o, '\n');
}
/* a type's telescope in the core forms: (Pi ((x A)...) B), (Pi (x A) B), (Pi x A B) */
static int core_tel(Node *T, Node **xs, Node **As, Node **R) {
    int n = 0;
    for (;;) {
        if (T->list && T->n == 3 && is(T->it[0], "Pi") && T->it[1]->list && T->it[1]->n >= 1 && T->it[1]->it[0]->list) {
            for (int i = 0; i < T->it[1]->n && n < 255; i++) { xs[n] = T->it[1]->it[i]->it[0]; As[n++] = T->it[1]->it[i]->it[1]; }
            T = T->it[2]; continue;
        }
        if (T->list && T->n == 3 && is(T->it[0], "Pi") && T->it[1]->list && T->it[1]->n == 2) { xs[n] = T->it[1]->it[0]; As[n++] = T->it[1]->it[1]; T = T->it[2]; continue; }
        if (T->list && T->n == 4 && is(T->it[0], "Pi") && n < 255) { xs[n] = T->it[1]; As[n++] = T->it[2]; T = T->it[3]; continue; }
        break;
    }
    *R = T; return n;
}
static bool univ_like(Node *A) { return is(A, "U") || is(A, "Level") || (A->list && A->n >= 1 && is(A->it[0], "U")); }
/* a checked definition in the surface language: its signature (a parameter implicit when a later
 * parameter's type or the result names it, and it is a level, a type or of a type parameter's type),
 * its explicit parameters as the clause's patterns */
static void conv_checked(Buf *o, Node *f) {
    const char *name = f->it[1]->s; Node *T = f->it[2], *t = f->it[3];
    Node *xs[256], *As[256], *R; int np = core_tel(T, xs, As, &R);
    bool imp[256] = { 0 };
    for (int q = 0; q < np; q++) {
        bool later = false, inR = mentions_name(R, xs[q]->s);
        for (int r = q + 1; r < np; r++) if (mentions_name(As[r], xs[q]->s)) later = true;
        bool param_ty = false; if (!As[q]->list) for (int r = 0; r < q; r++) if (!strcmp(As[q]->s, xs[r]->s)) param_ty = true;
        imp[q] = later || (inR && (univ_like(As[q]) || param_ty));
    }
    uint64_t mask = 0; for (int q = 0; q < np && q < 64; q++) if (imp[q]) mask |= 1ull << q;
    if (mask) imask_set(name, mask, np);
    /* the body's λs for the parameters, renamed to the signature's names */
    Node *b = t; int k = 0;
    while (k < np && b->list && b->n >= 3 && is(b->it[0], "lam")) {
        Node *nb = b->n == 3 ? b->it[2] : NULL;
        if (b->n > 3) { nb = nlist(b->n - 1); nb->it[0] = b->it[0]; for (int i = 2; i < b->n; i++) nb->it[i - 1] = b->it[i]; }
        Node *x = param_name(b->it[1]);
        if (strcmp(x->s, xs[k]->s)) nb = x_rename(nb, x->s, xs[k]->s);
        b = nb; k++;
    }
    {
        Node *h = b; while (h->list && h->n > 0 && h->it[0]->list) h = h->it[0];
        if (h->list && h->n > 0 && !h->it[0]->list && (is(h->it[0], "elim") || is(h->it[0], "case") || is(h->it[0], "if")))
            { GROW(COMPUTING, capcomputing, (size_t)ncomputing + 1); COMPUTING[ncomputing++] = strdup(name); }
    }
    int nlhs = k;   /* the parameters the clause binds (the rest stay in its value's type) */
    {
        /* an implicit parameter past the body's λs: the body applied to it */
        int last = -1; for (int q = 0; q < np; q++) if (imp[q]) last = q;
        if (last >= k) { Node *a = nlist(last - k + 2); a->it[0] = b; for (int q = k; q <= last; q++) a->it[q - k + 1] = xs[q]; b = a; nlhs = last + 1; }
    }
    Node *ty = R;
    for (int q = np - 1; q >= 0; q--) {
        bool used = mentions_name(ty, xs[q]->s);
        ty = sym4("Pi", used || imp[q] ? xs[q] : xfresh("_"), As[q], ty);
        if (imp[q]) ty = sym2("%impPi", ty);
    }
    sig_set(name, ty);
    bput(o, name); bput(o, " : "); pr(o, ty, 0, vlen(name) + 4); bputc(o, '\n');
    /* a copattern body: its fields as clauses */
    Node *dims[16]; int nd = 0; Node *rb = b;
    while (rb->list && rb->n >= 3 && is(rb->it[0], "plam") && nd < 16) { for (int q = 1; q < rb->n - 1 && nd < 16; q++) dims[nd++] = rb->it[q]; rb = rb->it[rb->n - 1]; }
    if (rb->list && rb->n >= 2 && is(rb->it[0], "record")) {
        for (int i = 1; i < rb->n; i++) {
            Node *fe = rb->it[i];
            int ex = 0; for (int q = 0; q < np; q++) if (!imp[q]) ex++;
            if (ex + nd) { bputc(o, '('); bput(o, name); for (int q = 0; q < np; q++) if (!imp[q]) { bputc(o, ' '); bput(o, xs[q]->s); } for (int q = 0; q < nd; q++) { bputc(o, ' '); bput(o, dims[q]->s); } bput(o, ")."); }
            else { bput(o, name); bputc(o, '.'); }
            bput(o, fe->it[0]->s); bput(o, " =");
            char *fl = pr_flat(fe->it[1], 0);
            if (cur_col(o) + 1 + vlen(fl) <= pr_width && !has_nl(fl)) { bputc(o, ' '); bput(o, fl); } else { bnl(o, 3); pr(o, fe->it[1], 0, 3); }
            free(fl); bputc(o, '\n');
        }
        return;
    }
    bput(o, name);
    for (int q = 0; q < nlhs; q++) if (!imp[q]) { bputc(o, ' '); bput(o, xs[q]->s); }
    bput(o, " =");
    char *fl = pr_flat(b, 0);
    if (cur_col(o) + 1 + vlen(fl) <= pr_width && !has_nl(fl)) { bputc(o, ' '); bput(o, fl); } else { bnl(o, 3); pr(o, b, 0, 3); }
    free(fl); bputc(o, '\n');
}
/* a core form in the surface language */
static void conv_form(Buf *o, Node *f) {
    if (f->cmt) pr_cmt(o, f->cmt);
    const char *h = f->it[0]->s;
    if (!strcmp(h, "def")) { pr_keepimp = pr_rawargs = true; conv_def(o, f); pr_keepimp = pr_rawargs = false; }   /* not elaborated: as given */
    else if (!strcmp(h, "declare")) { /* a later name is declared where it is first used */ }
    else if (!strcmp(h, "data")) conv_data(o, f);
    else if (!strcmp(h, "axiom")) {
        bput(o, "postulate");
        bool typed = false; for (int i = 1; i < f->n; i++) if (f->it[i]->list) typed = true;
        if (!typed) { for (int i = 1; i < f->n; i++) { bputc(o, ' '); bput(o, f->it[i]->s); } bputc(o, '\n'); }
        else for (int i = 1; i < f->n; i++) {
            bput(o, "\n  ");
            if (!f->it[i]->list) { bput(o, f->it[i]->s); continue; }
            bput(o, f->it[i]->it[0]->s); bput(o, " : "); pr(o, f->it[i]->it[1], 0, cur_col(o));
        }
        if (typed) bputc(o, '\n');
    }
    else if (!strcmp(h, "checked")) conv_checked(o, f);
    else if (!strcmp(h, "checked-fail")) { bput(o, "#fail\n"); conv_checked(o, f); }
    else if (!strcmp(h, "checked-data") || !strcmp(h, "checked-data-fail")) { if (h[12]) bput(o, "#fail\n"); pr_keepimp = true; conv_data(o, f->it[1]); pr_keepimp = false; }
    else if (!strcmp(h, "checked-codata") || !strcmp(h, "checked-codata-fail") || !strcmp(h, "codata")) {
        Node *c = h[0] == 'c' && h[1] == 'o' ? f : f->it[1];
        if (strlen(h) > 14) bput(o, "#fail\n");
        bput(o, "codata ");
        Node *hd = c->it[1];
        if (!hd->list) bput(o, hd->s);
        else { bput(o, hd->it[0]->s); for (int i = 1; i < hd->n; i++) { Node *p = hd->it[i]; if (p->list) { bput(o, " ("); bput(o, p->it[0]->s); bput(o, " : "); pr(o, p->it[1], 0, cur_col(o)); bputc(o, ')'); } else { bputc(o, ' '); bput(o, p->s); } } }
        bput(o, " where");
        pr_keepimp = true;
        for (int i = 2; i < c->n; i++) { bput(o, "\n  "); bput(o, c->it[i]->it[0]->s); bput(o, " : "); pr(o, c->it[i]->it[1], 0, cur_col(o)); }
        pr_keepimp = false;
        bputc(o, '\n');
    }
    else if (!strcmp(h, "mutual") || !strcmp(h, "mutual-fail")) {
        if (h[6]) bput(o, "#fail\n");
        bput(o, "mutual\n");
        for (int i = 1; i < f->n; i++) {
            Buf b = { 0 }; bput(&b, ""); conv_checked(&b, f->it[i]);
            /* indented */
            bput(o, "  ");
            for (char *c = b.b; *c; c++) { bputc(o, *c); if (*c == '\n' && c[1]) bput(o, "  "); }
            free(b.b);
        }
    }
    else if (!strcmp(h, "import")) {
        char *q = strdup(f->it[1]->s[0] == '"' ? f->it[1]->s + 1 : f->it[1]->s);
        size_t l = strlen(q); if (l && q[l - 1] == '"') q[--l] = 0;
        if (l > 4 && !strcmp(q + l - 4, ".ctt")) strcpy(q + l - 4, ".cub");
        bput(o, "import \""); bput(o, q); bput(o, "\"\n");
    }
    else if (!strcmp(h, "show")) { pr_keepimp = true; bput(o, "#show "); pr(o, f->it[1], 0, 7); bputc(o, '\n'); pr_keepimp = false; }
    else if (!strcmp(h, "same")) { pr_keepimp = true; bput(o, "#same "); pr(o, f->it[1], 1, 7); bput(o, " = "); pr(o, f->it[2], 1, cur_col(o)); bputc(o, '\n'); pr_keepimp = false; }
    else if (!strcmp(h, "check") || !strcmp(h, "check-fail")) {
        if (h[5]) bput(o, "#fail\n");
        pr_keepimp = h[5] != 0;   /* a rejection: the arguments as given */
        bput(o, "#check "); pr(o, f->it[2], 1, 8); bput(o, " : "); pr(o, f->it[1], 0, cur_col(o)); bputc(o, '\n'); pr_keepimp = false;
    }
    else if (!strcmp(h, "check-data")) { bput(o, "#check-data "); bput(o, f->it[1]->s); bputc(o, '\n'); }
    else if (!strcmp(h, "input")) { bput(o, "#input"); for (int i = 1; i < f->n; i++) { bputc(o, ' '); bput(o, f->it[i]->s); } bputc(o, '\n'); }
    else if (!strcmp(h, "supply")) { bput(o, "#supply "); bput(o, f->it[1]->s); bput(o, " = "); pr(o, f->it[2], 0, cur_col(o)); bputc(o, '\n'); }
    else if (!strcmp(h, "observe")) { bput(o, "#observe "); bput(o, f->it[1]->s); bputc(o, ' '); pr(o, f->it[2], 0, cur_col(o)); bputc(o, '\n'); }
    else { bput(o, "-- (not converted) "); char *b; size_t l; FILE *m = open_memstream(&b, &l); print_node(m, f); fclose(m); bput(o, b); free(b); bputc(o, '\n'); }
    if (pr_lost.n) { Buf c = { 0 }; bput(&c, ""); pr_cmt(&c, pr_lost.b); /* comments from inside the form, after it */ bput(o, c.b); free(c.b); pr_lost.n = 0; pr_lost.b[0] = 0; }
}
/* alpha-equivalence of desugared terms */
typedef struct { const char *a, *b; } APair;
static bool alpha(Node *x, Node *y, APair *env, int n) {
    if (x->list != y->list) return false;
    if (!x->list) {
        for (int i = n - 1; i >= 0; i--) {
            bool ma = !strcmp(env[i].a, x->s), mb = !strcmp(env[i].b, y->s);
            if (ma || mb) return ma && mb;
        }
        return !strcmp(x->s, y->s);
    }
    if (x->n != y->n) return false;
    if (x->n >= 3 && !x->it[0]->list && !y->it[0]->list && !strcmp(x->it[0]->s, y->it[0]->s)) {
        const char *k = x->it[0]->s;
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && x->n == 3) {
            env[n] = (APair){ x->it[1]->s, y->it[1]->s };
            return alpha(x->it[2], y->it[2], env, n + 1);
        }
        if (!strcmp(k, "let") && x->n == 4) {
            if (!alpha(x->it[2], y->it[2], env, n)) return false;
            env[n] = (APair){ x->it[1]->s, y->it[1]->s };
            return alpha(x->it[3], y->it[3], env, n + 1);
        }
    }
    for (int i = 0; i < x->n; i++) if (!alpha(x->it[i], y->it[i], env, n)) return false;
    return true;
}
static Node **xcollect; static int nxcollect, capxcollect; static bool x_collecting;
static FILE *conv_out;   /* where --print writes (stdout itself silenced) */
/* a constructor or field declared later than a definition of its name: the name is the
 * constructor's or the field's (the definition's implicit arguments and signature no longer apply) */
static void shadow_decl(Node *f) {
    if (!f->list || f->n < 2 || f->it[0]->list) return;
    const char *h = f->it[0]->s;
    Node *d = f;
    if (!strcmp(h, "checked-data") || !strcmp(h, "checked-data-fail") || !strcmp(h, "checked-codata") || !strcmp(h, "checked-codata-fail")) d = f->it[1];
    if (!d->list || d->n < 2 || d->it[0]->list || (!is(d->it[0], "data") && !is(d->it[0], "codata"))) return;
    for (int i = 2; i < d->n; i++) {
        Node *c = d->it[i];
        Node *nm = c->list ? (c->n ? c->it[0] : NULL) : c;
        if (!nm || nm->list) continue;
        imask_set(nm->s, 0, 0); sig_set(nm->s, NULL);
    }
}
/* The cache of a module holds its elaborated core forms, in order: a cached (trusted) module is
 * elaborated again only for the surface's own bookkeeping, and each form it produces is replaced by
 * the recorded one, so it runs as the checked program did (its implicit arguments solved, its
 * guesses decided) without running the checker. xrec: where an unchecked module's forms are
 * recorded; xrep: a cached module's recorded forms. */
static FILE *xrec; static bool xrec_bad;
static Node **xrep; static int nxrep, ixrep; static bool xrep_bad;
static void x_rec_form(Node *f) {
    char *b; size_t l; FILE *m = open_memstream(&b, &l); print_node(m, f); fclose(m);
    const char *saved = src; src = b; Node *g = read_node(); bool more = g && (skip(), *src != 0); src = saved;
    if (!g || more || !node_eq(f, g)) xrec_bad = true;   /* a form that does not read back: the module is not cached */
    fputs(b, xrec); fputc('\n', xrec); free(b);
}
static bool x_same_decl(Node *a, Node *b) {
    if (a->list != b->list || !a->list || a->n < 1 || b->n < 1) return false;
    if (!is(a->it[0], b->it[0]->list ? "" : b->it[0]->s)) return false;
    if (a->n > 1 && b->n > 1 && !a->it[1]->list && !b->it[1]->list) return !strcmp(a->it[1]->s, b->it[1]->s);
    return true;
}
static void x_rec_extra(Node *f) { if (xrec) x_rec_form(f); }
static void x_top(Node *f) {
    if (x_collecting) { GROW(xcollect, capxcollect, (size_t)nxcollect + 1); xcollect[nxcollect++] = f; return; }
    if (xrec) x_rec_form(f);
    if (xrep) {
        if (ixrep < nxrep && x_same_decl(xrep[ixrep], f)) f = xrep[ixrep];
        else xrep_bad = true;
        ixrep++;
    }
    shadow_decl(f);
    { bool sv = x_surface; x_surface = true; toplevel(f); x_surface = sv; }
    /* the record's extra forms after this one (a checked definition's usages) */
    if (xrep) while (ixrep < nxrep && xrep[ixrep]->list && xrep[ixrep]->n >= 2 && is(xrep[ixrep]->it[0], "usages")) toplevel(xrep[ixrep++]);
}
/* print a file of core forms in the surface language; with check, each form's printing is parsed
 * back and compared with it (desugared, up to the names of bound variables) as the form is loaded */
static void convert_file(const char *path, bool check) {
    char *text; { FILE *f = fopen(path, "rb"); if (!f) die("cannot open", path); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); text = malloc((size_t)n + 1); text[fread(text, 1, (size_t)n, f)] = 0; fclose(f); }
    const char *saved = src; src = text; keep_cmt = true;
    Node *f; Node **fs = NULL; size_t cap = 0; int nf = 0;
    while ((f = read_node())) { GROW(fs, cap, (size_t)nf + 1); fs[nf++] = f; }
    keep_cmt = false; src = saved;
    bput(&pr_lost, "");
    /* a name that is a word of the surface language is renamed */
    for (int i = 0; i < nf; i++) fs[i] = x_rename_all(fs[i], "then", "and-then");
    bool saved_ut = x_untyped_file; x_untyped_file = global_lookup("chk-def") < 0;
    bool saved_pt = pr_typed; pr_typed = !x_untyped_file;
    int bad = 0;
    for (int i = 0; i < nf; i++) {
        Buf o = { 0 }; bput(&o, "");
        conv_form(&o, fs[i]);
        if (!check) { fputs(o.b, conv_out ? conv_out : stdout); fflush(conv_out ? conv_out : stdout); }
        else if (!is(fs[i]->it[0], "declare")) {
            /* parse it back */
            nxcollect = 0; x_collecting = true;
            load_surface(o.b, "<printed>", false);
            x_collecting = false;
            Node *g = NULL;
            for (int k = 0; k < nxcollect; k++) if (!is(xcollect[k]->it[0], "declare")) g = xcollect[k];
            bool same = false;
            if (getenv("X_TRACE")) fprintf(stderr, "form %d %s\n", i, fs[i]->n > 1 && !fs[i]->it[1]->list ? fs[i]->it[1]->s : "");
            if (g && is(fs[i]->it[0], "def") && is(g->it[0], "def")) {
                APair env[4096];
                same = !strcmp(fs[i]->it[1]->s, g->it[1]->s) && alpha(ds(fs[i]->it[2]), ds(g->it[2]), env, 0);
            } else if (g) same = node_eq(fs[i], g);
            if (!same && g && is(fs[i]->it[0], "def")) {
                /* the first subterm that differs */
                Node *a = ds(fs[i]->it[2]), *b = ds(g->it[2]);
                for (int depth = 0; depth < 200 && a->list && b->list && a->n == b->n; depth++) {
                    int k = -1; APair env[4096];
                    for (int q = 0; q < a->n; q++) if (!alpha(a->it[q], b->it[q], env, 0)) { k = q; break; }
                    if (k < 0) break;
                    a = a->it[k]; b = b->it[k];
                }
                printf("FIRST: "); print_node(stdout, a); printf("\n  VS: "); print_node(stdout, b); printf("\n");
            }
            if (!same) {
                bad++;
                printf("DIFFERS: "); print_node(stdout, fs[i]); printf("\n  printed: %s  parsed: ", o.b);
                if (g) print_node(stdout, g); printf("\n");
            }
        }
        free(o.b);
        if (check) toplevel(fs[i]);
        else {
            /* printing only: the forms the printing needs (constructors, fields, names) are loaded */
            const char *h = fs[i]->it[0]->s;
            if (!strcmp(h, "data") || !strcmp(h, "codata") || !strcmp(h, "checked-data") || !strcmp(h, "checked-codata") || !strcmp(h, "check-data") || !strcmp(h, "declare") || !strcmp(h, "axiom") || !strcmp(h, "import")) {
                fflush(stdout); int so = dup(1); FILE *nul = fopen("/dev/null", "w"); dup2(fileno(nul), 1);
                Node *ff = fs[i];
                if (!strcmp(h, "import")) {
                    /* the module's surface version, when it has one */
                    char *q = strdup(ff->it[1]->s); size_t l = strlen(q);
                    if (l > 5 && !strcmp(q + l - 5, ".ctt\"")) { strcpy(q + l - 5, ".cub\""); ff = sym2("import", nsym(q)); }
                }
                toplevel(ff);
                fflush(stdout); dup2(so, 1); close(so); fclose(nul);
            }
        }
    }
    if (check) printf("%s: %d forms, %d differ\n", path, nf, bad);
    x_untyped_file = saved_ut; pr_typed = saved_pt;
}

/* ---------------- codes read back as core terms ----------------
 * A code (lib/code.cub) as printed: de Bruijn variables over the binders entered, checked
 * definitions and data types by index, a free variable by its source name. NULL: no term (a
 * variable without a name, made by the checker inside the solution). */
static const char **cn_names; static int ncn, capcn; static int cn_fresh;
static Node *cn_push(const char *p) { char b[32]; snprintf(b, sizeof b, "%%%s%d", p, cn_fresh++); GROW(cn_names, capcn, (size_t)ncn + 1); cn_names[ncn++] = strdup(b); return nsym(cn_names[ncn - 1]); }
static long cn_word(Node *w) { return !w->list && w->s[0] == '#' ? atol(w->s + 1) : -1; }
static int cn_list(Node *l, Node **out, int max) {
    int n = 0;
    while (l->list && l->n == 3 && is(l->it[0], "lcons")) { if (n == max) return -1; out[n++] = l->it[1]; l = l->it[2]; }
    return is(l, "lnil") ? n : -1;
}
static Node *cn(Node *c);
static Node *cn_bound(const char *p, Node *body, Node **x) { *x = cn_push(p); Node *r = cn(body); ncn--; return r; }
static Node *cn(Node *c) {
    if (!c->list) {
        const char *s = c->s;
        static const struct { const char *c, *t; } K0[] = { { "ci0", "0" }, { "ci1", "1" }, { "cftt", "tt" }, { "cfff", "ff" }, { "clevel", "Level" },
            { "clzero", "lzero" }, { "cword", "Word" }, { "csys0", "%sys0" }, { "cclock", "Clock" }, { "cclock0", "κ0" }, { NULL, NULL } };
        for (int i = 0; K0[i].c; i++) if (!strcmp(s, K0[i].c)) return !strcmp(K0[i].t, "%sys0") ? lst(1, nsym("sys")) : nsym(K0[i].t);
        return NULL;
    }
    const char *h = c->it[0]->s;
    Node *x, *a, *b, *d;
#define R(e) do { Node *r_ = (e); if (!r_) return NULL; } while (0)
    if (!strcmp(h, "cvar")) { long k = cn_word(c->it[1]); if (k < 0 || k >= ncn) return NULL; const char *nm = cn_names[ncn - 1 - k]; return !strncmp(nm, "%rec ", 5) ? lst(2, nsym("rec"), nsym(nm + 5)) : nsym(nm); }
    if (!strcmp(h, "cforce") && c->it[1]->list && is(c->it[1]->it[0], "cvar")) return cn(c->it[1]);
    if (!strcmp(h, "ctick") && c->n == 2) { Node *k = cn(c->it[1]); return k ? lst(2, nsym("Tick"), k) : NULL; }
    if (!strcmp(h, "celim") || !strcmp(h, "ccase")) {
        /* an elimination: each branch binds its fields, a hypothesis per recursive field (written
         * (rec x)), its dimensions */
        bool el = h[1] == 'e';
        long di = cn_word(c->it[1]); if (di < 0 || di >= nchkdata) return NULL;
        int D = CHKDATA[di];
        Node *P = el ? cn(c->it[2]) : NULL; if (el && !P) return NULL;
        Node *x = cn(c->it[el ? 4 : 3]); if (!x) return NULL;
        Node *bs[64]; int nb = cn_list(c->it[el ? 3 : 2], bs, 64); if (nb != DATA[D].ncons) return NULL;
        Node *r = nlist(nb + (el ? 3 : 2)); int k = 0;
        r->it[k++] = nsym(el ? "elim" : "case"); if (el) r->it[k++] = P; r->it[k++] = x;
        for (int i = 0; i < nb; i++) {
            ConInfo *ci = &CON[DATA[D].cons[i]];
            int nf = ci->nfields, nr = ci->nrec, ndm = ci->ndims, save = ncn;
            Node *pat = nlist(nf + ndm + 1); pat->it[0] = nsym(ci->name);
            Node *b = bs[i]; const char *recf[64]; int nrf = 0;
            for (int j = 0; j < nf + nr + ndm; j++) {
                if (!b->list || b->n != 2 || !(is(b->it[0], "clam") || is(b->it[0], "cdlam"))) { ncn = save; return NULL; }
                if (j < nf) { Node *xn = cn_push("q"); pat->it[j + 1] = xn; if (ci->recmask >> j & 1) recf[nrf++] = xn->s; }
                else if (j < nf + nr) { char nm[64]; snprintf(nm, sizeof nm, "%%rec %s", j - nf < nrf ? recf[j - nf] : "?"); GROW(cn_names, capcn, (size_t)ncn + 1); cn_names[ncn++] = strdup(nm); }
                else pat->it[nf + (j - nf - nr) + 1] = cn_push("j");
                b = b->it[1];
            }
            Node *body = cn(b);
            ncn = save;
            if (!body) return NULL;
            r->it[k++] = lst(2, pat, body);
        }
        return r;
    }
    if (!strcmp(h, "cpty") && c->n == 3) {
        /* a primitive type: lib/prim.cub's name for it (Bytes at its size) where loaded */
        long k = cn_word(c->it[1]);
        static const char *TN[] = { "Int64", "UInt32", "Float64", "Float32", "Bytes", "UInt64" };
        if (k < 0 || k > 5) return NULL;
        if (k == 4) { Node *n = cn(c->it[2]); if (!n) return NULL; return global_lookup("Bytes") >= 0 ? lst(2, nsym("Bytes"), n) : lst(2, nsym("%bytesty"), n); }
        return global_lookup(TN[k]) >= 0 ? nsym(TN[k]) : lst(2, nsym("%pty"), wordn(k));
    }
    if (!strcmp(h, "cprim") && c->n == 3) {
        /* a primitive operation (the encoding of lang.c's to_code): the arithmetic and conversion
         * operations read back as lib/prim.cub's checked primitives of the same arguments; the
         * byte operations take proofs at the checked level and have no reading here */
        static const char *PN[] = { "i64add", "i64sub", "i64mul", "i64eq", "i64lt", "u32add", "u32sub", "u32mul", "u32eq", "u32lt", "f64add", "f64sub", "f64mul", "f64div", "f64eq", "f64lt",
            "u64add", "u64sub", "u64mul", "u64eq", "u64lt", "u64div", "u64mod", "u64and", "u64or", "u64xor", "u64shl", "u64shr",
            "i64div", "i64mod", "i64and", "i64or", "i64xor", "i64shl", "i64shr",
            "i64", "i64toWord", "u32", "u32toWord", "f64", "f64ofi64", "i64off64", NULL, NULL, NULL, NULL, "u64", "u64toWord" };
        long k = cn_word(c->it[1]); if (k < 0) return NULL;
        int cls = (int)(k >> 8), sb = (int)(k & 255); const char *name;
        if (cls <= 2 && sb < PO_N && PN[sb] && global_lookup(PN[sb]) >= 0) name = PN[sb]; else return NULL;
        Node *as[16]; int n = cn_list(c->it[2], as, 16); if (n < 0) return NULL;
        Node *r = nlist(n + 1); r->it[0] = nsym(name);
        for (int i = 0; i < n; i++) { R(r->it[i + 1] = cn(as[i])); }
        return r;
    }
    if (!strcmp(h, "cfree")) { Node *v = c->it[1]; return !v->list && !(v->s[0] == 'v' && isdigit((unsigned char)v->s[1])) && strcmp(v->s, "?") ? v : NULL; }
    if (!strcmp(h, "cglob")) { long k = cn_word(c->it[1]); return k >= 0 && k < nchecked && !CHKHIDE[k] ? nsym(CHECKED[k]) : NULL; }
    if (!strcmp(h, "clam") || !strcmp(h, "cdlam")) { b = cn_bound("q", c->it[1], &x); return b ? sym3("lam", x, b) : NULL; }
    if (!strcmp(h, "cplam")) { b = cn_bound("j", c->it[1], &x); return b ? sym3("plam", x, b) : NULL; }
    if (!strcmp(h, "cforall")) { b = cn_bound("j", c->it[1], &x); return b ? sym3("forall", x, b) : NULL; }
    if (!strcmp(h, "cpi")) { long m = cn_word(c->it[1]); a = cn(c->it[2]); if (!a) return NULL; b = cn_bound("q", c->it[3], &x); if (!b) return NULL; if (m == 0 || m == 1) bnd_set_mult(x, (int)m); return sym4("Pi", x, a, b); }
    if (!strcmp(h, "csigma")) { a = cn(c->it[1]); if (!a) return NULL; b = cn_bound("q", c->it[2], &x); return b ? sym4("Sigma", x, a, b) : NULL; }
    if (!strcmp(h, "clet")) { a = cn(c->it[1]); if (!a) return NULL; b = cn_bound("q", c->it[2], &x); return b ? sym4("let", x, a, b) : NULL; }
    if (!strcmp(h, "cuniv")) { a = cn(c->it[1]); if (!a) return NULL; return is(a, "lzero") ? nsym("U") : sym2("U", a); }
    if (!strcmp(h, "cdata")) {
        long di = cn_word(c->it[1]); if (di < 0 || di >= nchkdata) return NULL;
        Node *ps[64]; int n = cn_list(c->it[2], ps, 64); if (n < 0) return NULL;
        if (!n) return nsym(DATA[CHKDATA[di]].name);
        Node *r = nlist(n + 1); r->it[0] = nsym(DATA[CHKDATA[di]].name);
        for (int i = 0; i < n; i++) { R(r->it[i + 1] = cn(ps[i])); }
        return r;
    }
    if (!strcmp(h, "ccon")) {
        long di = cn_word(c->it[1]), k = cn_word(c->it[2]); if (di < 0 || di >= nchkdata || k < 0 || k >= DATA[CHKDATA[di]].ncons) return NULL;
        Node *fs[64], *ds_[16]; int n = cn_list(c->it[3], fs, 64), m = cn_list(c->it[4], ds_, 16); if (n < 0 || m < 0) return NULL;
        const char *cname = CON[DATA[CHKDATA[di]].cons[k]].name;
        if (!n && !m) return nsym(cname);
        Node *r = nlist(n + m + 1); r->it[0] = nsym(cname);
        for (int i = 0; i < n; i++) { R(r->it[i + 1] = cn(fs[i])); }
        for (int i = 0; i < m; i++) { R(r->it[n + i + 1] = cn(ds_[i])); }
        return r;
    }
    if (!strcmp(h, "cproj")) {
        long di = cn_word(c->it[1]), k = cn_word(c->it[2]); if (di < 0 || di >= nchkdata || k < 0 || k >= DATA[CHKDATA[di]].nflds) return NULL;
        a = cn(c->it[3]); return a ? lst(2, nsym(FLD[DATA[CHKDATA[di]].flds[k]].name), a) : NULL;
    }
    if (!strcmp(h, "ceq0") || !strcmp(h, "ceq1")) { a = cn(c->it[1]); return a ? sym3("=", a, nsym(h[3] == '0' ? "0" : "1")) : NULL; }
    /* words: literals, successor, the operations, the eliminator */
    if (!strcmp(h, "cwlit") && c->n == 2 && !c->it[1]->list) return c->it[1];
    if (!strcmp(h, "cwsucc") && c->n == 2) { a = cn(c->it[1]); return a ? lst(2, nsym("%wsucc"), a) : NULL; }
    if (!strcmp(h, "cwop") && c->n == 4) {
        static const char *WO[] = { "%weq", "%wlt", "%wadd", "%wmax", "%wsub", "%wmul", "%wdiv", "%wmod", "%wand", "%wor", "%wxor", "%wshl", "%wshr" };
        long k = cn_word(c->it[1]); if (k < 0 || k > 12) return NULL;
        a = cn(c->it[2]); b = cn(c->it[3]); return a && b ? lst(3, nsym(WO[k]), a, b) : NULL;
    }
    if (!strcmp(h, "cwelim") && c->n == 5) {
        /* the motive, when the code has one (some P); otherwise a hole of the elaborator's, solved
         * against the expected type */
        a = cn(c->it[1]); b = cn(c->it[2]); d = cn(c->it[3]); if (!(a && b && d)) return NULL;
        bool pend = false; Node *mot = NULL;
        if (c->it[4]->list && c->it[4]->n == 2 && is(c->it[4]->it[0], "some")) mot = cn(c->it[4]->it[1]);
        if (!mot) mot = hole_or_sol(c, 0, &pend);
        return lst(5, nsym("%welimP"), mot, a, b, d);
    }
    if (!strcmp(h, "csysc")) {
        /* a system: its branches in order */
        Node *bs[64]; int n = 0; Node *s = c;
        while (s->list && is(s->it[0], "csysc")) { if (n == 64) return NULL; a = cn(s->it[1]); b = cn(s->it[2]); if (!a || !b) return NULL; bs[n++] = lst(2, a, b); s = s->it[3]; }
        if (!is(s, "csys0")) return NULL;
        Node *r = nlist(n + 1); r->it[0] = nsym("sys");
        for (int i = 0; i < n; i++) r->it[i + 1] = bs[i];
        return r;
    }
    static const struct { const char *c, *t; int n; } F[] = {
        { "capp", "", 2 }, { "cpair", "pair", 2 }, { "cfst", "fst", 1 }, { "csnd", "snd", 1 }, { "cpathp", "PathP", 3 },
        { "cpapp", "@", 2 }, { "cneg", "~", 1 }, { "cmeet", "/\\", 2 }, { "cjoin", "\\/", 2 }, { "cfand", "&", 2 }, { "cfor", "|", 2 },
        { "ccomp", "comp", 3 }, { "cgluet", "Glue", 2 }, { "cglue", "glue", 2 }, { "cunglue", "unglue", 2 }, { "chcomp", "hcompD", 2 },
        { "cforce", "force", 1 }, { "cthunk", "delay", 1 }, { "clsuc", "lsuc", 1 }, { "clmax", "lmax", 2 }, { "cann", "the", 2 }, { NULL, NULL, 0 } };
    for (int q = 0; F[q].c; q++) if (!strcmp(h, F[q].c)) {
        if (c->n != F[q].n + 1) return NULL;
        Node *r = nlist(F[q].n + (F[q].t[0] ? 1 : 0)); int k = 0;
        if (F[q].t[0]) r->it[k++] = nsym(F[q].t);
        for (int i = 0; i < F[q].n; i++) { d = cn(c->it[i + 1]); if (!d) return NULL; r->it[k++] = d; }
        if (!strcmp(h, "cann")) { Node *t = r->it[1]; r->it[1] = r->it[2]; r->it[2] = t; }
        return r;
    }
#undef R
    return NULL;
}

/* ---------------- elaboration ----------------
 * A term of a checked definition becomes a core term in which each implicit argument is a hole
 * (%hole #k) inside the application holding it (%spine ...), and each λ or application whose kind
 * the syntax does not tell is a guess (%guess #k #g t). The checker finds the holes and tells the
 * wrong guesses; holes and guesses are keyed by where they stand in the source, so a decision holds
 * from one run to the next. */
typedef struct { Node *key; int idx; Node *sol; Node *syn; bool inst, tent; int tried; } EHole;   /* syn: found from the signatures (a surface term); inst: an instance hole, tent: its solution a candidate under trial, tried: candidates given up (E28) */
typedef struct { Node *key; int kind; } EGuess;
static EHole *EH; static int neh, capeh;
static EGuess *EG; static int neg, capeg;
static int e_hole(Node *key, int idx) {
    for (int i = 0; i < neh; i++) if (EH[i].key == key && EH[i].idx == idx) return i;
    GROW(EH, capeh, (size_t)neh + 1); EH[neh] = (EHole){ key, idx, NULL, NULL, false, false, 0 }; return neh++;
}
static int e_guess(Node *key, int deflt) {
    for (int i = 0; i < neg; i++) if (EG[i].key == key) return i;
    GROW(EG, capeg, (size_t)neg + 1); EG[neg] = (EGuess){ key, deflt }; return neg++;
}
static Node *wordn(long n) { char b[32]; snprintf(b, sizeof b, "#%ld", n); return nsym(b); }
/* the scope: local names, each a term or a dimension variable */
typedef struct { const char *n; bool dim; Node *ty; } Sc;
static Sc *esc; static int nesc, capesc;
static void e_push(const char *n, bool dim) { GROW(esc, capesc, (size_t)nesc + 1); esc[nesc++] = (Sc){ n, dim, NULL }; }
static Sc *e_find(const char *n) { for (int i = nesc - 1; i >= 0; i--) if (!strcmp(esc[i].n, n)) return &esc[i]; return NULL; }
static void e_pusht(const char *n, Node *ty) { GROW(esc, capesc, (size_t)nesc + 1); esc[nesc++] = (Sc){ n, false, ty }; }
/* the types of the clause compiler's λs, by node */
typedef struct { Node *lam; Node *ty; } LamTy;
static LamTy *LT; static int nlt_, caplt_;
static Node *lam_t(Node *x, Node *ty, Node *b) { Node *r = sym3("%lam", x, b); GROW(LT, caplt_, (size_t)nlt_ + 1); LT[nlt_++] = (LamTy){ r, ty }; return r; }
static Node *lamty_of(Node *l) { for (int i = nlt_ - 1; i >= 0; i--) if (LT[i].lam == l) return LT[i].ty; return NULL; }
/* each checked definition's signature (its implicit parameters marked) */
typedef struct { char *name; Node *sig; } SigE;
static SigE *SIGS; static int nsigs, capsigs;
static Node *sig_of(const char *n) { for (int i = nsigs - 1; i >= 0; i--) if (!strcmp(SIGS[i].name, n)) return SIGS[i].sig; return NULL; }
static void sig_set(const char *n, Node *s) { GROW(SIGS, capsigs, (size_t)nsigs + 1); SIGS[nsigs++] = (SigE){ strdup(n), s }; }
static Node *subst_sym(Node *t, const char *x, Node *e);
static Node *subst_multi(Node *t, const char **ps, Node **vs, int n);
/* the syntactic type of a term, when its head's signature tells it: a variable of known type, a
 * definition applied (its signature's parameters replaced by the arguments, its implicit ones
 * found the same way); NULL when not told */
static Node *syn_type(Node *a, int depth);
static bool e_head_kw(const char *s);
/* t with the names ps replaced by vs at once (a λ landing at an application's head reduced) */
static Node *subst_multi(Node *t, const char **ps, Node **vs, int n) {
    if (!t->list) { for (int i = 0; i < n; i++) if (vs[i] && !strcmp(ps[i], t->s)) return vs[i]; return t; }
    if (t->n == 0) return t;
    if (!t->it[0]->list && t->n >= 3 && (is(t->it[0], "lam") || is(t->it[0], "plam") || is(t->it[0], "forall") || is(t->it[0], "%lam") || is(t->it[0], "Pi") || is(t->it[0], "Sigma") || is(t->it[0], "let"))) {
        /* a binder shadows; one that would capture a substituted value's free name is renamed */
        const char *x = t->it[1]->s; const char *ps2[256]; Node *vs2[256]; int m = 0;
        for (int i = 0; i < n && m < 256; i++) if (strcmp(ps[i], x)) { ps2[m] = ps[i]; vs2[m++] = vs[i]; }
        bool capt = false;
        for (int i = 0; i < m && !capt; i++) if (vs2[i] && mentions_name(vs2[i], x)) {
            bool used = false;
            for (int k = 2; k < t->n && !used; k++) if (mentions_name(t->it[k], ps2[i])) used = true;
            if (used) capt = true;
        }
        if (capt) {
            Node *y = xfresh(x); Node *t2 = nlist(t->n); t2->it[0] = t->it[0]; t2->it[1] = y;
            bool pi0 = is(t->it[0], "Pi") || is(t->it[0], "Sigma") || is(t->it[0], "let");
            for (int k = 2; k < t->n; k++) t2->it[k] = (pi0 && k == 2) ? t->it[k] : x_rename(t->it[k], x, y->s);
            return subst_multi(t2, ps, vs, n);
        }
        Node *r = nlist(t->n); r->it[0] = t->it[0]; r->it[1] = t->it[1];
        bool pi = is(t->it[0], "Pi") || is(t->it[0], "Sigma") || is(t->it[0], "let");
        for (int i = 2; i < t->n; i++) r->it[i] = (pi && i == 2) ? subst_multi(t->it[i], ps, vs, n) : subst_multi(t->it[i], ps2, vs2, m);
        return r;
    }
    if (!t->it[0]->list && (is(t->it[0], "case") || is(t->it[0], "elim"))) {
        /* alternatives (pattern body): the pattern's variables bind in the body, and an
         * alternative is not an application */
        int first = is(t->it[0], "elim") ? 3 : 2;
        Node *r = nlist(t->n); r->it[0] = t->it[0];
        for (int i = 1; i < first && i < t->n; i++) r->it[i] = subst_multi(t->it[i], ps, vs, n);
        for (int i = first; i < t->n; i++) {
            Node *a = t->it[i];
            if (!a->list || a->n != 2) { r->it[i] = a; continue; }
            Node *pat = a->it[0];
            if (pat->list) for (int q = 1; q < pat->n; q++) if (!pat->it[q]->list && !pat_wild(pat->it[q])) {
                const char *x = pat->it[q]->s; bool capt = false;
                for (int z = 0; z < n && !capt; z++) if (vs[z] && strcmp(ps[z], x) && mentions_name(vs[z], x) && mentions_name(a->it[1], ps[z])) capt = true;
                if (capt) {
                    Node *y = xfresh(x); Node *p2 = nlist(pat->n); for (int w = 0; w < pat->n; w++) p2->it[w] = pat->it[w];
                    p2->it[q] = y; a = lst(2, p2, x_rename(a->it[1], x, y->s)); pat = p2;
                }
            }
            const char *ps2[256]; Node *vs2[256]; int m = 0;
            for (int z = 0; z < n && m < 256; z++) {
                bool bound = false;
                if (pat->list) { for (int q = 1; q < pat->n; q++) if (!pat->it[q]->list && !strcmp(pat->it[q]->s, ps[z])) bound = true; }
                else if (!strcmp(pat->s, ps[z])) bound = true;
                if (!bound) { ps2[m] = ps[z]; vs2[m++] = vs[z]; }
            }
            r->it[i] = lst(2, pat, subst_multi(a->it[1], ps2, vs2, m));
        }
        return r;
    }
    Node *r = nlist(t->n);
    for (int i = 0; i < t->n; i++) r->it[i] = subst_multi(t->it[i], ps, vs, n);
    /* ((f a) b) as (f a b) */
    Node *h = r->it[0];
    if (r->n >= 2 && h->list && h->n >= 2 && !h->it[0]->list && !e_head_kw(h->it[0]->s)) {
        Node *r2 = nlist(h->n + r->n - 1);
        for (int i = 0; i < h->n; i++) r2->it[i] = h->it[i];
        for (int i = 1; i < r->n; i++) r2->it[h->n + i - 1] = r->it[i];
        return r2;
    }
    /* (λ x → b) a */
    if (r->n >= 2 && h->list && h->n == 3 && !h->it[0]->list && (is(h->it[0], "lam") || is(h->it[0], "%lam"))) {
        const char *x = h->it[1]->s; Node *a = r->it[1];
        Node *b = subst_multi(h->it[2], &x, &a, 1);
        if (r->n == 2) return b;
        Node *r2 = nlist(r->n - 1); r2->it[0] = b; for (int i = 2; i < r->n; i++) r2->it[i - 1] = r->it[i];
        return subst_multi(r2, NULL, NULL, 0);   /* the next λ meets the next argument */
    }
    return r;
}
static bool syn_match(Node *P, Node *X, const char **ps, int np, Node **sol) {
    if (!X->list && !strcmp(X->s, "%?")) return true;   /* what is expected there is not known: anything */
    if (!P->list) {
        /* solved already: taken to agree (a different spelling of the same value; the checker sees) */
        for (int i = 0; i < np; i++) if (!strcmp(ps[i], P->s)) { if (!sol[i]) sol[i] = X; return true; }
        return !X->list && !strcmp(P->s, X->s);
    }
    if (!X->list || X->n != P->n) return false;
    if (P->n >= 3 && !P->it[0]->list && !X->it[0]->list && !strcmp(P->it[0]->s, X->it[0]->s) && !P->it[1]->list && !X->it[1]->list && strcmp(P->it[1]->s, X->it[1]->s)
        && (is(P->it[0], "Pi") || is(P->it[0], "Sigma") || is(P->it[0], "lam") || is(P->it[0], "plam") || is(P->it[0], "%lam"))) {
        /* binders named apart: the same up to renaming */
        const char *y = X->it[1]->s; Node *xv = P->it[1];
        Node *X2 = nlist(X->n); X2->it[0] = X->it[0]; X2->it[1] = xv;
        for (int i = 2; i < X->n; i++) X2->it[i] = (i == X->n - 1) ? subst_multi(X->it[i], &y, &xv, 1) : X->it[i];
        X = X2;
    }
    for (int i = 0; i < P->n; i++) if (!syn_match(P->it[i], X->it[i], ps, np, sol)) return false;
    return true;
}
/* a path type in one shape: a ≡ b (Path A a b, a non-dependent PathP, ≡ with its implicit arguments) */
static Node *syn_canon(Node *t) {
    if (!t->list || t->n == 0) return t;
    Node *h = t->it[0];
    if (!h->list) {
        if (is(h, "Path") && t->n == 4) return sym3("_≡_", syn_canon(t->it[2]), syn_canon(t->it[3]));
        if (is(h, "_≡_") && t->n >= 3) return sym3("_≡_", syn_canon(t->it[t->n - 2]), syn_canon(t->it[t->n - 1]));
        if (is(h, "PathP") && t->n == 4 && t->it[1]->list && t->it[1]->n == 3 && (is(t->it[1]->it[0], "lam") || is(t->it[1]->it[0], "plam") || is(t->it[1]->it[0], "%lam"))
            && !mentions_name(t->it[1]->it[2], t->it[1]->it[1]->s)) return sym3("_≡_", syn_canon(t->it[2]), syn_canon(t->it[3]));
        if ((is(h, "lam") || is(h, "plam") || is(h, "%lam")) && t->n == 3) return sym3(h->s, t->it[1], syn_canon(t->it[2]));
    }
    /* implicit arguments written out: left to the explicit ones */
    int m = 0; for (int i = 0; i < t->n; i++) if (!(i > 0 && t->it[i]->list && t->it[i]->n == 2 && is(t->it[i]->it[0], "%imp"))) m++;
    if (m == 1) return syn_canon(t->it[0]);
    Node *r = nlist(m); m = 0;
    for (int i = 0; i < t->n; i++) if (!(i > 0 && t->it[i]->list && t->it[i]->n == 2 && is(t->it[i]->it[0], "%imp"))) r->it[m++] = syn_canon(t->it[i]);
    return r;
}
static bool mentions_unknown(Node *t) { return mentions_name(t, "%?"); }
/* the elimination a case without a motive is elaborated as, by the case node and the type expected */
typedef struct { Node *t, *ex, *e; } CMot;
static CMot *CM; static int ncm; static size_t capcm;
static Node *cm_get(Node *t, Node *ex) {
    for (int i = ncm - 1; i >= 0; i--) if (CM[i].t == t && node_eq(CM[i].ex, ex)) return CM[i].e;
    return NULL;
}
static void cm_put(Node *t, Node *ex, Node *e) { GROW(CM, capcm, (size_t)ncm + 1); CM[ncm++] = (CMot){ t, ex, e }; }
/* the type expected of the application being elaborated, when its context tells it (unknowns as %?) */
static Node *app_expect, *syn_expect;
/* a λ where a function is due: its body's type, the binder at the domain (NULL when not told) */
static Node *syn_lam_type(Node *a, Node *ty, const char **ps, Node **sol, int q, int depth) {
    if (!(a->list && a->n == 3 && !a->it[0]->list && is(a->it[0], "lam") && !a->it[1]->list)) return NULL;
    Node *P = ty; while (P->list && P->n == 2 && is(P->it[0], "%impPi")) P = P->it[1];
    if (!(P->list && P->n == 4 && is(P->it[0], "Pi"))) return NULL;
    Node *A = subst_multi(P->it[2], ps, sol, q);
    bool open = false;
    for (int k = 0; k < q; k++) if (!sol[k] && mentions_name(A, ps[k])) open = true;
    if (open) {
        /* the domain is not yet known: the binder's written type gives it, and tells the parameters
         * in the domain */
        Node *W = bnd_ty(a->it[1]); if (!W) return NULL;
        Node *tmp[256]; for (int k = 0; k < q; k++) tmp[k] = sol[k];
        if (syn_match(A, W, ps, q, tmp) || (({ for (int k = 0; k < q; k++) tmp[k] = sol[k]; }), syn_match(syn_canon(A), syn_canon(W), ps, q, tmp)))
            for (int k = 0; k < q; k++) sol[k] = tmp[k];
        A = W;
    }
    int save = nesc; e_pusht(a->it[1]->s, A);
    Node *bt = syn_type(a->it[2], depth + 1);
    nesc = save;
    return bt ? sym4("Pi", a->it[1], A, bt) : NULL;
}
/* the implicit arguments of h applied to args, found from the arguments' syntactic types */
static int syn_solve(const char *h, Node **args, int n, Node **sol, int depth) {
    Node *sg = sig_of(h); if (!sg || depth > 8) return 0;
    const char *ps[256]; Node *ty[256]; bool imp[256], pinst[256]; Node *rest[257]; int np = 0;
    Node *t = sg;
    for (;;) {
        bool im = false;
        if (t->list && t->n == 2 && is(t->it[0], "%impPi")) { im = true; t = t->it[1]; }
        if (!(t->list && t->n == 4 && is(t->it[0], "Pi")) || np == 255) break;
        /* the parameters named apart from the arguments' variables */
        char nm[64]; snprintf(nm, sizeof nm, "%%s%d.%s", np, t->it[1]->s); Node *fx = nsym(strdup(nm));
        const char *old = t->it[1]->s;
        ps[np] = fx->s; ty[np] = t->it[2]; imp[np] = im; pinst[np] = im && bnd_inst(t->it[1]); rest[np] = t; np++;
        t = subst_multi(t->it[3], &old, &fx, 1);
    }
    rest[np] = t;
    for (int q = 0; q < np; q++) sol[q] = NULL;
    int i = 0;
    for (int q = 0; q < np && i < n; q++) {
        Node *a;
        if (imp[q]) {
            if (!(args[i]->list && args[i]->n == 2 && is(args[i]->it[0], "%imp"))) continue;
            if (bnd_inst(args[i]->it[0]) && !pinst[q]) continue;   /* {{e}} waits for an instance position */
            a = args[i++]->it[1];
            if (!a->list && !strcmp(a->s, "%hole")) continue;
            if (!a->list && !strcmp(a->s, "_")) continue;
        } else a = args[i++];
        sol[q] = a;
        Node *at = syn_type(a, depth + 1);
        if (!at) at = syn_lam_type(a, ty[q], ps, sol, q, depth);
        if (at) {
            Node *pt = ty[q];
            for (int k = 0; k < q; k++) if (sol[k] && sol[k]->list && sol[k]->n == 3 && is(sol[k]->it[0], "lam") && mentions_name(pt, ps[k])) {
                /* a function argument applied in the type: reduced */
                Node *only[256]; for (int r = 0; r < q; r++) only[r] = NULL; only[k] = sol[k];
                pt = subst_multi(pt, ps, only, q);
            }
            Node *tmp[256]; for (int k = 0; k < q; k++) tmp[k] = sol[k];
            if (syn_match(pt, at, ps, q, tmp)) for (int k = 0; k < q; k++) sol[k] = tmp[k];
            else {
                for (int k = 0; k < q; k++) tmp[k] = sol[k];
                if (syn_match(syn_canon(pt), syn_canon(at), ps, q, tmp)) for (int k = 0; k < q; k++) sol[k] = tmp[k];
            }
        }
    }
    if (getenv("CUB_DBGSYN") && !strcmp(getenv("CUB_DBGSYN"), h)) {
        fprintf(stderr, "syn_solve %s depth %d:", h, depth);
        for (int q = 0; q < np; q++) { fprintf(stderr, " [%s=", ps[q]); if (sol[q]) print_node(stderr, sol[q]); fprintf(stderr, "]"); }
        fprintf(stderr, "\n");
        for (int k = 0; k < n; k++) { Node *a = args[k]; if (a->list && a->n == 2 && is(a->it[0], "%imp")) a = a->it[1]; Node *at = syn_type(a, depth + 1); fprintf(stderr, "  arg %d : ", k); if (at) print_node(stderr, at); else fprintf(stderr, "?"); fprintf(stderr, "\n"); }
    }
    /* a solved implicit argument's own type tells the earlier ones */
    for (int round = 0; round < 2; round++)
        for (int q = np - 1; q > 0; q--) {
            if (!imp[q] || !sol[q]) continue;
            bool need = false; for (int k = 0; k < q; k++) if (!sol[k] && mentions_name(ty[q], ps[k])) need = true;
            if (!need) continue;
            Node *at = syn_type(sol[q], depth + 1); if (!at) continue;
            Node *tmp[256]; for (int k = 0; k < q; k++) tmp[k] = sol[k];
            if (syn_match(ty[q], at, ps, q, tmp) || (({ for (int k = 0; k < q; k++) tmp[k] = sol[k]; }), syn_match(syn_canon(ty[q]), syn_canon(at), ps, q, tmp)))
                for (int k = 0; k < q; k++) sol[k] = tmp[k];
        }
    for (int q = 0; q < np; q++)
        if (sol[q] && !sol[q]->list && isdigit((unsigned char)sol[q]->s[0]) && !ty[q]->list && is(ty[q], "Level")) {
            /* a level written as a number */
            Node *l = nsym("lzero"); for (long k = atol(sol[q]->s); k > 0; k--) l = lst(2, nsym("lsuc"), l);
            sol[q] = l;
        }
    /* a λ given where an earlier parameter is now known: its type */
    for (int q = 0; q < np; q++) {
        if (!sol[q] || !sol[q]->list || !is(sol[q]->it[0], "lam")) continue;
        bool need = false; for (int k = 0; k < q; k++) if (!sol[k] && mentions_name(ty[q], ps[k])) need = true;
        if (!need) continue;
        Node *at = syn_lam_type(sol[q], ty[q], ps, sol, q, depth); if (!at) continue;
        Node *tmp[256]; for (int k = 0; k < q; k++) tmp[k] = sol[k];
        if (syn_match(ty[q], at, ps, q, tmp)) for (int k = 0; k < q; k++) sol[k] = tmp[k];
    }
    if (depth == 0 && app_expect) {
        /* the result type against the type expected: the parameters left unknown */
        int q = 0, k = 0;
        for (; q < np; q++) {
            if (!imp[q]) { if (k >= n) break; k++; }
            else if (k < n && args[k]->list && args[k]->n == 2 && is(args[k]->it[0], "%imp")) k++;
        }
        Node *t = rest[q];
        while (t && t->list && t->n == 2 && is(t->it[0], "%impPi")) t = t->it[1];
        if (t && k == n) {
            Node *tmp[256]; for (int r = 0; r < q; r++) tmp[r] = NULL;
            /* what is known put in first (a function argument applied, reduced) */
            Node *t2 = subst_multi(t, ps, sol, q);
            if (syn_match(syn_canon(t2), syn_canon(app_expect), ps, q, tmp))
                for (int r = 0; r < q; r++) if (!sol[r] && tmp[r] && imp[r] && !mentions_unknown(tmp[r])) sol[r] = tmp[r];
        }
    }
    return np;
}
static Node *syn_type(Node *a, int depth) {
    if (depth > 8) return NULL;
    if (a->list && a->n == 2 && is(a->it[0], "%num") && data_lookup("Nat") >= 0 && !e_find("Nat")) return nsym("Nat");   /* a numeral (where no interval stands) is a natural number */
    if (a->list && a->n == 2 && is(a->it[0], "rec") && !a->it[1]->list) { char nm[256]; snprintf(nm, sizeof nm, "%%rec %s", a->it[1]->s); Sc *s = e_find(nm); return s ? s->ty : NULL; }
    if (!a->list && !e_find(a->s) && con_lookup(a->s) >= 0) {
        int c = con_lookup(a->s), d = CON[c].data;
        return CON[c].nfields == 0 && DATA[d].nparams == 0 ? nsym(DATA[d].name) : NULL;
    }
    if (!a->list) { Sc *s = e_find(a->s); if (s) return s->ty; Node *sg = sig_of(a->s); if (sg && !(sg->list && (is(sg->it[0], "Pi") || is(sg->it[0], "%impPi")))) return sg; return NULL; }
    if (a->n >= 2 && (a->it[0]->list || e_find(a->it[0]->s)) && !(a->it[0]->list && a->it[0]->n >= 1 && !a->it[0]->it[0]->list && e_head_kw(a->it[0]->it[0]->s) && !is(a->it[0]->it[0], "rec"))) {
        /* a variable (or hypothesis) of a known Π type, applied: its codomain at the arguments */
        Node *T = syn_type(a->it[0], depth + 1);
        for (int i = 1; T && i < a->n; i++) {
            while (T->list && T->n == 2 && is(T->it[0], "%impPi")) T = T->it[1];
            if (!(T->list && T->n == 4 && is(T->it[0], "Pi"))) return NULL;
            const char *x = T->it[1]->s; Node *v = a->it[i];
            T = subst_multi(T->it[3], &x, &v, 1);
        }
        return T;
    }
    if (a->n == 2 && !a->it[0]->list && (is(a->it[0], "fst") || is(a->it[0], "snd"))) {
        /* a projection of a pair of known Σ type */
        Node *T = syn_type(a->it[1], depth + 1);
        if (!T || !(T->list && T->n == 4 && is(T->it[0], "Sigma"))) return NULL;
        if (is(a->it[0], "fst")) return T->it[2];
        const char *x = T->it[1]->s; Node *v = sym2("fst", a->it[1]);
        return subst_multi(T->it[3], &x, &v, 1);
    }
    if (a->n == 4 && is(a->it[0], "if")) { Node *T = syn_type(a->it[2], depth + 1); return T ? T : syn_type(a->it[3], depth + 1); }
    if (a->n >= 2 && !a->it[0]->list && !e_find(a->it[0]->s) && con_lookup(a->it[0]->s) >= 0) {
        /* a constructor applied: its data type at the parameters its fields' types give */
        int c = con_lookup(a->it[0]->s), d = CON[c].data; Node *dd = DATA[d].decl;
        if (!dd || CON[c].nfields != a->n - 1) return NULL;
        int np = DATA[d].nparams; const char *ps[64]; Node *sol[64];
        Node *hd = dd->it[1];
        for (int q = 0; q < np && q < 64; q++) { ps[q] = param_name(hd->it[q + 1])->s; sol[q] = NULL; }
        Node *cd = dd->it[2 + CON[c].index];
        for (int q = 0; q < CON[c].nfields; q++) {
            Node *f = cd->it[q + 1]; if (!f->list || f->n != 2) return NULL;
            Node *at = syn_type(a->it[q + 1], depth + 1);
            if (at) syn_match(f->it[1], at, ps, np, sol);
        }
        if (!np) return nsym(DATA[d].name);
        Node *r = nlist(np + 1); r->it[0] = nsym(DATA[d].name);
        for (int q = 0; q < np; q++) { if (!sol[q]) return NULL; r->it[q + 1] = sol[q]; }
        return r;
    }
    if (a->n < 2 || a->it[0]->list || e_find(a->it[0]->s)) return NULL;
    const char *h = a->it[0]->s;
    Node *sg = sig_of(h); if (!sg) return NULL;
    Node *sol[256]; int np = syn_solve(h, a->it + 1, a->n - 1, sol, depth);
    /* the result type after the parameters the arguments cover */
    Node *t = sg; int q = 0, i = 0, n = a->n - 1;
    const char *ps[256];
    while (q < np) {
        bool im = false;
        if (t->list && t->n == 2 && is(t->it[0], "%impPi")) { im = true; t = t->it[1]; }
        if (!im) { if (i >= n) break; i++; } else if (i < n && a->it[i + 1]->list && a->it[i + 1]->n == 2 && is(a->it[i + 1]->it[0], "%imp")) i++;
        ps[q] = t->it[1]->s; t = t->it[3]; q++;
    }
    if (i < n) return NULL;
    for (int k = q - 1; k >= 0; k--) if (!sol[k] && mentions_name(t, ps[k])) return NULL;
    t = subst_multi(t, ps, sol, q);
    while (t->list && t->n == 2 && is(t->it[0], "%impPi")) return NULL;
    return t;
}
static bool e_head_kw(const char *s) {
    return keyword(s) || prim_ix(s) >= 0 || !strcmp(s, "%impPi") || !strcmp(s, "%num") || !strcmp(s, "%imp") || !strcmp(s, "%lam") || !strcmp(s, "sys");
}
/* does x stand where an interval does, in a surface term? */
static bool dim_use(Node *t, const char *x) {
    if (!t->list || t->n == 0) return false;
    Node *h = t->it[0];
    if (!h->list) {
        const char *k = h->s;
        if (((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "%lam") || !strcmp(k, "forall")) && t->n == 3 && !strcmp(t->it[1]->s, x))) return false;
        bool iv_all = !strcmp(k, "~") || !strcmp(k, "/\\") || !strcmp(k, "\\/");
        for (int i = 1; i < t->n; i++) {
            Node *a = t->it[i];
            bool iv = iv_all || (!strcmp(k, "=") && i == 1) || (!strcmp(k, "@") && i == 2);
            int c = con_lookup(k); if (c >= 0 && !e_find(k) && i - 1 >= CON[c].nfields) iv = true;
            if (iv && !a->list && !strcmp(a->s, x)) return true;
            if (dim_use(a, x)) return true;
        }
        return false;
    }
    for (int i = 0; i < t->n; i++) if (dim_use(t->it[i], x)) return true;
    return false;
}
static bool iv_certain(Node *a) {
    if (!a->list) { if (!strcmp(a->s, "0") || !strcmp(a->s, "1")) return true; Sc *s = e_find(a->s); return s && s->dim; }
    return a->n >= 1 && !a->it[0]->list && (is(a->it[0], "~") || is(a->it[0], "/\\") || is(a->it[0], "\\/"));
}
static bool lit01(Node *a) { return a->list && a->n == 2 && is(a->it[0], "%num") && (is(a->it[1], "0") || is(a->it[1], "1")); }
static Node *el(Node *t);
/* a syntactic solution, the same node every run (its own holes keyed by it) */
static Node *syn_once(Node *key, int idx, Node *sol) { int k = e_hole(key, idx); if (!EH[k].syn) EH[k].syn = sol; return EH[k].syn; }
/* a solution with the holes of its own (a quoted word recursion's motive) filled by theirs once found */
static Node *sol_fill(Node *t) {
    if (!t->list || t->n == 0) return t;
    if (t->n == 2 && !t->it[0]->list && is(t->it[0], "%hole") && !t->it[1]->list && t->it[1]->s[0] == '#') {
        long q = atol(t->it[1]->s + 1);
        return q >= 0 && q < neh && EH[q].sol ? sol_fill(EH[q].sol) : t;
    }
    Node *r = NULL;
    for (int i = 0; i < t->n; i++) {
        Node *c = sol_fill(t->it[i]);
        if (c != t->it[i] && !r) { r = nlist(t->n); for (int j = 0; j < i; j++) r->it[j] = t->it[j]; }
        if (r) r->it[i] = c;
    }
    return r ? r : t;
}
static Node *hole_or_sol(Node *key, int idx, bool *pend) {
    int k = e_hole(key, idx);
    if (EH[k].sol) return sol_fill(EH[k].sol);
    *pend = true;
    return sym2("%hole", wordn(k));
}
/* ---------------- instance search (E28) ----------------
 * An instance hole's type, as far as the signature and the solutions so far tell it (unknown parts
 * %? or holes), is matched against each instance's result type, the instance's own binders
 * matching anything; the latest match is the candidate, elaborated in the hole's place (its own
 * implicit and instance arguments then holes of their own). A candidate the checker rejects is
 * given up for the next; none left, the hole stays and the checker reports it. */
static bool inst_unknown(Node *t) { if (!t->list) return !strcmp(t->s, "%?") || !strcmp(t->s, "%hole") || !strcmp(t->s, "_"); return t->n >= 1 && !t->it[0]->list && (is(t->it[0], "%hole") || is(t->it[0], "%?")); }
static bool inst_has_unknown(Node *t) { if (inst_unknown(t)) return true; if (!t->list) return false; for (int i = 0; i < t->n; i++) if (inst_has_unknown(t->it[i])) return true; return false; }
/* an application as one list, (f a b), however it was nested */
static Node *inst_flat(Node *t) {
    if (!t->list || t->n == 0) return t;
    if (t->it[0]->list && t->it[0]->n > 0 && !t->it[0]->it[0]->list && !e_head_kw(t->it[0]->it[0]->s)) {
        Node *h = inst_flat(t->it[0]);
        Node *r = nlist(h->n + t->n - 1);
        for (int i = 0; i < h->n; i++) r->it[i] = h->it[i];
        for (int i = 1; i < t->n; i++) r->it[h->n + i - 1] = inst_flat(t->it[i]);
        return r;
    }
    Node *r = nlist(t->n); for (int i = 0; i < t->n; i++) r->it[i] = inst_flat(t->it[i]);
    return r;
}
/* pat against t; a pattern variable (one of pv) binds to what it meets (bv; the first binding
 * stands, the next must agree syntactically or be unknown) */
static bool inst_match(Node *pat, Node *t, const char **pv, Node **bv, int npv) {
    if (inst_unknown(t)) return true;
    if (!pat->list) {
        for (int i = 0; i < npv; i++) if (!strcmp(pv[i], pat->s)) { if (bv) { if (!bv[i]) bv[i] = t; else if (!node_eq(bv[i], t)) return false; } return true; }
        return !t->list && !strcmp(pat->s, t->s);
    }
    if (!t->list) return false;
    if (pat->n != t->n) return false;
    for (int i = 0; i < pat->n; i++) if (!inst_match(pat->it[i], t->it[i], pv, bv, npv)) return false;
    return true;
}
/* a candidate for want: a variable in scope of that type (a local instance), else the skip-th (from
 * the latest) instance whose result type matches; NULL when there is none */
static Node *inst_pick(Node *want, int skip) {
    if (!want) return NULL;
    want = inst_flat(want);
    if (skip == 0) for (int i = nesc - 1; i >= 0; i--) if (esc[i].ty && !esc[i].dim && inst_match(inst_flat(esc[i].ty), want, NULL, NULL, 0) && inst_match(want, inst_flat(esc[i].ty), NULL, NULL, 0)) return nsym(esc[i].n);
    for (int i = ninst - 1; i >= 0; i--) {
        /* the instance's binders: implicit ones are pattern variables, given to the candidate as
         * the match binds them ({A}); its own instance arguments are left to be found */
        const char *pv[64]; Node *bv[64] = { 0 }; bool pinst[64]; int npv = 0; bool explicit_ = false;
        Node *x = INST[i].sig;
        for (;;) {
            bool im = false;
            if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im = true; x = x->it[1]; }
            if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
            if (!im) explicit_ = true;
            if (npv < 64) { pv[npv] = x->it[1]->s; pinst[npv] = bnd_inst(x->it[1]); npv++; }
            x = x->it[3];
        }
        if (explicit_ || !inst_match(inst_flat(x), want, pv, bv, npv)) continue;
        if (skip > 0) { skip--; continue; }
        Node *c = nsym(INST[i].name); int na = 0;
        for (int q = 0; q < npv; q++) if (!pinst[q] && bv[q]) na = q + 1;   /* up to the last bound one */
        if (!na) return c;
        Node *r = nlist(na + 1); r->it[0] = c;
        for (int q = 0; q < na; q++) r->it[q + 1] = sym2("%imp", !pinst[q] && bv[q] ? bv[q] : nsym("%hole"));
        return r;
    }
    return NULL;
}
/* an instance hole at position pos of an application t: solved by a candidate when its type is
 * known enough (the head constant and its arguments' heads) */
static Node *inst_hole(Node *t, int pos, Node *want, bool *pend) {
    int k = e_hole(t, pos);
    EH[k].inst = true;
    if (!EH[k].sol && want && !inst_has_unknown(want)) {
        Node *c = inst_pick(want, EH[k].tried);
        if (c) { EH[k].sol = el(c); EH[k].tent = true; }
    }
    return hole_or_sol(t, pos, pend);
}
/* an application: the head's implicit arguments inserted (eagerly, up to the next explicit one),
 * each operand a function's or a path's by what it is */
static const char *tfname;
/* ---------------- later positions: tick insertion ----------------
 * A term given where ▸ κ X is expected, and not itself later, is a λ over a fresh tick α: inside
 * it, a later subterm standing where a present value is due is applied to α, and a guarded
 * recursive call (%gcall) is a call of the fixed point's later self at α. The checker's
 * timelessness rule guarantees what is inserted: α is applied only to what was bound before it */
static bool is_later(Node *T) {
    while (T && T->list && T->n == 2 && is(T->it[0], "%impPi")) T = T->it[1];
    return T && T->list && T->n == 4 && !T->it[0]->list && is(T->it[0], "Pi") && T->it[2]->list && T->it[2]->n == 2 && !T->it[2]->it[0]->list && is(T->it[2]->it[0], "Tick");
}
static bool has_gcall(Node *e) {
    if (!e->list) return false;
    if (e->n && !e->it[0]->list && is(e->it[0], "%gcall")) return true;
    for (int i = 0; i < e->n; i++) if (has_gcall(e->it[i])) return true;
    return false;
}
/* the parameters of an application's head: their types (NULL unknown) and which are implicit */
static int head_params(Node *h, Node **ty, bool *imp, int cap) {
    int n = 0;
    if (!h->list && !e_find(h->s) && con_lookup(h->s) >= 0) {
        int c = con_lookup(h->s); Node *dd = DATA[CON[c].data].decl;
        if (!dd) return 0;
        Node *cd = dd->it[2 + CON[c].index];
        for (int q = 0; q < CON[c].nfields && n < cap; q++) { Node *f = cd->it[q + 1]; ty[n] = f->list && f->n == 2 ? f->it[1] : NULL; imp[n++] = false; }
        return n;
    }
    Node *T = !h->list && !e_find(h->s) ? sig_of(h->s) : syn_type(h, 1);
    for (Node *x = T; x && n < cap; ) {
        bool im = false;
        if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im = true; x = x->it[1]; }
        if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
        ty[n] = x->it[2]; imp[n++] = im; x = x->it[3];
    }
    return n;
}
/* the ticks of the inserted λs being elaborated, outermost first: a guarded recursive call takes
 * the outermost (the fixed point's self is then a stream from that step on, usable at every later
 * one, and its later parts can be given the inner ticks) */
static const char *wtick[64]; static int nwtick;
static bool guard_on; static int guard_q = -1;
static bool tick_pos;   /* the term about to be elaborated stands at a later position */
static int fn_later_depth = -1;   /* a λ being elaborated whose body, this many λs down, stands at a later position (18.18) */   /* a guarded fixed point being compiled; its clock parameter */
static Node *tick_lift(Node *e, Node *al) {
    if (!e->list || e->n == 0) return e;
    Node *h = e->it[0];
    if (!h->list && is(h, "%gcall")) {
        /* the fixed point's self, one tick later, at the outermost tick */
        Node *self = lst(2, nsym("%self"), nwtick ? nsym(wtick[0]) : al);
        if (e->n == 1) return self;
        Node *r = nlist(e->n); r->it[0] = self;
        for (int i = 1; i < e->n; i++) r->it[i] = e->it[i];
        e = r; h = r->it[0];
    }
    if (!h->list && e_head_kw(h->s) && !is(h, "%imp")) {
        const char *k = h->s;
        Node *r = nlist(e->n); r->it[0] = h;
        if ((!strcmp(k, "lam") || !strcmp(k, "%lam") || !strcmp(k, "plam")) && e->n == 3) { r->it[1] = e->it[1]; r->it[2] = tick_lift(e->it[2], al); return r; }
        if (!strcmp(k, "case") || !strcmp(k, "elim")) {
            int from = !strcmp(k, "elim") ? 3 : 2;
            for (int i = 1; i < from; i++) r->it[i] = i == from - 1 ? tick_lift(e->it[i], al) : e->it[i];
            Node *sc = r->it[from - 1]; Node *st = syn_type(sc, 1);
            if (is_later(st)) r->it[from - 1] = lst(2, sc, al);
            for (int i = from; i < e->n; i++) { Node *a = e->it[i]; r->it[i] = a->list && a->n == 2 ? lst(2, a->it[0], tick_lift(a->it[1], al)) : a; }
            return r;
        }
        for (int i = 1; i < e->n; i++) r->it[i] = tick_lift(e->it[i], al);
        return r;
    }
    /* an application: each argument at its parameter */
    Node *ty[256]; bool imp[256]; int np = head_params(h, ty, imp, 256);
    Node *r = nlist(e->n); r->it[0] = h->list ? tick_lift(h, al) : h;
    int q = 0;
    for (int i = 1; i < e->n; i++) {
        Node *a = e->it[i];
        if (a->list && a->n == 2 && !a->it[0]->list && is(a->it[0], "%imp")) {
            while (q < np && !imp[q]) q++;
            r->it[i] = sym2("%imp", tick_lift(a->it[1], al)); q++; continue;
        }
        while (q < np && imp[q]) q++;
        Node *P = q < np ? ty[q] : NULL; q++;
        if (is_later(P)) { r->it[i] = a; continue; }   /* a later position: its own tick, when it is wrapped */
        Node *a2 = tick_lift(a, al);
        if (!is_later(P) && is_later(syn_type(a2, 1))) a2 = lst(2, a2, al);
        r->it[i] = a2;
    }
    return r;
}
/* the type an application has after its arguments, as its head's signature gives it (its
 * implicit arguments left as they are): enough to tell a later type from a present one */
static Node *result_shape(Node *a) {
    Node *h = a->list && a->n ? a->it[0] : a;
    int na = a->list ? a->n - 1 : 0;
    Node *T = NULL;
    if (!h->list && e_find(h->s)) T = e_find(h->s)->ty;
    else if (!h->list && con_lookup(h->s) < 0) T = sig_of(h->s);
    if (!T) return NULL;
    int i = 0;
    for (;;) {
        bool im = false;
        if (T->list && T->n == 2 && is(T->it[0], "%impPi")) { im = true; T = T->it[1]; }
        if (!(T->list && T->n == 4 && is(T->it[0], "Pi"))) break;
        if (im) { if (i < na && a->it[i + 1]->list && a->it[i + 1]->n == 2 && is(a->it[i + 1]->it[0], "%imp")) i++; T = T->it[3]; continue; }
        if (i >= na) break;
        i++; T = T->it[3];
    }
    return i == na ? T : NULL;
}
static bool wants_tick(Node *a) {
    if (a->list && a->n == 3 && !a->it[0]->list && (is(a->it[0], "lam") || is(a->it[0], "%lam"))) return false;
    if (has_gcall(a)) return true;
    Node *T = syn_type(a, 0);
    if (!T) T = result_shape(a);
    if (!T && a->list && a->n && !a->it[0]->list && !e_find(a->it[0]->s) && con_lookup(a->it[0]->s) >= 0) return true;   /* a constructor: present */
    return T && !is_later(T);
}
/* made once per argument: the elaboration's rounds see the same nodes (its holes are keyed by them) */
typedef struct { Node *a, *w; } TWrap;
static TWrap *TW; static int ntw, captw;
static Node *tick_wrap(Node *a) {
    for (int i = 0; i < ntw; i++) if (TW[i].a == a) return TW[i].w;
    Node *al = xfresh("α"); Node *w = sym3("lam", al, tick_lift(a, al));
    GROW(TW, captw, (size_t)ntw + 1); TW[ntw++] = (TWrap){ a, w };
    return w;
}
/* the body of a tick λ written out, its guarded calls lifted to the tick (made once per λ) */
static Node *tick_body(Node *lam, Node *al) {
    for (int i = 0; i < ntw; i++) if (TW[i].a == lam) return TW[i].w;
    Node *w = tick_lift(lam->it[2], al);
    GROW(TW, captw, (size_t)ntw + 1); TW[ntw++] = (TWrap){ lam, w };
    return w;
}
/* a clock argument nothing determines: the one clock in scope, when there is one */
static Node *the_clock(void) {
    Node *k = NULL;
    for (int i = nesc - 1; i >= 0; i--) {
        if (!esc[i].ty || esc[i].ty->list || !is(esc[i].ty, "Clock")) continue;
        bool shadowed = false; for (int j = i + 1; j < nesc; j++) if (!strcmp(esc[j].n, esc[i].n)) shadowed = true;
        if (shadowed) continue;
        if (k && strcmp(k->s, esc[i].n)) return NULL;
        k = nsym(esc[i].n);
    }
    return k;
}
/* λ-head reductions made this definition, by application node: the rounds see the same nodes; reset
 * with the hole tables at each definition (a node's address is reused once its arena is released) */
static Node **BK, **BV; static int nbk, capbk;
static Node *el_app(Node *t, Node *h, Node **args, int n) {
    /* a λ written at the head of an application (a motive applied in a signature): reduced here, where
     * the checker would not infer the λ's type (E28) */
    if (h->list && h->n == 3 && !h->it[0]->list && is(h->it[0], "lam") && !h->it[1]->list && n >= 1) {
        /* made once per application: the rounds see the same nodes (holes are keyed by them) */
        for (int i = 0; i < nbk; i++) if (BK[i] == t) return el(BV[i]);
        const char *x = h->it[1]->s; Node *b = subst_multi(h->it[2], &x, &args[0], 1);
        if (n > 1) { Node *r = nlist(n); r->it[0] = b; for (int i = 1; i < n; i++) r->it[i] = args[i]; b = r; }
        if (nbk == capbk) { capbk = capbk ? capbk * 2 : 64; BK = realloc(BK, sizeof *BK * (size_t)capbk); BV = realloc(BV, sizeof *BV * (size_t)capbk); }
        BK[nbk] = t; BV[nbk++] = b;
        return el(b);
    }
    if (!h->list && is(h, "%gcall")) {
        die("a recursive call needed now, not under ▸ (guarded recursion calls itself only where a later value is expected), in", tfname);
    }
    Node *out[512]; Node *orig[512]; int no = 0;
    bool pend = false;
    Node *hd;
    IMask *im = !h->list && !e_find(h->s) ? imask_of(h->s) : NULL;
    Node *ssol[256]; int nss = 0;
    if (im) nss = syn_solve(h->s, args, n, ssol, 0);
    app_expect = NULL;
    /* each parameter's type with what is known of the earlier ones (the rest %?) */
    Node *pty[256]; const char *pnm[256]; int npt = 0;
    if (im) {
        Node *sg = sig_of(h->s);
        for (Node *x = sg; x && npt < 255; ) {
            if (x->list && x->n == 2 && is(x->it[0], "%impPi")) x = x->it[1];
            if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
            pnm[npt] = x->it[1]->s; pty[npt] = x->it[2]; npt++; x = x->it[3];
        }
    }
    if (im) {
        hd = h;
        int pos = 0, i = 0;
        while (pos < im->len && no < 500) {
            if (im->m >> pos & 1) {
                bool given = i < n && args[i]->list && args[i]->n == 2 && is(args[i]->it[0], "%imp") && (!bnd_inst(args[i]->it[0]) || (im->inst >> pos & 1));   /* {{e}} waits for an instance position */
                if (given && !is(args[i]->it[1], "%hole")) { orig[no] = args[i]->it[1]; out[no++] = el(args[i]->it[1]); i++; }
                else if (given) { i++; orig[no] = NULL; out[no++] = pos < nss && ssol[pos] ? el(syn_once(t, pos, ssol[pos])) : hole_or_sol(t, pos, &pend); }
                else if (pos < nss && ssol[pos]) { orig[no] = NULL; out[no++] = el(syn_once(t, pos, ssol[pos])); }
                else if (pos < npt && !pty[pos]->list && is(pty[pos], "Clock") && the_clock()) { orig[no] = NULL; out[no++] = the_clock(); }
                else if (im->inst >> pos & 1) {
                    Node *vs[256]; Node *unk = nsym("%?");
                    for (int r = 0; r < pos; r++) { int kr = e_hole(t, r); vs[r] = r < nss && ssol[r] ? ssol[r] : EH[kr].sol ? EH[kr].sol : unk; }
                    orig[no] = NULL; out[no++] = inst_hole(t, pos, pos < npt ? subst_multi(pty[pos], pnm, vs, pos) : NULL, &pend);
                } else { orig[no] = NULL; out[no++] = hole_or_sol(t, pos, &pend); }
            } else {
                if (i >= n) break;
                if (args[i]->list && args[i]->n == 2 && is(args[i]->it[0], "%imp")) die("an implicit argument {..} where an explicit one is due, applying", h->s);
                if (pos < npt) {
                    Node *vs[256]; Node *unk = nsym("%?");
                    for (int r = 0; r < pos; r++) vs[r] = r < nss && ssol[r] ? ssol[r] : unk;
                    syn_expect = subst_multi(pty[pos], pnm, vs, pos);
                }
                Node *ai = args[i];
                if (syn_expect && is_later(syn_expect) && wants_tick(ai)) ai = tick_wrap(ai);
                orig[no] = ai; out[no++] = el(ai); i++; syn_expect = NULL;
            }
            pos++;
        }
        for (; i < n && no < 500; i++) {
            if (args[i]->list && args[i]->n == 2 && is(args[i]->it[0], "%imp")) die("an implicit argument {..} beyond the implicit ones of", h->s);
            orig[no] = args[i]; out[no++] = el(args[i]);
        }
    } else {
        ford_head = true; hd = el(h); ford_head = false;
        {
            /* an indexed constructor without its equality fields: refl at the declared index
             * expressions over the field arguments (fording) */
            Ford *fd = !h->list && !e_find(h->s) && con_lookup(h->s) >= 0 ? ford_of(h->s) : NULL;
            if (fd && n == fd->nfields) {
                Node **a2 = calloc((size_t)n + (size_t)fd->nidx + 1, sizeof(Node *));
                for (int i = 0; i < n; i++) a2[i] = args[i];
                for (int q = 0; q < fd->nidx; q++) {
                    Node *e = fd->idx[q];
                    for (int i = 0; i < n; i++) e = subst_sym(e, fd->fields[i], args[i]);
                    a2[n + q] = sym3("plam", xfresh("i"), e);
                }
                args = a2; n += fd->nidx;
            }
        }
        Node *fty[256]; bool fimp[256]; int nft = !h->list && !e_find(h->s) && con_lookup(h->s) >= 0 ? head_params(h, fty, fimp, 256) : 0;
        for (int i = 0; i < n && no < 500; i++) {
            if (args[i]->list && args[i]->n == 2 && is(args[i]->it[0], "%imp")) die("an implicit argument {..} applied to something without implicit arguments", NULL);
            Node *ai = args[i];
            if (i < nft && is_later(fty[i]) && wants_tick(ai)) ai = tick_wrap(ai);   /* a constructor's later field */
            /* a constructor's field that is a function into a later type, given a λ: its body (under
             * that many λs) stands at a later position (18.18) */
            if (i < nft && fty[i] && ai->list && ai->n == 3 && !ai->it[0]->list && is(ai->it[0], "lam")) {
                int d = 0; Node *T = fty[i];
                while (T && T->list && T->n == 4 && !T->it[0]->list && is(T->it[0], "Pi") && !is_later(T)) { d++; T = T->it[3]; }
                if (d > 0 && is_later(T)) fn_later_depth = d;
            }
            if (i < nft && is_later(fty[i])) tick_pos = true;   /* (a tick λ written out there binds a tick) */
            orig[no] = ai; out[no++] = !ai->list && !strcmp(ai->s, "%hole") ? hole_or_sol(ai, 0, &pend) : el(ai);
            tick_pos = false;
        }
    }
    bool is_con = !h->list && !e_find(h->s) && con_lookup(h->s) >= 0;
    bool is_fld = !h->list && !e_find(h->s) && fld_lookup(h->s) >= 0;   /* its first operand projected */
    Node *cur = hd; bool sp = pend; pend = false;
    (void)sp;
    /* holes among the arguments so far make the spine one to solve */
    bool holes = false;
    for (int i = 0; i < no; i++) {
        Node *a = out[i];
        if (a->list && a->n == 2 && is(a->it[0], "%hole")) holes = true;
        int kind = 0;   /* 0 application, 1 path application, 2 a guess */
        if (!is_con && !(is_fld && i == 0)) { if (iv_certain(a)) kind = 1; else if (orig[i] && lit01(orig[i])) kind = 2; }
        if (kind && holes) { cur = sym2("%spine", cur); holes = false; }
        if (kind == 0) cur = lst(2, cur, a);
        else if (kind == 1) cur = sym3("@", cur, a);
        else {
            int g = e_guess(orig[i], 2);
            cur = EG[g].kind == 3 ? sym4("%guess", wordn(g), wordn(3), sym3("@", cur, a)) : sym4("%guess", wordn(g), wordn(2), lst(2, cur, a));
        }
    }
    if (holes) cur = sym2("%spine", cur);
    return cur;
}
static Node **field_types(int c, Node *T, Node **fv);
static bool bindable_free(const char *s);
/* a motive (λ ix1 … ixk v → R) a1 … ak (k ≥ 1 applications of a λ of depth k + 1): the λ and k */
static bool ix_motive(Node *P, Node **P2, int *k) {
    int n = 0; Node *f = P;
    while (f->list && f->n == 2 && f->it[0]->list) { f = f->it[0]; n++; }
    if (!n || !f->list) return false;
    Node *b = f;
    for (int d = 0; d <= n; d++) { if (!(b->list && b->n == 3 && !b->it[0]->list && (is(b->it[0], "lam") || is(b->it[0], "%lam")))) return false; b = b->it[2]; }
    *P2 = f; *k = n; return true;
}
static Node *el_alts(Node *t, int from) {
    Node *r = nlist(t->n); r->it[0] = t->it[0];
    Node *sty = syn_type(t->it[from - 1], 0);
    for (int i = 1; i < from; i++) r->it[i] = el(t->it[i]);
    for (int i = from; i < t->n; i++) {
        Node *a = t->it[i];
        if (!a->list || a->n != 2) { r->it[i] = a; continue; }
        int save = nesc;
        Node *p = a->it[0];
        {
            /* an indexed constructor's pattern without its equality fields: padded (fording) */
            const char *cn = p->list ? (p->it[0]->list ? NULL : p->it[0]->s) : p->s;
            Ford *fd = cn ? ford_of(cn) : NULL;
            int got = p->list ? p->n - 1 : 0;
            if (fd && con_lookup(cn) >= 0 && got == CON[con_lookup(cn)].nfields - fd->nidx) {
                Node *q2 = nlist(got + fd->nidx + 1); q2->it[0] = p->list ? p->it[0] : p;
                for (int q = 0; q < got; q++) q2->it[q + 1] = p->it[q + 1];
                for (int q = 0; q < fd->nidx; q++) q2->it[got + q + 1] = xfresh("e");
                p = q2; a = lst(2, p, a->it[1]);
            }
        }
        if (p->list) {
            int c = con_lookup(p->it[0]->s);
            Node *fv[64]; for (int q = 1; q < p->n && q <= 64; q++) fv[q - 1] = p->it[q];
            Node **fty = c >= 0 && sty ? field_types(c, sty, fv) : NULL;
            for (int q = 1; q < p->n; q++) {
                if (fty && CON[c].nfields >= q && fty[q - 1]) e_pusht(p->it[q]->s, fty[q - 1]);
                else e_push(p->it[q]->s, CON[c].nfields < q);
                /* an induction hypothesis's type: the motive at the field */
                if (from == 3 && q - 1 < CON[c].nfields && (CON[c].recmask >> (q - 1) & 1)) {
                    char nm[256]; snprintf(nm, sizeof nm, "%%rec %s", p->it[q]->s);
                    const char *x = "%m"; Node *P = t->it[1];
                    /* a path-typed field's hypothesis is a line over the motive: its type is the
                     * checker's to state */
                    if (CON[c].rdim[q - 1]) e_push(strdup(nm), false);
                    else {
                        /* an indexed type's motive, (λ ix… v → R) n…: at the field's own index */
                        Node *P2 = NULL; int k = 0; Node *ap = lst(2, P, p->it[q]);
                        if (ix_motive(P, &P2, &k) && fty && fty[q - 1] && fty[q - 1]->list && fty[q - 1]->n > k) {
                            Node *ft = fty[q - 1]; ap = P2;
                            for (int z = 0; z < k; z++) ap = lst(2, ap, ft->it[ft->n - k + z]);
                            ap = lst(2, ap, p->it[q]);
                        }
                        e_pusht(strdup(nm), subst_multi(ap, &x, NULL, 0));
                    }
                }
            }
        }
        else e_push(p->s, false);
        if (from == 3) {
            Node *P = t->it[1];
            Node *P2 = NULL; int k = 0;
            bool lamP = P->list && P->n == 3 && !P->it[0]->list && (is(P->it[0], "lam") || is(P->it[0], "%lam")) && !P->it[1]->list;
            if (lamP || ix_motive(P, &P2, &k)) {
                Node *cv = p;
                if (p->list) {
                    int c = con_lookup(p->it[0]->s);
                    if (c >= 0 && CON[c].nfields < p->n - 1) cv = NULL;   /* a path constructor: its boundary is not this */
                }
                if (cv) { const char *w = P->it[1]->s; syn_expect = lamP ? subst_multi(P->it[2], &w, &cv, 1) : subst_multi(lst(2, P, cv), NULL, NULL, 0); }
            }
        }
        r->it[i] = lst(2, p, el(a->it[1]));
        nesc = save;
    }
    return r;
}
/* ---- by-lin (F32, item 32): the elaborator calling the certificate search on a hole. The goal
 * `Lew a b` and the hypotheses `h : Lew l r` in scope are read as linear codes over their atoms
 * (plusw, %wsucc, word literals; any other subterm an atom), lib/lin.cub's omega-cert finds the
 * multipliers at elaboration time (untrusted), and the hole becomes lib/linproof.cub's
 * cert-sound at the environment of the atoms — the verified checker's run, `cert-check … ≡ true`,
 * is refl, decided by the checker's own normalization. */
static Node *LATOM[64]; static int nlatom;
static int lin_atom(Node *t) {
    for (int i = 0; i < nlatom; i++) if (node_eq(LATOM[i], t)) return i;
    if (nlatom >= 64) die("by-lin: too many atoms in", tfname);
    LATOM[nlatom] = t; return nlatom++;
}
static bool lin_lit(Node *t) { return !t->list && t->s[0] == '#' && isdigit((unsigned char)t->s[1]); }
/* the search's code (cwop #2 / cwsucc / cwlit / cvar) and the checker's (lc-add / lc-suc / lc-lit / lc-var) */
static Node *lin_code(Node *t, bool typed) {
    if (lin_lit(t)) return lst(2, nsym(typed ? "lc-lit" : "cwlit"), t);
    if (t->list && t->n == 3 && !t->it[0]->list && !strcmp(t->it[0]->s, "plusw") && !e_find("plusw")) {
        Node *a = lin_code(t->it[1], typed), *b = lin_code(t->it[2], typed);
        return typed ? lst(2, lst(2, nsym("lc-add"), a), b) : lst(2, lst(2, lst(2, nsym("cwop"), wordn(2)), a), b);
    }
    if (t->list && t->n == 2 && !t->it[0]->list && !strcmp(t->it[0]->s, "%wsucc")) return lst(2, nsym(typed ? "lc-suc" : "cwsucc"), lin_code(t->it[1], typed));
    return lst(2, nsym(typed ? "lc-var" : "cvar"), wordn(lin_atom(t)));
}
/* the word core: goal ga ≤ gb from hypotheses hl ≤ hr with proofs hp */
static Node *lin_core(Node *ex, Node *at, Node *ga, Node *gb, Node **hl, Node **hr, Node **hp, int nh) {
    if (global_lookup("cert-sound") < 0 || global_lookup("omega-cert") < 0) die("by-lin needs lib/lin.cub and lib/linproof.cub loaded, in", tfname);
    nlatom = 0;
    Node *hs = nsym("lnil");
    for (int z = nh - 1; z >= 0; z--) hs = lst(2, lst(2, nsym("lcons"), sym3("pair", lin_code(hl[z], false), lin_code(hr[z], false))), hs);
    Node *call = lst(2, lst(2, lst(2, nsym("omega-cert"), hs), lin_code(ga, false)), lin_code(gb, false));
    if (getenv("CUBICAL_SHOW_LIN")) { fprintf(stderr, "[by-lin] "); print_node(stderr, call); fputc('\n', stderr); for (int z = 0; z < nlatom; z++) { fprintf(stderr, "  atom %d: ", z); print_node(stderr, LATOM[z]); fputc('\n', stderr); } }
    long steps = 0; char *res = readback(call, &steps);
    const char *saved = src; src = res; Node *rn = read_node(); src = saved; free(res);
    if (!rn || !rn->list || rn->n != 2 || !is(rn->it[0], "some")) die("by-lin: no certificate for the goal from the hypotheses in scope (the search refutes or does not reach it), in", tfname);
    Node *ms = nsym("munil"); Node *cs[256]; int nc = 0;
    for (Node *l = rn->it[1]; l->list && l->n == 3 && is(l->it[0], "lcons") && nc < 256; l = l->it[2]) cs[nc++] = l->it[1];
    for (int z = nc - 1; z >= 0; z--) ms = lst(2, lst(2, nsym("mucons"), cs[z]), ms);
    Node *ths = nsym("hnil");
    for (int z = nh - 1; z >= 0; z--) ths = lst(2, lst(2, lst(2, nsym("hcons"), lin_code(hl[z], true)), lin_code(hr[z], true)), ths);
    Node *ta = lin_code(ga, true), *tb = lin_code(gb, true);
    Node *v = xfresh("v"), *envb = nlatom ? LATOM[nlatom - 1] : wordn(0);
    for (int z = nlatom - 2; z >= 0; z--) envb = sym4("if", lst(3, nsym("weq"), v, wordn(z)), LATOM[z], envb);
    Node *env = sym3("lam", v, envb);
    Node *holds = nsym("unit");
    for (int z = nh - 1; z >= 0; z--) holds = sym3("pair", hp[z], holds);
    Node *pf = lst(8, nsym("cert-sound"), env, ths, ms, ta, tb, holds, sym3("lam", xfresh("i"), nsym("true")));
    if (nbk == capbk) { capbk = capbk ? capbk * 2 : 64; BK = realloc(BK, sizeof *BK * (size_t)capbk); BV = realloc(BV, sizeof *BV * (size_t)capbk); }
    BK[nbk] = at; BV[nbk++] = pf;
    syn_expect = ex;
    return el(pf);
}
/* the hypotheses in scope of a two-argument type former (Lew, leZ): innermost binding of each name */
static int lin_hyps(const char *former, const char **hn, Node **hl, Node **hr, int max) {
    int nh = 0;
    for (int i = nesc - 1; i >= 0 && nh < max; i--) {
        bool seen = false;
        for (int z = i + 1; z < nesc; z++) if (!strcmp(esc[z].n, esc[i].n)) seen = true;
        Node *T = esc[i].ty;
        if (!seen && !esc[i].dim && T && T->list && T->n == 3 && !T->it[0]->list && !strcmp(T->it[0]->s, former)) { hn[nh] = esc[i].n; hl[nh] = T->it[1]; hr[nh++] = T->it[2]; }
    }
    return nh;
}
/* over Int: leZ a b is the word claim a₁ + b₂ ≤ b₁ + a₂ on toZ2's coordinates; each addZ, negZ and
 * literal met adds the equations that tie its coordinates to its parts' (toZ2-fromZ2-rel,
 * toZ2-neg-fst2/snd2, computation), each as two inequalities by eq-le */
static Node *ZT[64]; static int nzt;
static Node *zc(Node *t, int k) { return lst(2, nsym(k == 1 ? "fst" : "snd"), lst(2, nsym("toZ2"), t)); }
static bool zhead(Node *t, const char *h, int n) { return t->list && t->n == n + 1 && !t->it[0]->list && !strcmp(t->it[0]->s, h) && !e_find(h); }
/* a Nat numeral: 3, or (%num 3) as the parser writes it; -1 otherwise */
static long z_num(Node *t) {
    if (t->list && t->n == 2 && !t->it[0]->list && !strcmp(t->it[0]->s, "%num")) t = t->it[1];
    return !t->list && isdigit((unsigned char)t->s[0]) ? atol(t->s) : -1;
}
static void z_walk(Node *t) {
    for (int i = 0; i < nzt; i++) if (node_eq(ZT[i], t)) return;
    bool lit = (zhead(t, "pos", 1) || zhead(t, "negsuc", 1)) && z_num(t->it[1]) >= 0;
    if (zhead(t, "addZ", 2)) { z_walk(t->it[1]); z_walk(t->it[2]); }
    else if (zhead(t, "negZ", 1)) z_walk(t->it[1]);
    else if (!lit) return;
    if (nzt >= 64) die("by-lin: too many integer subterms in", tfname);
    ZT[nzt++] = t;
}
static Node *by_lin_int(Node *ex, Node *at, Node *ga, Node *gb) {
    if (global_lookup("toZ2-fromZ2-rel") < 0 || global_lookup("eq-le") < 0 || global_lookup("toZ2-neg-fst2") < 0) die("by-lin over Int needs lib/intproof.cub loaded, in", tfname);
    const char *hn[64]; Node *ul[64], *ur[64]; int nu = lin_hyps("leZ", hn, ul, ur, 64);
    Node *hl[256], *hr[256], *hp[256]; int nh = 0;
    for (int z = 0; z < nu; z++) { hl[nh] = lst(3, nsym("plusw"), zc(ul[z], 1), zc(ur[z], 2)); hr[nh] = lst(3, nsym("plusw"), zc(ur[z], 1), zc(ul[z], 2)); hp[nh++] = nsym(hn[z]); }
    nzt = 0; z_walk(ga); z_walk(gb); for (int z = 0; z < nu; z++) { z_walk(ul[z]); z_walk(ur[z]); }
#define EQ2(L, R, P) do { if (nh + 2 > 256) die("by-lin: too many hypotheses in", tfname); \
        hl[nh] = (L); hr[nh] = (R); hp[nh++] = lst(4, nsym("eq-le"), (L), (R), (P)); \
        hl[nh] = (R); hr[nh] = (L); hp[nh++] = lst(4, nsym("eq-le"), (R), (L), lst(2, nsym("symw"), (P))); } while (0)
    for (int i = 0; i < nzt; i++) {
        Node *t = ZT[i];
        if (zhead(t, "addZ", 2)) {
            Node *zp = lst(3, nsym("plusw"), zc(t->it[1], 1), zc(t->it[2], 1)), *zn = lst(3, nsym("plusw"), zc(t->it[1], 2), zc(t->it[2], 2));
            EQ2(lst(3, nsym("plusw"), zc(t, 1), zn), lst(3, nsym("plusw"), zp, zc(t, 2)), lst(3, nsym("toZ2-fromZ2-rel"), zn, zp));
        } else if (zhead(t, "negZ", 1)) {
            EQ2(zc(t, 1), zc(t->it[1], 2), lst(2, nsym("toZ2-neg-fst2"), t->it[1]));
            EQ2(zc(t, 2), zc(t->it[1], 1), lst(2, nsym("toZ2-neg-snd2"), t->it[1]));
        } else {
            long k = z_num(t->it[1]); bool neg = !strcmp(t->it[0]->s, "negsuc");
            Node *k1 = wordn(neg ? 0 : k), *k2 = wordn(neg ? k + 1 : 0);
            EQ2(zc(t, 1), k1, sym3("lam", xfresh("i"), k1));
            EQ2(zc(t, 2), k2, sym3("lam", xfresh("i"), k2));
        }
    }
#undef EQ2
    return lin_core(ex, at, lst(3, nsym("plusw"), zc(ga, 1), zc(gb, 2)), lst(3, nsym("plusw"), zc(gb, 1), zc(ga, 2)), hl, hr, hp, nh);
}
static Node *by_lin(Node *ex, Node *at) {
    /* made once per occurrence: the rounds see the same nodes (guesses are keyed by them) */
    for (int i = 0; i < nbk; i++) if (BK[i] == at) { syn_expect = ex; return el(BV[i]); }
    if (ex && ex->list && ex->n == 3 && !ex->it[0]->list && !strcmp(ex->it[0]->s, "leZ")) return by_lin_int(ex, at, ex->it[1], ex->it[2]);
    if (!(ex && ex->list && ex->n == 3 && !ex->it[0]->list && !strcmp(ex->it[0]->s, "Lew"))) die("by-lin: the goal must be written Lew a b or leZ a b (the type expected here), in", tfname);
    const char *hn[64]; Node *hl[64], *hr[64], *hp[64]; int nh = lin_hyps("Lew", hn, hl, hr, 64);
    for (int z = 0; z < nh; z++) hp[z] = nsym(hn[z]);
    return lin_core(ex, at, ex->it[1], ex->it[2], hl, hr, hp, nh);
}
static Node *el(Node *t) {
    Node *ex = syn_expect; syn_expect = NULL;
    bool tickpos = tick_pos || (ex && is_later(ex)); tick_pos = false;
    if (t->list && t->n > 0 && (t->it[0]->list || !e_head_kw(t->it[0]->s))) { app_expect = ex; return el_app(t, t->it[0], t->it + 1, t->n - 1); }
    if (!t->list) {
        if (!strcmp(t->s, "%hole")) { bool p = false; Node *h = hole_or_sol(t, 0, &p); return p ? sym2("%spine", h) : h; }
        if (!strcmp(t->s, "by-lin") && !e_find(t->s) && global_lookup(t->s) < 0) return by_lin(ex, t);
        if (e_find(t->s)) return t;
        if (imask_of(t->s)) { app_expect = ex; return el_app(t, t, NULL, 0); }
        if (!ford_head && con_lookup(t->s) >= 0 && ford_of(t->s)) { app_expect = ex; return el_app(t, t, NULL, 0); }   /* an indexed constructor without fields: its refl */
        return t;
    }
    if (t->n == 0) return t;
    Node *h = t->it[0];
    if (h->list || !e_head_kw(h->s)) return el_app(t, h, t->it + 1, t->n - 1);
    const char *k = h->s;
    if (!strcmp(k, "%num")) return t;
    if (!strcmp(k, "U") && t->n == 2 && t->it[1]->list && t->it[1]->n == 2 && is(t->it[1]->it[0], "%num")) return sym2("U", t->it[1]->it[1]);   /* a level as a number */
    if (!strcmp(k, "%imp")) die("an implicit argument {..} outside an application", NULL);
    if (!strcmp(k, "%impPi")) return el(t->it[1]);
    /* a Π expected: the binder's type, and the body's */
    Node *exA = NULL, *exB = NULL;
    if (ex && (!strcmp(k, "%lam") || !strcmp(k, "lam")) && t->n == 3) {
        Node *P = ex; while (P->list && P->n == 2 && is(P->it[0], "%impPi")) P = P->it[1];
        if (P->list && P->n == 4 && is(P->it[0], "Pi") && !P->it[1]->list) {
            exA = P->it[2]; const char *y = P->it[1]->s; Node *xv = t->it[1];
            exB = strcmp(y, xv->s) ? subst_multi(P->it[3], &y, &xv, 1) : P->it[3];
            if (mentions_unknown(exA)) exA = NULL;
        }
    }
    if (!strcmp(k, "%lam")) { int save = nesc; e_pusht(t->it[1]->s, lamty_of(t)); syn_expect = exB; Node *b = el(t->it[2]); nesc = save; return sym3("lam", t->it[1], b); }

    if (!strcmp(k, "lam") && t->n == 3) {
        Node *W = bnd_ty(t->it[1]);   /* a binder written with its type: a function's λ, the binder at that type */
        bool certain = dim_use(t->it[2], t->it[1]->s);
        int g = certain || W ? -1 : e_guess(t, 0);
        bool dim = certain || (g >= 0 && EG[g].kind == 1);
        if (!exA && W && !dim) exA = W;
        int save = nesc; if (!dim && exA) e_pusht(t->it[1]->s, exA); else e_push(t->it[1]->s, dim);
        bool wt = !strncmp(t->it[1]->s, "%α", strlen("%α")) && nwtick < 64;   /* an inserted tick λ */
        /* a tick λ written out, with a guarded recursive call under it: the call is the fixed
         * point's later self at the (outermost) tick, as under an inserted one */
        Node *body = t->it[2];
        /* a λ's body where a later value is due (a continuation into ▸ κ X, 18.18): given its tick */
        int fl = fn_later_depth; fn_later_depth = -1;
        bool body_tickpos = false;
        if (!wt && !dim && ((exB && is_later(exB) && !mentions_unknown(exB)) || fl == 1) && wants_tick(body)) body = tick_wrap(body);
        else if (!wt && !dim && fl == 1) body_tickpos = true;   /* (a tick λ written out there binds a tick) */
        else if (!wt && !dim && fl > 1 && body->list && body->n == 3 && !body->it[0]->list && is(body->it[0], "lam")) fn_later_depth = fl - 1;
        if (!wt && !dim && nwtick < 64 && tickpos && has_gcall(body)) {
            wt = true; wtick[nwtick++] = t->it[1]->s;
            body = tick_body(t, t->it[1]);
        } else if (wt) wtick[nwtick++] = t->it[1]->s;
        syn_expect = dim ? NULL : exB; if (body_tickpos) tick_pos = true; Node *b = el(body); tick_pos = false; nesc = save;
        if (wt) nwtick--;
        Node *r = sym3(dim ? "plam" : "lam", t->it[1], b);
        return g >= 0 ? sym4("%guess", wordn(g), wordn(EG[g].kind), r) : r;
    }
    if (!strcmp(k, "pair") && t->n == 3 && ex) {
        /* a pair where a Σ is expected: each component at its type (a later one given a tick) */
        Node *S = ex; while (S->list && S->n == 2 && is(S->it[0], "%impPi")) S = S->it[1];
        if (S->list && S->n == 4 && is(S->it[0], "Sigma") && !S->it[1]->list) {
            Node *A = S->it[2], *B = mentions_name(S->it[3], S->it[1]->s) ? NULL : S->it[3];
            Node *c[2] = { t->it[1], t->it[2] }, *T[2] = { A, B }, *o[2];
            for (int q = 0; q < 2; q++) {
                if (T[q] && mentions_unknown(T[q])) T[q] = NULL;
                if (T[q] && is_later(T[q]) && wants_tick(c[q])) c[q] = tick_wrap(c[q]);
                syn_expect = T[q]; o[q] = el(c[q]); syn_expect = NULL;
            }
            return sym3("pair", o[0], o[1]);
        }
    }
    if ((!strcmp(k, "plam") || !strcmp(k, "forall")) && t->n == 3) { int save = nesc; e_push(t->it[1]->s, true); Node *b = el(t->it[2]); nesc = save; return sym3(k, t->it[1], b); }
    if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma")) && t->n == 4) {
        Node *A = el(t->it[2]); int save = nesc; e_pusht(t->it[1]->s, t->it[2]); Node *B = el(t->it[3]); nesc = save;
        return sym4(k, t->it[1], A, B);
    }
    if (!strcmp(k, "let") && t->n == 4) {
        Node *lt = syn_type(t->it[2], 0);
        Node *e = el(t->it[2]); int save = nesc; if (lt) e_pusht(t->it[1]->s, lt); else e_push(t->it[1]->s, false);
        syn_expect = ex; Node *b = el(t->it[3]); nesc = save; return sym4("let", t->it[1], e, b);
    }
    if (!strcmp(k, "the") && t->n == 3) { Node *T = el(t->it[1]); syn_expect = t->it[1]; Node *e = el(t->it[2]); return sym3("the", T, e); }
    if (!strcmp(k, "case") && t->n >= 3 && ex && !mentions_unknown(ex) && !getenv("CUB_NOCASEMOTIVE")) {
        /* the type expected: the motive, constant (its stuck values then say their type). Made once
         * per case and expected type, so a hole in it keeps its key from one elaboration to the next */
        Node *e = cm_get(t, ex);
        if (!e) {
            e = nlist(t->n + 1); e->it[0] = nsym("elim"); e->it[1] = sym3("lam", nsym("%cm"), ex);
            for (int i = 1; i < t->n; i++) e->it[i + 1] = t->it[i];
            cm_put(t, ex, e);
        }
        return el_alts(e, 3);
    }
    if (!strcmp(k, "if") && t->n == 4 && ex && !mentions_unknown(ex) && !getenv("CUB_NOCASEMOTIVE") && con_lookup("true") >= 0) {
        Node *e = cm_get(t, ex);
        if (!e) {
            e = lst(5, nsym("elim"), sym3("lam", nsym("%cm"), ex), t->it[1], lst(2, lst(1, nsym("true")), t->it[2]), lst(2, lst(1, nsym("false")), t->it[3]));
            cm_put(t, ex, e);
        }
        return el_alts(e, 3);
    }
    if (!strcmp(k, "case")) return el_alts(t, 2);
    if (!strcmp(k, "elim")) return el_alts(t, 3);
    if (!strcmp(k, "=")) return sym3("=", el(t->it[1]), t->it[2]);
    if (!strcmp(k, "record")) {
        Node *r = nlist(t->n); r->it[0] = h;
        for (int i = 1; i < t->n; i++) r->it[i] = t->it[i]->list ? lst(2, t->it[i]->it[0], el(t->it[i]->it[1])) : t->it[i];
        return r;
    }
    if (!strcmp(k, "sys")) {
        Node *r = nlist(t->n); r->it[0] = h;
        for (int i = 1; i < t->n; i++) r->it[i] = lst(2, el(t->it[i]->it[0]), el(t->it[i]->it[1]));
        return r;
    }
    Node *r = nlist(t->n); r->it[0] = h;
    for (int i = 1; i < t->n; i++) r->it[i] = el(t->it[i]);
    return r;
}

/* ---------------- checked definitions by clauses ----------------
 * The signature's telescope gives the parameters (their names, types, which are implicit) and the
 * result type R. A split on a variable v is the elimination of v with the motive
 *     λ v → let (the variables rebuilt from v) in Π (the later parameters) → R,
 * applied to the later parameters; its alternative for a constructor c binds c's fields, rebuilds
 * v (and what was rebuilt from v) by let, and binds the later parameters again. A recursive call on
 * a recursive field of a split is that split's induction hypothesis (rec x), applied to the later
 * parameters. */
typedef struct { const char *v; Node *def; int gp; Node *ty; } CVar;   /* a variable rebuilt by a let, its type */
typedef struct { const char *v; int gp; Node *E; Node **rf; int nrf; const char **rs; } Frame;   /* a split's IH: the recursive fields and their shapes (ConInfo.rsh) */
static CVar *cdefs; static int ncdefs, capcdefs;
static Frame *frames; static int nframes, capframes;
static Node *subst_sym(Node *t, const char *x, Node *e);
static Node *subst_multi(Node *t, const char **ps, Node **vs, int n);
/* the substitution a split's index unification has made (E28): an outer index variable by the
 * constructor's index expression (the goal rewritten), a field by the value unification found for
 * it; applied to the types and goals built below the split and to the clauses' right sides */
static const char *SGX[64]; static Node *SGV[64]; static int nsg;
static Node *sg_apply(Node *t) { return t && nsg ? subst_multi(t, SGX, SGV, nsg) : t; }
static void sg_push(const char *x, Node *v) { if (nsg >= 64) die("internal: too many index substitutions in", tfname); Node *w = sg_apply(v); SGX[nsg] = x; SGV[nsg++] = w; }
static Node *ty_of_raw(const char *v) { for (int i = ncdefs - 1; i >= 0; i--) if (!strcmp(cdefs[i].v, v)) return cdefs[i].ty; for (int i = 0; i < tsig.np; i++) if (!strcmp(tsig.pn[i]->s, v)) return tsig.pty[i]; return NULL; }
static Node *ty_of(const char *v) { return sg_apply(ty_of_raw(v)); }
/* the types of constructor c's fields, the variable it splits being of type T (D at its parameters):
 * the declaration's field types, its parameters and earlier fields replaced */
static Node **field_types(int c, Node *T, Node **fv) {
    int d = CON[c].data, nf = CON[c].nfields;
    Node **r = calloc((size_t)nf + 1, sizeof(Node *));
    Node *dd = DATA[d].decl;
    if (!T || !dd) return r;
    Node *hd = T; Node *args[64]; int na = 0;
    if (T->list) { hd = T->it[0]; for (int i = 1; i < T->n && na < 64; i++) args[na++] = T->it[i]; }
    if (hd->list || strcmp(hd->s, DATA[d].name) || na != DATA[d].nparams) return r;
    Node *cd = dd->it[2 + CON[c].index];
    Node *dh = dd->it[1];
    for (int q = 0; q < nf; q++) {
        Node *f = cd->it[q + 1];
        if (!f->list || f->n != 2) return r;
        Node *A = f->it[1];
        for (int k = 0; k < na; k++) A = subst_sym(A, param_name(dh->it[k + 1])->s, args[k]);
        for (int k = 0; k < q; k++) { Node *pf = cd->it[k + 1]; A = subst_sym(A, pf->it[0]->s, fv[k]); }
        r[q] = A;
    }
    return r;
}
static int gp_of(const char *v) { for (int i = ncdefs - 1; i >= 0; i--) if (!strcmp(cdefs[i].v, v)) return cdefs[i].gp; for (int i = 0; i < tsig.np; i++) if (!strcmp(tsig.pn[i]->s, v)) return i; return -1; }
static Node *subst_sym(Node *t, const char *x, Node *e) {
    if (!t->list) return !strcmp(t->s, x) ? e : t;
    if (t->n >= 3 && !t->it[0]->list && (is(t->it[0], "lam") || is(t->it[0], "plam") || is(t->it[0], "%lam")) && !strcmp(t->it[1]->s, x)) return t;
    if (t->n == 4 && !t->it[0]->list && is(t->it[0], "let") && !strcmp(t->it[1]->s, x)) return sym4("let", t->it[1], subst_sym(t->it[2], x, e), t->it[3]);
    Node *r = nlist(t->n);
    for (int i = 0; i < t->n; i++) r->it[i] = subst_sym(t->it[i], x, e);
    return r;
}
/* an expression with the rebuilt variables replaced by what they are */
static Node *expand(Node *e) {
    for (int k = 0; k < 64; k++) {
        Node *r = e;
        for (int i = ncdefs - 1; i >= 0; i--) if (cdefs[i].def) r = subst_sym(r, cdefs[i].v, cdefs[i].def);
        if (node_eq(r, e)) return r;
        e = r;
    }
    return e;
}
/* course-of-values recursion: a split of a parameter computes, with each value, the values at every
 * value below (Below v: for each recursive field r, the value at r and Below r). A variable below
 * the split has its pair (value, Below) as an expression; a split of it reverts the pair, so that
 * in its branches the pair's Below unfolds to the pairs of its own recursive fields */
typedef struct { const char *w; Node *e; const char *v; int gp; Node *E; Node *M; Node *Bl; } Deep;
static Deep *deeps; static int ndeeps, capdeeps;
static bool cov_on, cov_armed; static jmp_buf cov_jb;
/* guarded recursion: a definition with a clock parameter (the first, guard_q) whose recursion is not
 * structural is a guarded fixed point: each call is (%gcall the arguments after the clock), made a
 * call of the fixed point's own later self where it stands under ▸ (a later position) */

/* a recursive call on a field recursive under paths, Π codomains and Σ components (18.18): the
 * argument matched against the field's shape, f (k x) as (rec k) x, f (p @ i) as (rec p) @ i, f (fst
 * p) as (rec p) (or fst (rec p) when both components are recursive); NULL when it is not such a call */
static const char *shape_end(const char *s) { int depth = 1; while (depth > 0) { char d = *s++; if (d == 'D') depth--; else if (d == 'B') depth++; } return s; }
static Node *rec_by_shape_walk(const char **sp, Node **ops, int i, Node *h) {
    char c = *(*sp)++;
    Node *op = i >= 0 ? ops[i] : NULL;
    switch (c) {
    case 'D': return i < 0 ? h : NULL;
    case 'P': if (!op || !is(op->it[0], "@")) return NULL; return rec_by_shape_walk(sp, ops, i - 1, sym3("@", h, op->it[2]));
    case 'F': if (!op || (!op->it[0]->list && (is(op->it[0], "@") || is(op->it[0], "fst") || is(op->it[0], "snd")))) return NULL;
        return rec_by_shape_walk(sp, ops, i - 1, lst(2, h, op->it[op->n - 1]));
    case '1': case '2': if (!op || !is(op->it[0], c == '1' ? "fst" : "snd")) return NULL; return rec_by_shape_walk(sp, ops, i - 1, h);
    case 'B': {
        if (!op || !(is(op->it[0], "fst") || is(op->it[0], "snd"))) return NULL;
        bool f1 = is(op->it[0], "fst");
        const char *a = *sp, *b = shape_end(a);
        *sp = shape_end(b);
        const char *q = f1 ? a : b;
        return rec_by_shape_walk(&q, ops, i - 1, sym2(f1 ? "fst" : "snd", h));
    }
    default: return NULL;
    }
}
static Node *rec_by_shape(const char *sh, Node *want, Node *cand, Node *hyp) {
    /* the argument's spine from the field outward: path applications, applications, projections */
    Node *ops[64]; int nops = 0; Node *w = want;
    while (nops < 64 && w->list) {
        if (w->n == 3 && !w->it[0]->list && is(w->it[0], "@")) { ops[nops++] = w; w = w->it[1]; }
        else if (w->n == 2 && !w->it[0]->list && (is(w->it[0], "fst") || is(w->it[0], "snd"))) { ops[nops++] = w; w = w->it[1]; }
        else if (w->n >= 2 && !(!w->it[0]->list && e_head_kw(w->it[0]->s))) {
            ops[nops++] = w;
            if (w->n == 2) w = w->it[0];
            else { Node *fn = nlist(w->n - 1); for (int k = 0; k < w->n - 1; k++) fn->it[k] = w->it[k]; w = fn; }
        } else break;
    }
    if (!node_eq(cand, w)) return NULL;
    const char *sp = sh;
    return rec_by_shape_walk(&sp, ops, nops - 1, hyp);
}
/* recursion on a parameter that is not the first while an earlier one changes: the split of that
 * parameter is made outermost, the earlier parameters its motive's Πs (reverted as later ones are),
 * so the hypothesis is applied to their new values. perm_p: the parameter tried (-1: the order as
 * written); perm_next: the next candidate after p, a parameter some clause matches by constructor
 * whose type lets it stand before an earlier one (in the order as written, perm0) */
static int perm_p = -1, perm_np; static Node **perm_pty, **perm_pn; static bool *perm_imp, *perm_con;
static int perm_dest(int p) { int d = 0; for (int q = 0; q < p; q++) if (mentions_name(perm_pty[p], perm_pn[q]->s)) d = q + 1; return d; }
static int perm_next(int p) { for (int q = p < 0 ? 1 : p + 1; q < perm_np; q++) if (perm_con[q] && !perm_imp[q] && perm_dest(q) < q) return q; return -1; }
static Deep *deep_of(const char *w) { for (int i = ndeeps - 1; i >= 0; i--) if (!strcmp(deeps[i].w, w)) return &deeps[i]; return NULL; }
static Node *rec_xform(Node *e);
static Node *rec_call(Node *e, Node **args, int n) {
    /* the call's arguments at the parameters' positions (implicit ones not given) */
    Node *at[256] = { 0 }; int i = 0;
    for (int k = 0; k < tsig.np && i <= n; k++) {
        int q = tsig.ord ? tsig.ord[k] : k;
        if (tsig.imp[q]) { if (i < n && imp_at(args[i], tsig.pn[q])) at[q] = args[i++]->it[1]; continue; }
        if (i < n) at[q] = args[i++];
    }
    if (guard_on) {
        /* a call of the guarded fixed point: the clock and what comes before it unchanged */
        /* the clock, what comes before it and the implicit parameters are the fixed point's own */
        for (int q = 0; q < tsig.np; q++) if ((q <= guard_q || tsig.imp[q]) && at[q] && !(!at[q]->list && !strcmp(at[q]->s, tsig.pn[q]->s)))
            die("a guarded recursive call changes the clock, a parameter before it or an implicit parameter, in", tfname);
        int nl = 0; for (int q = guard_q + 1; q < tsig.np; q++) if (!tsig.imp[q]) nl++;
        for (int q = guard_q + 1; q < tsig.np; q++) if (!at[q] && !tsig.imp[q]) die("a guarded recursive call must give every argument, in", tfname);
        Node *c = nlist(nl + 1); c->it[0] = nsym("%gcall");
        { int k = 1; for (int q = guard_q + 1; q < tsig.np; q++) if (!tsig.imp[q]) c->it[k++] = rec_xform(at[q]); }
        if (i == n) return c;
        Node *r = nlist(n - i + 1); r->it[0] = c;
        for (int k = i; k < n; k++) r->it[k - i + 1] = rec_xform(args[k]);
        return r;
    }
    if (i < n) {
        /* more arguments than parameters: the call at the parameters, applied to the rest (a path
         * at a dimension, or a function value) */
        Node *c = rec_call(e, args, i);
        Node *r = nlist(n - i + 1); r->it[0] = c;
        for (int k = i; k < n; k++) r->it[k - i + 1] = rec_xform(args[k]);
        return r;
    }
    for (int s = ndeeps - 1; s >= 0; s--) {
        Deep *d = &deeps[s];
        int p = d->gp;
        bool ok = at[p] != NULL;
        for (int q = 0; q < p && ok; q++) if (at[q] && !(!at[q]->list && !strcmp(at[q]->s, tsig.pn[q]->s))) ok = false;
        if (!ok) continue;
        Node *want = expand(rec_xform(at[p]));
        Node *cand = expand(subst_sym(d->E, d->v, nsym(d->w)));
        if (!node_eq(cand, want)) continue;
        int nl = 0; for (int q = p + 1; q < tsig.np; q++) if (tsig.rev[q]) nl++;
        Node *call = sym2("fst", d->e);
        if (!nl) return call;
        Node *ap = nlist(nl + 1); ap->it[0] = call; int k = 1;
        for (int q = p + 1; q < tsig.np; q++) if (tsig.rev[q]) {
            if (!at[q] && !tsig.imp[q]) die("a recursive call must give every argument after the one it recurses on:", tfname);
            ap->it[k++] = at[q] ? rec_xform(at[q]) : nsym("%hole");
        }
        return ap;
    }
    for (int s = nframes - 1; s >= 0; s--) {
        Frame *f = &frames[s];
        int p = f->gp;
        bool ok = at[p] != NULL;
        for (int q = 0; q < p && ok; q++) if (at[q] && !(!at[q]->list && !strcmp(at[q]->s, tsig.pn[q]->s))) ok = false;
        if (!ok) continue;
        Node *want = expand(rec_xform(at[p]));
        /* a call on a path-typed field applied to dimensions, f (p @ m ...): the field's hypothesis
         * is a line, applied to the same dimensions (18.17) */
        Node *dims[16]; int ndm = 0; Node *head = want;
        while (head->list && head->n == 3 && !head->it[0]->list && is(head->it[0], "@") && ndm < 16) { dims[ndm++] = head->it[2]; head = head->it[1]; }
        for (int r = 0; r < f->nrf; r++) {
            Node *cand = expand(subst_sym(f->E, f->v, f->rf[r]));
            bool whole = node_eq(cand, want);
            Node *call = lst(2, nsym("rec"), f->rf[r]);
            if (!whole && !(ndm && node_eq(cand, head))) {
                /* under Π codomains and Σ components (18.18): by the field's shape */
                if (!f->rs[r]) continue;
                Node *byshape = rec_by_shape(f->rs[r], want, cand, call);
                if (!byshape) continue;
                call = byshape; whole = true;
            }
            /* the hypothesis at the later parameters */
            int nl = 0; for (int q = p + 1; q < tsig.np; q++) if (tsig.rev[q]) nl++;
            if (!whole) for (int k = ndm - 1; k >= 0; k--) call = sym3("@", call, dims[k]);
            if (!nl) return call;
            Node *ap = nlist(nl + 1); ap->it[0] = call; int k = 1;
            for (int q = p + 1; q < tsig.np; q++) if (tsig.rev[q]) {
                if (!at[q] && !tsig.imp[q]) die("a recursive call must give every argument after the one it recurses on:", tfname);
                ap->it[k++] = at[q] ? rec_xform(at[q]) : nsym("%hole");
            }
            return ap;
        }
    }
    (void)e;
    if (cov_armed && !guard_on && guard_q >= 0) longjmp(cov_jb, 2);   /* a clock: a guarded fixed point */
    if (cov_armed && !cov_on) longjmp(cov_jb, 1);   /* deeper than a field: again, with the values below */
    if (cov_armed && !guard_on && perm_next(perm_p) >= 0) longjmp(cov_jb, 3);   /* on a later parameter while an earlier one changes: again, that parameter's split outermost */
    char m[256]; snprintf(m, sizeof m, "%s: a recursive call is not on a constructor field or a field's field from one of the definition's own matches (structural recursion), in", tfname);
    print_node(stderr, e); fputc('\n', stderr);
    die(m, tfname);
    return NULL;
}
static Node *rec_xform(Node *e) {
    if (!e->list) { if (!strcmp(e->s, tfname) && !e_find(e->s)) return rec_call(e, NULL, 0); return e; }
    if (e->n == 0) return e;
    Node *h = e->it[0];
    if (!h->list && !strcmp(h->s, tfname)) {
        Node **a = calloc((size_t)e->n, sizeof(Node *));
        for (int i = 1; i < e->n; i++) a[i - 1] = e->it[i];
        return rec_call(e, a, e->n - 1);
    }
    if (!h->list && (is(h, "lam") || is(h, "plam") || is(h, "%lam")) && e->n == 3 && !strcmp(e->it[1]->s, tfname)) return e;
    Node *r = nlist(e->n);
    for (int i = 0; i < e->n; i++) r->it[i] = rec_xform(e->it[i]);
    return r;
}
/* the later parameters (after p) as Π types around R; as λs; as arguments */
/* the later parameters a split reverts: those the recursive calls change, or whose types depend on an
 * earlier parameter */
static Node *later_pi(int p) { Node *r = tsig.R; for (int q = tsig.np - 1; q > p; q--) if (tsig.rev[q]) r = sym4("Pi", tsig.pn[q], tsig.pty[q], r); return sg_apply(r); }
static Node *later_lam(int p, Node *b) { for (int q = tsig.np - 1; q > p; q--) if (tsig.rev[q]) b = lam_t(tsig.pn[q], sg_apply(tsig.pty[q]), b); return b; }
static Node *later_app(int p, Node *f) {
    int nl = 0; for (int q = p + 1; q < tsig.np; q++) if (tsig.rev[q]) nl++;
    if (!nl) return f;
    Node *r = nlist(nl + 1); r->it[0] = f; int k = 1;
    for (int q = p + 1; q < tsig.np; q++) if (tsig.rev[q]) r->it[k++] = tsig.pn[q];
    return r;
}
static bool mentions_any(Node *t, const char *v) { return mentions_name(t, v); }
/* a rebuilt variable's value, annotated with its type when it is known */
static Node *typed_def(CVar *c) { return c->ty ? sym3("the", c->ty, c->def) : c->def; }
/* index unification at a split (E28, as Agda's): the scrutinee's index arguments against a
 * constructor's index expressions over its fields — a clash makes the branch absurd, the same
 * constructor recurses on the arguments, a field met by a term it does not occur in is solved */
typedef struct { int con, arg; } UStep;
typedef struct { int q, depth, ca; UStep path[16]; } UClash;
typedef struct { int k, q, depth; Node *val; UStep path[16]; } USol;
typedef struct { bool clash, stuck; UClash cl; int nsol; USol sol[16]; } UState;
/* a constructor application (a numeral of a type with zero and suc read as one), its arguments */
static int u_con(Node *t, Node **args, int *na) {
    *na = 0;
    if (!t->list) {
        if (isdigit((unsigned char)t->s[0])) {
            long k = atol(t->s); int z = con_lookup("zero"), sc = con_lookup("suc");
            if (z < 0 || sc < 0) return -1;
            if (k == 0) return z;
            char b[32]; snprintf(b, sizeof b, "%ld", k - 1); args[0] = nsym(b); *na = 1; return sc;
        }
        if (e_find(t->s)) return -1;
        int c = con_lookup(t->s); return c >= 0 && CON[c].nfields == 0 ? c : -1;
    }
    if (t->n == 0 || t->it[0]->list || e_find(t->it[0]->s)) return -1;
    int c = con_lookup(t->it[0]->s);
    if (c < 0 || CON[c].nfields != t->n - 1 || t->n - 1 > 16) return -1;
    for (int i = 1; i < t->n; i++) args[i - 1] = t->it[i];
    *na = t->n - 1; return c;
}
static void u_walk(Node *a, Node *b, int q, UStep *path, int depth, Node **fv, int nfo, UState *st) {
    if (st->clash || st->stuck) return;
    if (node_eq(a, b)) return;
    if (!b->list) for (int z = 0; z < nfo; z++) if (!strcmp(fv[z]->s, b->s)) {
        for (int w = 0; w < st->nsol; w++) if (st->sol[w].k == z) { if (!node_eq(st->sol[w].val, a)) st->stuck = true; return; }
        if (mentions_name(a, b->s) || st->nsol >= 16 || depth == 0) { st->stuck = true; return; }
        USol *so = &st->sol[st->nsol++]; so->k = z; so->q = q; so->depth = depth; so->val = a;
        for (int i = 0; i < depth; i++) so->path[i] = path[i];
        return;
    }
    Node *aa[16], *ba[16]; int na, nb;
    int ca = u_con(a, aa, &na), cb = u_con(b, ba, &nb);
    if (ca < 0 || cb < 0) { st->stuck = true; return; }
    if (ca != cb) {
        st->clash = true; st->cl.q = q; st->cl.depth = depth; st->cl.ca = ca;
        for (int i = 0; i < depth; i++) st->cl.path[i] = path[i];
        return;
    }
    if (depth >= 15) { st->stuck = true; return; }
    for (int i = 0; i < na; i++) { path[depth] = (UStep){ ca, i }; u_walk(aa[i], ba[i], q, path, depth + 1, fv, nfo, st); }
}
/* case x of … along a position: at the end of path the constructor `last` gives yes, any other
 * no; above it, the constructor on the path recurses into its argument and any other gives no */
static Node *u_case(Node *x, UStep *path, int depth, int at, int last, Node *yes, Node *no, Node *dflt_inner) {
    int target = at < depth ? path[at].con : last;
    int dd = CON[target].data;
    Node *cs = nlist(2 + DATA[dd].ncons); cs->it[0] = nsym("case"); cs->it[1] = x;
    for (int ci = 0; ci < DATA[dd].ncons; ci++) {
        int c2 = DATA[dd].cons[ci], nf = CON[c2].nfields;
        Node *pat = nlist(nf + 1); pat->it[0] = nsym(CON[c2].name);
        for (int q = 0; q < nf; q++) pat->it[q + 1] = xfresh("u");
        Node *b;
        if (c2 != target) b = no;
        else if (at < depth) b = (at + 1 == depth && dflt_inner) ? pat->it[path[at].arg + 1] : u_case(pat->it[path[at].arg + 1], path, depth, at + 1, last, yes, no, dflt_inner);
        else b = yes;
        cs->it[2 + ci] = lst(2, nf ? pat : lst(1, nsym(CON[c2].name)), b);
    }
    return cs;
}
static Node *ccm_t(Node **vars, int nv, Row *rows, int nr);
static Node *ccm_t(Node **vars, int nv, Row *rows, int nr) {
    if (!nr) die("the clauses do not cover every case of", tfname);
    int j = -1;
    for (int q = 0; q < nv; q++) if (!pat_wild(rows[0].p[q]) && pat_con(rows[0].p[q]) >= 0) { j = q; break; }
    if (j >= 0) {
        /* split the leftmost column any clause matches by constructor: a recursion on an earlier
         * parameter must re-split the later ones it changes, so they are split inside it */
        for (int q = 0; q < j; q++) {
            bool any = false;
            for (int r = 0; r < nr && !any; r++) if (!pat_wild(rows[r].p[q]) && pat_con(rows[r].p[q]) >= 0) any = true;
            if (any) { j = q; break; }
        }
    }
    if (j < 0) {
        Node *e = rows[0].rhs; row_reached(&rows[0]);
        for (int q = 0; q < nv; q++) if (!pat_wild(rows[0].p[q]) && strcmp(rows[0].p[q]->s, vars[q]->s)) e = x_rename(e, rows[0].p[q]->s, vars[q]->s);
        return rec_xform(sg_apply(e));
    }
    const char *v = vars[j]->s;
    int gp = gp_of(v);
    if (gp < 0) die("internal: a split on an unknown variable", v);
    int jr = 0; while (jr < nr && (pat_wild(rows[jr].p[j]) || pat_con(rows[jr].p[j]) < 0)) jr++;
    int d = CON[pat_con(rows[jr].p[j])].data, N = DATA[d].ncons;
    /* the rebuilt variables that depend on v, in order */
    int dep[256], nd = 0;
    for (int i = 0; i < ncdefs; i++) {
        if (!cdefs[i].def) continue;
        bool m = mentions_any(cdefs[i].def, v);
        for (int q = 0; q < nd && !m; q++) if (mentions_any(cdefs[i].def, cdefs[dep[q]].v)) m = true;
        if (m && nd < 256) dep[nd++] = i;
    }
    /* a split of a parameter of a recursive type, when recursion goes deeper than a field: with the
     * values below. A split of a variable below such a split: its pair reverted */
    Deep *dv = deep_of(v);
    bool cov = false;
    if (cov_on && !dv && !strcmp(tsig.pn[gp]->s, v)) {
        bool rec = false, paths = false;
        for (int ci = 0; ci < N; ci++) { int c = DATA[d].cons[ci]; if (CON[c].recmask) rec = true; if (CON[c].ndims) paths = true; }
        cov = rec && !paths && global_lookup("Unit") >= 0;
    }
    Node *mot = later_pi(gp);
    Node *Mlam = NULL, *Bllam = NULL, *ihv = NULL;
    if (cov) {
        /* M v: the value's type; Below v: by elimination into types */
        Node *Mv = mot;
        for (int q = nd - 1; q >= 0; q--) if (mentions_name(Mv, cdefs[dep[q]].v)) Mv = sym4("let", nsym(cdefs[dep[q]].v), typed_def(&cdefs[dep[q]]), Mv);
        Mlam = sym3("%lam", nsym(v), Mv);
        Node *ba = nlist(N + 3); ba->it[0] = nsym("elim"); ba->it[1] = sym3("%lam", xfresh("b"), nsym("U")); ba->it[2] = nsym(v);
        for (int ci = 0; ci < N; ci++) {
            int c = DATA[d].cons[ci], nf = CON[c].nfields;
            Node *pat = nlist(nf + 1); pat->it[0] = nsym(CON[c].name);
            Node *rf[64]; int nrf = 0;
            for (int q = 0; q < nf; q++) { pat->it[q + 1] = xfresh("f"); if (CON[c].recmask >> q & 1 && nrf < 64) rf[nrf++] = pat->it[q + 1]; }
            Node *prod = nrf ? NULL : nsym("Unit");
            for (int k = nrf - 1; k >= 0; k--) {
                Node *Mr = subst_sym(Mv, v, rf[k]);
                Node *pk = sym4("Sigma", xfresh("_"), Mr, sym2("rec", rf[k]));
                prod = prod ? sym4("Sigma", xfresh("_"), pk, prod) : pk;
            }
            ba->it[3 + ci] = lst(2, nf ? pat : nsym(CON[c].name), prod);
            if (!nf) ba->it[3 + ci] = lst(2, lst(1, nsym(CON[c].name)), prod);
        }
        Bllam = sym3("%lam", nsym(v), ba);
        mot = sym4("Sigma", xfresh("_"), Mv, ba);
        mot = sym3("%lam", nsym(v), mot);
    } else {
        if (dv) {
            /* the pair of v reverted */
            const char *vv = dv->v; Node *vn = nsym(v);
            Node *Mv = subst_sym(dv->M->it[2], vv, vn), *Bv = subst_sym(dv->Bl->it[2], vv, vn);
            ihv = xfresh("ih");
            mot = sym4("Pi", ihv, sym4("Sigma", xfresh("_"), Mv, Bv), mot);
        }
        for (int q = nd - 1; q >= 0; q--) if (mentions_name(mot, cdefs[dep[q]].v)) mot = sym4("let", nsym(cdefs[dep[q]].v), typed_def(&cdefs[dep[q]]), mot);
        mot = sym3("%lam", nsym(v), mot);
    }
    /* an indexed type (fording) whose index arguments are variables n…: the motive written as a λ
     * over the indices applied to them, (λ ix… v → body[n := ix]) n…, so the checker types an
     * induction hypothesis at the field's own index (E28: the index unified at the split) */
    Node *motx = mot, *ixlam = NULL; int ixk = 0; const char *ixv[64];
    if (!cov && !dv && DATA[d].nidx > 0) {
        Node *vt = ty_of(v); int np = DATA[d].nparams, k = DATA[d].nidx;
        if (vt && vt->list && vt->n == np + 1 && !vt->it[0]->list && !strcmp(vt->it[0]->s, DATA[d].name)) {
            const char *ixn[64]; Node *fresh[64]; bool ok = k <= 64;
            for (int q = 0; ok && q < k; q++) {
                Node *a = vt->it[1 + np - k + q];
                if (a->list || !bindable_free(a->s) || !strcmp(a->s, v)) ok = false;
                for (int z = 0; ok && z < q; z++) if (!strcmp(ixn[z], a->s)) ok = false;
                if (ok) { ixn[q] = a->s; fresh[q] = xfresh("ix"); }
            }
            if (ok) {
                Node *inner = sym3("%lam", nsym(v), subst_multi(mot->it[2], ixn, fresh, k));
                for (int q = k - 1; q >= 0; q--) inner = sym3("%lam", fresh[q], inner);
                motx = inner; ixlam = inner; ixk = k;
                for (int q = 0; q < k; q++) ixv[q] = ixn[q];
                for (int q = 0; q < k; q++) motx = lst(2, motx, nsym(ixn[q]));
            }
        }
    }
    /* the frame of this split: x_gp in terms of v */
    Node *E = expand(tsig.pn[gp]);
    Node **alts = calloc((size_t)N + 1, sizeof(Node *)); int na = 0;
    for (int ci = 0; ci < N; ci++) {
        int c = DATA[d].cons[ci];
        int nf = CON[c].nfields + CON[c].ndims;
        Node **fv = calloc((size_t)nf + 1, sizeof(Node *));
        for (int r = 0; r < nr && nf; r++) if (pat_con(rows[r].p[j]) == c) {
            Node *p = rows[r].p[j];
            for (int q = 0; q < nf; q++) if (!fv[q] && p->list && q + 1 < p->n && !p->it[q + 1]->list && !pat_wild(p->it[q + 1]) && pat_con(p->it[q + 1]) < 0) {
                bool clash = false;
                for (int z = 0; z < q; z++) if (fv[z] && !strcmp(fv[z]->s, p->it[q + 1]->s)) clash = true;
                for (int z = 0; z < nv; z++) if (!strcmp(vars[z]->s, p->it[q + 1]->s)) clash = true;
                for (int z = 0; z < tsig.np; z++) if (!strcmp(tsig.pn[z]->s, p->it[q + 1]->s)) clash = true;
                if (!clash) fv[q] = p->it[q + 1];
            }
            break;
        }
        for (int q = 0; q < nf; q++) if (!fv[q]) fv[q] = xfresh("f");
        int nv2 = nv - 1 + nf;
        Node **v2 = calloc((size_t)nv2 + 1, sizeof(Node *));
        for (int q = 0; q < j; q++) v2[q] = vars[q];
        for (int q = 0; q < nf; q++) v2[j + q] = fv[q];
        for (int q = j + 1; q < nv; q++) v2[q - 1 + nf] = vars[q];
        Row *r2 = calloc((size_t)nr + 1, sizeof(Row)); int n2 = 0;
        bool eq_named = false;   /* a clause names an equality field: its branch is not rewritten */
        for (int r = 0; r < nr; r++) {
            Node *p = rows[r].p[j]; int pc = pat_con(p);
            if (pc == c && p->list) {
                Ford *fe = ford_of(CON[c].name);
                if (fe && p->n - 1 == nf) for (int q = nf - fe->nidx; q < nf; q++) if (!pat_wild(p->it[q + 1]) || strcmp(p->it[q + 1]->s, "_")) eq_named = true;
            }
            if (!pat_wild(p) && pc >= 0 && pc != c) continue;
            Row x; x.p = calloc((size_t)nv2 + 1, sizeof(Node *)); x.rhs = rows[r].rhs; x.id = rows[r].id;
            for (int q = 0; q < j; q++) x.p[q] = rows[r].p[q];
            for (int q = j + 1; q < nv; q++) x.p[q - 1 + nf] = rows[r].p[q];
            if (pc == c) {
                int got = p->list ? p->n - 1 : 0;
                Ford *fd = ford_of(CON[c].name);
                if (fd && got == nf - fd->nidx) { Node *q2 = nlist(nf + 1); q2->it[0] = p->list ? p->it[0] : p; for (int q = 0; q < got; q++) q2->it[q + 1] = p->it[q + 1]; for (int q = got; q < nf; q++) q2->it[q + 1] = nsym("_"); p = q2; got = nf; }
                if (got != nf) die("a pattern has the wrong number of fields for", CON[c].name);
                for (int q = 0; q < nf; q++) x.p[j + q] = p->it[q + 1];
            } else {
                for (int q = 0; q < nf; q++) x.p[j + q] = nsym("_");
                if (!pat_wild(p) && strcmp(p->s, v)) x.rhs = x_rename(x.rhs, p->s, v);
            }
            r2[n2++] = x;
        }
        /* index unification when the scrutinee's indices are not all variables */
        UState us = { 0 };
        Ford *fu = !cov && !dv && !ixlam && DATA[d].nidx > 0 ? ford_of(CON[c].name) : NULL;
        int nfo = fu ? nf - fu->nidx : 0;
        if (fu && !eq_named && !CON[c].ndims) {
            Node *vt = ty_of(v); int np = DATA[d].nparams, k = DATA[d].nidx;
            if (vt && vt->list && vt->n == np + 1 && !vt->it[0]->list && !strcmp(vt->it[0]->s, DATA[d].name) && fu->nfields == nfo) {
                const char *fx[64]; Node *fvv[64]; int nfx = 0;
                for (int z = 0; z < nfo && z < 64; z++) { fx[nfx] = fu->fields[z]; fvv[nfx++] = fv[z]; }
                for (int q = 0; q < k; q++) {
                    UStep path[16];
                    u_walk(vt->it[1 + np - k + q], subst_multi(fu->idx[q], fx, fvv, nfx), q, path, 0, fv, nfo, &us);
                }
                /* a solved field matched by more than a variable is left as it was */
                for (int w = 0; w < us.nsol && !us.stuck; w++)
                    for (int r = 0; r < n2; r++) { Node *pp = r2[r].p[j + us.sol[w].k]; if (!pat_wild(pp) && pat_con(pp) >= 0) us.stuck = true; }
            } else us.stuck = true;
        } else us.stuck = true;
        if (us.stuck) { us.clash = false; us.nsol = 0; }

        /* v rebuilt, and what was rebuilt from it */
        Node *cv = nsym(CON[c].name);
        if (nf) { cv = nlist(nf + 1); cv->it[0] = nsym(CON[c].name); for (int q = 0; q < nf; q++) cv->it[q + 1] = fv[q]; }
        int save_defs = ncdefs, save_frames = nframes;
        Node *vty = ty_of(v);
        Node **fty = field_types(c, vty, fv);
        CVar vdef = { v, cv, gp, vty };
        GROW(cdefs, capcdefs, (size_t)ncdefs + 1); cdefs[ncdefs++] = vdef;
        for (int q = 0; q < nd; q++) { GROW(cdefs, capcdefs, (size_t)ncdefs + 1); cdefs[ncdefs++] = cdefs[dep[q]]; }
        for (int q = 0; q < nf; q++) { GROW(cdefs, capcdefs, (size_t)ncdefs + 1); cdefs[ncdefs++] = (CVar){ fv[q]->s, NULL, gp, q < CON[c].nfields ? fty[q] : NULL }; }
        /* the recursive fields' hypotheses */
        int save_deeps = ndeeps;
        Node *tuple = NULL;
        if (cov || dv) {
            /* each recursive field's pair: the hypothesis (a split with the values below), or from
             * the reverted pair's Below */
            Node *rfs[64]; int nrf = 0;
            for (int q = 0; q < CON[c].nfields; q++) if (CON[c].recmask >> q & 1 && nrf < 64) rfs[nrf++] = fv[q];
            for (int k = 0; k < nrf; k++) {
                Node *e;
                if (cov) e = sym2("rec", rfs[k]);
                else {
                    e = sym2("snd", ihv);
                    for (int z = 0; z < k; z++) e = sym2("snd", e);
                    if (k < nrf - 1) e = sym2("fst", e);
                }
                Deep dd = { rfs[k]->s, e, cov ? v : dv->v, gp, cov ? E : dv->E, cov ? Mlam : dv->M, cov ? Bllam : dv->Bl };
                GROW(deeps, capdeeps, (size_t)ndeeps + 1); deeps[ndeeps++] = dd;
            }
            if (cov) {
                for (int k = nrf - 1; k >= 0; k--) { Node *ek = sym2("rec", rfs[k]); tuple = tuple ? sym3("pair", ek, tuple) : ek; }
                if (!tuple) tuple = nsym("unit");
            }
        } else {
            Frame fr = { v, gp, E, calloc((size_t)nf + 1, sizeof(Node *)), 0, calloc((size_t)nf + 1, sizeof(char *)) };
            for (int q = 0; q < CON[c].nfields; q++) if (CON[c].recmask >> q & 1) { fr.rs[fr.nrf] = CON[c].rsh[q]; fr.rf[fr.nrf++] = fv[q]; }
            GROW(frames, capframes, (size_t)nframes + 1); frames[nframes++] = fr;
        }
        /* the goal rewritten by a variable index: below the split the index is the constructor's */
        Ford *fdr0 = ixlam ? ford_of(CON[c].name) : NULL;
        bool plan_rw = fdr0 && fdr0->nidx == ixk && !eq_named && nd == 0 && !CON[c].ndims && fdr0->nfields == nf - ixk;
        int sg0 = nsg;
        if (plan_rw) {
            const char *fx[64]; Node *fvv[64]; int nfx = 0;
            for (int z = 0; z < nf - ixk && z < 64; z++) { fx[nfx] = fdr0->fields[z]; fvv[nfx++] = fv[z]; }
            for (int q = 0; q < ixk; q++) sg_push(ixv[q], subst_multi(fdr0->idx[q], fx, fvv, nfx));
        }
        for (int w = 0; w < us.nsol; w++) sg_push(fv[us.sol[w].k]->s, us.sol[w].val);
        Node *body;
        Node *G = sg_apply(tsig.R);
        if (us.clash) {
            /* absurd: transport the identity on G along the equation into G, through a type that is
             * G → G at the scrutinee's constructor and G at the clashing one */
            Node *e = fv[nfo + us.cl.q], *i = xfresh("i"), *g = xfresh("g");
            Node *disc = u_case(lst(2, e, i), us.cl.path, us.cl.depth, 0, us.cl.ca, sym4("Pi", xfresh("_"), G, G), G, NULL);
            body = lst(4, nsym("comp"), sym3("lam", i, disc), sym3("lam", xfresh("i"), lst(1, nsym("sys"))), sym3("lam", g, g));
        } else if (!n2) {
            /* no clause for this constructor: covered when a later variable's type, under the
             * substitution so far, admits no constructor (each clashes); split it, every branch
             * absurd (as Agda's coverage checker splits on) */
            int col = -1, dq = -1;
            for (int q = 0; q < nv2 && col < 0; q++) {
                Node *qt = ty_of(v2[q]->s);
                Node *hd = qt ? (qt->list ? qt->it[0] : qt) : NULL;
                int dq2 = hd && !hd->list ? data_lookup(hd->s) : -1;
                if (dq2 < 0 || DATA[dq2].nidx == 0 || !qt->list || qt->n != DATA[dq2].nparams + 1) continue;
                bool all = DATA[dq2].ncons > 0;
                for (int ci2 = 0; ci2 < DATA[dq2].ncons && all; ci2++) {
                    int c3 = DATA[dq2].cons[ci2]; Ford *f3 = ford_of(CON[c3].name);
                    if (!f3) { all = false; break; }
                    UState u3 = { 0 }; Node *fv3[64]; const char *fx[64]; int no3 = f3->nfields;
                    if (no3 > 64) { all = false; break; }
                    for (int z = 0; z < no3; z++) { fv3[z] = xfresh("f"); fx[z] = f3->fields[z]; }
                    int np3 = DATA[dq2].nparams, k3 = DATA[dq2].nidx;
                    for (int q3 = 0; q3 < k3 && !u3.clash && !u3.stuck; q3++) { UStep path[16]; u_walk(qt->it[1 + np3 - k3 + q3], subst_multi(f3->idx[q3], fx, fv3, no3), q3, path, 0, fv3, no3, &u3); }
                    if (!u3.clash) all = false;
                }
                if (all) { col = q; dq = dq2; }
            }
            if (col < 0) die("the clauses do not cover every case of", tfname);
            Row *sr = calloc((size_t)DATA[dq].ncons + 1, sizeof(Row));
            for (int ci2 = 0; ci2 < DATA[dq].ncons; ci2++) {
                int c3 = DATA[dq].cons[ci2], nf3 = CON[c3].nfields + CON[c3].ndims;
                sr[ci2].p = calloc((size_t)nv2 + 1, sizeof(Node *));
                for (int q = 0; q < nv2; q++) sr[ci2].p[q] = nsym("_");
                Node *pp = nsym(CON[c3].name);
                if (nf3) { pp = nlist(nf3 + 1); pp->it[0] = nsym(CON[c3].name); for (int z = 0; z < nf3; z++) pp->it[z + 1] = nsym("_"); }
                sr[ci2].p[col] = pp; sr[ci2].rhs = nsym("%unreachable"); sr[ci2].id = -1;
            }
            body = ccm_t(v2, nv2, sr, DATA[dq].ncons);
        } else body = ccm_t(v2, nv2, r2, n2);
        if (plan_rw) {
            bool bad = mentions_name(body, v);
            for (int q = nf - ixk; q < nf; q++) if (mentions_name(body, fv[q]->s)) bad = true;
            if (bad) { nsg = sg0; plan_rw = false; body = ccm_t(v2, nv2, r2, n2); }
        }
        if (us.nsol && !us.clash) {
            /* solved fields: the branch was built with each replaced by its value; peel them one
             * transport at a time, innermost first — wrap w takes the fields that depend on the
             * w-th solved field (in field order, transitively: a dependent telescope) from their
             * types with solutions w.. substituted to their types with only w+1.. substituted,
             * along the path from the value to the field, the equation's argument at the field's
             * position (λ i → proj (e i)) */
            for (int w = 0; w < us.nsol; w++) {
                USol *so = &us.sol[w]; const char *fk = fv[so->k]->s;
                if (mentions_name(G, fk) || mentions_name(G, v)) die("index unification: the goal depends on a solved field (name the equality field and transport by hand) in", tfname);
                bool inz[64] = { 0 }; int Z[64], nz = 0;
                for (int z = 0; z < nfo && z < 64; z++) {
                    if (z == so->k || !fty[z]) continue;
                    bool dep = mentions_name(fty[z], fk);
                    for (int a = 0; a < nz && !dep; a++) if (mentions_name(fty[z], fv[Z[a]]->s)) dep = true;
                    if (dep) { if (mentions_name(G, fv[z]->s)) die("index unification: the goal depends on a field to transport (name the equality field) in", tfname); inz[z] = true; Z[nz++] = z; }
                }
                (void)inz;
                if (!nz) continue;
                /* the substitutions: base = solutions w.. by their values; line = solution w by the
                 * path, w+1.. by their values */
                const char *bx[16], *lx[16]; Node *bv[16], *lvv[16]; int nb = 0, nl = 0;
                Node *e = fv[nfo + so->q], *i = xfresh("i");
                Node *pi = u_case(lst(2, e, i), so->path, so->depth, 0, -1, NULL, so->val, so->val);
                for (int w2 = w; w2 < us.nsol; w2++) {
                    bx[nb] = fv[us.sol[w2].k]->s; bv[nb++] = us.sol[w2].val;
                    lx[nl] = fv[us.sol[w2].k]->s; lvv[nl++] = w2 == w ? pi : us.sol[w2].val;
                }
                Node *line = G, *base = body;
                for (int a = nz - 1; a >= 0; a--) {
                    Node *zt = fty[Z[a]];
                    line = sym4("Pi", fv[Z[a]], subst_multi(zt, lx, lvv, nl), line);
                    base = lam_t(fv[Z[a]], subst_multi(zt, bx, bv, nb), base);
                }
                Node *ap = nlist(nz + 1);
                ap->it[0] = lst(4, nsym("comp"), sym3("lam", i, line), sym3("lam", xfresh("i"), lst(1, nsym("sys"))), base);
                for (int a = 0; a < nz; a++) ap->it[a + 1] = fv[Z[a]];
                body = ap;
            }
        }
        nframes = save_frames; ndeeps = save_deeps;
        /* the field entries are scoping only */
        ncdefs = save_defs;
        body = later_lam(gp, body);
        nsg = sg0;
        if (cov) body = sym3("pair", body, tuple);
        if (dv) {
            const char *vv = dv->v;
            Node *T = sym4("Sigma", xfresh("_"), subst_sym(dv->M->it[2], vv, cv), subst_sym(dv->Bl->it[2], vv, cv));
            body = lam_t(ihv, T, body);
        }
        /* the goal rewritten by the index equations (E28, as Agda's unification at the split): when
         * no clause names them and the motive is over the indices, the branch is checked at the
         * constructor's own indices with refl for the equations, and transported along them to
         * the motive at v — the line λ i → P (e (~ i)) … (c fs (λ j → e (~ i ∨ j)) …), at 0 the
         * constructor's index (e 1) and refl, at 1 the index n and e itself */
        Ford *fdr = ixlam ? ford_of(CON[c].name) : NULL;
        bool mentions_eq = false, rewrote = false;
        if (fdr) for (int q = nf - ixk; q < nf; q++) if (mentions_name(body, fv[q]->s)) mentions_eq = true;
        if (plan_rw && fdr && fdr->nidx == ixk && !eq_named && !mentions_eq && !mentions_name(body, v) && nd == 0 && !CON[c].ndims) {
            Node *i = xfresh("i"), *jd = xfresh("j");
            Node *cvl = nlist(nf + 1); cvl->it[0] = nsym(CON[c].name);
            for (int q = 0; q < nf - ixk; q++) cvl->it[q + 1] = fv[q];
            for (int q = nf - ixk; q < nf; q++) cvl->it[q + 1] = sym3("lam", jd, lst(2, fv[q], sym3("\\/", sym2("~", i), jd)));
            /* the motive's body at those (reduced here: a λ applied is not inferable) */
            const char *xs[65]; Node *vs[65]; Node *L = ixlam; int nx = 0;
            for (int q = nf - ixk; q < nf; q++) { xs[nx] = L->it[1]->s; vs[nx++] = lst(2, fv[q], sym2("~", i)); L = L->it[2]; }
            xs[nx] = L->it[1]->s; vs[nx++] = nf ? cvl : nsym(CON[c].name); L = L->it[2];
            L = subst_multi(L, xs, vs, nx);
            /* a goal not depending on the indices (vlen : Vec A n → Nat) needs no transport */
            if (mentions_name(L, i->s)) { body = lst(4, nsym("comp"), sym3("lam", i, L), sym3("lam", xfresh("i"), lst(1, nsym("sys"))), body); rewrote = true; }
        }
        /* v rebuilt (and what depends on it) where the branch mentions it */
        for (int q = nd - 1; q >= 0; q--) if (mentions_name(body, cdefs[dep[q]].v)) body = sym4("let", nsym(cdefs[dep[q]].v), typed_def(&cdefs[dep[q]]), body);
        if (!rewrote && mentions_name(body, v)) body = sym4("let", nsym(v), typed_def(&vdef), body);
        Node *pat = nlist(nf + 1); pat->it[0] = nsym(CON[c].name);
        for (int q = 0; q < nf; q++) pat->it[q + 1] = fv[q];
        alts[na++] = lst(2, pat, body);
    }
    /* a motive not depending on v: a case */
    bool dep_mot = mentions_name(mot->it[2], v) || cov || dv;
    for (int q = gp + 1; q < tsig.np; q++) if (tsig.rev[q]) dep_mot = true;   /* applied to the reverted: inferred, so with its motive */
    if (motx != mot) dep_mot = true;
    Node *el_ = nlist(na + (dep_mot ? 3 : 2)); int k0 = 0;
    el_->it[k0++] = nsym(dep_mot ? "elim" : "case"); if (dep_mot) el_->it[k0++] = motx; el_->it[k0++] = nsym(v);
    for (int i = 0; i < na; i++) el_->it[k0++] = alts[i];
    if (cov) el_ = sym2("fst", el_);
    if (dv) el_ = lst(2, el_, dv->e);
    return later_app(gp, el_);
}
/* free names of a signature not bound anywhere: its implicit type (and level) parameters */
static bool bindable_free(const char *s) {
    if (e_find(s) || global_lookup(s) >= 0 || con_lookup(s) >= 0 || fld_lookup(s) >= 0 || keyword(s) || constant_sym(s) || reserved(s)) return false;
    if (s[0] == '%' || s[0] == '#' || isdigit((unsigned char)s[0]) || !strcmp(s, "I") || !strcmp(s, "Level") || !strcmp(s, "_")) return false;
    return true;
}
static Names *clk_names;   /* free names standing where a clock does (Tick κ, a data type's Clock parameter) */
static void clock_args(Node *t, Names *bound) {
    if (!clk_names || !t->list || t->n < 2 || t->it[0]->list) return;
    const char *h = t->it[0]->s;
    if (!strcmp(h, "Tick") && t->n == 2 && !t->it[1]->list && !names_has(bound, t->it[1]->s)) { names_add(clk_names, t->it[1]->s); return; }
    int d = data_lookup(h);
    Node *dd = d >= 0 ? (Node *)DATA[d].decl : NULL;
    if (d < 0 && !names_has(bound, h) && sig_of(h)) {
        /* a defined family (G κ): its explicit Clock parameters */
        int q = 1;
        for (Node *x = sig_of(h); x && q < t->n; ) {
            if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { x = x->it[1]; if (x->list && x->n == 4) x = x->it[3]; continue; }
            if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
            if (!x->it[2]->list && is(x->it[2], "Clock") && !t->it[q]->list && !names_has(bound, t->it[q]->s)) names_add(clk_names, t->it[q]->s);
            q++; x = x->it[3];
        }
        return;
    }
    if (!dd || !dd->list || dd->n < 2 || !dd->it[1]->list) return;
    Node *hd = dd->it[1];
    for (int q = 1; q < t->n && q < hd->n; q++) {
        Node *p = hd->it[q];
        if (p->list && p->n == 2 && !p->it[1]->list && is(p->it[1], "Clock") && !t->it[q]->list && !names_has(bound, t->it[q]->s)) names_add(clk_names, t->it[q]->s);
    }
}
static void free_in_sig(Node *t, Names *bound, Names *out, Names *lvl, bool under_U) {
    if (!t->list) { if (!names_has(bound, t->s) && bindable_free(t->s)) { names_add(out, t->s); if (under_U) names_add(lvl, t->s); } return; }
    if (t->n == 0) return;
    clock_args(t, bound);
    if (!t->it[0]->list) {
        const char *k = t->it[0]->s;
        if ((!strcmp(k, "Pi") || !strcmp(k, "Sigma")) && t->n == 4) {
            free_in_sig(t->it[2], bound, out, lvl, false);
            int save = bound->n; names_add(bound, t->it[1]->s); free_in_sig(t->it[3], bound, out, lvl, false); bound->n = save; return;
        }
        if ((!strcmp(k, "lam") || !strcmp(k, "plam") || !strcmp(k, "forall")) && t->n == 3) { int save = bound->n; names_add(bound, t->it[1]->s); free_in_sig(t->it[2], bound, out, lvl, false); bound->n = save; return; }
        if (!strcmp(k, "let") && t->n == 4) { free_in_sig(t->it[2], bound, out, lvl, false); int save = bound->n; names_add(bound, t->it[1]->s); free_in_sig(t->it[3], bound, out, lvl, false); bound->n = save; return; }
        if (!strcmp(k, "U") && t->n == 2) { free_in_sig(t->it[1], bound, out, lvl, true); return; }
        if (!strcmp(k, "%num")) return;
        if (!strcmp(k, "=") && t->n == 3) { free_in_sig(t->it[1], bound, out, lvl, false); return; }
        for (int i = 1; i < t->n; i++) free_in_sig(t->it[i], bound, out, lvl, false);
        return;
    }
    for (int i = 0; i < t->n; i++) free_in_sig(t->it[i], bound, out, lvl, false);
}
static GVar *gvar_of(const char *s) { for (int i = ngvar - 1; i >= 0; i--) if (!strcmp(GVAR[i].name, s)) return &GVAR[i]; return NULL; }
static Node *autobind(Node *sig) {
    Names b = { 0 }, out = { 0 }, lvl = { 0 }, clk = { 0 };
    clk_names = &clk;
    free_in_sig(sig, &b, &out, &lvl, false);
    clk_names = NULL;
    if (ngvar) {
        /* the generalized variables among the free names, with those their types mention, in
         * declaration order (a type mentions only earlier ones), each {x : T} before the rest */
        bool need[256] = { 0 }; bool any = false;
        for (int i = 0; i < out.n; i++) { GVar *g = gvar_of(out.v[i]); if (g) { need[g - GVAR] = true; any = true; } }
        if (any) {
            for (int q = ngvar - 1; q >= 0; q--) if (need[q]) {
                Names tb = { 0 }, tout = { 0 }, tl = { 0 };
                free_in_sig(GVAR[q].type, &tb, &tout, &tl, false);
                for (int i = 0; i < tout.n; i++) { GVar *g = gvar_of(tout.v[i]); if (g && g - GVAR < q) need[g - GVAR] = true; }
                free(tb.v); free(tout.v); free(tl.v);
            }
            for (int q = ngvar - 1; q >= 0; q--) if (need[q]) sig = sym2("%impPi", sym4("Pi", nsym(GVAR[q].name), GVAR[q].type, sig));
            /* the rest of the free names: those not generalized */
            Names out2 = { 0 };
            for (int i = 0; i < out.n; i++) if (!gvar_of(out.v[i])) names_add(&out2, out.v[i]);
            free(out.v); out = out2;
            /* and what the variables' types leave free (a type parameter of a variable's type) */
            Names b2 = { 0 };
            for (int q = 0; q < ngvar; q++) if (need[q]) free_in_sig(GVAR[q].type, &b2, &out, &lvl, false);
            Names out3 = { 0 };
            for (int i = 0; i < out.n; i++) if (!gvar_of(out.v[i])) names_add(&out3, out.v[i]);
            free(out.v); out = out3; free(b2.v);
        }
    }
    for (int i = out.n - 1; i >= 0; i--) if (!names_has(&lvl, out.v[i]) && !names_has(&clk, out.v[i])) sig = sym2("%impPi", sym4("Pi", nsym(out.v[i]), nsym("U"), sig));
    /* clocks before the types (a type may be indexed by a clock) */
    for (int i = out.n - 1; i >= 0; i--) if (names_has(&clk, out.v[i]) && !names_has(&lvl, out.v[i])) sig = sym2("%impPi", sym4("Pi", nsym(out.v[i]), nsym("Clock"), sig));
    free(clk.v);
    for (int i = lvl.n - 1; i >= 0; i--) sig = sym2("%impPi", sym4("Pi", nsym(lvl.v[i]), nsym("Level"), sig));
    free(b.v); free(out.v); free(lvl.v);
    return sig;
}
/* the telescope of a signature: its leading Πs */
static void tele(Node *sig) {
    int n = 0; Node *t = sig;
    for (;;) {
        bool imp = false;
        if (t->list && t->n == 2 && is(t->it[0], "%impPi")) { imp = true; t = t->it[1]; }
        if (!(t->list && t->n == 4 && is(t->it[0], "Pi"))) break;
        n++; t = t->it[3]; (void)imp;
    }
    tsig.np = n; tsig.pn = calloc((size_t)n + 1, sizeof(Node *)); tsig.pty = calloc((size_t)n + 1, sizeof(Node *)); tsig.imp = calloc((size_t)n + 1, sizeof(bool));
    t = sig; n = 0;
    for (;;) {
        bool imp = false;
        if (t->list && t->n == 2 && is(t->it[0], "%impPi")) { imp = true; t = t->it[1]; }
        if (!(t->list && t->n == 4 && is(t->it[0], "Pi"))) break;
        tsig.pn[n] = t->it[1]; tsig.pty[n] = t->it[2]; tsig.imp[n] = imp; n++;
        t = t->it[3];
    }
    tsig.R = t;
}
/* A signature whose result is a defined name applied, the clauses giving more patterns than it has
 * parameters (lx : World Linux St, lx c st = …): the name unfolded while its definition is λs over
 * a Π, and that Π's parameters taken as the definition's. Only where the parameters are found
 * changes: the definition is checked at the signature as written, so a wrong unfolding (a binder
 * deeper in the definition capturing an argument's name) is a failed check, never an accepted one. */
typedef struct { char *name; Node *body; } BodE;
static BodE *BODS; static int nbods; static size_t capbods;
static void body_set(const char *n, Node *b) { GROW(BODS, capbods, (size_t)nbods + 1); BODS[nbods++] = (BodE){ strdup(n), b }; }
static Node *body_of(const char *n) { for (int i = nbods - 1; i >= 0; i--) if (!strcmp(BODS[i].name, n)) return BODS[i].body; return NULL; }
static Node *sig_unfold(Node *R) {
    Node *h = R->list ? (R->n ? R->it[0] : NULL) : R;
    if (!h || h->list) return NULL;
    for (int q = 0; q < tsig.np; q++) if (!strcmp(tsig.pn[q]->s, h->s)) return NULL;
    Node *S = sig_of(h->s), *b = body_of(h->s);
    if (!S || !b || checked_index(h->s) < 0) return NULL;
    const char *ps[64]; Node *vs[64]; int np = 0, i = 1, na = R->list ? R->n : 1;
    for (;;) {
        bool imp = false;
        if (S->list && S->n == 2 && is(S->it[0], "%impPi")) { imp = true; S = S->it[1]; }
        if (!(S->list && S->n == 4 && is(S->it[0], "Pi"))) break;
        if (!(b->list && b->n == 3 && !b->it[0]->list && (is(b->it[0], "%lam") || is(b->it[0], "lam")) && !b->it[1]->list)) break;
        if (np == 64) return NULL;
        Node *given = i < na ? R->it[i] : NULL;
        bool gimp = given && given->list && given->n == 2 && !given->it[0]->list && is(given->it[0], "%imp");
        if (imp) { vs[np] = gimp ? given->it[1] : nsym("%hole"); if (gimp) i++; }
        else { if (!given || gimp) break; vs[np] = given; i++; }
        ps[np++] = b->it[1]->s;
        S = S->it[3]; b = b->it[2];
    }
    if (i != na) return NULL;   /* applied to more or to fewer than its λs */
    /* the leading binders of the unfolding, renamed */
    Node *lead[64]; bool limp[64]; int nl = 0; Node *x = b;
    for (; nl < 64; nl++) {
        bool imp = false; Node *y = x;
        if (y->list && y->n == 2 && is(y->it[0], "%impPi")) { imp = true; y = y->it[1]; }
        if (!(y->list && y->n == 4 && is(y->it[0], "Pi")) || y->it[1]->list) break;
        Node *bn = y->it[1], *A = y->it[2], *B = y->it[3];
        /* (a name the clauses cannot write: none of their variables, and no argument's, is captured;
         * a parameter all clauses name alike takes that name when the type does not mention it) */
        { Node *f = xfresh("u"); B = subst_sym(B, bn->s, f); bn = f; }
        lead[nl] = sym3("Pi", bn, A); limp[nl] = imp; x = B;
    }
    for (int k = nl - 1; k >= 0; k--) { x = sym4("Pi", lead[k]->it[1], lead[k]->it[2], x); if (limp[k]) x = sym2("%impPi", x); }
    return np ? subst_multi(x, ps, vs, np) : x;
}
static void tele_more(Lhs *ls, int ncl) {
    /* only when a clause matches past the signature's parameters by a constructor or a numeral
     * (patterns that are all variables are the result's own λs, as before) */
    {
        int have = 0; for (int q = 0; q < tsig.np; q++) if (!tsig.imp[q]) have++;
        bool con = false;
        for (int c = 0; c < ncl; c++) { int e = 0; for (int k = 0; k < ls[c].np; k++) { Node *pt = ls[c].ps[k]; if (pt->list && pt->n == 2 && is(pt->it[0], "%imp")) continue; if (e++ >= have && (pt->list || pat_con(pt) >= 0)) con = true; } }
        if (!con) return;
    }
    int ne = 0; for (int k = 0; k < ls[0].np; k++) if (!(ls[0].ps[k]->list && ls[0].ps[k]->n == 2 && is(ls[0].ps[k]->it[0], "%imp"))) ne++;
    for (int round = 0; round < 16; round++) {
        int have = 0; for (int q = 0; q < tsig.np; q++) if (!tsig.imp[q]) have++;
        if (ne <= have) return;
        Node *t = sig_unfold(tsig.R);
        if (!t) return;
        int add = 0; Node *x = t;
        for (;;) { if (x->list && x->n == 2 && is(x->it[0], "%impPi")) x = x->it[1]; if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break; add++; x = x->it[3]; }
        int n = tsig.np;
        Node **pn = calloc((size_t)n + add + 1, sizeof(Node *)), **pty = calloc((size_t)n + add + 1, sizeof(Node *)); bool *imp = calloc((size_t)n + add + 1, sizeof(bool));
        for (int q = 0; q < n; q++) { pn[q] = tsig.pn[q]; pty[q] = tsig.pty[q]; imp[q] = tsig.imp[q]; }
        x = t;
        for (;;) {
            bool im = false;
            if (x->list && x->n == 2 && is(x->it[0], "%impPi")) { im = true; x = x->it[1]; }
            if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
            pn[n] = x->it[1]; pty[n] = x->it[2]; imp[n] = im; n++; x = x->it[3];
        }
        tsig.pn = pn; tsig.pty = pty; tsig.imp = imp; tsig.np = n; tsig.R = t;
        { Node *y = t; for (;;) { if (y->list && y->n == 2 && is(y->it[0], "%impPi")) y = y->it[1]; if (!(y->list && y->n == 4 && is(y->it[0], "Pi"))) break; y = y->it[3]; } tsig.R = y; }
    }
}
/* does every call of f in e give, at parameter q, the clause's own variable there? */
static bool uniform_in(Node *e, const char *f, int q, Node *pat) {
    if (!e->list) return strcmp(e->s, f) != 0;
    if (e->n >= 1 && !e->it[0]->list && !strcmp(e->it[0]->s, f)) {
        int i = 1;
        for (int k = 0; k < tsig.np; k++) {
            if (tsig.imp[k]) { if (i < e->n && imp_at(e->it[i], tsig.pn[k])) i++; continue; }
            if (k == q) { if (i >= e->n) return false; Node *a = e->it[i]; if (a->list || pat->list || pat_wild(pat) || strcmp(a->s, pat->s)) return false; }
            i++;
        }
        for (int k = 1; k < e->n; k++) if (!uniform_in(e->it[k], f, q, pat)) return false;
        return true;
    }
    for (int i = 0; i < e->n; i++) if (!uniform_in(e->it[i], f, q, pat)) return false;
    return true;
}
/* the checked definition's body from its clauses */
static Node *clauses_typed(const char *f, Lhs *ls, Node **rhs, int n) {
    tfname = f;
    tsig.ord = NULL; perm_np = 0;
    /* the patterns at the parameters: implicit ones given as {p} or else the parameter itself; the
     * patterns past the parameters are dimensions (the result is a path) */
    int extra = -1;
    /* the clauses' patterns cover the parameters up to the last explicit one they give; the rest is
     * the result */
    {
        int ne = 0; for (int k = 0; k < ls[0].np; k++) if (!(ls[0].ps[k]->list && ls[0].ps[k]->n == 2 && is(ls[0].ps[k]->it[0], "%imp"))) ne++;
        int cut = 0, seen = 0;
        for (int q = 0; q < tsig.np; q++) { if (!tsig.imp[q]) { if (seen == ne) break; seen++; } cut = q + 1; }
        if (seen == ne && cut < tsig.np) {
            Node *R = tsig.R;
            for (int q = tsig.np - 1; q >= cut; q--) { R = sym4("Pi", tsig.pn[q], tsig.pty[q], R); if (tsig.imp[q]) R = sym2("%impPi", R); }
            tsig.np = cut; tsig.R = R;
        }
    }
    Row *rows = calloc((size_t)n + 1, sizeof(Row));
    for (int i = 0; i < n; i++) {
        Node **ps = calloc((size_t)tsig.np + 64, sizeof(Node *)); int k = 0;
        for (int q = 0; q < tsig.np; q++) {
            if (tsig.imp[q]) { if (k < ls[i].np && imp_at(ls[i].ps[k], tsig.pn[q])) ps[q] = pat_norm(ls[i].ps[k++]->it[1]); else ps[q] = tsig.pn[q]; continue; }
            if (k >= ls[i].np) die("a clause with fewer patterns than parameters:", f);
            ps[q] = ls[i].ps[k++];
            if (ps[q]->list && pat_con(ps[q]) < 0 && !(ps[q]->n == 2 && is(ps[q]->it[0], "%num")) && !(ps[q]->n >= 1 && !ps[q]->it[0]->list && is(ps[q]->it[0], "rec"))) die("not a pattern (a variable, _, a numeral or a constructor at patterns), in a clause of", f);
        }
        int ex = ls[i].np - k;
        if (extra < 0) extra = ex; else if (extra != ex) die("clauses with different numbers of patterns:", f);
        for (int q = 0; q < ex; q++) ps[tsig.np + q] = ls[i].ps[k + q];
        rows[i] = (Row){ ps, rhs[i], i };
    }
    if (extra < 0) extra = 0;
    /* a parameter whose patterns are all one variable is named by it, when the signature does not
     * use its name */
    for (int q = 0; q < tsig.np; q++) {
        const char *nm = NULL; bool same = true;
        for (int i = 0; i < n; i++) { Node *p = rows[i].p[q]; if (pat_wild(p)) continue; if (p->list || pat_con(p) >= 0) { same = false; break; } if (!nm) nm = p->s; else if (strcmp(nm, p->s)) same = false; }
        if (!same || !nm || !strcmp(nm, tsig.pn[q]->s)) continue;
        bool used = mentions_name(tsig.R, tsig.pn[q]->s);
        for (int z = q + 1; z < tsig.np; z++) if (mentions_name(tsig.pty[z], tsig.pn[q]->s)) used = true;
        bool taken = false; for (int z = 0; z < tsig.np; z++) if (!strcmp(tsig.pn[z]->s, nm)) taken = true;
        if (!used && !taken && bindable_free(nm)) tsig.pn[q] = nsym(nm);
    }
    /* the dimensions' names */
    Node **dn = calloc((size_t)extra + 1, sizeof(Node *));
    for (int q = 0; q < extra; q++) {
        Node *p = rows[0].p[tsig.np + q];
        dn[q] = !p->list && !pat_wild(p) ? p : xfresh("i");
        for (int i = 0; i < n; i++) { Node *pp = rows[i].p[tsig.np + q]; if (pp->list || pat_con(pp) >= 0) die("a dimension is matched only by a variable in", f); if (!pat_wild(pp) && strcmp(pp->s, dn[q]->s)) rows[i].rhs = x_rename(rows[i].rhs, pp->s, dn[q]->s); rows[i].p[tsig.np + q] = nsym("_"); }
    }
    /* which later parameters a split reverts */
    tsig.rev = calloc((size_t)tsig.np + 1, sizeof(bool));
    for (int q = 0; q < tsig.np; q++) {
        for (int r = 0; r < q; r++) if (mentions_name(tsig.pty[q], tsig.pn[r]->s)) tsig.rev[q] = true;
        if (!tsig.imp[q]) for (int i = 0; i < n && !tsig.rev[q]; i++) if (!uniform_in(rows[i].rhs, f, q, rows[i].p[q])) tsig.rev[q] = true;
    }
    ncdefs = 0; nframes = 0; ndeeps = 0;
    Node **vars = calloc((size_t)tsig.np + 1, sizeof(Node *));
    for (int q = 0; q < tsig.np; q++) vars[q] = tsig.pn[q];
    /* each clause's right side under its dimensions (a path's λs, guessed: a line by default) */
    for (int i = 0; i < n; i++) for (int q = extra - 1; q >= 0; q--) rows[i].rhs = sym3("lam", dn[q], rows[i].rhs);
    cov_on = false; cov_armed = true; guard_on = false;
    guard_q = -1;
    for (int q = 0; q < tsig.np; q++) if (!tsig.pty[q]->list && is(tsig.pty[q], "Clock")) { guard_q = q; break; }
    /* the order as written, for a recursion on a later parameter (perm_p) */
    int np = tsig.np;
    Node **pn0 = tsig.pn, **pty0 = tsig.pty; bool *imp0 = tsig.imp, *rev0 = tsig.rev;
    Node ***rp0 = calloc((size_t)n + 1, sizeof(Node **));
    for (int i = 0; i < n; i++) rp0[i] = rows[i].p;
    perm_p = -1; perm_np = np; perm_pn = pn0; perm_pty = pty0; perm_imp = imp0;
    perm_con = calloc((size_t)np + 1, sizeof(bool));
    for (int q = 0; q < np; q++) for (int i = 0; i < n; i++) if (!pat_wild(rows[i].p[q]) && pat_con(rows[i].p[q]) >= 0) perm_con[q] = true;
    bool *ru_saved = row_used; int rn_saved = row_n;   /* (a clause compiled inside another's) */
    int nsg_in = nsg;
    {
        int how = setjmp(cov_jb);
        if (how == 1) cov_on = true;
        if (how == 2) guard_on = true;
        if (how == 3) {
            /* parameter p moved to stand before every earlier parameter its type does not mention */
            int p = perm_next(perm_p), d = perm_dest(p);
            perm_p = p; cov_on = false;
            int *ord = calloc((size_t)np + 1, sizeof(int));
            tsig.pn = calloc((size_t)np + 1, sizeof(Node *)); tsig.pty = calloc((size_t)np + 1, sizeof(Node *));
            tsig.imp = calloc((size_t)np + 1, sizeof(bool)); tsig.rev = calloc((size_t)np + 1, sizeof(bool));
            for (int k = 0; k < np; k++) ord[k] = k < d || k > p ? k : k == p ? d : k + 1;
            for (int k = 0; k < np; k++) { int q = ord[k]; tsig.pn[q] = pn0[k]; tsig.pty[q] = pty0[k]; tsig.imp[q] = imp0[k]; tsig.rev[q] = rev0[k]; vars[q] = pn0[k]; }
            /* (a moved parameter whose type mentions one now before it is reverted, as written ones are) */
            for (int q = 0; q < np; q++) for (int r = 0; r < q; r++) if (mentions_name(tsig.pty[q], tsig.pn[r]->s)) tsig.rev[q] = true;
            for (int i = 0; i < n; i++) {
                Node **ps = calloc((size_t)np + (size_t)extra + 1, sizeof(Node *));
                for (int k = 0; k < np; k++) ps[ord[k]] = rp0[i][k];
                for (int k = 0; k < extra; k++) ps[np + k] = rp0[i][np + k];
                rows[i].p = ps;
            }
            tsig.ord = ord;
        }
        if (how) { ncdefs = 0; nframes = 0; ndeeps = 0; nsg = nsg_in; }
    }
    rows_track(n);
    Node *body = ccm_t(vars, tsig.np, rows, n);
    rows_check(f); row_used = ru_saved; row_n = rn_saved;
    /* the parameters as written again: the definition's λs, and what the caller reads */
    tsig.pn = pn0; tsig.pty = pty0; tsig.imp = imp0; tsig.ord = NULL; perm_np = 0;
    if (guard_on) {
        /* the fixed point over the parameters after the clock: fix {κ} {Π later → R} (λ %self → λ later → body) later */
        int q0 = guard_q;
        Node *Bty = tsig.R, *inner = body;
        int nl = 0;
        for (int q = tsig.np - 1; q > q0; q--) if (!tsig.imp[q]) { Bty = sym4("Pi", tsig.pn[q], tsig.pty[q], Bty); inner = lam_t(tsig.pn[q], tsig.pty[q], inner); nl++; }
        Node *fx = lst(5, nsym("fix"), sym2("%imp", nsym("%hole")), sym2("%imp", tsig.pn[q0]), sym2("%imp", Bty), sym3("lam", nsym("%self"), inner));
        if (nl) { Node *a = nlist(nl + 1); a->it[0] = fx; int k = 1; for (int q = q0 + 1; q < tsig.np; q++) if (!tsig.imp[q]) a->it[k++] = tsig.pn[q]; fx = a; }
        body = fx;
    }
    cov_armed = cov_on = guard_on = false;
    if (getenv("CUB_SHOWCLAUSES")) { fprintf(stderr, "[clauses] %s = ", f); print_node(stderr, body); fprintf(stderr, "\n"); }
    for (int q = tsig.np - 1; q >= 0; q--) body = lam_t(tsig.pn[q], tsig.pty[q], body);
    return body;
}
/* recursion along a measure. Each call f a.. becomes (%rcall a..) (every parameter given); the
 * clauses compile without recursion; the definition is then recursion on a bound k of the measure:
 *     f ps = (elim (λ k → Π ps → m ps < k → R) (suc (m ps))
 *               (zero → λ ps h → Empty-rec h)
 *               (suc k → λ ps h → body[%rcall as ↦ rec k as (<-step (m as) (m ps) k (L : m as < m ps) h)]))
 *            ps (<-suc-self (m ps))
 * L is the using expression (checked at the call's obligation), unit when none is given (the
 * obligation then computes to Unit) */
static Node *wf_m, *wf_l, *wf_here;   /* the measure (in the parameters' names), the using expression, the measure at this clause's patterns */
static Node *wf_xform(Node *e, const char *f) {
    if (!e->list) { if (!strcmp(e->s, f) && !e_find(e->s)) die("decreasing by: a call of the definition must give its arguments:", f); return e; }
    if (e->n == 0) return e;
    Node *h = e->it[0];
    if (!h->list && !strcmp(h->s, f)) {
        Node *at[256] = { 0 }; int i = 1;
        for (int q = 0; q < tsig.np; q++) {
            if (tsig.imp[q]) { if (i < e->n && e->it[i]->list && e->it[i]->n == 2 && is(e->it[i]->it[0], "%imp")) at[q] = wf_xform(e->it[i++]->it[1], f); else at[q] = tsig.pn[q]; continue; }
            if (i >= e->n) die("decreasing by: a call of the definition must give every argument:", f);
            at[q] = wf_xform(e->it[i++], f);
        }
        /* the arguments, the measure at them and here, and the proof that it decreased: in the
         * clause, so that its matches rebuild what the measure mentions */
        const char *ps[256]; for (int q = 0; q < tsig.np; q++) ps[q] = tsig.pn[q]->s;
        Node *M2 = subst_multi(wf_m, ps, at, tsig.np);
        Node *prf = sym3("the", lst(3, nsym("_<_"), M2, wf_here), wf_l ? wf_l : nsym("unit"));
        Node *c = nlist(tsig.np + 4); c->it[0] = nsym("%rcall");
        for (int q = 0; q < tsig.np; q++) c->it[q + 1] = at[q];
        c->it[tsig.np + 1] = M2; c->it[tsig.np + 2] = wf_here; c->it[tsig.np + 3] = prf;
        if (i == e->n) return c;
        Node *r = nlist(e->n - i + 1); r->it[0] = c; for (int k = i; k < e->n; k++) r->it[k - i + 1] = wf_xform(e->it[k], f);
        return r;
    }
    if (!h->list && (is(h, "lam") || is(h, "plam") || is(h, "%lam")) && e->n == 3 && !strcmp(e->it[1]->s, f)) return e;
    Node *r = nlist(e->n);
    for (int i = 0; i < e->n; i++) r->it[i] = wf_xform(e->it[i], f);
    return r;
}
static Node *wf_calls(Node *e, Node *M, Node *L, Node *k, Node *h) {
    if (!e->list || e->n == 0) return e;
    if (!e->it[0]->list && is(e->it[0], "%rcall")) {
        Node *as[256];
        for (int q = 0; q < tsig.np; q++) as[q] = wf_calls(e->it[q + 1], M, L, k, h);
        Node *M2 = wf_calls(e->it[tsig.np + 1], M, L, k, h), *M1 = wf_calls(e->it[tsig.np + 2], M, L, k, h), *prf = wf_calls(e->it[tsig.np + 3], M, L, k, h);
        Node *step = lst(6, nsym("<-step"), M2, M1, k, prf, h);
        Node *r = nlist(tsig.np + 2); r->it[0] = sym2("rec", k);
        for (int q = 0; q < tsig.np; q++) r->it[q + 1] = as[q];
        r->it[tsig.np + 1] = step;
        return r;
    }
    Node *r = nlist(e->n);
    for (int i = 0; i < e->n; i++) r->it[i] = wf_calls(e->it[i], M, L, k, h);
    return r;
}
static Node *clauses_wf(const char *f, Lhs *ls, Node **rhs, int n, Node *m, Node *L) {
    if (global_lookup("<-step") < 0 || global_lookup("_<_") < 0) die("decreasing by needs the order of lib/std.cub (_<_, <-step, <-suc-self), for", f);
    int np0 = tsig.np; Node *orig[256];
    for (int q = 0; q < np0 && q < 256; q++) orig[q] = tsig.pn[q];
    Node **rs2 = calloc((size_t)n + 1, sizeof(Node *));
    wf_m = m; wf_l = L;
    for (int i = 0; i < n; i++) {
        /* the measure at the clause's patterns (a parameter not matched: its name) */
        Node *pt[256]; const char *ps[256]; int k = 0;
        for (int q = 0; q < tsig.np; q++) {
            ps[q] = tsig.pn[q]->s; pt[q] = tsig.pn[q];
            if (tsig.imp[q]) { if (k < ls[i].np && ls[i].ps[k]->list && ls[i].ps[k]->n == 2 && is(ls[i].ps[k]->it[0], "%imp")) { Node *p = ls[i].ps[k++]->it[1]; if (!pat_wild(p)) pt[q] = p; } continue; }
            if (k < ls[i].np) { Node *p = ls[i].ps[k++]; if (!pat_wild(p)) pt[q] = p; }
        }
        wf_here = subst_multi(m, ps, pt, tsig.np);
        rs2[i] = wf_xform(rhs[i], f);
    }
    /* the bound's hypothesis, an implicit last parameter: the clauses' matches refine it */
    Node *k = xfresh("k"), *kb = xfresh("kb"), *h = xfresh("h");
    Node *R0 = tsig.R;
    tsig.pn[np0] = h; tsig.pty[np0] = lst(3, nsym("_<_"), m, lst(2, nsym("suc"), k)); tsig.imp[np0] = true; tsig.np = np0 + 1;
    Node *B = clauses_typed(f, ls, rs2, n);
    if (tsig.np != np0 + 1) die("decreasing by: the clauses must match every parameter, in", f);
    for (int q = 0; q < tsig.np; q++) { if (!(B->list && B->n == 3)) die("internal: decreasing by", f); B = B->it[2]; }
    tsig.np = np0; tsig.R = R0;
    /* the measure in the parameters' names as the clauses named them */
    for (int q = 0; q < tsig.np; q++) if (strcmp(orig[q]->s, tsig.pn[q]->s)) { m = subst_sym(m, orig[q]->s, tsig.pn[q]); B = subst_sym(B, orig[q]->s, tsig.pn[q]); }
    Node *mot = sym4("Pi", h, lst(3, nsym("_<_"), m, kb), tsig.R);
    for (int q = tsig.np - 1; q >= 0; q--) mot = sym4("Pi", tsig.pn[q], tsig.pty[q], mot);
    mot = sym3("%lam", kb, mot);
    Node *zb = sym3("lam", h, lst(2, nsym("Empty-rec"), h));
    for (int q = tsig.np - 1; q >= 0; q--) zb = sym3("lam", tsig.pn[q], zb);
    Node *sb = sym3("lam", h, wf_calls(B, m, L, k, h));
    for (int q = tsig.np - 1; q >= 0; q--) sb = sym3("lam", tsig.pn[q], sb);
    Node *el_ = lst(5, nsym("elim"), mot, sym3("the", nsym("Nat"), lst(2, nsym("suc"), m)), lst(2, lst(1, nsym("zero")), zb), lst(2, lst(2, nsym("suc"), k), sb));
    Node *app = nlist(tsig.np + 2); app->it[0] = el_;
    for (int q = 0; q < tsig.np; q++) app->it[q + 1] = tsig.pn[q];
    app->it[tsig.np + 1] = lst(2, nsym("<-suc-self"), m);
    Node *body = app;
    for (int q = tsig.np - 1; q >= 0; q--) body = lam_t(tsig.pn[q], tsig.pty[q], body);
    return body;
}
/* copattern clauses (f p...).fld = e: λ the parameters (and dimensions), a record of the fields */
static Node *cop_body; static int cop_np;
static Node *copattern_clauses(const char *f, Lhs *ls, Node **rhs, int n, bool typed) {
    int np = ls[0].np;
    for (int i = 0; i < n; i++) { if (!ls[i].field) die("copattern and ordinary clauses mixed in", f); if (ls[i].np != np) die("copattern clauses with different numbers of patterns:", f); }
    Node **vars = calloc((size_t)np + 1, sizeof(Node *));
    int k = 0;
    if (typed) {
        /* the signature's parameters, implicit ones by name; the patterns past them dimensions */
        Node *sg = NULL; (void)sg;
    }
    for (int q = 0; q < np; q++) {
        Node *p = ls[0].ps[q];
        vars[q] = !p->list && !pat_wild(p) && pat_con(p) < 0 ? p : xfresh("a");
    }
    Node *r = nlist(n + 1); r->it[0] = nsym("record");
    for (int i = 0; i < n; i++) {
        Node *e = rhs[i];
        for (int q = 0; q < np; q++) {
            Node *p = ls[i].ps[q];
            if (p->list || pat_con(p) >= 0) die("a copattern clause's patterns are variables, in", f);
            if (!pat_wild(p) && strcmp(p->s, vars[q]->s)) e = x_rename(e, p->s, vars[q]->s);
        }
        r->it[i + 1] = lst(2, ls[i].field, e);
    }
    cop_body = r; cop_np = np; (void)k;
    Node *b = r;
    for (int q = np - 1; q >= 0; q--) b = sym3(typed ? "%cop" : "lam", vars[q], b);
    return b;
}
/* a typed copattern body: the signature's parameters as λs (implicit ones by their names), the
 * patterns past them dimensions (guessed λs) */
static Node *copattern_wrap(Node *b) {
    Node *ps[256]; int n = 0;
    while (b->list && b->n == 3 && is(b->it[0], "%cop")) { if (n < 256) ps[n++] = b->it[1]; b = b->it[2]; }
    int k = 0;
    Node *ex[256]; int ne = 0;
    for (int q = 0; q < tsig.np; q++) {
        if (tsig.imp[q]) continue;
        if (k >= n) die("a copattern clause with fewer patterns than parameters", NULL);
        if (strcmp(ps[k]->s, tsig.pn[q]->s)) b = x_rename(b, ps[k]->s, tsig.pn[q]->s);
        k++;
    }
    for (; k < n; k++) ex[ne++] = ps[k];
    for (int q = ne - 1; q >= 0; q--) b = sym3("lam", ex[q], b);
    for (int q = tsig.np - 1; q >= 0; q--) b = lam_t(tsig.pn[q], tsig.pty[q], b);
    return b;
}
/* a checked definition (or, name NULL, a check): elaborated, checked, its holes and guesses
 * settled by the checker, until the checker's answer is final */
/* An elaboration's rounds make nodes nothing keeps once the definition is settled (each round's
 * terms, the holes' guesses, the checker's verdicts read back): they are made inside an arena mark,
 * the settled type and term copied out (el_copy), and the mark released, so the memory a large
 * definition's elaboration takes is returned before the next one. The tables keyed by nodes made
 * during the elaboration (hole tables, λ-head reductions, case motives, tick wraps, λ types) are cut
 * back to what they held before it. */
static Node *el_copy(Node *t) {
    Node *x = calloc(1, sizeof(Node)); x->list = t->list; x->s = t->s; x->n = t->n; x->cmt = t->cmt;
    if (t->list) { x->it = calloc((size_t)t->n + 1, sizeof(Node *)); for (int i = 0; i < t->n; i++) x->it[i] = el_copy(t->it[i]); }
    return x;
}
static void el_free(Node *t) {
    if (t->list) { for (int i = 0; i < t->n; i++) el_free(t->it[i]); free(t->it); }
    free(t);
}
typedef struct { ArMark m; int nlt, ncm, ntw; bool on; } ElMark;
static ElMark el_mark(void) {
    static int off = -1; if (off < 0) off = getenv("CUBICAL_NOELMARK") != NULL;
    ElMark e = { ar_mark(), nlt_, ncm, ntw, !off }; return e;
}
static void el_release(ElMark e) {
    neh = neg = 0; nbk = 0; nesc = 0;
    if (!e.on) return;
    nlt_ = e.nlt; ncm = e.ncm; ntw = e.ntw;
    ar_release(e.m);
}
/* a definition named as a loaded constructor would be read as the constructor everywhere (the
 * constructor wins name lookup), so its body would never be used: refused */
static void no_ctor_name(const char *name) {
    if (name && con_lookup(name) >= 0) die("a definition may not reuse a constructor's name (the constructor would be read instead):", name);
}
static void x_checked(const char *name, Node *sig, Node *body, bool expect, uint64_t mask, int mlen) {
    no_ctor_name(name);
    neh = neg = 0; nbk = 0;
    /* its own calls (a copattern definition's) take implicit arguments as later uses do */
    if (name && mask) imask_set(name, mask, mlen);
    if (name && mask) imask_inst(name, sig);
    if (name) sig_set(name, sig);
    if (name) body_set(name, body);
    if (name && pending_instance) { inst_add(name, sig); pending_instance = false; }
    if (name) {
        Node *h = body;
        for (;;) {
            if (h->list && h->n >= 3 && !h->it[0]->list && (is(h->it[0], "lam") || is(h->it[0], "%lam"))) { h = h->it[h->n - 1]; continue; }
            if (h->list && h->n > 0 && h->it[0]->list) { h = h->it[0]; continue; }
            break;
        }
        if (h->list && h->n > 0 && !h->it[0]->list && (is(h->it[0], "elim") || is(h->it[0], "case") || is(h->it[0], "if") || is(h->it[0], "%clauses")))
            { GROW(COMPUTING, capcomputing, (size_t)ncomputing + 1); COMPUTING[ncomputing++] = strdup(name); }
        if (getenv("CUB_DBGCOMP")) { fprintf(stderr, "%s: %s\n", name, is_computing(name) ? "computing" : "rigid"); if (h->list && h->n && !h->it[0]->list) fprintf(stderr, "  head %s\n", h->it[0]->s); }
    }
    Node *T = NULL, *t = NULL;
    ElMark em = el_mark();
    /* an elaboration that cannot go on (a recursive call needed now, ...): the definition rejected */
    jmp_buf ejb;
    jmp_buf *outer_jb = die_jb;   /* --serve's way out, restored after this check */
    int outer_depth = toplevel_depth;   /* (a form left half done: the sweep's nesting as it was) */
    if (setjmp(ejb)) {
        die_jb = outer_jb; toplevel_depth = outer_depth;
        nesc = 0; nwtick = 0; syn_expect = app_expect = NULL;
        bool ok = !expect;
        if (ok) passes++; else failures++;
        printf("%s: %s %s\n   reason: %s\n", ok ? "ok" : "FAIL", expect ? "checks" : "rejected", name ? name : "(check)", die_msg);
        el_release(em);
        return;
    }
    for (int round = 0; ; round++) {
        die_jb = &ejb;
        nesc = 0; T = core(el(sig));
        nesc = 0; syn_expect = sig; t = core(el(body)); syn_expect = NULL;
        die_jb = outer_jb;
        if (trusted) break;
        if (getenv("CUBICAL_SHOW_ELAB") && !round) { fprintf(stderr, "[elab] %s : ", name ? name : "(check)"); print_node(stderr, T); fprintf(stderr, "\n  = "); print_node(stderr, t); fprintf(stderr, "\n"); }
        Node *dT = ds(T), *dt = ds(t);
        /* a definition the store holds as checked (F34): its key found, the checker is not run */
        if (name && expect && !copattern_body(dt) && !has_hole(T) && !has_hole(t)) {
            char key[65]; key_def(ds(strip_marks(T)), ds(strip_marks(t)), key, NULL, NULL);   /* as the definition will be recorded */
            if (store_checked(key)) { forced_result = strdup("ok"); forced_steps = -1; break; }
        }
        Node *q;
        if (name && copattern_body(dt)) {
            /* a copattern definition: its body against its type, its own calls a variable */
            Node *T2 = sym4("Pi", nsym("%self"), T, T), *t2 = sym3("lam", nsym("%self"), x_rename(t, name, "%self"));
            q = nl3("%%chk-codef-dry", nl2("%raw", aclosed(ds(T2))), nl2("%raw", aclosed(ds(t2))));
        } else q = nl3("%%chk-def", nl2("%raw", aclosed(dT)), nl2("%raw", aclosed(dt)));
        long steps = 0;
        char *r = readback(q, &steps);
        const char *saved = src; src = r; Node *rn = read_node(); src = saved;
        bool again = false;
        if (rn && rn->list && rn->n == 2 && is(rn->it[0], "elab")) {
            Node *xs[256]; int nx = cn_list(rn->it[1], xs, 256);
            for (int i = 0; i < nx; i++) {
                Node *pr_ = xs[i];
                if (!pr_->list || pr_->n != 3 || !is(pr_->it[0], "pair")) continue;
                long k = cn_word(pr_->it[1]);
                Node *o = pr_->it[2];
                if (k < 0 || k >= neh || EH[k].sol || !o->list || o->n != 2 || !is(o->it[0], "some")) { if (getenv("CUBICAL_SHOW_ELAB")) fprintf(stderr, "[elab] hole #%ld: %s\n", k, k < 0 || k >= neh ? "no such hole" : EH[k].sol ? "solved already" : "no solution"); continue; }
                ncn = 0; Node *sol = cn(o->it[1]);
                if (!sol && getenv("CUBICAL_SHOW_ELAB")) { fprintf(stderr, "[elab] hole #%ld: solution not readable: ", k); print_node(stderr, o->it[1]); fputc('\n', stderr); }
                if (sol) { EH[k].sol = sol; again = true; }
            }
            if (!again) { if (getenv("CUBICAL_SHOW_ELAB")) fprintf(stderr, "[elab] no usable solution in %.2000s\n", r); free(r); r = strdup("(no opq #32 opq opq)"); }
        } else if (rn && rn->list && rn->n == 2 && is(rn->it[0], "redo")) {
            long k = cn_word(rn->it[1]);
            if (k >= 0 && k < neg && round < 200) { EG[k].kind ^= 1; again = true; }
        }
        if (!again && strcmp(r, "ok") && round < 400) {
            /* an instance candidate under trial may be the fault: the latest is given up (E28) */
            for (int k = neh - 1; k >= 0; k--) if (EH[k].inst && EH[k].tent && EH[k].sol) { EH[k].tried++; EH[k].sol = NULL; EH[k].tent = false; again = true; break; }
        }
        if (again && round < 400) { free(r); continue; }
        forced_result = r; forced_steps = steps;
        if (name && copattern_body(dt) && !strcmp(r, "ok")) { free(r); forced_result = NULL; }
        break;
    }
    /* settled: the elaboration's marks removed */
    if (!has_hole(T) && !has_hole(t)) { T = strip_marks(T); t = strip_marks(t); }
    if (getenv("CUBICAL_SHOW_ELAB")) { fprintf(stderr, "%s : ", name ? name : "(check)"); print_node(stderr, T); fprintf(stderr, "\n  = "); print_node(stderr, t); fprintf(stderr, "\n"); }
    if (em.on) { T = el_copy(T); t = el_copy(t); }
    el_release(em);
    bool kept = x_collecting;   /* a collected form is read later: its type and term stay */
    if (name) x_top(lst(4, nsym(expect ? "checked" : "checked-fail"), nsym(name), T, t));
    else x_top(lst(3, nsym(expect ? "check" : "check-fail"), T, t));
    /* the copies are the checked form's, which keeps none of them once handled (its tables take
     * their own copies, perm_copy): freed (CUBICAL_NOELFREE keeps them, to compare) */
    { static int keepc = -1; if (keepc < 0) keepc = getenv("CUBICAL_NOELFREE") != NULL;
      if (em.on && !kept && !keepc) { el_free(T); el_free(t); } }
    if (forced_result) { free((char *)forced_result); forced_result = NULL; }
}

/* a record: R params = (f1 : A1) × ... × An, and each field's projection */
static void x_record(const char *R, Node **ps, int np, Node **fn, Node **ft, int nf, bool expect) {
    if (!nf) die("a record needs a field:", R);
    { GROW(RECS, caprecs, (size_t)nrecs + 1); RecInfo *ri = &RECS[nrecs++]; ri->name = strdup(R); ri->nf = nf; ri->fn = calloc((size_t)nf, sizeof(char *)); for (int k = 0; k < nf; k++) ri->fn[k] = strdup(fn[k]->s); }
    Node *S = ft[nf - 1];
    for (int i = nf - 2; i >= 0; i--) S = sym4("Sigma", fn[i], ft[i], S);
    Node *ty = nsym("U"), *body = S;
    for (int i = np - 1; i >= 0; i--) { ty = sym4("Pi", ps[i]->it[0], ps[i]->it[1], ty); body = sym3("%lam", ps[i]->it[0], body); }
    tsig.np = 0;
    x_checked(R, ty, body, expect, 0, np);
    /* the projections: {params} (r : R params) → Ak, the earlier fields r's own */
    Node *Rp = nsym(R);
    if (np) { Rp = nlist(np + 1); Rp->it[0] = nsym(R); for (int i = 0; i < np; i++) Rp->it[i + 1] = ps[i]->it[0]; }
    for (int k = 0; k < nf; k++) {
        Node *r = nsym("%r");
        Node *acc = r;
        for (int j = 0; j < k; j++) acc = sym2("snd", acc);
        Node *proj = k == nf - 1 ? acc : sym2("fst", acc);
        Node *A = ft[k];
        for (int j = 0; j < k; j++) {
            Node *pj = r; for (int q = 0; q < j; q++) pj = sym2("snd", pj);
            pj = j == nf - 1 ? pj : sym2("fst", pj);
            A = subst_sym(A, fn[j]->s, pj);
        }
        Node *pty = sym4("Pi", r, Rp, A), *pb = sym3("%lam", r, proj);
        uint64_t mask = 0;
        for (int i = np - 1; i >= 0; i--) { pty = sym2("%impPi", sym4("Pi", ps[i]->it[0], ps[i]->it[1], pty)); pb = sym3("%lam", ps[i]->it[0], pb); mask |= 1ull << i; }
        tsig.np = 0;
        x_checked(fn[k]->s, pty, pb, expect, mask, np + 1);
    }
}
static void el_once(Node *sig, Node *body, Node **T, Node **t) { neh = neg = 0; nbk = 0; nesc = 0; *T = core(el(sig)); nesc = 0; *t = core(el(body)); }
/* foreign name "symbol" : T (D27): see p_decl */
static void x_foreign(const char *nm, Tok *st, Node *sig, Tok *nt) {
    Node *sg = autobind(resolve_keep(sig)); tele(sg);
    if (tsig.np && tsig.imp[0]) x_sem("a foreign function's type has no implicit arguments:", nt);
    neh = neg = 0; nbk = 0; nesc = 0;
    Node *T = strip_marks(core(el(sg)));
    unsigned char kind[8]; int n = 0; Node *xs[8]; int nx = 0;
    Node *x = T;
    unsigned char passed[8];
    for (;;) {
        if (!(x->list && x->n == 4 && is(x->it[0], "Pi"))) break;
        Node *xn = x->it[1], *A = x->it[2];
        int m = bnd_mult(xn);
        if (nx == 8) x_sem("foreign: at most 8 arguments, in", nt);
        passed[nx] = m != 0;
        if (m != 0) {
            int k = -1;
            if (!A->list) k = !strcmp(A->s, "Int64") ? PT_I64 : !strcmp(A->s, "UInt32") ? PT_U32 : !strcmp(A->s, "Float64") ? PT_F64 : !strcmp(A->s, "Word") || !strcmp(A->s, "UInt64") ? 4 : -1;
            else if (A->n == 2 && is(A->it[0], "Bytes")) k = PT_BYTES;
            if (k < 0) x_sem("foreign: an argument's type is Int64, UInt32, Float64, Word or Bytes n (or at multiplicity 0), in", nt);
            kind[n++] = (unsigned char)k;
        }
        xs[nx++] = xn;
        x = x->it[3];
    }
    int ret = !x->list ? (!strcmp(x->s, "Int64") ? PT_I64 : !strcmp(x->s, "UInt32") ? PT_U32 : !strcmp(x->s, "Float64") ? PT_F64 : !strcmp(x->s, "Word") || !strcmp(x->s, "UInt64") ? 4 : -1) : -1;
    if (ret < 0) x_sem("foreign: the result type is Int64, UInt32, Float64 or Word, in", nt);
    if (n < 1 || n > 6) x_sem("foreign: one to six arguments passed, in", nt);
    bool dbl = ret == PT_F64; for (int i = 0; i < n; i++) if ((kind[i] == PT_F64) != dbl) x_sem("foreign: every argument and the result are doubles, or none is, in", nt);
    if (dbl && n > 4) x_sem("foreign: at most four double arguments, in", nt);
    char *sym = strdup(st->s[0] == '"' ? st->s + 1 : st->s); size_t sl = strlen(sym); if (sl && sym[sl - 1] == '"') sym[sl - 1] = 0;
    int k = ffi_add(sym, n, kind, (unsigned char)ret);
    /* the computation: λ over every argument, the call over the passed ones */
    Node *call = nlist(n + 2); call->it[0] = nsym("%ffi"); call->it[1] = wordn(k);
    { int j = 2; for (int i = 0; i < nx; i++) if (passed[i]) call->it[j++] = xs[i]; }
    Node *body = call;
    for (int i = nx - 1; i >= 0; i--) body = sym3("lam", xs[i], body);
    sig_set(nm, sg);
    x_top(lst(4, nsym("foreign"), nsym(nm), T, body));
}
static void x_primitive(const char *nm, bool tick, Node *sig, Node *b, Tok *nt) {
    Node *sg = autobind(resolve_keep(sig)); tele(sg);
    uint64_t mask = 0; for (int q = 0; q < tsig.np && q < 64; q++) if (tsig.imp[q]) mask |= 1ull << q;
    if (mask) imask_set(nm, mask, tsig.np);
    sig_set(nm, sg);
    neh = neg = 0; nbk = 0; nesc = 0;
    Node *T = core(el(sg));
    if (has_hole(T)) x_sem("a primitive's type gives every argument (no implicit argument left to find), in", nt);
    T = strip_marks(T);
    if (b) x_top(lst(4, nsym(tick ? "primitive-tick" : "primitive"), nsym(nm), T, b));
    else { if (tick) x_sem("a tick primitive needs its computation", nt); x_top(lst(3, nsym("primitive"), nsym(nm), T)); }
}
/* a term to read (#show, #same) in a checked file: elaborated, its implicit arguments found by
 * inferring its type */
static bool has_hole(Node *t) { if (!t->list) return false; if (t->n >= 1 && is(t->it[0], "%hole")) return true; for (int i = 0; i < t->n; i++) if (has_hole(t->it[i])) return true; return false; }
static Node *strip_marks(Node *t) {
    if (!t->list || t->n == 0) return t;
    if (is(t->it[0], "%spine")) return strip_marks(t->it[1]);
    if (is(t->it[0], "%guess")) return strip_marks(t->it[3]);
    Node *r = nlist(t->n); for (int i = 0; i < t->n; i++) r->it[i] = strip_marks(t->it[i]);
    return r;
}
/* does t name a definition the checker does not know (an unchecked one)? */
static bool has_unchecked(Node *t) {
    if (!t->list) { if (e_find(t->s) || con_lookup(t->s) >= 0 || keyword(t->s) || constant_sym(t->s)) return false; long g = global_lookup(t->s); return g >= 0 && global_checked(g) < 0 && data_lookup(t->s) < 0 && fld_lookup(t->s) < 0; }
    for (int i = 0; i < t->n; i++) if (has_unchecked(t->it[i])) return true;
    return false;
}
static Node *x_solve_expr(Node *e) {
    if (!x_typed) return core(e);
    neh = neg = 0; nbk = 0;
    Node *t = NULL;
    for (int round = 0; round < 400; round++) {
        nesc = 0; t = core(el(e));
        if (!has_hole(t) || has_unchecked(t)) break;
        long steps;
        char *r = readback(nl2("%%infer-ok", nl2("%raw", aclosed(ds(t)))), &steps);
        const char *saved = src; src = r; Node *rn = read_node(); src = saved;
        bool again = false;
        if (rn && rn->list && rn->n == 2 && is(rn->it[0], "elab")) {
            Node *xs[256]; int nx = cn_list(rn->it[1], xs, 256);
            for (int i = 0; i < nx; i++) {
                Node *p = xs[i]; if (!p->list || p->n != 3) continue;
                long k = cn_word(p->it[1]); Node *o = p->it[2];
                if (k < 0 || k >= neh || EH[k].sol || !o->list || o->n != 2 || !is(o->it[0], "some")) { if (getenv("CUBICAL_SHOW_ELAB")) fprintf(stderr, "[elab] hole #%ld: %s\n", k, k < 0 || k >= neh ? "no such hole" : EH[k].sol ? "solved already" : "no solution"); continue; }
                ncn = 0; Node *sol = cn(o->it[1]);
                if (!sol && getenv("CUBICAL_SHOW_ELAB")) { fprintf(stderr, "[elab] hole #%ld: solution not readable: ", k); print_node(stderr, o->it[1]); fputc('\n', stderr); } if (sol) { EH[k].sol = sol; again = true; }
            }
        } else if (rn && rn->list && rn->n == 2 && is(rn->it[0], "redo")) {
            long k = cn_word(rn->it[1]); if (k >= 0 && k < neg) { EG[k].kind ^= 1; again = true; }
        }
        free(r);
        if (!again) break;
    }
    return strip_marks(t);
}

/* a declaration left by an error (p_decl): the elaborator's state as before a declaration */
static void decl_reset(void) {
    neh = neg = 0; nbk = 0; nesc = 0; nwtick = 0; syn_expect = app_expect = NULL;
    cov_armed = cov_on = guard_on = false; tsig.ord = NULL; perm_np = 0; nsg = 0;
}
