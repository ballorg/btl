# BTL::CMultiHashMap

## Overview

A structure-of-arrays open-addressing hash table with unique keys: linear probing, Fibonacci (golden-ratio) bucket mapping, backward-shift deletion (no tombstones), and **no stored size** — the live count is recovered by popcounting the 1-bit slot-state column.

Family members:

- `CMultiHashMapBase< I, N, TI, C, K, Ts... >` — protected core (probing, growth, rehash).
- `CMultiHashMapImpl< … >` — public facade (`Insert`, `Find`, `Remove`, iteration, copy/move).
- `CMultiHashMap< I, K, C, Ts... >` (heap) / `CBufferMultiHashMap< I, N, K, C, Ts... >` (inline) — capacity wrappers.
- `CHashMap< I, K, V, C, N >` — single-value map spelling with `Key()`/`Value()`.
- `EHashSlotState` — 1-bit `FREE`/`OCCUPIED` state enum (packed column).
- **Aliases:** `HashMap_t`/`HashMap8_t`…`HashMap64_t`, `BufferHashMap*_t`, `MultiHashMap*_t`, `BufferMultiHashMap*_t`.

## Declaration

- **Namespace:** `BTL`
- **Module:** [associative](../modules/associative.md)
- **Kind:** class templates; `CMultiHashMapBase` derives from the hash policy `C` (default [CFibonacciHash](BTL.CFibonacciHash.md), empty base) and from `CBufferMultiVector< I, N, TI, EHashSlotState, HashKeyColumn_t< K >, Ts... >`
- **Declared in:** [include/ball/types/hashmap.hpp](../../include/ball/types/hashmap.hpp)

## Purpose

Expected-O(1) unordered key lookup as a map, set (`Ts...` empty), or multi-column map, with per-slot overhead of exactly one state bit and SoA column layout.

## Data Structure

The SoA row count **is** the bucket count (always a power of two; `INITIAL_CAPACITY == 8`). Columns in order:

1. **Slot state** — `EHashSlotState`, bit-packed (1 bit/bucket); value-initialization yields `FREE`, so freshly grown tables start empty for free. Only two states exist because deletion keeps probe chains gap-free.
2. **Key** — `HashKeyColumn_t< K >` (tagged [CReflect](BTL.CReflect.md)) so a key never aliases a same-typed value column.
3. **Value columns `Ts...`** — stored as supplied.

A key's home bucket is `Hasher()( key )` mapped through the Fibonacci multiply-shift onto the current capacity; collisions probe linearly with mask wrap-around. There is no separate count member: `Count()` popcounts the state bits one 64-bit word at a time. An empty map has bucket count 0; the first insert sizes the table to `INITIAL_CAPACITY`.

## Storage Model

Bucket columns follow the [CMultiVector storage model](BTL.CMultiVector.md): inline column buffers up to `N` rows (`CBufferMultiHashMap`), then one shared heap block. Resizing goes through `Rehash`: live rows are snapshotted into an always-heap-backed scratch `CMultiVector` (`SetCountRaw` plus move-construction), so large inline maps do not duplicate `N` rows on the call stack. The moved-from old rows are then dropped (releasing overflow storage when present), the table is sized to the new power-of-two bucket count, and each row is re-probed into its new home. The packed state column is cleared as one byte range rather than one slot at a time. `GrowTable` doubles the bucket count; the table never shrinks on removal.

## Ownership and Lifetime

Bucket storage is owned by the `CBufferMultiVector` base. All buckets' payload cells are value-constructed when the table is sized (grow constructs every row); stale cells left by removal or growth are ignored while their state bit reads `FREE`.

## Type Relationships

- Hash policy: [CFibonacciHash](BTL.CFibonacciHash.md) (reachable via `Hasher()`).
- Storage: [CMultiVector](BTL.CMultiVector.md); iterator: [CSlotIterator](BTL.CSlotIterator.md); ordered counterpart: [CMultiRBTree](BTL.CMultiRBTree.md).

## Invariants

- Bucket count is 0 or a power of two ≥ 8, so wrap-around is a mask and the state column is a whole number of bytes.
- Probe chains are contiguous: between any occupied slot and its home bucket there is no `FREE` slot along the probe path (maintained by backward-shift deletion).
- Live count ≤ 4/5 · capacity is the growth target, enforced lazily.

## Invalidation Rules

Slot indices, iterators, and references are stable only until the next mutation: an `Insert` that grows the table rehashes every entry into new slots (and relocates all columns), and a `Remove` backward-shifts trailing cluster entries between slots. Reads and non-growing inserts do not move existing entries.

## Operations

- `Insert( key, values... )` — rejects duplicates (`NIL_INDEX`); its hot path performs probing and placement in one loop, keeping the packed-state and key-column bases hoisted through the operation. Growth is triggered lazily: only a probe reaching `PROBE_GROW_THRESHOLD` (13) — the local symptom of high load — pays for the exact popcount load check (`ShouldGrow`, target load 4/5); a completely full cycle grows unconditionally.
- `Find` / `Contains` / `FindIterator` — probe until match or `FREE`.
- `Remove( key )` — backward-shift deletion (Knuth 6.4R): slides trailing cluster entries whose home bucket permits it back over the hole, keeping chains contiguous with no tombstones.
- `Rehash( capacity )` — snapshots live rows into a raw-reserved scratch table (`SetCountRaw` + move-construct), resets the table to the new power-of-two size, and re-probes every row (run-time-only path; used by `GrowTable`, which doubles).
- Iteration: `FirstIndex`/`NextIndex`/`EndIndex` scan occupied slots in bucket order; forward-only [CSlotIterator](BTL.CSlotIterator.md) `begin()/end()`; `BALL_HASHMAP_FOREACH` macro. `Clear()` marks every slot free without releasing storage.
- Column access `Get< TN >( slot )` (0 = key); `CHashMap` adds `Key()`/`Value()`.
- Copy/move (same or cross capacity) rebuild by re-inserting every row; move clears the source.

## Usage

```cpp
BTL::HashMap32_t< BTL::size32_t, int > map;
auto i = map.Insert( 7u, 100 );          // slot index or NIL_INDEX if present
if ( map.Contains( 7u ) )
    map.Value( map.Find( 7u ) ) += 1;
BALL_HASHMAP_FOREACH( map, it )          // occupied slots, bucket order
    Use( map.Key( it ), map.Value( it ) );
```

## Notes

Iteration order is bucket order, which is a function of hashing — not insertion or key order. For the inline `CBufferMultiHashMap`, choose `N` as a power of two ≥ 8 to actually stay inline.
