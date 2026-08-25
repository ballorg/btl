# BTL::CHashMap

## Overview

A structure-of-arrays open-addressing hash table with unique keys: linear probing, Fibonacci (golden-ratio) bucket mapping, backward-shift deletion (no tombstones), and **no stored size** — the live count is recovered by popcounting the 1-bit slot-state column.

Family members:

- `CHashMapBase< I, N, TI, C, K, Ts... >` — protected core (probing, growth, rehash).
- `CHashMapImpl< … >` — public facade (`Insert`, `Find`, `Remove`, iteration, copy/move).
- `CHashMap< I, K, C, Ts... >` (heap) / `CBufferHashMap< I, N, K, C, Ts... >` (inline) — capacity wrappers.
- `CHashMap< I, K, C, Ts... >` provides `Key()` and indexed or typed column access through `Get`; the variadic `HashMap*_t` aliases cover sets, one-value maps, and multi-column maps with the default hash policy.
- `EHashSlotState` — 1-bit `FREE`/`OCCUPIED` state enum (packed column).
- **Aliases:** variadic `HashMap_t`/`HashMap8_t`…`HashMap64_t` and `BufferHashMap*_t`.

## Declaration

- **Namespace:** `BTL`
- **Module:** [associative](../modules/associative.md)
- **Kind:** class templates; `CHashMapBase` derives from the hash policy `C` (default [CFibonacciHash](BTL.CFibonacciHash.md), empty base) and from `CBufferVector< I, N, EHashSlotState, HashKeyColumn_t< K >, Ts... >`
- **Declared in:** [include/ball/types/hashmap.hpp](../../include/ball/types/hashmap.hpp)

## Purpose

Expected-O(1) unordered key lookup as a map, set (`Ts...` empty), or multi-column map, with per-slot overhead of exactly one state bit and SoA column layout.

## Data Structure

The SoA row count **is** the bucket count (always a power of two of at least 8 once initialized). Columns in order:

1. **Slot state** — `EHashSlotState`, bit-packed (1 bit/bucket); value-initialization yields `FREE`, so freshly grown tables start empty for free. Only two states exist because deletion keeps probe chains gap-free.
2. **Key** — `HashKeyColumn_t< K >` (tagged [CReflect](BTL.CReflect.md)) so a key never aliases a same-typed value column.
3. **Value columns `Ts...`** — stored as supplied.

A key's home bucket is `Hasher()( key )` mapped through the Fibonacci multiply-shift onto the current capacity; collisions probe linearly with mask wrap-around. For a `CBufferHashMap` still using its inline `N` buckets, the compiler receives the corresponding index width as a constant, avoiding a capacity-log2 calculation on every lookup; an overflowed buffer falls back to the run-time-capacity path. There is no separate count member: `Count()` popcounts the state bits one 64-bit word at a time. An empty heap map has bucket count 0 and its first insert creates the minimum eight buckets (one byte of packed states). A buffered map is constructed empty with its power-of-two `N >= 8` buckets already active.

## Storage Model

Bucket columns follow the [multi-column CVector storage model](BTL.CVector.md): inline column buffers up to `N` rows (`CBufferHashMap`), then one shared heap block. A power-of-two `N >= 8` is the buffered form's reserve mechanism: construction activates those `N` inline rows without allocation, and the first insertion uses them directly without `ResetTable` or intermediate rehashes. Resizing goes through `Rehash` only when the active table really grows: live rows are snapshotted into an always-heap-backed scratch multi-column `CVector` (`SetCountRaw` plus move-construction), so a large `CBufferHashMap` does not duplicate its inline capacity on the stack. The table is then sized to the new power-of-two bucket count and each row is re-probed into its new home. The packed state column is cleared as one byte range rather than one slot at a time. `GrowTable` doubles the bucket count; the table never shrinks on removal.

## Ownership and Lifetime

Bucket storage is owned by the `CBufferVector` base. All buckets' payload cells are value-constructed when the table is sized (grow constructs every row); stale cells left by removal or growth are ignored while their state bit reads `FREE`.

## Type Relationships

- Hash policy: [CFibonacciHash](BTL.CFibonacciHash.md) (reachable via `Hasher()`).
- Storage: [CVector](BTL.CVector.md); iterator: [CSlotIterator](BTL.CSlotIterator.md); ordered counterpart: [CRBTree](BTL.CRBTree.md).

## Invariants

- Bucket count is 0 or a power of two ≥ 8, so wrap-around is a mask and the state column is a whole number of bytes.
- Probe chains are contiguous: between any occupied slot and its home bucket there is no `FREE` slot along the probe path (maintained by backward-shift deletion).
- Live count ≤ 3/4 · capacity is the growth target, enforced lazily.

## Invalidation Rules

Slot indices, iterators, and references are stable only until the next mutation: an `Insert` that grows the table rehashes every entry into new slots (and relocates all columns), and a `Remove` backward-shifts trailing cluster entries between slots. Reads and non-growing inserts do not move existing entries.

## Operations

- `Insert( key, values... )` — rejects duplicates (`NIL_INDEX`); its hot path performs probing and placement in one loop, keeping the packed-state and key-column bases hoisted through the operation. Growth is triggered lazily without a size field: only a probe reaching `PROBE_GROW_THRESHOLD` (8) pays for the exact popcount load check (`ShouldGrow`, target load 3/4); a completely full cycle grows unconditionally.
- `Find` / `Contains` / `FindIterator` — probe until match or `FREE`; while a buffered map remains at `N`, lookup selects its packed-state and key inline bases together and uses them directly, avoiding repeated inline/heap resolution in the probe path.
- `Remove( key )` — backward-shift deletion (Knuth 6.4R): slides trailing cluster entries whose home bucket permits it back over the hole, keeping chains contiguous with no tombstones.
- `Rehash( capacity )` — snapshots live rows into a raw-reserved scratch table (`SetCountRaw` + move-construct), resets the table to the new power-of-two size, and re-probes every row (run-time-only path; used by `GrowTable`, which doubles).
- Iteration: `FirstIndex`/`NextIndex`/`EndIndex` scan occupied slots in bucket order; forward-only [CSlotIterator](BTL.CSlotIterator.md) `begin()/end()`; `BALL_HASHMAP_FOREACH` macro. `Clear()` marks every slot free without releasing storage.
- Column access `Get< TN >( slot )` (0 = key); `CHashMap` adds `Key()`/`Get< 1 >()`.
- Copy/move (same or cross capacity) rebuild by re-inserting every row; move clears the source.

## Usage

```cpp
BTL::HashMap32_t< BTL::size32_t > set;
BTL::HashMap32_t< BTL::size32_t, int > map;
BTL::HashMap32_t< BTL::size32_t, int, float > columns;

set.Insert( 3u );
auto i = map.Insert( 7u, 100 );          // slot index or NIL_INDEX if present
columns.Insert( 9u, 200, 0.5f );
if ( map.Contains( 7u ) )
    map.Get< 1 >( map.Find( 7u ) ) += 1;
BALL_HASHMAP_FOREACH( map, it )          // occupied slots, bucket order
    Use( map.Key( it ), map.Get< 1 >( it ) );
```

## Notes

Iteration order is bucket order, which is a function of hashing — not insertion or key order. For the inline `CBufferHashMap`, choose `N` as a power of two ≥ 8 to actually stay inline. The convenience aliases fix the hash policy to `CFibonacciHash< I >`; instantiate `CHashMap` or `CBufferHashMap` directly when a custom policy is required.
