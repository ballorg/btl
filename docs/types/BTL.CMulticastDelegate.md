# BTL::CMulticastDelegate

## Overview

A broadcast list of `void`-returning [delegates](BTL.CDelegate.md): callers add bindings and receive a stable `DelegateHandle_t` to remove them later; `Broadcast( args... )` invokes every live entry. A lock counter makes removal during a broadcast safe (slots are invalidated in place and physically compacted later).

## Declaration

- **Namespace:** `BTL`
- **Module:** [delegates](../modules/delegates.md)
- **Kind:** class template `CMulticastDelegate< typename... Ts >`; companion value type `DelegateHandle_t`; alias `MulticastDelegate_t< void( Ts... ) >`; macros `BALL_DECLARE_MULTICAST_DELEGATE(_EVENT)`
- **Declared in:** [include/ball/types/delegate.hpp](../../include/ball/types/delegate.hpp)

## Purpose

An event/observer primitive: many listeners, one broadcast, handle- and owner-based removal.

## Data Structure

Two members: a lock counter (`m_nLocks`, plain `size32_t` — a TODO in the source notes it is not atomic) and `Vector32_t< DelegateHandle_t, SingleDelegate_t > m_Events` — a two-column [SoA container](BTL.CVector.md) pairing each slot's handle with its delegate. A slot with an invalid handle is free and reusable. `DelegateHandle_t` wraps a `uint_t` id drawn from a process-wide monotonically increasing counter (`INVALID_ID == ~0`); its move resets the source.

## Ownership and Lifetime

Owns its delegate entries (which in turn own their bound implementations, but not the target objects). Handles are value tokens; a reset or stale handle simply fails to match.

## Type Relationships

- Slots: [CVector](BTL.CVector.md); entries: [CDelegate](BTL.CDelegate.md).

## Invariants

- A slot is live iff its handle is valid; `Broadcast` never executes invalidated slots.
- Physical compaction happens only while unlocked, so indices held by the broadcast loop stay valid for its duration.

## Invalidation Rules

`DelegateHandle_t` values remain valid across slot growth, compaction, and unrelated removals — removal matches by id, not by position. Slot *indices* and references into `m_Events` are internal and unstable: adds can grow the container and removals swap slots. Removing during `Broadcast` is safe (the slot is only invalidated, not moved).

## Operations

- Adding: `Add` (copy/move of a prepared delegate; reuses the first free slot, else appends), `AddStatic`, `AddLambda`, `AddMember` (const/non-const), `operator+=`; each returns the new handle.
- Removal: `Remove( handle )` (resets the handle), `RemoveObject( ptr )` (clears every binding owned by the object), `operator-=`, `RemoveAll`. When locked (inside `Broadcast`), removal only invalidates the slot; unlocked removal swaps the slot with the last row and pops it.
- `Broadcast( args... )`: locks, executes every slot with a valid handle in slot order, unlocks. `Compress` compacts free slots; `GetSize` counts live bindings; `IsBoundTo` checks a handle.
- `BALL_DECLARE_MULTICAST_DELEGATE_EVENT( name, owner, ... )` emits a subclass whose `Broadcast`/`Remove*` are private except to `owner` — listeners can subscribe, only the owner can fire.

## Notes

The lock counter guards against reentrant structural changes from within callbacks, not against concurrent access from other threads.
