# BTL::CMultiVector

## Overview

The growable structure-of-arrays container: an arbitrary pack of column types `Ts...` with one shared row count, where all overflowed columns live in a **single shared heap block** laid out column after column. `CMultiVector` is the heap-backed wrapper, `CBufferMultiVector` the inline-buffer wrapper; the red-black tree and hash map are built directly on the latter.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class templates
  - `CMultiVectorBase< typename I, I N, typename TI, typename... Ts >` — derives from `CViewBase< I, N, TI, Ts... >` and `CAllocatorBase`
  - `CMultiVectorImpl< class B, typename I, typename TI, typename... Ts >`
  - `CMultiVector< I, TI, Ts... >`, `CBufferMultiVector< I, N, TI, Ts... >`
- **Declared in:** [include/ball/types/multivector.hpp](../../include/ball/types/multivector.hpp)
- **Aliases:** `MultiVector_t< Ts... >`, `MultiVector8_t` … `MultiVector64_t`, `BufferMultiVector_t< N, Ts... >` and width variants

## Purpose

Cache-friendly parallel columns (SoA) under one count — the substrate for row-oriented data where per-column iteration matters, and for the associative containers' node storage.

## Data Structure

State is inherited from [CViewBase](BTL.CViewBase.md): the shared count plus one inline-buffer/heap-pointer union per column. Each column stores its elements contiguously (SoA); rows are related only by sharing an index. Capacity is derived (`BitCeil( Count() )`), never stored, and an empty container holds count 0 with no heap allocation.

## Storage Model

While the count fits `COMMON_FIXED_COUNT` (the smallest per-column inline capacity), every column lives in its own inline buffer. Once it overflows, **one** allocation holds all columns: the block is sized by summing each column's storage for the power-of-two capacity, aligned per column (`Math_RoundUp` to the column's alignment), and each column's pointer is the block base plus its computed offset. Packed (sub-byte) columns occupy bit-array segments of the same block. The block base is recovered as the first column's overflow pointer (`FirstData`), so no extra member is stored. Growth allocates a new block, relocates every column into it (`RelocateColumn`: byte copy when `MTypeInfo< T >::IS_MEMMOVE_SAFE`, else move-construct + destroy per element), and frees the old block; shrinking below the inline capacity migrates all columns back inline.

## Ownership and Lifetime

Owns the shared overflow block (freed in the base destructor) and the element lifetimes in every non-packed column (`CMultiVectorImpl::~` destructs all rows column by column). Packed columns hold no objects, only bits.

## Type Relationships

- Base storage of [CMultiRBTree](BTL.CMultiRBTree.md) and [CMultiHashMap](BTL.CMultiHashMap.md).
- Column access via `Get< T >( mv, i )` / `Get< K >( mv )` from [meta/get.hpp](../../include/ball/types/meta/get.hpp).
- Used by [CMulticastDelegate](BTL.CMulticastDelegate.md) for its handle/callback slots.

## Invariants

- All columns share one count; overflowed columns share one allocation whose layout is a pure function of (capacity, column pack).
- Rows `[0, Count())` are constructed in every non-packed column; after `SetCountRaw` growth, the new rows are raw until the caller constructs them.

## Invalidation Rules

Growth, `Remove`, `SetCount(Raw)`, `Insert`, copy assignment, and shrink relocate columns: all pointers, references, and views into any column are invalidated by any operation that changes the row count. Row *indices* below the mutation point remain meaningful across `Insert`/`Remove` (rows shift), but addresses do not.

## Operations

- Row operations (`CMultiVectorImpl`): `Insert` (one value per column, a view, or `InsertMultiple` identical rows), `AddToHead`/`AddToTail`, `Remove` (destructs the range and shifts every column's tail left), `SetCount`/`Resize`/`Grow` (constructs/destructs the delta per column), `SetCountRaw` (reserves without constructing new rows — bulk-build path), `RemoveAll`/`Clear`/`Purge`, `InitFirst` (constant-expression-safe construction of row 0).
- Row search (`CMultiVectorBase`): `Find`/`FindFrom`/`RFind`/`RFindFrom` comparing an entire row against one probe value per column (batched over the first column).
- Copy/move from views rebuilds contents; move assignment is currently a content copy.
- `AssignRow` addresses columns **positionally**, so same-typed columns are written correctly during row insertion even though typed access would alias.

## Usage

```cpp
BTL::MultiVector32_t< float, int > mv;   // two columns, shared count
mv.AddToTail( 1.5f, 7 );
float &x = Get< float >( mv, 0 );        // column access by type
```

## Notes

**Same-type columns alias.** Typed column access resolves to the first column of that type, and the heap layout would overlap same-typed columns. Give every column a distinct type — wrap duplicates in per-position [CReflect](BTL.CReflect.md) tags (`BALL_REFLECT_TAGGED(_TEMPLATE)`), as the tree/hash map do for their key columns.
