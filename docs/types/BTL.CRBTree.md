# BTL::CRBTree

## Overview

A structure-of-arrays red-black tree with unique keys: node metadata (color, left, right, parent) and payload (key + value columns) each live in their own column of a [CBufferVector](BTL.CVector.md). Storage is kept **dense** by compact-on-erase, the NIL sentinel is the out-of-band index `-1` (no reserved row), and the root is reachable in O(1) through an encoding in slot 0's parent cell.

Family members:

- `CRBTreeBase< I, N, TI, C, K, Ts... >` — protected core (links, rotations, fix-ups, validation).
- `CRBTreeImpl< … >` — public facade (`Insert`, `Find`, `Remove`, bounds, iteration, cross-capacity copy/move).
- `CRBTree< I, C, K, Ts... >` (heap) / `CBufferMultiRBTree< I, N, C, K, Ts... >` (inline) — capacity wrappers.
- `CRBTree< I, C, K, Ts... >` provides `Key()` and indexed or typed column access through `Get`; the `RBTree*_t` aliases select the one-value form.
- `ERBTreeColor` — 1-bit color enum (packed column); `CRBTreeLess< T >` — default strict-weak-order comparator (transparent for `T = void`).
- **Aliases:** `RBTree_t`/`RBTree8_t`…`RBTree64_t`, `BufferRBTree*_t`, `MultiRBTree*_t`, `BufferMultiRBTree*_t`.

## Declaration

- **Namespace:** `BTL`
- **Module:** [associative](../modules/associative.md)
- **Kind:** class templates; `CRBTreeBase` derives from the comparator `C` (empty base) and from `CBufferVector< I, N, ERBTreeColor, RBTreeLeftColumn_t< I >, RBTreeRightColumn_t< I >, RBTreeParentColumn_t< I >, RBTreeKeyColumn_t< K >, Ts... >`
- **Declared in:** [include/ball/types/rbtree.hpp](../../include/ball/types/rbtree.hpp)

## Purpose

An ordered associative container (map, set, or multi-column map — `Ts...` empty makes a set) with the classic O(log n) guarantees but SoA storage: linear, hole-free rows that iterate cache-friendly in slot order and pack the color into one bit per node.

## Data Structure

One SoA row per live node, columns in order:

1. **Color** — `ERBTreeColor`, bit-packed (1 bit/node).
2. **Left / Right / Parent** — index columns (`I`), each reflect-tagged so the three same-typed columns never alias.
3. **Key** — `RBTreeKeyColumn_t< K >` (a tagged [CReflect](BTL.CReflect.md)), unwrapped to `K&` by the accessors.
4. **Value columns `Ts...`** — stored as supplied.

Structural conventions:

- `NIL_INDEX == I(-1)` is "no node"/end/no child; it owns no row, and `ColorOf( NIL )` reads black so the balancing code can treat missing children as black leaves.
- **Dense storage**: erase compacts by relocating the last row into the vacated slot (`CompactAfterRemoval`) and rewiring its neighbours, so every slot in `[0, Count())` is a live node and occupancy is just a range test.
- **Encoded root**: slot 0's parent cell always stores `~R` where `R` is the root's index; slot 0's real parent is displaced into the root's own (by definition NIL) parent cell. `ParentOf`/`SetParent` remap the two cells, making `RootIndex()` an O(1) decode with no dedicated member and no element relocation.

## Storage Model

Node rows follow the [multi-column CVector storage model](BTL.CVector.md): inline column buffers up to the common inline capacity, then one shared heap block for all columns, with capacity derived as `BitCeil( count )`. Insert always appends at the tail (storage stays dense because erase compacts), so growth is amortized; erase shrinks the row count by one and may migrate or reallocate across a capacity boundary.

## Ownership and Lifetime

Node storage is owned by the `CBufferVector` base (shared heap block or inline buffers). Payload elements are constructed per node on insert and destructed on erase/compaction; the comparator is carried as an empty base, reachable via `Comparator()`.

## Type Relationships

- Storage: [CVector](BTL.CVector.md); column tags: [CReflect](BTL.CReflect.md); iterator: [CSlotIterator](BTL.CSlotIterator.md).
- Hash-based counterpart: [CHashMap](BTL.CHashMap.md).

## Invariants

- Standard red-black invariants (root black, no red-red edge, equal black-height), checkable via `Validate()`.
- Storage dense: live nodes exactly fill `[0, TreeCount())`.
- Slot 0's parent cell holds the encoded root pointer whenever the tree is non-empty.
- The comparator must induce a strict weak ordering.

## Invalidation Rules

- Node **indices are not stable across erase**: compaction moves the last row into the freed slot. `Remove` returns the remapped successor index; any other cached index or iterator must be re-checked after an erase.
- Any insert or erase can grow or shrink the underlying storage, relocating every column — pointers and references into node payloads are invalidated by any mutation.
- The unordered reverse sweep (`BALL_RBTREE_FOREACH_UNORDERED_REVERSE`) is the one iteration form that tolerates removing the current node.

## Operations

- Mutation: `Insert( key, values... )` (BST descent + red insert + `InsertFixup`; duplicate keys return `NIL_INDEX`), `Remove( index )` / `RemoveNode` (successor transplant + `EraseFixup` + compaction; returns the *remapped* successor index), `FindAndRemove`, `Clear`.
- Search: `Find`, `LowerBound`, `UpperBound`, `Contains`, plus `*Iterator` forms — all O(log n) descents.
- Iteration: `FirstIndex`/`NextIndex`/`PrevIndex`/`EndIndex` (in-order), `begin()/end()` via [CSlotIterator](BTL.CSlotIterator.md) (bidirectional; `--end()` yields the rightmost node), and the `BALL_RBTREE_FOREACH*` macros including dense unordered sweeps.
- Column access: `Get< TN >( index | iterator )` by column position (0 = key) or `Get< T >` by unique column type; `CRBTree` adds `Key()`/`Get< 1 >()`.
- `Validate()` — full structural check (links, ordering, red/black rules, black-height, dense-slot coverage); used by tests.
- Cross-capacity `CopyFrom`/`MoveFrom` rebuild row-by-row in key order (O(m + n log n)); move clears the source.

## Usage

```cpp
BTL::RBTree32_t< int, float > map;          // key int, value float
auto i = map.Insert( 42, 1.5f );            // node index or NIL_INDEX if duplicate
if ( map.Contains( 42 ) )
    map.Get< 1 >( map.Find( 42 ) ) = 2.5f;
BALL_RBTREE_FOREACH( map, it )              // ascending key order
    Use( map.Key( it ), map.Get< 1 >( it ) );
```

## Notes

The balancing logic follows the textbook (CLRS) insert/erase fix-up model, with `NIL_INDEX` standing in for the shared NIL sentinel; the in-source comments document each case. Copy and move between capacity flavors are structural rebuilds, never pointer steals.
