{-# OPTIONS --cubical #-}
module nat where

open import Cubical.Foundations.Prelude

data ℕ : Type where
  zero' : ℕ
  suc' : ℕ → ℕ

add : ℕ → ℕ → ℕ
add zero' n = n
add (suc' m) n = suc' (add m n)

add-zero : (n : ℕ) → add n zero' ≡ n
add-zero zero' = refl
add-zero (suc' n) = cong suc' (add-zero n)

add-suc : (m n : ℕ) → add m (suc' n) ≡ suc' (add m n)
add-suc zero' n = refl
add-suc (suc' m) n = cong suc' (add-suc m n)

add-comm : (m n : ℕ) → add m n ≡ add n m
add-comm zero' n = sym (add-zero n)
add-comm (suc' m) n = cong suc' (add-comm m n) ∙ sym (add-suc n m)

-- cub: #same add (suc' (suc' zero')) (suc' zero') = suc' (suc' (suc' zero'))
-- cub: #same add-comm (suc' zero') (suc' (suc' zero')) i1 = suc' (suc' (suc' zero'))
