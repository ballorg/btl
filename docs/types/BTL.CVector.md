# BTL::CVector

## Overview

The growable single-column container, assembled from three layers over [CView](BTL.CView.md): `CVectorBase` (memory management), `CVectorImpl` (element operations), and the public wrappers `CVector` (heap-backed, `N == 0`) and `CBufferVector` (inline buffer of `N` elements). [CString](BTL.CString.md) reuses the same stack with a string view base.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class templates
  - `CVectorBase< class B, typename I, typename T, class A = CAllocator< I, T > >` — derives from allocator `A` and view base `B`
  - `CVectorImpl< class B, typename I, typename T >`
  - `CVector< I, T, A >`, `CBufferVector< I, N, T, A >`
- **Declared in:** [include/ball/types/vector.hpp](../../include/ball/types/vector.hpp)
- **Aliases:** `Vector_t< T >`, `Vector8_t` … `Vector64_t`, `BufferVector_t< T, N >` and width variants

## Purpose

A `std::vector` replacement with inline-buffer-first storage, index-type parameterization, support for bit-packed element types, and `constexpr`-friendly operation.

## Data Structure

No members are added over the view base: the state is the inherited count plus the inline-buffer/heap-pointer union. Capacity is **derived**, not stored — always `BitCeil( Count() )` — so the container knows how much memory it holds purely from its count. Elements `[0, Count())` are constructed; an empty container holds count 0 and no heap allocation. For packed element types the same storage is a bit array and all element movement happens through bit shifts.

## Storage Model

Elements live in the inline buffer while the count fits the inline capacity (`FIXED_COUNT`; for `N == 0`, whatever fits in the pointer's own bytes) and on a contiguous heap block owned through the [CAllocator](BTL.CAllocator.md) policy otherwise. Growth reallocates to the next power of two via `Realloc`/`Alloc` inside `EnsureCapacity`; crossing the inline boundary migrates elements heap↔inline (`MoveToHeap`/`MoveToFixed`), and shrinking back under the inline capacity frees the heap block. Storage is always contiguous.

## Ownership and Lifetime

Owns its elements and its heap allocation (through the [allocator](BTL.CAllocator.md) policy base). Elements are constructed/destructed exactly with the logical count. Move-from (via the view-layer swap) leaves the source empty.

## Type Relationships

- Derives from `A` ([CAllocator](BTL.CAllocator.md)) and `B` ([CView](BTL.CView.md) or [CStringView](BTL.CStringView.md)).
- The multi-column analogue is [CMultiVector](BTL.CMultiVector.md).
- Uses the [element helpers](../modules/utilities.md) for construction, copies, and shifts.

## Invariants

- Capacity is exactly `BitCeil( Count() )`; elements `[0, Count())` are constructed, storage beyond is raw (or zeroed, for packed storage).
- Not thread-safe; concurrent access requires external synchronization.

## Invalidation Rules

Any operation that changes the count can reallocate or migrate storage: `Insert`, `AddTo*`, `Remove`, `SetCount`/`Grow`, `Replace*`, `CopyFrom`, and assignment all potentially invalidate every pointer, reference, iterator, and view into the elements. Shrinking is not exempt — dropping back under the inline capacity actively relocates data into the inline buffer and frees the heap block. Only operations that leave the count unchanged (element reads/writes, `Find`) preserve addresses.

## Operations

- `CVectorBase`: `EnsureCapacity( n )` — grows/shrinks the allocation to `BitCeil( n )` via `Realloc`/`Alloc`, migrates data heap↔inline (`MoveToHeap`, `MoveToFixed`), and manages the packed-storage equivalents; `CopyFrom` for views; frees the heap block in its destructor. It moves bytes only — element construction/destruction is the next layer's job.
- `CVectorImpl`: the element-correct API — `Insert` (single, view, C array, variadic `InsertMultiple`), `AddToHead`/`AddToTail` (+`*Multiple`), `Remove`, `RemoveAll`/`Purge`, `Grow`/`SetCount` (value-constructs or destructs the delta), `Replace` (range/first/all/character), `ReplaceRange`, plus constructors/assignment from views, C arrays, and other vectors. `EnsureInsert( i, n )` is the shared grow-then-shift-gap primitive; its returned gap is raw memory the caller must construct into. The destructor destructs all elements (the base then frees memory).
- Wrappers: `CVector` ↔ `CBufferVector` cross-conversions copy contents.

## Usage

```cpp
BTL::Vector32_t< int > v;          // heap-backed, 32-bit indexes
v.AddToTail( 1 );
v.AddMultipleToTail( 2, 3, 4 );
v.Insert( 1, 9 );                  // {1, 9, 2, 3, 4}
BTL::BufferVector_t< int, 8 > b;   // stays inline up to 8 elements
```

## Notes

The protected `EnsureInsert` hands out a raw-memory gap: the caller must construct into it before the elements are read or destroyed. `BALL_FIND_BATCH_COUNT` (default 4) tunes the unrolled probing used by the inherited find operations.
