# BTL::CViewBase

## Overview

The multi-column (SoA) view core: a shared row count over one [CElementsPack](BTL.CElementsPack.md) with one column per type in `Ts...`. It supplies everything that does not require ownership — typed element access, packed-bit storage operations, batched linear search, slicing, and shallow copy/move/swap — and is the base of both the single-column [CView](BTL.CView.md) and the owning [CMultiVector](BTL.CMultiVector.md).

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class template `CViewBase< typename I, I N, typename TI, typename... Ts >` (`I` index type, `N` inline capacity, `TI` column-index type)
- **Declared in:** [include/ball/types/viewbase.hpp](../../include/ball/types/viewbase.hpp)

## Purpose

One implementation of "N parallel arrays with a common count" shared by views and owning containers, including transparent support for bit-packed columns.

## Data Structure

Two members: the logical row count `m_nCount` and the per-column storage pack `m_Elements` ([CElementsPack](BTL.CElementsPack.md)). There is no capacity and no ownership flag. Columns whose type is bit-packed (per [MFixedMetadata](BTL.CFixedBase.md)) are read and written bit-by-bit through `PackedGetBy`/`PackedSetBy`; a `PackedRef_t` proxy makes packed `Get` assignable.

## Storage Model

Borrowed (or, in the inline case, embedded): for each column, the live storage is the inline buffer while the count fits that column's inline capacity, otherwise the heap pointer — decided per access via the overflow predicates, never cached. The view never allocates; owning derived classes manage the heap side of the same fields.

## Ownership and Lifetime

Non-owning. Copy is a shallow pointer/count copy; destruction does nothing. When used as the base of an owning container, the derived class imposes ownership on the same fields.

## Type Relationships

- Base of [CView](BTL.CView.md) (single column) and [CMultiVectorBase](BTL.CMultiVector.md) (owning SoA).
- `Pack_t` is [CElementsPack](BTL.CElementsPack.md).
- `INVALID_INDEX` is `I(-1)`; `Fixed_t` is `MFixed< I >`.

## Invariants

- All columns always share the same logical count.
- A view is valid only while the data it points to outlives it and while no owning container reallocates that data.

## Invalidation Rules

`Subview`/slices return views that alias the parent's storage; any growth or shrink of an owning container over the same data invalidates them and the parent view alike. The view itself never invalidates anything — it has no mutating storage operations beyond repointing.

## Operations

- Sizes: `Count`, `Size` (count × summed per-column byte stride), `SizeBy< T >`, fixed capacity queries, `Empty`, overflow predicates.
- Typed access by column type or index: `BaseBy`/`DataBy`/`FixedDataBy` (+ packed forms), `Get< T >( i )`, `Front`/`Back`, typed `begin`/`end`.
- Packed-bit machinery: size/offset math, single-bit get/set, `PackedClearRowsBy`, `PackedShiftRowsLeftBy`/`RightBy` (bitwise row shifting), value get/set with masking.
- Search: `FindBy`/`RFindBy` over one column, with unrolled batch probing (`FindBatchForward/Reverse`, width `BALL_FIND_BATCH_COUNT`, default 4).
- Slicing: `Subview`, `First`, `Last`, `DropFront`, `DropBack` (all columns advance together); `Const()` conversion to the all-`const` view.
- State transfer: `Set( count, pointers... )`, shallow `CopyFrom`, swap-based `MoveFrom` (leaves the source empty), `StoreFirstElement` (constant-expression-safe construction of row 0 in the inline buffers).
