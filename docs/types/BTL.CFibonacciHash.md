# BTL::CFibonacciHash

## Overview

The stateless Fibonacci (multiplicative golden-ratio) hashing policy used by the [hash map](BTL.CMultiHashMap.md), pluggable the same way `CRBTreeLess` is for the tree. It splits hashing into two composable steps — reduce a key to a working word, then map that word onto a power-of-two table — and the entire chain is `constexpr`, so compile-time and run-time hashing produce bit-identical results.

## Declaration

- **Namespace:** `BTL`
- **Module:** [associative](../modules/associative.md)
- **Kind:** class template `CFibonacciHash< typename U = size_t, uint7_t INDEX = 90 >`, derived from the constants trait `MFibonacci< U >`
- **Declared in:** [include/ball/types/hash.hpp](../../include/ball/types/hash.hpp); constants in [include/ball/types/meta/fibonacci.hpp](../../include/ball/types/meta/fibonacci.hpp)
- **Aliases:** `Hash_t` (`size_t` word), `Hash8_t` … `Hash64_t`

## Purpose

Well-distributed bucket indices from integer and string keys with one multiply and one shift, no modulo, and no lookup tables — the multiplier itself is *derived* at compile time rather than pasted.

## Data Structure

The policy holds no state (usable as an empty base or `BALL_NO_UNIQUE_ADDRESS` member). All data is `static constexpr`, supplied by `MFibonacci< U >`:

- `MULTIPLIER` — the odd golden-ratio constant `A_N ≈ 2^N / φ` for the word width `N = BITS`, carved by `ReferenceMultiplier()` from the canonical 64-bit reference `0x9E3779B97F4A7C15`. The `consteval` calculation covers widths from 1 through 64 bits and forces the result odd so multiplication is a bijection modulo `2^N`.
- `OFFSET_BASIS` — an FNV-style basis retained by `MFibonacci< U >`.
- `FOLD_SEED` — `Fibonacci_NumberConst( INDEX )`, narrowed to the working word; `INDEX` is limited to the largest 64-bit Fibonacci index (`93`).
- `fib_t` is the unsigned 64-bit reference word returned by the Fibonacci helpers.
- `Fibonacci_Number( I n )` computes iterative Fibonacci numbers with a caller-selected index type `I`, while `Fibonacci_NumberConst( I n )` requires immediate compile-time evaluation.

## Ownership and Lifetime

Stateless and trivially copyable; no lifetime concerns.

## Type Relationships

- Default hash policy `C` of the [CMultiHashMap family](BTL.CMultiHashMap.md); the map calls the static `Make` and `IndexForCapacity` operations.
- `MFibonacci` requires an **unsigned** word (`MFixedMetadata< U >::IS_UNSIGNED`), widths 1..64.
- The storage overload is parameterized by `S` and accepts types exposing `Index_t`, `Base()`, and `Count()`; [CViewBase](BTL.CViewBase.md)-derived `CView` and `CStringView` types satisfy this interface.

## Invariants

- `MULTIPLIER` is odd (static-asserted) — no bucket is unreachable.
- `IndexForCapacity` requires a power-of-two capacity (asserted).

## Operations

- `Make( key )` — hashing: integers cast to the unsigned word (the multiply does the mixing); strings are accepted as `CStringView`, while compatible `S` storage ranges fold unit-by-unit through `Base()`/`Count()` with an FNV-1a-shaped `Append` (xor then multiply by `MULTIPLIER`), endianness- and signedness-independent.
- `Index( word, indexBits )` — `(word * MULTIPLIER) >> (BITS - indexBits)`; returns zero directly when `indexBits == 0`, avoiding a shift by the full word width.
- `IndexForCapacity( word, capacity )` — power-of-two convenience form; derives the shift from the capacity via hardware popcount at run time and a shift-walk log2 under constant evaluation.

## Notes

Exercised standalone by [case07_hash.cpp](../../src/ball/types/tests/case07_hash.cpp), including constant-expression checks for every public hashing/indexing form and run-time parity checks across every representable power-of-two capacity for 8-, 16-, 32-, and 64-bit words.
