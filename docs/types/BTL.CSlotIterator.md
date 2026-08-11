# BTL::CSlotIterator

## Overview

The iterator shared by the [red-black tree](BTL.CMultiRBTree.md) and the [hash map](BTL.CMultiHashMap.md). Both containers iterate by walking slot indices of a node-owning container and dereferencing to the key; only the *step* differs (in-order successor vs. next-occupied-slot scan), so the iterator delegates every traversal decision to its owner.

## Declaration

- **Namespace:** `BTL`
- **Module:** [associative](../modules/associative.md)
- **Kind:** class template `CSlotIterator< typename Owner, bool IS_CONST >`
- **Declared in:** [include/ball/types/slotiterator.hpp](../../include/ball/types/slotiterator.hpp)

## Purpose

One iterator implementation for any container that exposes the traversal contract: `Index_t`/`Key_t` member types, a `NIL_INDEX` end sentinel, `Key( i )`, `IsOccupied( i )` (debug asserts), `NextIndex( i )`, and — only if `operator--` is actually used — `PrevIndex( i )`.

## Data Structure

Two members: the current slot index and a (const-qualified when `IS_CONST`) pointer to the owner. Default construction yields an unowned end iterator.

## Ownership and Lifetime

Borrows the owner; valid only while the owner outlives it and while the slot it points to is not invalidated by mutation (tree compaction, hash growth/backward shift).

## Type Relationships

- `iterator`/`const_iterator` of [CMultiRBTree](BTL.CMultiRBTree.md) (bidirectional — the tree supplies `PrevIndex`) and of [CMultiHashMap](BTL.CMultiHashMap.md) (forward-only — `operator--` is a template body that is simply never instantiated).

## Invariants

`m_iSlot == Owner::NIL_INDEX` is the end state; any other value must be an occupied slot when dereferenced or stepped.

## Invalidation Rules

The iterator is invalidated by any owner mutation that moves or removes the slot it points to: tree erase (compaction relocates the last node), hash-map growth (rehash moves every entry), and hash-map removal (backward shift moves cluster entries). Destruction of the owner invalidates all its iterators.

## Operations

Dereference to the key (`operator*`/`->`, asserted occupied), pre/post increment via `NextIndex`, pre/post decrement via `PrevIndex` (`--end()` yields the last slot), cross-constness `==`/`!=`, implicit non-const → const conversion, and `SlotIndex()` to recover the raw index for column access (`Get< TN >( it )`).
