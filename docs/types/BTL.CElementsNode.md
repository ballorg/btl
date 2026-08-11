# BTL::CElementsNode

## Overview

Internal per-column storage node used by [CElementsPack](BTL.CElementsPack.md). It centralizes the union and operations that previously appeared in both `CElementsPack` specializations.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class template `CElementsNode< typename I, I N, typename T >`
- **Declared in:** [include/ball/types/elementspack.hpp](../../include/ball/types/elementspack.hpp)

## Purpose

Represent the storage alternatives for one SoA column while leaving column lookup and traversal to `CElementsPack` and `MPack`.

## Data Structure

The node contains a union of an inline `CArray`, an external element pointer, an inline packed-byte array, and an external packed-byte pointer. It stores no discriminator; the owner selects the active representation from its row count and the column metadata.

## Storage Model

The inline array and pointer occupy the same union storage. `N` determines the requested inline capacity; for `N == 0`, `MElementsPack` derives a fallback from the pointer size. Packed columns use the same byte budget with their logical bit width.

## Ownership and Lifetime

`CElementsNode` does not allocate or release memory. Its pointer alternatives may be borrowed by views or managed by an owning container above `CElementsPack`.

## Type Relationships

The class derives aliases and storage constants from `MElementsPack< I, N, T >`. `CElementsPack` stores one node for every `Ts` entry inside an `MPack`.

## Invariants

- Exactly one union representation is active at a time.
- The active representation is determined externally; the node carries no count or tag.
- Value construction requires `FIXED_COUNT > 0`.

## Operations

- Access ordinary and packed inline buffers and external pointers.
- Select inline or external storage from an overflow flag.
- Activate and copy inline buffers during constant evaluation.
- Shallow-copy or swap pointers for overflow storage and copy or swap inline storage.

## Notes

This is an implementation type. Container users normally interact with `CViewBase`, `CVector`, or `CMultiVector`.
