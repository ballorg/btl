# BTL::CVector

## Overview

The unified growable SoA container. `CVector< I, T >` is the one-column case; adding types after `T` adds parallel columns under the same row count and overflow allocation. `CBufferVector` provides the corresponding inline-buffer forms. [CString](BTL.CString.md) reuses the one-column implementation with `CStringView` as its view base.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class templates
  - `CVectorBase< A, B, I, N, TI, Ts... >` — common owning storage over allocator policy `A` and view base `B`
  - [`CVector_Packed_Iterator`](BTL.CVector_Packed_Iterator.md) — byte-pointer/bit-offset iterator used for explicitly selected packed columns
  - `CVectorImpl< class B, typename I, typename TI, typename... Ts >` — allocator-independent implementation over owning base `B`; its one-column specialization adds linear editing operations
  - `VectorImpl_t< I, N, Ts... >` — compact composition alias shared by the public heap and inline wrappers
  - `CVector< I, T, Ts... >`, `CBufferVector< I, N, T, Ts... >`
- **Declared in:** [include/ball/types/vector.hpp](../../include/ball/types/vector.hpp)
- **Composition helpers:** `MVectorViewTypes` and `MVectorAllocatorType` are declared separately under [`meta/`](../../include/ball/types/meta/).
- **Aliases:** `Vector_t< T, Ts... >`, `Vector8_t` … `Vector64_t`, `BufferVector_t< N, T, Ts... >` and width variants

## Purpose

A growable single-column or SoA container with inline-buffer-first storage, index-type parameterization, support for bit-packed element types, and `constexpr`-friendly operation.

## Data Structure

All vector arities use `CVectorBase`: ordinary vectors place it over [CViewBase](BTL.CViewBase.md), while strings place it over `CStringView`. `CVectorBase` is the only vector layer that receives and uses allocator policy `A`; `CVectorImpl` only implements container operations. State consists of the shared count and one inline-buffer/heap-pointer union per column. Capacity is **derived**, not stored — always `BitCeil( Count() )`. Rows `[0, Count())` are constructed in every non-packed column.

## Storage Model

Every vector uses the same SoA allocation path. A one-column vector therefore produces one contiguous segment in the shared block; with multiple columns the aligned segments are laid out column by column. Packed columns occupy bit-array segments. Crossing the inline boundary migrates all columns together.

When a byte-relocatable one-column vector changes heap capacity, it asks the allocator to resize the existing block. The Windows allocator can extend the pre-reserved virtual-address range in place; otherwise the allocator performs the equivalent allocate-copy-free operation. Multi-column layouts and element types requiring move construction retain explicit relocation because their column offsets or object lifetimes must be rebuilt.

For byte-relocatable single-column elements, middle insertion and removal use the platform `memmove` path directly. Tail insertion and suffix removal skip the zero-length shift; suffix removal commits the new count without resolving the active storage pointer. When the operation remains inside the current geometric allocation, only the logical row count is updated; the active storage pointer is not recomputed or recommitted.

Contiguous copy, relocation, construction, destruction, and shift loops do not issue per-element software prefetch hints. Their addresses are sequential and predictable to the processor, while the byte-relocatable paths already use the platform `memcpy`/`memmove` implementations.

`GetStorage<T>( vector, count[, block] )`, provided through `meta/get.hpp`, is the compact free-function form of `CVectorBase::StorageBy<T>` used by vector implementations when resolving a column pointer for a logical count and optional overflow block.

## Ownership and Lifetime

Owns its elements and its shared heap allocation through the allocator policy inherited only by `CVectorBase` (`CAllocatorBase` for ordinary vectors). Elements are constructed/destructed exactly with the logical count. Move-from (via the view-layer swap) leaves the source empty.

## Type Relationships

- One-column `CVector` receives its familiar linear accessors directly from `CVectorImpl` over the same [CViewBase](BTL.CViewBase.md) state.
- With a non-empty `Ts...`, the public type exposes typed/positional column access from [CViewBase](BTL.CViewBase.md).
- `begin<T>()`/`end<T>()` select a column explicitly, defaulting `T` to `FirstColumn_t`. Ordinary columns return native pointers for the lowest-overhead release path. Packed columns return `CVector_Packed_Iterator`, which advances a byte pointer and bit offset because a physical `T*` element does not exist.
- Uses the [element helpers](../modules/utilities.md) for construction, copies, and shifts.

## Invariants

- Capacity is exactly `BitCeil( Count() )`; elements `[0, Count())` are constructed, storage beyond is raw (or zeroed, for packed storage).
- `TYPE_COUNT` is declared once by `CViewBase` as the compile-time SoA column count and exposed by the owning layers through `using Base_t::TYPE_COUNT`; dependent view-selection aliases qualify it as `Base_t::TYPE_COUNT` so it remains a constant expression across C++ module partitions on MSVC.
- Not thread-safe; concurrent access requires external synchronization.

## Invalidation Rules

Any operation that changes the count can reallocate or migrate storage: `Insert`, `AddTo*`, `Remove`, `SetCount`/`Grow`, `Replace*`, `CopyFrom`, and assignment all potentially invalidate every pointer, reference, iterator, and view into the elements. Shrinking is not exempt — dropping back under the inline capacity actively relocates data into the inline buffer and frees the heap block. Only operations that leave the count unchanged (element reads/writes, `Find`) preserve addresses.

## Operations

- `CVectorBase`: common one-or-many-column allocation, relocation, copying, packed storage, and heap↔inline migration.
- `CVectorImpl`: shared view adaptation and row-wise `Insert`, `AddToHead`/`AddToTail`, `Remove`, `SetCount`/`SetCountRaw`, and whole-row or per-column search. Its one-column specialization additionally supplies array/view insertion, `Replace`/`ReplaceRange`, and related linear conveniences. Single-element `AddToHead` and `AddToTail` reserve their destination through the same protected `EnsureInsert` path used by the other insertion operations.
- Wrappers: `CVector` ↔ `CBufferVector` constructors and assignments copy contents. Their rvalue forms and `MoveFrom` transfer the contents and leave the source empty; differing inline layouts mean this is a content transfer rather than guaranteed pointer stealing.
- Convenience append: one-column wrappers accept `vector += element` and `vector += otherVector`; appending a container adds all of its elements at the tail. Self-append is supported through a temporary copy.

## Usage

```cpp
BTL::Vector32_t< int > v;          // heap-backed, 32-bit indexes
v.AddToTail( 1 );
v += 2;
v.AddMultipleToTail( 2, 3, 4 );
v.Insert( 1, 9 );                  // {1, 9, 2, 3, 4}
BTL::BufferVector_t< 8, int > b;   // stays inline up to 8 elements
BTL::Vector32_t< float, int > soa;  // two columns, shared row count
soa.AddToTail( 1.5f, 7 );
for ( auto row : soa )
	row.Get< int >() += 1;
```

## Notes

The protected `EnsureInsert` hands out a raw-memory gap: the caller must construct into it before the elements are read or destroyed. Multi-column access by type requires distinct column types; wrap duplicates in distinct [CReflect](BTL.CReflect.md) tags. `BALL_FIND_BATCH_COUNT` (default 8) tunes batched probing.
