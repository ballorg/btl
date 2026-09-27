# Module: concurrency (atomics, locks, and the thread-safe SoA)

## Overview

Everything Ball needs to share a container between threads: an atomic value, a small family of mutexes built from it, the RAII guards that own them, and a thread-safe structure-of-arrays container that puts one of those mutexes over one [CVector](../types/BTL.CVector.md).

The layer is freestanding like the rest of the library. No standard-library synchronization header is involved: atomics go through the compiler's own intrinsics, and the mutexes are spin locks with a scheduler-yield fallback, needing only a spin hint and a yield entry point from the platform.

In module builds the declarations are owned by `Ball.Types:Atomic`, `Ball.Types:Lock` and `Ball.Types:Concurrent`, all re-exported by the public `Ball.Types` module.

## Responsibilities

- Lock-free atomic values of 1, 2, 4 and 8 bytes, over `__atomic_*` on GCC and Clang and `_Interlocked*` on MSVC ([CAtomic](../types/BTL.CAtomic.md)).
- Exclusive and reader/writer mutexes, a null mutex, and RAII guards including a deadlock-free two-mutex guard ([CMutex family](../types/BTL.CMutex.md)).
- A thread-safe SoA container whose only state is the storage and the one mutex guarding it ([CConcurrent family](../types/BTL.CConcurrent.md)).
- Per-element atomic operations on the SoA columns declared atomic, and a clear boundary between what those guarantee and what needs the mutex.

## Public Interface

| Family | Types | Convenience aliases |
| --- | --- | --- |
| Atomic value | `CAtomic< T >`, `EMemoryOrder`, `AtomicThreadFence` | `AtomicBool_t`, `AtomicUInt32_t`, `AtomicUInt64_t`, `AtomicSize_t`, … |
| Mutexes | `CMutex`, `CSharedMutex`, `CNullMutex`, `CSpinBackoff` | — |
| Lock guards | `CUniqueLock< TMutex >`, `CSharedLock< TMutex >`, `CScopedLock< TMutex >`, `DEFER_LOCK`, `ADOPT_LOCK` | — |
| Concurrent SoA | `CConcurrentBase`, `CConcurrent`, `CBufferConcurrent`, `CAtomic< I, T0, Ts... >` | `Concurrent(16/32/64)_t`, `BufferConcurrent(32/64)_t< N >`, `AtomicConcurrent(32/64)_t` |
| Policies | `CMutexSynchronization`, `CSharedMutexSynchronization`, `CAtomicSynchronization` | — |

Traits: `IS_ATOMIC< T >` and `AtomicValue_t< T >` identify an atomic column and name the value it carries; both match the one-argument `CAtomic< T >` only.

`CAtomic` is one name over both atomic shapes, separated by argument count: `CAtomic< T >` is the value, a partial specialization declared in the `Atomic` partition, and `CAtomic< I, T0, Ts... >` is the unsynchronized SoA container, a partial specialization in the `Concurrent` partition. The primary template the `Atomic` partition declares is never defined; see [CConcurrent](../types/BTL.CConcurrent.md) for why.

## Dependencies

[containers](containers.md) (the `CBufferVector` that holds the rows), [meta](meta.md) (`MIndexType`, index sequences, the trivially-copyable and integral traits, `MFixedMetadata` for packed columns). The global fragments pull in the intrinsic declarations from `c/atomic.h`, the spin and yield hooks from `c/thread.h`, and the assertion macros.

Nothing in this module depends on [associative](associative.md), [strings](strings.md) or [reflection](reflection.md), though a user's SoA columns may of course use reflect tags to keep same-typed columns apart.

## Data Structures

- [BTL::CAtomic](../types/BTL.CAtomic.md) — atomic value, memory-order enumeration, per-width backend, atomic-column traits.
- [BTL::CMutex](../types/BTL.CMutex.md) — the mutex family, the spin backoff, and the RAII guards.
- [BTL::CConcurrent](../types/BTL.CConcurrent.md) — the thread-safe SoA family, its synchronization policies and its access objects.

## Relationships

The layer stacks cleanly: `CAtomic` is the only primitive, the mutexes are one `CAtomic` plus a waiting strategy, the guards own a mutex, and the container owns a `CBufferVector` and one mutex. Nothing above reaches around anything below.

One synchronization domain covers a whole container, never a column or an element, which is what makes it impossible for a reader to see one column of a row updated and another not. Atomic columns sit *inside* that storage rather than beside it — an atomic operation on a row takes shared access to keep the row from being relocated, then performs a single lock-free instruction on the actual field. Composing several of those is not a multi-field atomic update; that requires the mutex, and the container says so explicitly.
