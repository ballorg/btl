# Module: containers (linear storage)

## Overview

The linear-storage stack: fixed arrays, non-owning views, the inline/heap storage substrate, and the unified owning vector with single-column and multi-column SoA specializations. See [architecture.md](../architecture.md#the-storage-stack) for how the layers compose.

In module builds each container component is owned by its matching internal partition (`Allocator`, `Array`, `Elements`, `ElementsPack`, `ViewBase`, `View`, `VectorIterator`, or `Vector`) and is re-exported by `Ball.Types`; shared base and meta declarations remain in `Core`.

## Responsibilities

- Own and grow contiguous element storage with an inline-buffer-first strategy.
- Provide non-owning views over the same layouts with identical accessors.
- Support bit-packed columns for sub-byte element types.
- Keep operations `constexpr`-usable where possible (compile-time construction of small inline containers works).

## Public Interface

| Type family | Document | Role |
| --- | --- | --- |
| `CArray< I, T, N >`, `CEmptyArray` | [BTL.CArray.md](../types/BTL.CArray.md) | fixed-size owning array (also the inline buffer of `CElementsPack`) |
| `CElementsPack< I, N, TI, Ts... >`, `CElementsNode< I, N, T >`, `MElementsPack` | [BTL.CElementsPack.md](../types/BTL.CElementsPack.md), [BTL.CElementsNode.md](../types/BTL.CElementsNode.md) | typed column pack and its per-column inline / heap / packed storage node |
| `CViewBase< I, N, TI, Ts... >` | [BTL.CViewBase.md](../types/BTL.CViewBase.md) | multi-column view: shared count, typed access, packed bits, find |
| `EPrefetchAccess` | [BTL.EPrefetchAccess.md](../types/BTL.EPrefetchAccess.md) | compile-time read/write mode for `CViewBase::PrefetchRow` |
| `CView< I, T, N >` | [BTL.CView.md](../types/BTL.CView.md) | single-column view; `View_t`/`BufferView_t` alias families |
| `CVectorBase/Impl`, `CVector< I, T, Args... >`, `CBufferVector` | [BTL.CVector.md](../types/BTL.CVector.md) | growable single-column or SoA vector; variadic `Vector_t`/`BufferVector_t` families |
| `CVector_Packed_Iterator< I, T, CONST >` | [BTL.CVector_Packed_Iterator.md](../types/BTL.CVector_Packed_Iterator.md) | byte-pointer/bit-offset iterator for packed columns; non-packed columns use pointers |
| `CAllocatorBase`, `CAllocator< I, T >` | [BTL.CAllocator.md](../types/BTL.CAllocator.md) | aligned heap allocation policy |

Free helpers used throughout are in the [utilities module](utilities.md); the shared find batching width is configurable via `BALL_FIND_BATCH_COUNT` (default 8, in [viewbase.hpp](../../include/ball/types/viewbase.hpp)).

## Dependencies

[base](base.md), [meta](meta.md), [utilities](utilities.md); `memory.h` CRT imports through [CAllocator](../types/BTL.CAllocator.md).

## Data Structures

All types listed above; see individual documents.

## Relationships

- [strings](strings.md) derives `CString` from the vector stack with `CStringView` as its view layer.
- [associative](associative.md) builds the red-black tree and hash map on variadic `CBufferVector`.
- [delegates](delegates.md) uses `BufferVector_t` for inline delegate storage and `Vector32_t< DelegateHandle_t, SingleDelegate_t >` for multicast slot lists.

## Notes

- Growth model: capacity is derived (`BitCeil( Count() )`), never stored; any operation that changes the count across a power-of-two boundary reallocates and therefore invalidates pointers, references, and views into the data. Shrinking below the inline capacity migrates data back into the inline buffer (also a relocation).
- SoA columns are addressed by type; same-typed columns must be wrapped in distinct [CReflect](../types/BTL.CReflect.md) tags (see [architecture.md](../architecture.md#same-type-soa-columns-and-reflect-tags)).
