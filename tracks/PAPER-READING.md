# The paper, section by section

`docs/Hyperactive_Final_Paper.pdf` (11 November 2026, 69 pages), read in full on 2026-11-11/12 by
a Claude session (text from `pdftotext -layout`, all 3,815 lines). For every section and both
appendices: what it defines and states, its equations and worked examples, and whether the
repository has it (checked against the code at commit 7053a99, 2026-11-11; "absent" means not
found by reading the code and searching for the construction under any name).


## Abstract and §1 The language of computation
Definitions, checking, transformation, interpretation are computations in one language. §1.1: the
judgments Γ ⊢ A : U_ℓ, Γ ⊢ t : A, Γ ⊢ t ≡ u : A, with interval variables and faces; CCHM [9] plus
clocked guarded computation [44]; transport_P(p), ua(e) are operations of the language. §1.2:
Code, eval-g, the checker as a term observing values through %view; the specializer by
normalization and equality-directed rewriting, its results admitted by the same checking; the
initial Code and checker are bootstrap, distinct from the checked family of §6.1; no unrestricted
quotation. §1.3: open computations stay programs; processes and handlers as ordinary types; a
fleet, a message in flight, a database, an OS, a compiler are definitions in example programs,
not primitives.
Repo: the CCHM evaluator with HITs, Glue, clocks (in C); Code and eval-g (untyped bootstrap); the
checker as a term (untyped bootstrap); the specializer. Present.

## §2 Types, witnesses, and identity
Occurrences vs values. Fin(0)=0, Fin(n+1)=1+Fin(n), families v : Fin(n) → A (2.1);
Enumeration(A) = Σ n. A ≃ Fin(n) (2.2); HasCard(A,n) = ‖A ≃ Fin(n)‖ (2.3); Γ ⊢ t : A (2.4).
§2.1 Σ with fst/snd (2.5), Π (2.6), rec_ℕ computation (2.7); propositions vs types; truncation
forgets witnesses. §2.2 a ≤ b := Σc. a+c = b, a < b := Σc. a+suc c = b (2.8), propositions by
cancellation; transitivity by (2.9); Eval(e,v) inductive vs ⟦e⟧ = v related by an equivalence;
Rel(R) = Σa Σb R(a,b) (2.10), equivalences of presentations commute with projections.
Repo: no Enumeration/HasCard/order-as-Σ statements in the paper's form (lin has its own forms).

## §3 Computational equality
refl (3.1), Path with p(0)=a, p(1)=b (3.2), (⟨i⟩t)@r ⟼ t[i:=r] (3.3), transport (3.4);
Sing(a) = Σx. a = x contractible (3.5) via connection i∧j; Graph(f) = Σb Σa f(a) = b (3.6);
A ≃ Graph(f) (3.7), one composite reduces, the other by singleton contraction. §3.1 interval as
free De Morgan algebra, faces, comp_i A [φ ↦ u] a0 : A(1) (3.8). §3.2 definitional vs
propositional vs observational; f(n) = n+n, g(n) = 2n (3.9); funext (3.10); equality says nothing
about energy/time. §3.3 Transition = Σs Σs' T(s,s') (3.11); histories and observations. §3.4 the
judgments Γ;Ψ;φ ⊢ … (3.12), path intro/elim (3.13), comp for every type former (3.14): Π
contravariant transport then pointwise, Σ first then dependent second, Path in its extra
dimension, U and Glue by univalence.
Repo: isContr, funExt, comp rules (C core + untyped prelude comp-* terms). (3.7) Graph
equivalence: absent.

## §4 Equivalence, univalence, and structure
ua : (A ≃ B) → (A = B) (4.1); transport along ua(e) computes e (4.2). Vec(A,n) ≃ (Fin n → A)
(4.3) by index/tabulate with laws (4.4); Sorted : Vec(ℤ,n) → U (4.5) transported; structure must
be included: Monoid := Σ A Σ μ Σ e (Assoc × LeftUnit × RightUnit) (4.6); SIP. §4.1 h_F = e∘h∘e⁻¹
(4.8), P_F(w) = P(e⁻¹ w) (4.9), q_F (4.10); Package(X) = Σ f:X→X Π x P_X(x) → P_X(f x) (4.11)
transported as one value (4.12). §4.2 graph G : Fin n → Fin n → Bool (4.13), relabeling π·G
(4.14), positions (4.15); set quotient vs groupoid. §4.3 μ_B = e(μ_A(e⁻¹ b1, e⁻¹ b2)) (4.16);
quotients (points, paths, set-truncation), integers/rationals/polynomials/graphs as
representations.
Repo: ua, uaβ, pathToEquiv∘ua (section only) in std. Vec ≃ Fin→A, Sorted transport, Monoid,
Package, graph relabeling: absent.

## §5 Programming with languages and their semantics
§5.1 Ty(J,ℓ) = Σ A:Raw. Der(J ⊢ A : U_ℓ), Code(J,A) = Σ t:Raw. Der(J ⊢ t : |A|), Sub, Env;
structural de Bruijn subst with lift; semantic substitution lemma ⟦t[σ]⟧ρ ≡ ⟦t⟧(⟦σ⟧ρ) and
(t[σ])[τ] ≡ t[σ∘τ]. Language_ℓ = Σ Syn,Ctx,Ty,Static,Conf,Cmd,Step,Init,Observe. Law;
Step_p : Conf → Return + Fault + Tau(Conf) + Σc (Reply c → Conf); interp_L by guarded unfold.
Lift(L) = Σ p. Well(p), meaning, Spec, Attached; Trans_{L,M}(p) = Σ q:Lift(M). Rel_O(…);
erase(p,π) = p. A complete intrinsically typed language: types Byte|Unit|×|+|⇒, Var vz/vs, Tm
var lit lam app pair fst snd inl inr case; Env, lookup, eval by structural recursion; renaming,
substitution, lift, evalEnv; Substitution theorem eval(subst θ t) ρ = eval t (evalEnv θ ρ) with
its proof by induction and funext in the λ case.
§5.2 Expr(Γ) var|num|plus|let (5.1), ⟦·⟧ (5.2), D(let(u,v)) = subst0(D u, D v) (5.3), theorem
Π ⟦D e⟧ρ = ⟦e⟧ρ (5.4) using §5.1; reusing a theorem is application.
§5.3 Step : Cfg → Event → Cfg → U (5.5); Behavior(p,o) = ‖Σ τ. observe τ = o‖ (5.6).
§5.4 Lang := Σ Src Ctx Ty Tm Cfg Σ. Sem(…) (5.7); adequacy to a standard is separate.
§5.5 Step_{θ,p} (5.8); Signature {Cmd, Reply}; Machine {next : C → D_Σ(C,A,F)};
D_Σ(C,A,F) = A + F + C + Σc (Reply c → C) (5.9); Proc_Σ ≅ A + F + ▷Proc + Σc (Reply c → ▷Proc),
unfold (5.10); record Language {Source, Well, Input, Result, Fault, Config, Commands, init,
step}; meaning L p w x = unfold (L.step p) (L.init p w x); Property, Proof, Annotated (5.11).
Worked memory instance: Stmt Skip|ReadInto|Write|Seq|If, Expr Const|Reg|Xor, step equations
with continuation stack; handler μ; program Seq(ReadInto(r,a), Write(b, Xor(Reg r, Const 5a)))
with Read(a) = 0x35 gives Read(a), Write(b,0x6f), Done. RuleSig, All, StdDeriv (5.12);
Adequate(H) with both round trips (5.13).
§5.6 Byte = Fin 256 by 8 bits, Expr Input|Const|Xor, ⟦·⟧; p = Xor(Xor(Input,5a),5a);
P(e) = Π x. ⟦e⟧ x ≡ x; xorCancel by the Boolean table; annotate/erase; cancel with precedence;
cancelSound by structural recursion; preserve (e,ρ) = (cancel e, λx. cancelSound e x · ρ x); the
complete calculation (cancel p = Input).
Repo: none of §5.1's typed language, §5.2, the §5.5 Language/Machine record or its memory
example, RuleSig/Adequate, or §5.6 exist. (eqproof has a substitution-soundness lemma for its own
term syntax, not this.) Guarded processes exist (proc/obs).

## §6 Reflective programming and language extension
Code_L(Γ,A), eval_L (6.1); T : Π Code → Code (6.2); representation-changing result
Σ B Σ e Σ c' Π ρ e(eval c ρ) = eval c' ρ (6.3); specializer law ⟦S(I,p)⟧(x) = I(p,x) (6.4).
§6.1 Code_ℓ(Γ,A) := Σ c. Der_ℓ(Γ,c,A); eval_{ℓ,Γ,A} into Π ρ ⟦A⟧ρ; Val^nf with reify and
reifySound (6.5); Kripke NbE clauses for Π, Σ, PathP; normalize = reify ∘ eval_nf ∘ reflect;
typed specializer S_{A,B} : Code(∅,A→B) → Code(∅,A) → Code(∅,B) with eval(S(q,r)) = eval q (eval r);
first projection (6.6); second projection with lift_A on syntax datatypes (6.7). The checker
deciding derivability is a separate theorem; check(Γ,c,A) : Dec(Der(Γ,c,A)); a code for the
evaluator lives at an enclosing stage, not U : U. Substitution on checked syntax; transformations
return Σ c'. Path(⟦c⟧,⟦c'⟧); rewrites by cong C p, composed by concatenation, checked. Open terms:
neutrals; faces participate; call-by-need sharing. Reuse of reductions d : Γ;φ ⊢ t ⟶* u (6.8),
substStep (6.9).
§6.2 the repository's specializer, quoted verbatim (specialize, normalize-held, rewrite-all,
path-code, compose-p, apply-now, residual); worked example λx y.(x+7)+y with x := 5 gives
λy. 12+y. Encoder and byte meaning are separate obligations (§9.4).
§6.3 mul (6.10); ⟦·⟧_R over a semiring (6.11), ℕ[X_i] interpretation, ev_ρ (6.12), commuting
(6.13); Clauses(R) and fold (6.14), I_R(ρ), homomorphism law.
§6.4 Annotated_L(Γ,A) := Σ p. Property(p) (6.15); Transform_L (6.16); conservative extension
erase (6.17), ⟦x⟧_{L+} = ⟦erase x⟧_L (6.18).
§6.5 sharing; t(x,y) = fst(x+3,y), x := 5 gives 8.
Repo: §6.2 is the repository's code (present, untyped-checked; its results checked by
conversion or path). §6.1 typed Code_ℓ, NbE, reifySound, typed S, Futamura: absent. §6.3, §6.4:
absent.

## §7 Programming with machines and operating systems
Mem := Addr ⇀ Byte (7.1); State := Registers × Memory × PC × DeviceState (7.2);
step : State → Event → State → U (7.3); modular sum loop invariant (7.4). §7.1 Load relation
(7.5), richer memory states. §7.2 X = (E, tid, kind, loc, val, po, rf, mo) (7.6), Allowed(X),
Allowed_SC worked. §7.3 continuing computation representations. §7.4 tensors
T : Fin n1 × … → A (7.7), layouts, AD. §7.5 Host(K; P1…Pn) : Proc_Σdevice(A) (7.8), isolation
invariants. §7.6 Sig := Σ C. C → U; Proc_ℓ(S,A) Church-encoded; Step(S,A) sdone|scmd;
o-handle by recursion; o-idh, o-comph; o-handle-id, o-handle-bind; o-NaturalAt,
o-natural-path; fleet-proc : Beh → Nat → FState → Proc EnvS FState.
Repo: §7.6 present and proved (obs, proc, fleet). Memory/event models: definitional files exist
(mem/*, isa/tso, mem/rc11), several outside the checked library; (7.4) loop: no. Allowed_SC: the
TSO and RC11 files are the analogous objects.

## §8 Existing programming languages inside the host
CppMeaning : CppSyntax → Input → Trace → U (8.1); Safe(p) (8.2); §8.1 Annotation(d) (8.3);
§8.2 SourceBeh, TargetBeh, Conforms_B, Correct_L (8.4); TargetBeh by image of loaded execution
(8.5). §8.3 Word32 = Fin 2³², Bytes4, encodeLE (8.6–8.7), decodeLE (8.8), round trips (8.9),
add32 (8.10), addBytes (8.11), commuting equation (8.12). §8.4 inc_a and its law (8.13–8.14);
increment(std::uint32_t* p) with load/store/frame laws (8.15). §8.5 index(update v i a, j)
(8.16); U_w = Fin 2^w; Nat vs Word vs U64; division/shift conventions vs C++ definedness; floats;
enc/dec with round trips; little-endian U64 (8.17); store with read-after-write and frame (8.18);
allocation, separation, destructive reuse only on unique use. §8.6 C++ static and dynamic
categories (two tables); Clang as declared frontend assumption; the bounded frame decoder and its
specification (8.19).
Repo: C++/Rust readers exist (definitional, no theorems, C++ AST fails to check). (8.6)–(8.12),
(8.16)–(8.18) as statements: no (word/byte laws exist in other forms; store laws exist in the
compile lemma file). Other languages are out of the product's scope.

## §9 Native execution as a mathematical program transformation
TargetBeh(compile p) ⟹ SourceBeh (9.1); stack machine PUSHINPUT/PUSHCONST/XOR/HALT = 01, 02 b,
03, ff (9.2); C (9.3); for XOR example the 8 bytes 01 02 5a 03 02 5a 03 ff. §9.2 interpreter as
realization. §9.3 source (9.4), State (9.5), instruction table, step/run/trace, C and program
(9.6), #(e), ℓ(e) (9.7), stack-extension theorem run^{#e}_B(a,S,x,running) =
(a+ℓ(e), E(e,x)::S, x, running) (9.8), program theorem (9.9), trace table, composition with a
verified transformation (9.10). Block(a,P,Q) = Σ b Π s. P s → ExecTo(…) (9.12), seq with
StablePrefix and RelocOK (9.13); realize_M type; the 46-byte x86-64 frame decoder with its
assertion table and χ_b; decoder stability δ_T (9.11); toy ISA MOVI/XOR/STORE/HALT and its trace
(9.14); compiler as fold of a constructor algebra (9.15) with relational congruences (9.16);
ArtifactCorrect = SourceCorrect ∘ CompilerRefines (9.17); loading, native substitution only once
a target-correctness witness exists (JIT caches artifact with evidence); export; printer
correctness is about parsed meaning. §9.4 #jit, #build, #build-loop; mn-init, run-to, mn-run,
Correct1 (quoted), its fallback semantics, templates as separate obligations; general
Σ a Π ρ Σ τ obs equality.
Repo: §9.4 is the repository's code (present). One all-inputs Correct2w theorem (word addition).
(9.8)–(9.10) stack machine, toy ISA (9.14), Block/seq, compiler algebra, χ_b: absent (a stack
machine version existed 11-10 and was deleted). #jit binds bytes WITHOUT a target-correctness
witness, contrary to "native substitution" in §9.3.

## §10 Persistent, interactive, and distributed programs
Process_Σ(A) return|request|delay (10.1); Maintained(q,d) = Σ r. r = q(d) (10.2); handler
composition and safety transfer by simulation; progress needs fairness. §10.1 valid(S,D) (10.3),
read-dependency reuse. §10.2 Δq(q d, d) = q(δ d) (10.4); lens laws (10.5). §10.3 credit state S
and total (10.6), allocation (10.7), delivery (10.8), conservation by cancellation and induction;
duplication breaks it; Possible_i(o) (10.9); receipt-bearing delivery with unique ids. §10.4
Fiber_o(y) (10.10), SafeAt(y) (10.11), refinement of observations; K'(w') (10.12). §10.5 Dep
Present/Absent, valid, validAll (10.13); derive(f) (10.14); finite-map sum derivative (10.15);
Outcome, Read, Certificate, EffectScope, RequestToken, Producer (10.16); valid(S',D) (10.17);
validReuse (10.18); transitions demand/step/request/response/return/fault (10.19); stale
restart; read-dependency lemma; maint (10.20); typed editing context. §10.6 materialized view
(10.21); transactions; distribution, content identity ≠ execution; numerical programs, AD;
whole systems. §10.7 Π I(s) → I'(u s); mixed-version transition install/drainOld/routeNew/
retireOld with at-most-once ledger invariant by induction over the four constructors. §10.8
digest vs ≡ vs SemEq (10.22). §10.9 CompatibleRun(y) (10.23), restriction (10.24). §10.10
Safe(I,b,g) := Π cs. I(fleetRun b cs g), safe-ind; liveness needs fairness. §10.11 edit(D,d,d')
and reuse (10.25).
Repo: §10.10 present and proved, with a further worked fleet (key-value store: safety,
refinement to a sequential map, liveness under fairness, convergence, deployed over UDP).
Content-addressed store exists in C (trusted). (10.3)–(10.5), (10.6)–(10.8), receipts,
fibers/SafeAt, (10.14)–(10.15), §10.7 mixed versions: absent as checked statements.

## §11 Conclusion
The examples are deliberately heterogeneous programs in one language.

## Appendix A
Integers as pairs under (a,b) ∼ (c,d) ⟺ a+d = c+b (A.1), rationals, reals, complex
multiplication (A.2); Fourier on L²(ℝ) unitary (A.3), integral (A.4), F(f')(ξ) = 2πiξ F(f)(ξ)
(A.5). A.2 X = Fin 2ⁿ, X ≃ Σ b Σ x. f x = b (A.6), Sat(f), search_f : Dec(Sat f) (A.7).
A.3 a | b := Σ k. a·k = b (A.8), gcd/lcm as meet/join. A.4 F := E + iB, i∂_t F = ∇ × F (A.9).
Repo: integers exist as a data type with group laws (not as the quotient of A.1); A.2, A.3:
absent; A.3–A.4 analytic content not formalized.

## Appendix B
The name: hyper (higher structure, languages as data), active (constructions compute).
