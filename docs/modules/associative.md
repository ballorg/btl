# Module: associative (ordered and hashed containers)

## Overview

Key-addressed containers built as structure-of-arrays over [CBufferVector](../types/BTL.CVector.md): a red-black tree (ordered, unique keys) and an open-addressing hash map (unordered, unique keys). Both follow the same composition pattern — a policy empty base (comparator / hash policy), metadata columns in front of the payload columns (key + values), a protected `Base/Impl` split, and thin public wrappers in heap-backed and inline-buffer flavors.

## Responsibilities

- Ordered map/set/multi-column map: O(log n) insert/find/erase, in-order iteration ([CRBTree family](../types/BTL.CRBTree.md)).
- Hash map/set/multi-column map: expected O(1) insert/find/erase ([CHashMap family](../types/BTL.CHashMap.md)).
- Reusable hashing policy ([CFibonacciHash](../types/BTL.CFibonacciHash.md)) with derived compile-time constants ([MFibonacci](../types/BTL.CFibonacciHash.md)).
- A shared iterator ([CSlotIterator](../types/BTL.CSlotIterator.md)) parameterized by the owner's traversal contract.

## Public Interface

| Family | Wrappers | Convenience aliases |
| --- | --- | --- |
| Red-black tree | `CRBTree` (heap), `CBufferRBTree` (inline) | Variadic `RBTree(8/16/32/64)_t` and `BufferRBTree*_t` aliases for sets, maps, and multi-column maps |
| Hash map | `CHashMap` (heap), `CBufferHashMap` (inline) | Variadic `HashMap(8/16/32/64)_t` and `BufferHashMap*_t` aliases for sets, maps, and multi-column maps |
| Hash policy | `CFibonacciHash< U, INDEX >` | `Hash_t`, `Hash8_t` … `Hash64_t` |
| Comparator | `CRBTreeLess< T >` (and transparent `CRBTreeLess< void >`) | — |

Iteration macros: `BALL_RBTREE_FOREACH(_REVERSE)` (key order), `BALL_RBTREE_FOREACH_UNORDERED(_REVERSE)` (dense slot order; the reverse form tolerates removing the current node), `BALL_HASHMAP_FOREACH` (occupied-slot storage order).

## Dependencies

[containers](containers.md) (the SoA substrate), [utilities](utilities.md) (`PopCount`, `BitCeil`), [reflection](reflection.md) (reflect-tagged key/link columns), [meta](meta.md) (`MFibonacci`, sequences, `MIndexType`).

## Data Structures

- [BTL::CRBTree](../types/BTL.CRBTree.md) — tree core, facade, wrappers, `ERBTreeColor`, `CRBTreeLess`.
- [BTL::CHashMap](../types/BTL.CHashMap.md) — table core, facade, wrappers, `EHashSlotState`.
- [BTL::CFibonacciHash](../types/BTL.CFibonacciHash.md) — hashing policy + `MFibonacci` constants.
- [BTL::CSlotIterator](../types/BTL.CSlotIterator.md) — shared iterator.

## Relationships

Both containers tag their key column with a [CReflect](../types/BTL.CReflect.md) wrapper so a key never aliases a same-typed value column; users must likewise tag identically-typed value columns. Copy/move between capacity flavors is always a structural rebuild (row-by-row re-insert), not a pointer steal.
