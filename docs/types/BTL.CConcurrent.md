# CConcurrent

## Overview

`CConcurrent` and its siblings make one structure-of-arrays container safe to share between threads. The design rule is that the object is exactly two things — the SoA storage and the one synchronization primitive guarding it — and that nothing else is stored. The row count, the emptiness, the capacity and every element are read out of the storage under a lock; there is no cached size, no generation counter, no validity bitmap, no second representation to keep reconciled.

`CConcurrentBase< TSync, I, N, Ts... >` carries the whole implementation. The public spellings differ only in the synchronization policy and the inline capacity.

## Declaration

- **Namespace:** `BTL`
- **Module:** `Ball.Types` (partition `Ball.Types:Concurrent`)
- **Kind:** class templates and policy structures
  - `CConcurrentBase< typename TSync, typename I, I N, typename... Ts >`
  - `CConcurrent< typename I, typename... Ts >` — shared-mutex policy, heap-backed
  - `CBufferConcurrent< typename I, I N, typename... Ts >` — shared-mutex policy, inline buffer
  - `CAtomic< typename I, typename T0, typename... Ts >` — no locking; element operations synchronize themselves. This is a partial specialization of the name [CAtomic](BTL.CAtomic.md) shares with the atomic value: one argument selects the value, an index type plus at least one column selects this container
  - `CMutexSynchronization`, `CSharedMutexSynchronization`, `CAtomicSynchronization` — the policies
  - Nested: `CReadAccess`, `CWriteAccess`, `CRow< TStorage >`, `CRowIterator< TStorage >`
- **Declared in:** [include/ball/types/concurrent.hpp](../../include/ball/types/concurrent.hpp)
- **Aliases:** `Concurrent(16/32/64)_t`, `BufferConcurrent(32/64)_t< N >`, `AtomicConcurrent(32/64)_t`

## Purpose

Let several threads share one SoA container while guaranteeing that no thread ever observes it between two columns of the same logical update — and do so without introducing a parallel state model beside the storage.

## Data Structure

Two members, and only two:

- `m_Data` — a [CBufferVector](BTL.CVector.md)`< I, N, Ts... >`, the single authoritative state. Its `Ts...` are the SoA columns; each is a contiguous array and all of them share one row count, so the SoA invariant (every column reports the same logical size) is a property of the storage rather than something this class maintains.
- `m_Mutex` — one mutex, chosen by the policy, marked `mutable` so a read can lock through a `const` container.

Nothing derived from the contents is stored. `Size()` calls `m_Data.Count()` under a lock; `Capacity()` calls `m_Data.Capacity()`, which the vector itself derives from the row count. An `CAtomic< size_t >` shadow of the size would be faster to read and is deliberately absent: one authoritative state is worth more than a cheap read of a stale one.

The empty state is an empty storage. A moved-from container is an empty container, not a special state.

### Synchronization policies

| Policy | Mutex | Read access | Use |
| --- | --- | --- | --- |
| `CSharedMutexSynchronization` | [CSharedMutex](BTL.CMutex.md) | concurrent | the default |
| `CMutexSynchronization` | [CMutex](BTL.CMutex.md) | serialized | rare reads, or very short critical sections |
| `CAtomicSynchronization` | `CNullMutex` | unsynchronized | every column atomic and the shape never changes while shared |

`SHARED_READS` reports whether reads actually run concurrently under the chosen policy.

## Storage Model

Row storage follows the [CVector inline-buffer model](BTL.CVector.md): up to `N` rows live in the inline buffers (`CBufferConcurrent`), and past that the columns move together into one shared heap block. There is never an allocation per column or per element, and the thread-safe wrapper adds no per-element overhead at all — no lock, no flag and no version beside a row. The only synchronization cost is the one mutex guarding the whole SoA.

## Ownership and Lifetime

The container owns its storage and its mutex.

Copy construction, copy assignment, move construction and move assignment are all supported and all transfer **only the SoA contents**; the mutex is never copied. A copy reads the source under a shared lock; an assignment locks both objects through `CScopedLock`, which orders the two acquisitions by address so two threads assigning the same pair in opposite directions cannot deadlock. A move empties the source.

Duplication is a row-by-row rebuild rather than a block steal — the same convention [CRBTree](BTL.CRBTree.md) and [CHashMap](BTL.CHashMap.md) follow.

References, row handles and iterators obtained from an access object are borrowed from the storage and are valid **only while that access object is alive**. Nothing hands out a reference that outlives its lock.

## Type Relationships

- Stores a [CBufferVector](BTL.CVector.md), Ball's single SoA implementation; this container adds no storage substrate of its own.
- Guarded by [CMutex, CSharedMutex or CNullMutex](BTL.CMutex.md), held through `CUniqueLock` / `CSharedLock` / `CScopedLock`.
- A column declared [CAtomic](BTL.CAtomic.md)`< U >` gains per-element atomic operations; `IS_ATOMIC` is how the container decides which columns those apply to. The trait matches the one-argument form only, so the lock-free container spelling `CAtomic< I, Ts... >` is never mistaken for an atomic column.
- Column types must be pairwise distinct, the rule every Ball SoA container follows, because the substrate addresses columns by type. Wrap duplicates with `BALL_REFLECT_TAGGED(_TEMPLATE)` from the [reflection module](../modules/reflection.md).

## Invariants

- Every column holds the same number of logical elements, at every moment another thread can observe the container.
- No structural operation is visible half-applied: a push, insert, erase, resize, clear, swap or sort completes across all columns before its lock is released.
- Row `i` means the same logical element in every column, and a reordering applies one permutation to all of them.
- Nothing outside `m_Data` describes the contents.

## Invalidation Rules

- **Every structural modification invalidates every outstanding reference, row handle and iterator**, because it can relocate rows or reallocate the shared block. Since such a modification requires exclusive access, no other thread can be holding one at the time.
- Releasing an access object ends the validity of everything obtained through it.
- Row *indices* are stable only against operations that do not shift rows: `Erase` and `Insert` renumber every row after the affected one, and `SortBy` and `SwapElements` renumber arbitrarily.
- Copy assignment, move assignment, `Clear`, `Resize` and `Swap` invalidate everything.

## Operations

### Access objects (transactions)

- `ReadAccess()` — a `CReadAccess` holding shared access for its lifetime. Exposes `Size`, `Empty`, `Capacity`, `Get< FIELD >( i )`, `Row( i )`, `Column< FIELD >()`, `Data()` and `begin()`/`end()`. Everything read through one access object belongs to one consistent moment of the container.
- `WriteAccess()` — a `CWriteAccess` holding exclusive access. Adds `Set< FIELD >`, `SetRow`, `PushBack`, `Insert`, `Erase`, `PopBack`, `Resize`, `Clear`, `SwapElements` and `SortBy< FIELD >`.

Anything whose outcome depends on state it just read — the classic check-then-act — must run inside one access object. That is what makes the sequence a single transaction.

### One-shot operations

`Size`, `Empty`, `Capacity`, `Get< FIELD >`, `Set< FIELD >`, `SetRow`, `PushBack`, `Insert`, `Erase`, `Resize`, `Clear`, `SwapElements`, `SortBy< FIELD >` each take and release the lock themselves. `TryPopBack( out... )` removes the last row and returns its columns, testing emptiness and removing inside one exclusive transaction so two threads cannot both claim the same row.

### Snapshot

- `SnapshotInto( out )` — refills any Ball SoA container with an independent copy, read under a shared lock and released before returning.
- `Snapshot()` — the same as an independent concurrent container.

Neither stores anything about the snapshot in the container.

### Atomic element operations

`Load`, `Store`, `Exchange`, `FetchAdd`, `FetchSub` and `CompareExchange`, each templated on the column index and available only for a column declared `CAtomic< U >`. Each holds **shared** access for its duration: the shared lock is what pins the rows in place while the operation runs, and the atomic instruction is what orders the element against other threads.

They do **not** compose. Two atomic field updates on the same row can be observed half-done; when fields share an invariant, use `SetRow` or a `WriteAccess` transaction instead.

### Complexity

`R` is the row count and `C` the column count. Every entry adds one lock acquire and release.

| Operation | Complexity |
| --- | ---: |
| `Size`, `Empty`, `Capacity` | O(1) |
| `Get`, `Set`, atomic element operations | O(1) |
| `PushBack` | O(1) amortized |
| `Insert`, `Erase` at index `i` | O((R − i) · C) |
| `Resize` | O(\|new − old\| · C) |
| `SwapElements` | O(C) |
| `SortBy` | O(R log R) comparisons, each swap O(C) |
| `TryPopBack` | O(C) |
| Copy, move, assignment, `SnapshotInto` | O(R · C) |
| `Swap` | O((R + other R) · C) |

## Usage

```cpp
using Counter_t = BTL::CAtomic< BTL::uint64_t >;
using Soa_t = BTL::Concurrent32_t< BTL::uint64_t, double, Counter_t >;

Soa_t soa;

soa.PushBack( 7u, 3.5, Counter_t( 0u ) );          // one-shot: locks and releases

// A transaction: the size test and the erase it decides on are one operation,
// so no other thread can slip between them.
{
    auto access = soa.WriteAccess();

    if ( access.Size() > 1000 )
        access.Erase( 0 );

    access.SortBy< 0 >();                           // permutes every column alike
}

// Iteration holds its lock for the whole walk.
{
    auto access = soa.ReadAccess();

    for ( auto row : access )
        Use( row.Get< 0 >(), row.Get< 1 >() );
}

soa.FetchAdd< 2 >( 0, 1u );                         // lock-free element, shared lock
```

## Notes

The container deliberately offers no `Reserve`. Ball derives a vector's capacity from its row count (`BitCeil( Count() )`) and stores no capacity field, so reserving without resizing is not expressible; `Resize` is the operation that changes capacity, and it does so across the whole SoA under exclusive access.

`SnapshotInto` takes an out-parameter rather than returning an SoA by value because a multi-column [CVector](BTL.CVector.md) currently has no working copy or move constructor — its view-based copy path narrows an SoA view to a single column and does not compile when instantiated. Every duplication in this container therefore goes through a row-by-row rebuild, which is the library's established convention anyway. `Snapshot()` returns a concurrent container instead, whose copy constructor this class implements itself.

`CAtomicSynchronization` moves the whole burden onto the caller: with no lock taken, a structural modification must not run while another thread is inside the container, because it relocates rows and a relocation is a plain byte copy. It is meant for a container whose shape is fixed after construction and whose columns are all atomic.

That policy's container is spelled `CAtomic`, the same name as the atomic value, because both are the atomic storage this library offers and the argument count already separates them: `CAtomic< T >` is the value, `CAtomic< I, Ts... >` the container. The declaration lives in [atomic.hpp](../../include/ball/types/atomic.hpp) so the value can be a partial specialization of it; the container is a second partial specialization, `CAtomic< I, T0, Ts... >`, defined here, and the primary template stays undefined. It is deliberately not a definition of the primary template: MSVC 19.44 fails with an internal compiler error (C1001) when importing a partition that defines a primary class template declared in another partition, whereas a partial specialization imports cleanly. Its convenience aliases are `AtomicConcurrent(32/64)_t` rather than `Atomic(32/64)_t`, which would read as a 32- or 64-bit atomic value next to `AtomicUInt32_t`.

The concurrency guarantees are exercised by [case13_concurrent.cpp](../../src/ball/types/tests/case13_concurrent.cpp) — parallel appends, readers against writers, racing `TryPopBack`, atomic field updates, and sort/swap/copy under contention — and the whole suite runs clean under ThreadSanitizer through the `ThreadSanitizer` CMake preset.
