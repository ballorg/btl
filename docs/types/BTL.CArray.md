# BTL::CArray

## Overview

A fixed-size, owning array of `N` elements of one type — Ball's `std::array` analogue, and the building block used as the inline buffer inside [CElementsPack](BTL.CElementsPack.md). `CEmptyArray` is its zero-storage stand-in for `N == 0` columns.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class template `CArray< typename I, typename T, I N >`; companion `CEmptyArray< I, T >`
- **Declared in:** [include/ball/types/array.hpp](../../include/ball/types/array.hpp)
- **Aliases:** `Array_t< T, N >`, `Array8_t` … `Array64_t` (index width variants)

## Purpose

Contract-checked fixed storage without dynamic allocation, usable in `constexpr` contexts and inside unions (its only member is the raw element array).

## Data Structure

The type stores exactly `T m_Elements[ N ]` — no count, no capacity, no pointers. All size information is compile-time (`Count()`, `Size()`, `Stride()` are static). `CEmptyArray` stores nothing and reports a null base and zero count.

## Storage Model

Inline only: the elements are embedded in the object itself. There is no allocation, no resizing, and no sharing; the array lives wherever the object lives (stack, member, static storage, or inside the [CElementsPack](BTL.CElementsPack.md) union).

## Ownership and Lifetime

Elements are direct subobjects: constructed and destroyed with the array, copied/moved by the implicit special members. Nothing is heap-allocated; no operation invalidates pointers into a live array.

## Type Relationships

- Provides the `Fixed_t`/`Packed_Fixed_t` inline buffers of [CElementsPack](BTL.CElementsPack.md).
- Unlike [CView](BTL.CView.md) it owns its elements; unlike [CVector](BTL.CVector.md) it cannot grow.

## Invariants

`N` is fixed for the type; for `N == 0` the base pointer is `nullptr` and element accessors are unreachable (`Front`/`Back` are `static_assert`-guarded).

## Invalidation Rules

No operation on a live array invalidates pointers, references, or iterators into it — the storage address is fixed for the object's lifetime.

## Operations

Bounds-asserted element access (`Get`, `operator[]`, `Front`/`Back`), raw pointers (`Base`/`Data`), iterators, `Fill`/`Set`, copy/move from a same-sized C array, and linear `Find`/`RFind` returning `INVALID_INDEX` (`I(-1)`) when absent.
