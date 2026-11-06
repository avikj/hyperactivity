{-# OPTIONS --cubical #-}
module paths where

open import Cubical.Foundations.Prelude

private
  variable
    ℓ : Level
    A : Type ℓ

-- symmetry and composition of paths, as Cubical Agda's prelude writes them: the composition by
-- hcomp with the tube on both ends, and transport by transp with the empty formula
symA : {x y : A} → x ≡ y → y ≡ x
symA p = λ i → p (~ i)

compA : {x y z : A} → x ≡ y → y ≡ z → x ≡ z
compA {x = x} p q = λ i → hcomp (λ j → λ { (i = i0) → x ; (i = i1) → q j }) (p i)

transportA : {ℓ : Level} {A B : Type ℓ} → A ≡ B → A → B
transportA p a = transp (λ i → p i) i0 a

-- transp along a constant line on the face j is the value there: a path from transport to the value
transportRefl' : {ℓ : Level} {A : Type ℓ} (x : A) → transportA (λ i → A) x ≡ x
transportRefl' {A = A} x = λ j → transp (λ i → A) j x

-- the filler of the composition square as a path (a signature's Π is never over I here: a
-- function of dimensions is written as a path type)
fillA : {x y z : A} (p : x ≡ y) (q : y ≡ z) → PathP (λ j → x ≡ q j) p (compA p q)
fillA {x = x} p q = λ j i → hcomp (λ k → λ { (i = i0) → x ; (i = i1) → q (j ∧ k) ; (j = i0) → p i }) (p i)

-- cub: #check compA {Nat} {0} {0} {0} (λ i → 0) (λ i → 0) : PathP (λ i → Nat) 0 0
-- cub: #check transportA {lzero} {Nat} {Nat} (λ i → Nat) 3 : Nat
-- cub: #same transportRefl' {lzero} {Nat} 3 i0 = 3
-- cub: #check fillA {Nat} {0} {0} {0} (λ i → 0) (λ i → 0) : PathP (λ j → PathP (λ i → Nat) 0 0) (λ i → 0) (compA {Nat} {0} {0} {0} (λ i → 0) (λ i → 0))
-- cub: #fail
-- cub: #check compA {Nat} {0} {0} {1} (λ i → 0) (λ i → 0) : PathP (λ i → Nat) 0 1
