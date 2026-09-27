# CMutex

## Overview

The locking layer: three mutexes with a common contract, the waiting strategy they share, and the RAII guards that own them. Everything here is built from [CAtomic](BTL.CAtomic.md) and one platform hook, so the library stays freestanding — no standard-library synchronization header is involved.

`CMutex` is an exclusive mutex; `CSharedMutex` admits many readers or one writer; `CNullMutex` locks nothing and exists so a policy can opt out without a second code path.

## Declaration

- **Namespace:** `BTL`
- **Module:** `Ball.Types` (partition `Ball.Types:Lock`)
- **Kind:** classes and tag types
  - `CSpinBackoff` — escalating wait
  - `CMutex`, `CSharedMutex`, `CNullMutex` — the mutexes
  - `CUniqueLock< TMutex >`, `CSharedLock< TMutex >`, `CScopedLock< TMutex >` — RAII guards
  - `MDeferLock` / `DEFER_LOCK`, `MAdoptLock` / `ADOPT_LOCK` — constructor tags
- **Declared in:** [include/ball/types/lock.hpp](../../include/ball/types/lock.hpp); the spin-wait and yield hooks are declared in [include/ball/types/c/thread.h](../../include/ball/types/c/thread.h)

## Purpose

Guard a shared object as a whole. [CConcurrent](BTL.CConcurrent.md) uses one of these per container, and nothing finer: one domain over the complete SoA is what makes it impossible to observe one column updated and another not.

## Data Structure

Each mutex is a single `CAtomic< uint32_t >` and nothing else.

`CMutex` uses that word as a flag: zero is free, one is held.

`CSharedMutex` packs three things into it:

| Bits | Meaning |
| --- | --- |
| 31 (`WRITER`) | a writer owns the mutex |
| 30 (`WRITER_PENDING`) | a writer is waiting; arriving readers must queue |
| 0–29 (`READER_MASK`) | how many readers are currently inside |

The pending bit is what stops a stream of readers from starving a writer: once it is set, a new reader queues behind the writer instead of joining the batch already inside. A writer clears it as part of the same compare-exchange that takes ownership.

`CNullMutex` is empty; every member is a `static constexpr` no-op.

`CSpinBackoff` holds one round counter. Early rounds spin with the processor's spin-wait hint, doubling the spin length each time; past `SPIN_ROUNDS` the wait hands the time slice back to the scheduler instead. That keeps an uncontended acquire in user space while a long wait stops burning a core.

## Storage Model

Everything is inline: a mutex is four bytes, a guard is a pointer plus a flag. Nothing is allocated, and there is no kernel object to create — waiting is spinning with a yield fallback, through `BALL_CPU_RELAX` and `BALL_YIELD_THREAD`.

## Ownership and Lifetime

A mutex owns no resource. It is neither copyable nor movable, because a waiter holds its address for the whole wait.

`CUniqueLock` and `CSharedLock` hold a non-owning pointer to a mutex and a flag saying whether they currently hold it; the mutex must outlive the guard. Both are movable, so an access object can carry its lock out of the function that took it; a moved-from guard owns nothing and releases nothing. `CScopedLock` is neither copyable nor movable.

## Type Relationships

- Built on [CAtomic](BTL.CAtomic.md).
- All three mutexes satisfy one contract — `Lock`, `TryLock`, `Unlock`, `LockShared`, `TryLockShared`, `UnlockShared` — so a single guard template and a single container template cover every one of them. On `CMutex` the shared members are aliases of the exclusive ones: correct, simply not concurrent.
- Selected by the synchronization policies of [CConcurrent](BTL.CConcurrent.md).

## Invariants

- A held `CMutex` has a non-zero word; releasing one that is not held is an assertion failure.
- In `CSharedMutex`, `WRITER` and a non-zero reader count are never set at the same time.
- The reader count never wraps; overflowing it is an assertion failure.
- A guard holds its mutex exactly while its `Owns()` is true.

## Invalidation Rules

Neither mutexes nor guards move, so no address they hand out is invalidated. Destroying a mutex while a guard still refers to it, or while a thread is waiting on it, is undefined.

## Operations

- `Lock` / `TryLock` / `Unlock` — exclusive acquisition. `TryLock` never waits.
- `LockShared` / `TryLockShared` / `UnlockShared` — shared acquisition.
- `CUniqueLock( mutex )`, `CSharedLock( mutex )` — acquire on construction, release on destruction. The `DEFER_LOCK` and `ADOPT_LOCK` tags construct without acquiring, and construct around a lock already held, respectively.
- `CScopedLock( first, second )` — takes both exclusively, always lowest address first, so two threads locking the same pair from opposite sides agree on the order and cannot deadlock. Locking a mutex against itself is detected and taken once.

Every operation is O(1) uncontended; a contended acquire is bounded by how long the holder keeps the lock.

## Usage

```cpp
BTL::CSharedMutex mutex;

{
    BTL::CSharedLock< BTL::CSharedMutex > readers( mutex );   // many at once
    Read();
}

{
    BTL::CUniqueLock< BTL::CSharedMutex > writer( mutex );    // excludes everyone
    Write();
}

// Two objects at once, without an ordering rule at the call site.
BTL::CScopedLock< BTL::CSharedMutex > both( left.Mutex(), right.Mutex() );
```

## Notes

These are **spin locks with a yield fallback**, not kernel futexes. That is the right trade for the critical sections Ball's concurrent containers take — a handful of column writes — and it keeps the library free of any threading header or system synchronization object. A workload that holds a container locked across a long or blocking operation should not use them.

None of the mutexes is recursive: a thread that locks one twice deadlocks. `CSharedMutex` offers no upgrade — a thread holding shared access must release it before asking for exclusive access.

Because they are built from `CAtomic`'s acquire and release operations, ThreadSanitizer models the happens-before edges they create and reports genuine races through them rather than false positives on the locks themselves.
