# CAtomic

## Overview

`CAtomic< T >` is Ball's lock-free atomic value. It gives one 1-, 2-, 4- or 8-byte trivially copyable value the load, store, exchange, compare-exchange and read-modify-write operations the C++ memory model defines, without including a standard-library header: GCC and Clang go through the `__atomic_*` builtins, MSVC through its `_Interlocked*` intrinsics.

It exists so a value can be shared between threads, and so an SoA column can be made atomic — [CConcurrent](BTL.CConcurrent.md) offers per-element atomic operations on exactly the columns declared `CAtomic< U >`.

The name carries two shapes, told apart by argument count. `CAtomic< T >` — one argument — is this atomic value, written as a partial specialization. `CAtomic< I, T0, Ts... >` — an index type followed by one or more columns — is the unsynchronized concurrent SoA container, a second partial specialization documented in [CConcurrent](BTL.CConcurrent.md) and defined in [include/ball/types/concurrent.hpp](../../include/ball/types/concurrent.hpp). The primary template `CAtomic< I, Ts... >` is only declared here and never defined, so `CAtomic< I >` with no columns is an incomplete type. The first parameter therefore changes meaning with the argument count: `CAtomic< uint32_t >` is an atomic 32-bit value, while `CAtomic< uint32_t, uint64_t >` is a one-column SoA indexed by `uint32_t`.

## Declaration

- **Namespace:** `BTL`
- **Module:** `Ball.Types` (partition `Ball.Types:Atomic`)
- **Kind:** class template, enumeration, and supporting traits
  - `CAtomic< typename I, typename... Ts >` — primary template, declared here and never defined; both shapes are partial specializations of it
  - `CAtomic< typename I, typename T0, typename... Ts >` — partial specialization, the concurrent SoA, defined by [CConcurrent](BTL.CConcurrent.md)
  - `CAtomic< typename T >` — partial specialization, the atomic value described by this document
  - `EMemoryOrder` — `RELAXED`, `CONSUME`, `ACQUIRE`, `RELEASE`, `ACQ_REL`, `SEQ_CST`
  - `MAtomicWord< size_t SIZE >` — the unsigned word a width is carried in
  - `MAtomicOps< size_t SIZE >` — the per-width backend
  - `MIsAtomic< T >` / `IS_ATOMIC< T >`, `MAtomic< T >` / `AtomicValue_t< T >`
  - `AtomicThreadFence( EMemoryOrder )`
- **Declared in:** [include/ball/types/atomic.hpp](../../include/ball/types/atomic.hpp); the compiler intrinsics it needs are declared in [include/ball/types/c/atomic.h](../../include/ball/types/c/atomic.h)
- **Aliases:** `AtomicBool_t`, `AtomicChar_t`, `AtomicUInt16_t`, `AtomicUInt32_t`, `AtomicUInt64_t`, `AtomicSize_t`

## Purpose

Provide the ordering primitive the rest of the concurrency layer is built from — [CMutex and CSharedMutex](BTL.CMutex.md) are nothing but a `CAtomic< uint32_t >` and a waiting strategy — and let a value be shared between threads without a lock.

## Data Structure

One member: `m_nValue`, an unsigned word of exactly `sizeof( T )` bytes, aligned to its own size. The user's value is not stored as a `T`; it is bit-cast into that word on every write and back out on every read (`__builtin_bit_cast`, so the conversion is exact and constant-evaluable). That is what lets one backend serve integers, enumerations, pointers and floating-point values without a specialization each.

Natural alignment is what makes the access single-copy atomic on every target Ball builds for, and it is also what the SoA substrate gives a column, since it aligns to `alignof( CAtomic< T > )`.

## Storage Model

The value is stored inline; `sizeof( CAtomic< T > ) == sizeof( T )`. Nothing is allocated and nothing is shared. A `CAtomic` used as an SoA column is therefore laid out exactly like the equivalent plain column.

The backend is selected by width, not by type:

| Operation | GCC / Clang | MSVC |
| --- | --- | --- |
| Load | `__atomic_load_n` | plain read plus a barrier for an acquire order |
| Store | `__atomic_store_n` | plain write plus a barrier; `_InterlockedExchange` for `SEQ_CST` |
| Exchange, compare-exchange, fetch-* | the matching `__atomic_*` builtin | the matching full-barrier `_Interlocked*` |

The MSVC path is deliberately *stronger* than asked: its read-modify-writes are sequentially consistent whatever order the caller names, which is always sound and occasionally slower. The barrier is a compiler barrier on x86, whose hardware model only reorders store-load, and a `DMB ISH` on ARM.

## Ownership and Lifetime

`CAtomic` owns its value and nothing else. It has no reference to any other object and imposes no lifetime requirement beyond its own.

## Type Relationships

- Used by [CMutex, CSharedMutex](BTL.CMutex.md) as their entire state.
- Used by [CConcurrent](BTL.CConcurrent.md) as an opt-in column type; `IS_ATOMIC` is how that container decides which columns get atomic element operations.
- `AtomicValue_t< T >` unwraps `CAtomic< U >` to `U` and leaves any other type alone, so generic code can name the value a column carries.
- Both traits match the one-argument form only: `IS_ATOMIC< CAtomic< uint32_t > >` is true, while `IS_ATOMIC< CAtomic< uint32_t, float > >` is false and `AtomicValue_t` leaves that container alone. A concurrent SoA is not an atomic field, so a container never mistakes one for a column it can operate on atomically.

## Invariants

- `sizeof( T )` is 1, 2, 4 or 8, and `T` is trivially copyable; both are enforced by static assertion.
- The object is aligned to its own size, so every operation is lock-free.
- A default-constructed `CAtomic` holds a zero word.

## Invalidation Rules

The address of a `CAtomic` is stable for its lifetime; no operation moves it. Relocating one — which the SoA substrate does when a column grows or a row is erased — is a *non-atomic* byte copy and must not race with any access to it. See the note below.

## Operations

- `Load( order )`, `Store( value, order )`, `Exchange( value, order )` — order defaults to `SEQ_CST`.
- `CompareExchange( expected, desired, success, failure )` — writes `desired` only while the value still equals `expected`; on failure `expected` is updated with what was actually found, so the usual retry loop needs no extra read.
- `FetchAdd`, `FetchSub`, `FetchAnd`, `FetchOr`, `FetchXor` — constrained to integral `T`, since arithmetic on the bit-cast word of a float or a pointer would be meaningless. Each returns the value from *before* the operation.
- `operator T()`, `operator=`, `operator++`, `operator--` — sequentially consistent shorthands.
- `AtomicThreadFence( order )` — a standalone fence over the surrounding non-atomic accesses.

Every operation is O(1): a single instruction, or a short retry loop on a load-linked/store-conditional target.

## Usage

```cpp
BTL::CAtomic< BTL::uint32_t > nRefCount{ 1u };

nRefCount.FetchAdd( 1u, BTL::EMemoryOrder::RELAXED );

if ( nRefCount.FetchSub( 1u, BTL::EMemoryOrder::ACQ_REL ) == 1u )
    Destroy();

// The usual compare-exchange retry: the failed attempt refreshes nSeen for free.
BTL::CAtomic< BTL::uint32_t > nHighWater{ 0u };
BTL::uint32_t nSeen = nHighWater.Load( BTL::EMemoryOrder::RELAXED );

while ( nSeen < nCandidate && !nHighWater.CompareExchange( nSeen, nCandidate ) )
    ;
```

## Notes

`CAtomic` is **copyable and trivially copyable**, unlike `std::atomic`. That is a deliberate departure: an SoA column must be relocatable, because growing a [CVector](BTL.CVector.md), inserting into it or erasing from it moves rows with a raw byte copy. Those copies are not atomic. A `CAtomic` may therefore be copied only when no other thread can touch it — inside a container that means under exclusive access, which is exactly what a structural modification already holds ([CConcurrent](BTL.CConcurrent.md) guarantees this).

The default constructor value-initializes rather than leaving the value indeterminate, so a freshly grown SoA row starts at a defined zero.

`EMemoryOrder` is passed as an ordinary argument rather than a template parameter, matching the shape of the standard's API; callers pass a literal in practice and every backend folds it away.

The MSVC backend uses `_InterlockedExchange64` and friends for the 8-byte width, which 32-bit x86 does not provide; a `CAtomic` of that width is therefore supported on MSVC for 64-bit targets. GCC and Clang have no such restriction. The MSVC path is written against the documented intrinsics and is exercised by the Windows CI job rather than locally.
