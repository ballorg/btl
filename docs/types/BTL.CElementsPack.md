# BTL::CElementsPack

## Overview

The storage substrate of every view and container. It stores one [CElementsNode](BTL.CElementsNode.md) per column in an `MPack`, then exposes the nodes through type- and index-based storage accessors.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class template `CElementsPack< typename I, I N, typename TI, typename... Ts >`, with trait bases `MElementsPackBase< I, T >` / `MElementsPack< I, N, T >`
- **Declared in:** [include/ball/types/elementspack.hpp](../../include/ball/types/elementspack.hpp)

## Purpose

Give multi-column containers per-column storage that costs one pointer when empty, keeps up to `FIXED_COUNT` elements inline without allocation, and can be addressed by column type or index `K`.

## Data Structure

For each column type a [CElementsNode](BTL.CElementsNode.md) holds the four alternative storage representations. `CElementsPack` stores those nodes in `MPack< TI, CElementsNode< I, N, Ts >... >`, retaining the declared column order while reusing the existing pack traversal implementation.

`COMMON_FIXED_COUNT` is the minimum inline capacity across all columns, computed by a fold expression. It is the shared row threshold at which storage moves to external pointers.

## Storage Model

Each column uses a union of embedded inline storage and an external pointer. The active member is a function of the shared count and packed metadata, not a discriminator stored in the pack. `CElementsPack` allocates nothing itself.

## Ownership and Lifetime

The pack does not own the memory referenced by pointer alternatives. Owners such as [CVector](BTL.CVector.md) and [CMultiVector](BTL.CMultiVector.md) manage allocations; [CView](BTL.CView.md) and [CViewBase](BTL.CViewBase.md) treat pointers as borrowed.

## Type Relationships

Embedded as `m_Elements` in [CViewBase](BTL.CViewBase.md). `MElementsPack` computes per-column layout constants, `CElementsNode` owns each storage union, and `MPack` provides traversal by node type.

## Invariants

- The active union member is derived from the shared count and column metadata.
- Inline capacity is `FIXED_COUNT` elements or `FIXED_SIZE` bytes for packed columns.
- `COMMON_FIXED_COUNT` bounds the shared inline row count.

## Operations

- Default construction selects null pointer storage for every column; value, copy, and move construction and assignment are also available.
- `FixedBy`, `DataBy`, `BaseBy`, and packed counterparts by column type or index.
- `IsOverflowBy` and `IsPackedOverflowBy` overflow predicates.
- `StoreFixedElements` and `ActivatePackedFixed` for constant-evaluation-safe union activation.
- `CopyBy` and `SwapBy` for per-column pointer or inline storage operations.

## Notes

`MPack` type lookup returns the first matching node type. Same-typed SoA columns must therefore use distinct [CReflect](BTL.CReflect.md) tags; see [architecture.md](../architecture.md#same-type-soa-columns-and-reflect-tags).
