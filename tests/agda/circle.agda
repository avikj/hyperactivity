{-# OPTIONS --cubical #-}
module circle where

open import Cubical.Foundations.Prelude

-- the circle as a higher inductive type: a constructor given by a path type is a dimension with
-- that boundary (the target's own declaration form)
data S¹ : Type where
  base' : S¹
  loop' : base' ≡ base'

-- a function out of it by its action on the point and the loop
rot : S¹ → S¹
rot base' = base'
rot (loop' i) = loop' (~ i)

-- a loop space element composed with itself, by hcomp with the type written
twice : base' ≡ base'
twice = λ i → hcomp {A = S¹} (λ j → λ { (i = i0) → base' ; (i = i1) → loop' j }) (loop' i)

-- cub: #same rot base' = base'
-- cub: #same twice i0 = base'
-- cub: #same twice i1 = base'
