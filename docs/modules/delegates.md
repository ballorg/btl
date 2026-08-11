# Module: delegates

## Overview

Type-erased callable binding in the style of Unreal's delegates, in [delegate.hpp](../../include/ball/types/delegate.hpp): a single-cast [CDelegate](../types/BTL.CDelegate.md) that stores one bound callable (free function, member function, or lambda, each with optional pre-bound payload arguments) inside inline storage, and a [CMulticastDelegate](../types/BTL.CMulticastDelegate.md) that broadcasts to a list of `void`-returning delegates addressed by stable handles.

## Responsibilities

- Bind and invoke callables without templates at the call site: `Execute`, `ExecuteIfBound`, `IsBound`, `IsBoundTo( object )`.
- Store the bound implementation in inline storage (`DELEGATE_INLINE_ALLOCATION_SIZE`, default 64 bytes, in `Variant_t`-sized blocks) managed by `CDelegateBase`.
- Append pre-bound payload values after the call-time arguments (`MPack` storage).
- Multicast: add/remove by `DelegateHandle_t`, remove all bindings of an object, broadcast with reentrancy protection (a lock counter defers physical removal during `Broadcast`).
- Declaration macros: `BALL_DECLARE_DELEGATE(_RET)`, `BALL_DECLARE_MULTICAST_DELEGATE`, `BALL_DECLARE_MULTICAST_DELEGATE_EVENT` (owner-restricted broadcast).

## Public Interface

| Type | Document |
| --- | --- |
| `IDelegateBase`, `IDelegate< R, Ts... >`, `CStaticDelegate`, `CObjectDelegate`, `CLambdaDelegate`, `CDelegateBase`, `CDelegate< R, Ts... >`, `Delegate_t< Sig >` | [BTL.CDelegate.md](../types/BTL.CDelegate.md) |
| `CMulticastDelegate< Ts... >`, `DelegateHandle_t`, `MulticastDelegate_t< Sig >` | [BTL.CMulticastDelegate.md](../types/BTL.CMulticastDelegate.md) |

## Data Structures

- [BTL::CDelegate](../types/BTL.CDelegate.md) — single-cast delegate family (`IDelegateBase`, `IDelegate`, `CStaticDelegate`, `CObjectDelegate`, `CLambdaDelegate`, `CDelegateBase`)
- [BTL::CMulticastDelegate](../types/BTL.CMulticastDelegate.md) — broadcast list and `DelegateHandle_t`

## Dependencies

[containers](containers.md) (`BufferVector_t` inline storage, `MultiVector32_t` slot list), [meta](meta.md) (`MPack`, member-function traits, `Decay_t`, `MReturn`, `Variant_t`).

## Relationships

A delegate is not memmove-safe (its implementation object may hold interior state), which is why `MTypeInfo::IS_MEMMOVE_SAFE` gates relocation in [CMultiVector](../types/BTL.CMultiVector.md): delegate columns are relocated by move-construct + destroy. Exercised by [case10_delegate.cpp](../../src/ball/types/tests/case10_delegate.cpp).
