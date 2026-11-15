/* Layer: core: the machine implementing the primitive language (the four layers: see the language-implementation-remainder node of task-graph/hyperactivity.cub). */
/* core.h — the cubical interaction net.
 *
 * A term is a net of cells. Every cell has one principal port (port 0).
 * Values (V_*) have a positive principal port: they provide. Operations
 * (O_*) have a negative principal port: they consume one value there. An
 * active pair is an operation's principal port wired to a value's principal
 * port; its rule replaces the two cells by a net on their auxiliary wires.
 * Sharing is the duplicator O_DUP; erasure is O_ERA; substitution of
 * dimensions is O_SUBST. Intervals and faces are values carrying immutable
 * DNF payloads; a face clause is a substitution of endpoints, so being under
 * a face is reindexing and no cell consults an ambient context.
 */
#ifndef CORE_H
#define CORE_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* One thread (no workers): nothing is contended, so reference counts and statistics are plain
 * arithmetic and the net takes no locks. Set once, before any rewriting (net_start). */
extern bool net_solo;
static inline void rc_inc(uint32_t *p) { if (net_solo) (*p)++; else __atomic_fetch_add(p, 1, __ATOMIC_RELAXED); }
static inline uint32_t rc_dec(uint32_t *p) { return net_solo ? --*p : __atomic_sub_fetch(p, 1, __ATOMIC_ACQ_REL); }
static inline void rcl_inc(long *p) { if (net_solo) (*p)++; else __atomic_fetch_add(p, 1, __ATOMIC_RELAXED); }
static inline long rcl_dec(long *p) { return net_solo ? --*p : __atomic_sub_fetch(p, 1, __ATOMIC_ACQ_REL); }
static inline void cnt_add(long *p, long v) { if (net_solo) *p += v; else __atomic_fetch_add(p, v, __ATOMIC_RELAXED); }

/* ---------------- dimensions, intervals, faces, substitutions ---------------- */
typedef uint64_t Dim;   /* dimension names: never reused (a 64-bit count is never reached) */
/* a clause: sorted literals. Interval literal = dim<<1 | negated;
 * face literal = dim<<1 | endpoint. */
/* Payloads are immutable and reference counted; functions borrow their
 * arguments and return references their callers own; a cell owns its payload. */
typedef struct { uint32_t rc, n; uint64_t l[]; } Clause;   /* literals: dimension << 1 | polarity */
typedef struct { uint32_t rc, n; Clause *c[]; } Dnf;     /* join of clauses (or a system's clause list) */
typedef struct { uint32_t rc, n; Dim *d; Dnf **v; } Subst;
/* a natural number: s when b is NULL, else b's limbs (src/big.c) */
typedef struct { uint32_t rc, n; uint64_t l[]; } Big;
typedef struct { uint64_t s; Big *b; } WN;
Big *big_retain(Big *b);
void big_release(Big *b);
WN wn_small(uint64_t s);
WN wn_copy(const WN *x);
void wn_drop(WN *x);
bool wn_zero(const WN *x);
int wn_cmp(const WN *x, const WN *y);
WN wn_add(const WN *x, const WN *y);
WN wn_mul(const WN *x, const WN *y); WN wn_shl(const WN *x, uint64_t k); WN wn_shr(const WN *x, uint64_t k); WN wn_bits(const WN *x, const WN *y, int op); void wn_divmod(const WN *x, const WN *y, WN *q, WN *r);
WN wn_sub(const WN *x, const WN *y);
WN wn_succ(const WN *x);
WN wn_parse(const char *s);
void wn_print(FILE *o, const WN *x);
extern long stat_payloads;
Clause *clause_retain(Clause *c);
void clause_release(Clause *c);
Dnf *dnf_retain(Dnf *d);
void dnf_release(Dnf *d);
Subst *subst_retain(Subst *s);
void subst_release(Subst *s);
Dnf *clause_list(Clause *const *c, uint32_t n);

Dim  fresh_dim(void);
void dim_named(Dim d, int name);   /* a dimension made for a binder: its source name (src/lang.c) */
Dnf *dnf_zero(void);
Dnf *dnf_one(void);
Dnf *iv_atom(Dim i);
bool dnf_is_zero(const Dnf *x);
bool dnf_is_one(const Dnf *x);
bool dnf_mentions(const Dnf *x, Dim i);
bool clause_mentions(const Clause *c, Dim i);
Dnf *dnf_meet(const Dnf *x, const Dnf *y, bool face);
Dnf *dnf_join(const Dnf *x, const Dnf *y, bool face);
Dnf *iv_neg(const Dnf *x);
Dnf *iv_subst(const Dnf *x, const Subst *s);
Dnf *face_eq(const Dnf *r, int eps);           /* the face (r = eps) */
Dnf *face_subst(const Dnf *f, const Subst *s);
Dnf *face_of_clause(const Clause *c);
bool dnf_equal(const Dnf *x, const Dnf *y);
Dnf *face_forall(const Dnf *f, Dim i);
Dnf *face_neg(const Dnf *f);
Dnf *face_of_clauses(const Dnf *list);
Subst *subst_single(Dim i, const Dnf *r);
Subst *subst_of_clause(const Clause *c);
Subst *subst_then_clause(const Subst *s, const Clause *d); /* x ↦ s(x)[d], then d */
bool  subst_range_mentions(const Subst *s, Dim j);
Subst *subst_without(const Subst *s, Dim j);
Subst *subst_with(const Subst *s, Dim j, const Dnf *r);
Subst *subst_compose(const Subst *s, const Subst *t); /* s, then t */
Subst *subst_id(void);
const Dnf *subst_get(const Subst *s, Dim j);
typedef const char *(*DimName)(Dim);
void dnf_print(FILE *o, const Dnf *x, bool face, DimName nm);
void clause_print(FILE *o, const Clause *c, DimName nm);

/* ---------------- cells ---------------- */
enum {
    /* values: positive principal port */
    V_LAM = 1,  /* p=Record; ports 1.. = environment */
    V_PAIR,     /* 1=a 2=b */
    V_PI,       /* 1=A 2=B (a λ) */
    V_SIGMA,    /* 1=A 2=B (a λ) */
    V_U,        /* 1=level */
    V_PATHP,    /* 1=A (a line) 2=a 3=b */
    V_PLAM,     /* a=dim; 1=body */
    V_IV,       /* p=Dnf (interval) */
    V_FACE,     /* p=Dnf (face) */
    V_SYS,      /* p=Dnf (the branches' clauses, in order; NULL when none); ports 1.. = branch values */
    V_GLUE,     /* 1=A 2=S (a SYS of (T,e)) */
    V_GLUEV,    /* 1=S (a SYS) 2=a */
    /* neutrals: the last port is the neutral's type (V_UNK when unknown);
     * ty=1 when it is known */
    V_NVAR,     /* a=name; 1=type */
    V_NAPP,     /* 1=f 2=arg 3=type */
    V_NFST, V_NSND, /* 1=p 2=type */
    V_NPAPP,    /* p=Dnf r, x=annotated; 1=p [2=a 3=b] type */
    V_NCOMP,    /* 1=A 2=u 3=a0 4=type */
    V_NUNGLUE,  /* 1=S 2=b 3=type */
    V_NCASE,    /* p=CaseTab, x=has motive; 1=scrutinee [2=motive] environment, type */
    V_NTRANS,   /* a=data type, p=Dnf ψ; 1=x 2..k+1=parameter lines, type */
    V_NPROJ,    /* a=field; 1=x 2=type: a field of a neutral of a codata type */
    V_GLOBAL,   /* a=global id */
    V_UNK,      /* an unknown type */
    V_DATA,     /* a=data type; 1..k=parameters */
    V_CON,      /* a=constructor; 1..f=fields, then the dimensions (interval values) */
    V_HCOMP,    /* formal homogeneous composition in a data type: 1=tube 2=cap */
    V_TRANSH,   /* a formal transport of a formal composition: a=data type, p=Dnf ψ; 1=tube 2=cap 3..k+2=parameter lines (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”) */
    V_CASEF,    /* p=CaseTab, x=has motive; [1=motive] environment: the elimination as a function */
    V_REF,      /* p=Box: a reference to a value held once; copying it copies the reference, an operation
                 * meeting it copies the value out (as a definition is unfolded) */
    V_WORD,     /* a natural number in one cell: a=it, or p=Big when it does not fit in 64 bits */
    V_WSUSP,    /* 1=z 2=s 3=a word: the elimination of that word, computed when used (a hypothesis) */
    V_WTY,      /* the type of words */
    V_PTY,      /* a primitive type (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.2 “B12 — Representations as a construct”): a=PT_*; Bytes has 1=its length */
    V_PVAL,     /* a primitive value: x=PT_* (I64, U32, F64, F32), a=the bits */
    V_BYTES,    /* a byte string: p=Bytes (refcounted), x=PT_BYTES */
    V_THUNK,    /* p=Record (no parameter); 1..=environment: a subterm not yet built; an operation meeting
                 * it builds it, a duplicator holds it once (built in place on first read) */
    V_WPLUS,    /* a=k ≥ 1 (or p=Big); 1=a neutral word x: k successors of x */
    V_NWOP,     /* x=the operation (O_WEQ..O_WSHR); 1=a 2=b: a word operation stuck on a neutral */
    V_NWELIM,   /* 1=w 2=z 3=s: the elimination of a neutral word */
    V_NPRIM,    /* x=(class<<8)|sub: a primitive operation (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.2 “B12 — Representations as a construct”) stuck on a neutral: 1..n=its arguments in order.
                 * class 1=%op1 (sub PO_*), 2=%op2 (sub PO_*), 3=load (sub width), 4=store (sub width), 5=blen, 6=bytes */
    V_SUSP,     /* p=CaseTab, x=has motive; 1=field [2=motive] environment: an induction hypothesis, the
                 * elimination of a recursive field, computed when forced */
    V_REC,      /* a=codata type; 1..n=fields (held unbuilt): a copattern definition's body, met only by
                 * the projection that unfolded it (and by the checker, at its own definition) */
    V_DEFER,    /* ty=an operation's kind, x, a, p its own; 1..=its inputs (its ports 0..): an operation not yet
                 * run, an unbuilt value (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): a reader runs it (defer_build), a duplicator holds it once, a
                 * substitution waits on a reference to it. Made where a rule would otherwise build one
                 * operation per branch of a system (distribute, a composition over a system of families) */
    V_END,
    /* operations: negative principal port; the last port is the result */
    O_APP,      /* 0=f 1=arg 2=out */
    O_FORCE,    /* 0=thunk 1=out */
    O_FST, O_SND, /* 0=p 1=out */
    O_PAPPR,    /* x=annotated; 0=r 1=p [2=a 3=b] out */
    O_PAPPP,    /* p=Dnf r, x=annotated; 0=p [1=a 2=b] out */
    O_SUBST,    /* p=Subst; 0=value 1=out */
    O_DUP,      /* 0=value 1=out 2=out */
    O_ERA,      /* 0=value */
    O_COMP,     /* 0=u 1=A 2=a0 3=out */
    O_COMPA,    /* 0=A 1=u 2=a0 3=out */
    O_COMPH,    /* a=dim; 0=H 1=u 2=a0 3=out */
    O_COMPW,    /* comp at Word: 0=u (the tube) 1=a0 2=the line 3=out; an empty tube is the base */
    O_MKPLAM,   /* a=dim; 0=body 1=out */
    O_MKGLUE,   /* 0=S 1=A 2=out */
    O_MKGLUEV,  /* 0=S 1=a 2=out */
    O_UNGLUE,   /* 0=S 1=b 2=out */
    O_UNGLUE2,  /* 0=b 1=S 2=out */
    O_FACEOF,   /* 0=s 1=out */
    O_FNEG,     /* 0=face 1=out: the complement */
    O_NEG,      /* 0=r 1=out */
    O_BIN,      /* x=B_*; 0=a 1=b 2=out */
    O_BIN2,     /* x=B_*, p=Dnf a; 0=b 1=out */
    O_FEQ,      /* a=eps; 0=r 1=out */
    O_FORALL,   /* a=dim; 0=φ 1=out */
    O_SYSCONS,  /* 0=φ 1=t (thunk) 2=rest 3=out */
    O_SYSCONS2, /* p=Dnf φ; 0=rest 1=t 2=out */
    O_SYSMAP,   /* 0=s 1=f 2=out */
    O_TPART,    /* a=k; 0=a type 1=out: the type's k-th component */
    O_CONCHK,   /* a=constructor; 0=its boundary's face 1=the boundary 2..=fields, dims; out */
    O_CONW,     /* a=constructor, x=the dimension examined; 0=that dimension's value 1..nf=fields nf+1..=dims; out:
                 * a path constructor waits for its dimensions (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): none constant, no face of its boundary holds */
    O_HCOMP,    /* 0=tube 1=cap [2=F] out: a formal hcomp unless the tube is total; with F (a data type
                 * without path constructors) the composition goes into the fields, F u a, when the
                 * cap and every side of the tube are one constructor */
    O_HCM1,     /* 0=cap 1=tube 2=F 3=out 4=the tube again (to inspect); a: unused */
    O_HCM2,     /* 0=tube (inspected) 1=tube 2=cap 3=F 4=out; a: the cap's constructor */
    O_HCM3,     /* 0=a side at a fresh dimension, then the sides left, tube, cap, F, out; a: as above */
    O_TRANSD1,  /* a=data type; 0=ψ 1..k=parameter lines k+1=x out */
    O_TRANSD2,  /* a=data type, p=Dnf ψ; 0=x 1..k=parameter lines out */
    O_TRANSH,   /* a=data type, p=Dnf ψ: a substituted formal transport, its tube re-decided; 0=tube 1=cap 2..k+1=parameter lines out */
    O_CASE,     /* p=CaseTab, x=has motive; 0=scrutinee [1=motive] environment, out. Meeting a
                 * constructor it builds the selected branch's body: one interaction */
    /* the values as data, for terms about terms (the checker): */
    O_VIEW,     /* 0=value 1=out: the value's head as a View constructor (lib/code.cub), its parts the
                 * value's own ports */
    O_MKVAR,    /* a=count; 0=level (Num) 1=type 2=out: the variable at that level */
    O_MKDIM,    /* a=count; 0=level (Num) 1=out: the dimension at that level */
    O_IEQ,      /* 0=r 1=s 2=out */
    O_IEQB,     /* p=Dnf r; 0=s 1=out: r = s, a Bool */
    O_FEQL,     /* 0=a 1=b 2=out */
    O_FEQLB,    /* p=Dnf a; 0=b 1=out: a = b as faces, a Bool */
    O_CLAUSES,  /* 0=face 1=out: its clauses, each a face, as a list */
    O_DNF,      /* 0=an interval or a face 1=out: its clauses as lists of literals (%dnf) */
    O_MOTIVE,   /* 0=an elimination as a function (V_CASEF) 1=out: (some motive), or none (%motive) */
    O_RESTRICT, /* 0=a clause (a face) 1=value 2=out: the value under the clause */
    O_GLOB,     /* x=table, a=count; 0=index (Num) 1=out: an entry of the checker's tables, a reference */
    O_ABS,      /* 0=a dimension 1=value 2=out: the line binding that dimension */
    O_NTYPE,    /* 0=a neutral 1=out: its type, computed from its spine (only a variable stores one) */
    O_CALL,     /* 0=a definition 1..n=arguments out: a definition applied to its arguments; with all
                 * its parameters, its body is built in one interaction */
    O_WSUCC,    /* 0=w 1=out: w + 1 */
    O_PIMULT,   /* 0=T 1=out: the multiplicity (a word 0, 1, 2) of a Π type's binder; 2 for anything else (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.1 “B11 — Multiplicities (static side)”) */
    O_PIM,      /* 0=m (word) 1=A 2=B 3=out: a Π type with its binder's multiplicity m */
    O_FMULT, O_FMULT2, O_FMULT3,   /* (%fmult d c k): the multiplicity of field k of constructor c of checked data type d; words in turn */
    O_LINEAR,   /* 0=d (word) 1=out: is the checked data type d linear (a constructor with a field at 1)? a Bool */
    O_GSTEPS,   /* (%gsteps n): the steps checked definition n took to check (word; 0 unknown) */
    O_GCODE,    /* (%gcode n): some (the code of checked definition n as written, globals kept as cglob), or none */
    O_DNIDX,    /* (%dnidx d): the number of index parameters (the last ones) of checked data type d (word) */
    O_GUSAGE, O_GUSAGE2,   /* (%gusage n k): the computed usage (word) of parameter k of checked definition n; 2 when unknown */
    O_PTY,      /* (%pty k): the primitive type of kind k (a word): 0=k 1=out */
    O_BYTESTY,  /* (%bytesty n): the type Bytes n: 0=n 1=out */
    O_PVAL,     /* (%pval k b): the primitive value of kind k (word) with bits b (word): 0=k 1=b 2=out */
    O_PVAL2,    /* a=k: 0=b 1=out */
    O_POP1,     /* x=the operation: 0=a 1=out: a unary primitive operation (conversions) */
    O_POP2, O_POP2B,   /* x=the operation: 0=a 1=b 2=out: a binary primitive operation; a read, then b */
    O_BYTES,    /* (%bytes n): n zero bytes: 0=n 1=out */
    O_LOAD, O_LOADB,   /* x=width (1,2,4,8): (%load b off): 0=b 1=off 2=out */
    O_STORE, O_STOREB, O_STOREC,   /* x=width: (%store b off v): a copy of b with v at off: 0=b 1=off 2=v 3=out */
    O_BLEN,     /* (%blen b): 0=b 1=out, the length */
    O_BEQ, O_BEQB,   /* (%beq a b): 0=a 1=b 2=out, true when both are byte strings of equal contents (the checker's conversion); O_BEQB: p=a's bytes, 0=b 1=out */
    O_PTROF,    /* (%ptr-of b): 0=b 1=out, the address of its bytes, a word (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C20; trusted) */
    O_REGAT,    /* (%region-at n a): 0=n 1=a 2=out, the n bytes at address a, copied (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.10 “C20–C22 — Memory and state”, its part C20; trusted) */
    O_REGATA,   /* its second step: 0=a 1=out, a = n */
    O_FFI,      /* (%ffi k a1 .. an): x=the foreign table entry, 0..n-1=arguments, n=out; a=the arguments read so far, p=them (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.11 “D24–D27 — The world”, its part D27; trusted) */
    O_NATIVE,   /* (%native k g a1 .. an): x=the foreign table entry of a #jit definition, a=the definition g itself; 0..n-1=arguments, n=out;
                   p=the argument values read so far. Every argument crossing (by the entry's JT): the bytes are called; otherwise g is applied (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §5.5 “Bytes, the boundary, and what is trusted”) */
    O_WEQ, O_WLT, O_WADD, O_WMAX, O_WSUB, O_WMUL, O_WDIV, O_WMOD, O_WAND, O_WOR, O_WXOR, O_WSHL, O_WSHR,   /* the last eight compute on numerals only (x / 0 = 0, x mod 0 = x) */
     /* 0=a 1=b 2=out: on words; first a, then b. Equality,
                 * order, max and truncated subtraction recurse on both (cancelling successors), addition
                 * on b; a word with a neutral part leaves the operation stuck (V_NWOP) */
    O_WOP2,     /* x=the operation; 0=b 1=a (its value) 2=out */
    O_WELIM,    /* (%welim w z s): 0=w 1=z 2=s 3=out: #0 ↦ z, #(n+1) ↦ s #n (the elimination of #n) */
    O_PEND,     /* p=Dnf r (0 or 1); 0=the type of p 1=p 2=out: p r, an endpoint when the type is a path type */
    O_CLOS,     /* 0=f 1=g 2=out */
    O_CLOS2,    /* 0=g 1=f 2=out: (csame pairs) when f, g are λs of one record, the pairs of their
                 * environments; else (cdiff f g), both untouched */
    O_BRANCH,   /* 0=an elimination (V_CASEF) 1=constructor (Num) 2=parameters (a list) 3=out */
    O_BRANCHC,  /* p=CaseTab, a=count; 0=constructor (Num) 1=parameters, then the environment, out */
    O_BRANCHL,  /* p=CaseTab, a=constructor, x=parameters read; 0=the rest of the list, then the
                 * environment, then the parameters read, out: branch a at the parameters */
    O_FWD,      /* 0=the value 1=where it is held 2=a reader: a read waiting for a held value that has
                 * not arrived; when it arrives the reader gets its one-level copy and the value goes
                 * on to be held. Not counted as a step: it delivers a read already counted */
    O_SHARE,    /* 0=value 1=out 2=out: a part of a read value that had not arrived when it was read;
                 * shared as the read would have shared it (as O_DUP). Not counted as a step */
    O_ROOT,     /* 0: an observation; never interacts */
    O_PROJ,     /* a=field; 0=value 1=out: a field of a codata value. Meeting a stuck call of a copattern
                 * definition it unfolds the call (O_UNFOLD) and takes the field of the body */
    O_UNFOLD,   /* 0=a stuck call of a copattern definition (its head, applications and path
                 * applications) 1=out: the definition's body applied to the same arguments */
    O_DAPPLY,   /* 0=a data type 1=f 2=out: f applied to the data type's parameters */
    O_TICKAPP,  /* 0=an argument 1=a stuck call of a tick-unfolding definition, short of that argument
                 * 2=out: given the tick constant ⋄ the call unfolds (the body applied to its
                 * arguments and ⋄); given anything else it stays a stuck application */
    O_DEEP,     /* 0=a value 1=out: the value with every constructor field below it a value too (a #jit
                 * definition's argument converted by its type, read whole before the call); a constructor's
                 * fields forced in turn through O_DEEPC, anything else delivered as it is */
    O_DEEPC,    /* a=the constructor, p=DeepAcc (the fields' values so far); 0=the next field's value,
                 * 1..=the fields after it (unforced), last=out */
    K_END
};
enum { B_MEET, B_JOIN, B_FAND, B_FOR };

typedef uint64_t Port;            /* cell pointer << 16 | port index; 0 = none */
/* ---------------- keys: terms as names ----------------
 * A key names a term: a compiled body with each free position replaced by the key of the value
 * bound there; a value by its constructor over its parts' keys; a held value (a box) by the key of
 * what was put in it, which a copy out of it, and a value rebuilt from the fields a case took from
 * it, carry too. Keys are interned, so one term is one pointer (compared structurally when made).
 * A cell whose parts all have keys has one; a cell with an unkeyed part has none, and nothing
 * built on it is keyed. */
typedef struct Key {
    uint32_t rc, n;               /* n = children */
    uint64_t h;                   /* hash */
    uint16_t tag;                 /* K_* */
    uint64_t a;                   /* a number, global, constructor, variable name, ... */
    void *p;                      /* a payload: Dnf, Big, Record, CaseTab (by pointer) */
    struct Key *c[];
} Key;
enum { K_WORD = 1, K_IV, K_FACE, K_VAR, K_GLOBAL, K_CON, K_DATA, K_LAM, K_PAIR, K_PI, K_SIGMA, K_PATHP, K_U,
       K_APP, K_FST, K_SND, K_PAPP, K_CASE, K_FORCE, K_SUB, K_SUBST, K_DIM, K_TERM, K_WTY, K_UNFOLD };
Key *key_retain(Key *k);
Key *key_global(long g);
Key *key_con0(int c);              /* a constructor with no fields */
void key_release(Key *k);
extern long stat_hits, stat_keys;  /* readings: memo hits; keys with a shared value made */

/* A port names a principal port (cell, 0), an auxiliary port (cell, k ≥ 1; named only while a cell
 * is wired up), or a variable: a slot standing for a wire between two auxiliary ports, or for a
 * value not yet produced. Each side of a wire links what it has into the slot; the second to arrive
 * finds the first there, links the two directly and frees the slot. A cell's slots hold what its
 * auxiliary ports are wired to: a principal port or a variable. Cells carry no back pointers, so a
 * cell is written only by its maker (its slots, once, before it is reachable), by the rule
 * consuming it, or by a compare-and-swap on a slot of a held value (copy_top); rules take no locks. */
typedef struct Var { Port p; struct Var *next; Key *key; } Var;   /* key: what the wire's producer is named (key_of_port) */
#define VARI 0xffff
static inline bool is_var(Port p) { return (p & 0xffff) == VARI; }
static inline Var *PV(Port p) { return (Var *)(uintptr_t)(p >> 16); }
static inline Port VARP(Var *v) { return ((uint64_t)(uintptr_t)v << 16) | VARI; }
Port net_peek(Port p);            /* what a port leads to: a principal port, or 0 while nothing is there yet */

typedef struct Cell {
    uint16_t kind, n;             /* n = number of ports */
    uint32_t x;
    uint32_t ty;                  /* ty: TY_KNOWN a neutral's type is known, TY_COFIX its head is a copattern definition
                                   * (it unfolds under projection) */
    uint64_t a;
    void *p;
    Key *key;                     /* the term this cell is (a value) or will produce (a thunk, a call, an
                                   * application, a case, a duplicator's outputs), or NULL */
    struct Entry *e;              /* a producer registered as pending on its key (memo), or NULL */
    Port port[];
} Cell;

enum { TY_KNOWN = 1, TY_COFIX = 2, TY_TICK = 4 };   /* TY_TICK: a stuck call of a definition that unfolds at ⋄;
                                                    * its a counts the arguments still before the tick */
static inline Port P(Cell *c, int k) { return ((uint64_t)(uintptr_t)c << 16) | (uint64_t)k; }
static inline Cell *PC(Port p) { return (Cell *)(uintptr_t)(p >> 16); }
static inline int PI(Port p) { return (int)(p & 0xffff); }
static inline bool is_value(int k) { return k > 0 && k < V_END; }

Cell *cell(int kind, int n);
void cell_free(Cell *c);
void link(Port a, Port b);
void net_start(int workers);       /* worker threads besides the caller */
void net_push(Cell *op, Cell *v);  /* enqueue an active pair */
bool net_observe(Cell *root, long limit); /* reduce until root's port holds a value (root NULL: until no pair is pending); workers then pause */
void net_drain(void);
void net_mem(long *slabs, long *keys);
void net_scan_payloads(void (*cellf)(void *), void (*keyf)(void *));   /* every cell's payload pointer to cellf, every key's to keyf (either NULL: skipped; workers parked) */
Cell *global_res(long g);          /* the cell holding a definition's value */

/* data types (higher inductive types, Coquand–Huber–Mörtberg): declared by
 * (data ...); the net's rules are generic over these tables */
typedef struct { char *name; int data, index, nfields, ndims; long bdy, trans; int nrec; uint64_t recmask; unsigned char rdim[64], fmult[64]; const char *rsh[64]; } ConInfo;
/* bdy, trans: globals or -1; recmask: the recursive fields (of type the data type itself, or a path line into it,
 * rdim[k] dimensions deep, or under Π codomains and Σ components), nrec of them. rsh[k]: the shape of a
 * recursive field's type as a string, prefix form: D the data type, P<s> a path line into s, F<s> a function
 * into s, 1<s> a pair whose first component is s, 2<s> whose second is, B<s><t> both (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.17 “E28a — Induction hypotheses for path-typed recursive fields” and §18.18 “E28b — Strict positivity under Π codomains and Σ components”) */
typedef struct { char *name; int nparams, ncons; int *cons; long comp, transh; void *decl; int co, nflds; int *flds; int nidx; } DataInfo;
/* nidx: the last nidx parameters are indices (fording; PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.12 “E28 — Elaboration”): a constructor's recursive field may give them freely */
/* decl: the (data ...) form. co: a codata type (no constructors): its fields flds (indices into FLD);
 * comp its composition, a copattern definition */
typedef struct { char *name; int data, index; long fty; int mult; } FldInfo;
/* primitive types (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.2 “B12 — Representations as a construct”) */
enum { PT_I64, PT_U32, PT_F64, PT_F32, PT_BYTES, PT_U64, PT_N };
typedef struct { long rc; size_t n; unsigned char b[]; } Bytes;
/* the primitive operations of O_POP1/O_POP2, by x */
enum { PO_I64ADD, PO_I64SUB, PO_I64MUL, PO_I64EQ, PO_I64LT, PO_U32ADD, PO_U32SUB, PO_U32MUL, PO_U32EQ, PO_U32LT,
       PO_F64ADD, PO_F64SUB, PO_F64MUL, PO_F64DIV, PO_F64EQ, PO_F64LT,
       /* the modular 64-bit type UInt64 (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §5.3 “The target is a data type with a meaning”), and the operations Int64 lacked; division and
        * modulus are total (x/0 = 0, x%0 = x), shifts take their count mod 64: no operation has an
        * undefined case, in the machine or in the template that prints it */
       PO_U64ADD, PO_U64SUB, PO_U64MUL, PO_U64EQ, PO_U64LT, PO_U64DIV, PO_U64MOD, PO_U64AND, PO_U64OR, PO_U64XOR, PO_U64SHL, PO_U64SHR,
       PO_I64DIV, PO_I64MOD, PO_I64AND, PO_I64OR, PO_I64XOR, PO_I64SHL, PO_I64SHR,
       PO_I64OFW, PO_WOFI64, PO_U32OFW, PO_WOFU32, PO_F64OFW, PO_F64OFI64, PO_I64OFF64, PO_F64BITS, PO_F64OFBITS, PO_F32BITS, PO_F32OFBITS,
       PO_U64OFW, PO_WOFU64, PO_N };   /* fty: λ params x. the field's type at x */
extern FldInfo *FLD;
extern ConInfo *CON;
const char *data_first_con(int d);
extern DataInfo *DATA;
extern long case_hcomp_global;
/* an elimination's branches: one record per constructor, parameters its fields, [the induction
 * hypotheses of its recursive fields when mask has the branch,] its dimensions; environment the
 * variables the branches share, supplied once by the case cell's ports */
/* variables and dimensions at de Bruijn levels, made by %var and %dim and read by %view */
#define LVL_TAG (1ull << 62)
int data_checked_index(int d);
int checked_data(long k);   /* the data type of checked index k, or -1 */
long global_steps(long n);        /* the steps checked definition n took to check; 0 unknown */
int global_usage(long n, long k);   /* the computed usage of parameter k of checked definition n (0, 1, 2); 2 unknown */     /* a data type's index in the checker's signature, or -1 */
int con_index(const char *name);   /* a constructor by name, or -1 */
enum { TB_VAL, TB_TY, TB_DTAB, TB_DENT, TB_TRANS, TB_N };   /* TB_TRANS: a data type's transport as a function */
long table_global(int table, long i);  /* the global holding entry i of a checker table, or -1 */
/* copattern definitions: the body a head unfolds to; a global's index in the checker's signature (or -1) */
long global_unfold(long g);
int global_tickar(long g);         /* a tick-unfolding definition's number of parameters (the last the tick), or 0 */
int global_checked(long g);
Port cofix_head(long g, long tyg);   /* the stuck head of copattern definition g, of type global tyg (or -1) */
/* a definition's body as one record taking all its parameters (NULL when it is not a λ) */
struct Record *global_nary(long g);
/* key: the name of what the box holds (a reference's identity). e: the memo entry whose value this is */
typedef struct FdInfo { uint32_t n; unsigned taint : 1, inexact : 1; Dim d[]; } FdInfo;   /* taint: a record that may make a dimension is reachable; inexact: a bound from pending parts' inputs (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”, its part A8b) */   /* the free dimensions of a box's value, once known */
typedef struct Box { struct Cell *root; long rc; Key *key; struct Entry *e; Port in; FdInfo *fd; int fdtries; } Box;   /* in: the wire its value is built into (memo); fd, fdtries: box_fd */
/* motive: the elimination's motive as a record over the same environment, or NULL. It is not a port:
 * only the two rules that read it (a formal hcomp met, and a stuck elimination typing its neutral)
 * build it, from the environment */
typedef struct CaseTab { int d, nenv; uint64_t mask; struct Record **br; struct Record *motive; } CaseTab;
Port con_build(int c, const Port *fields, const Port *dims);
Port apply_gid(long g, const Port *args, int n);

/* readings */
extern long stat_steps, stat_kind[K_END];
void stat_gather_kinds(void);
void stat_gather(void);
extern long stat_cells;
extern FILE *trace_out;   /* CUBICAL_TRACE: interactions of a (show ...) reading, one per line */

/* ---------------- compiled syntax ---------------- */
enum {
    S_VAR, S_LAM, S_APP, S_PAIR, S_FST, S_SND, S_PI, S_SIGMA, S_U, S_PATHP, S_PLAM, S_PAPP,
    S_IV0, S_IV1, S_NEG, S_BIN, S_FEQ, S_FTT, S_FFF, S_FORALL, S_FACEOF, S_FNEG, S_SYS0, S_SYSCONS,
    S_SYSMAP, S_COMP, S_GLUE, S_GLUEV, S_UNGLUE, S_GLOBAL, S_AXIOM, S_LET, S_FORCE,
    S_DATA, S_CON, S_HCOMPD, S_TRANSD, S_CASE,  /* id: the data type or constructor; S_CASE: tab */
    S_PRIM,     /* x=the operation cell, n operands */
    S_GLOBTAB,  /* x=the table; k[0]=the index */
    S_BOX,      /* k[0]: the value, held once and referred to */
    S_WORD,     /* id: a word literal #n */
    S_WORDTY,   /* the type Word */
    S_THUNK,    /* rec: a subterm held unbuilt (V_THUNK), its environment k[0..n-1] */
    S_CALL,     /* id: the definition; k: its arguments */
    S_PROJ,     /* id: the field; k[0]: the value */
    S_REC       /* id: the codata type; k: the fields */
};
struct Record;
typedef struct Syn {
    int tag, n, slot;
    uint32_t x;
    long id;
    struct Record *rec;
    struct CaseTab *tab;
    Big *big;                       /* S_WORD: the number when it does not fit in id */
    struct Syn **k;
    uint64_t sh;                    /* structural hash, once computed (lang.c, interning records) */
} Syn;   /* S_AXIOM: slot = the global holding its type, or -1 */
/* a binder level: λ (one parameter), thunk (none), or a case branch (its fields, hypotheses and
 * dimensions, in slots 0..nparam-1). Its frame has slots: the parameters, the environment (maximal subterms free of this level,
 * lifted to the enclosing level and wired to the cell's ports), and locals. */
typedef struct Record {
    int nparam, nslots, nenv;
    int *env_slot;
    int *uses;          /* occurrences of each slot in the body */
    Syn *body;
    int where;          /* the definition it was compiled in (CUBICAL_PROFILE) */
    int mk;             /* A8 (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): may building it make a dimension (reach %dim)? 0 unknown, 1 no, 2 yes (record_mkdim) */
    uint64_t h;         /* its interning hash, once computed (kept when the body is dropped) */
    struct RecGone *gone;   /* the body dropped (lang.c's records_sweep): only keys name it; NULL while it has one */
} Record;
int record_mkdim(Record *r);        /* A8 (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.16 “A8 — Proposal: formal transported compositions”): 1 when building r (or anything it calls) may make a fresh free dimension */
int casetab_mkdim(const struct CaseTab *t);
int global_mkdim(long g);
/* profile: applications of λs and forcings of thunks, by the definition they were compiled in */
extern long *prof_count, *prof_dup; extern const char **prof_name; extern int prof_n, prof_on; extern long prof_dupk[64], prof_erak[64], prof_con[4096], prof_glob[65536];

/* build the body of r: param (or 0), env providers; returns the provider */
Port record_build(const Record *r, Port param, const Port *env);
Port record_build_n(const Record *r, const Port *params, const Port *env);
void port_split(Port prov, int m, Port *out);
/* build a top-level body with its own slots */
Port top_build(Syn *body, int nslots, const int *uses);

/* globals: each definition's net is built once; a use copies its value */
long global_axiom_ty(long g);   /* a constant with no computation: its type's global (or -1); else -2 */
Port global_copy(long g, Key *as); /* 0 when the global is an unsupplied input; as: the key the copy carries (the name of the cell that referred to it; NULL: the definition's) */
void global_park(long g, Cell *op, Cell *glob);
/* foreign functions (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.11 “D24–D27 — The world”, its part D27): a C symbol at a primitive type; the arguments' kinds (PT_*, or 4 a word), its result's.
 * The same table holds the #jit entries (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §5.5 “Bytes, the boundary, and what is trusted”): fn the bytes, jt the parameters' and the result's types
 * (n of them, then the result), each argument's kind 4 a word, 5 a primitive value, 8 converted by its type,
 * 9 not read */
#define NATIVE_MAXW 32
/* #jit (PLAN.md revision 8, 9 Nov 2026, git 91167ad638c0:PLAN.md, §5.5 “Bytes, the boundary, and what is trusted”): the type of a compiled definition's parameter or result, read off its checked type,
 * which decides how a value crosses (src/net.c jit_in, jit_read). A data type's fields are found by
 * name in its declaration, at its parameters, when first needed (fl: per constructor, per field) */
typedef struct JT { int k, pt, d, np; struct JT *a, *b; struct JT **ps; struct JT ***fl; } JT;
enum { JT_NO, JT_WORD, JT_PRIM, JT_BYTES, JT_DATA, JT_PAIR, JT_NAT, JT_ERASED };
JT *jt_field(JT *t, int c, int j);   /* lang.c */
typedef struct { const char *sym; void *fn; int n; unsigned char kind[NATIVE_MAXW]; unsigned char ret; struct JT **jt; } Ffi;
uint64_t ffi_invoke(const Ffi *e, const uint64_t *a, int n);
extern Ffi *FFI; extern int nffi;
int ffi_add(const char *sym, int n, const unsigned char *kind, unsigned char ret);
long global_lookup(const char *name);
const char *name_of(long id);

#endif
