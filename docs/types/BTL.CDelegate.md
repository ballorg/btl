# BTL::CDelegate

## Overview

A single-cast, type-erased delegate: binds one callable — a free/static function, a member function on an object pointer, or an arbitrary callable object — optionally together with pre-bound payload values, and invokes it through a uniform `Execute( Ts... )` interface. The bound implementation lives in inline storage owned by `CDelegateBase`.

## Declaration

- **Namespace:** `BTL`
- **Module:** [delegates](../modules/delegates.md)
- **Kind:** class hierarchy
  - interfaces `IDelegateBase` (lifecycle: `GetOwner`, `CopyConstruct`, `MoveConstruct`, `Destroy`) and `IDelegate< R, Ts... >` (adds `Execute`)
  - implementations `CStaticDelegate< R( Ts... ), TPayload... >`, `CObjectDelegate< IsConst, T, R( Ts... ), TPayload... >` (alias `CMemberDelegate`), `CLambdaDelegate< LT, R( Ts... ), TPayload... >`
  - storage owner `CDelegateBase`; public wrapper `CDelegate< R, Ts... >`
  - signature alias `Delegate_t< R( Ts... ) >`; macros `BALL_DECLARE_DELEGATE(_RET)`
- **Declared in:** [include/ball/types/delegate.hpp](../../include/ball/types/delegate.hpp)

## Purpose

Callable storage decoupled from the callable's concrete type, with owner-aware queries (`IsBoundTo`, `ClearIfBoundTo`) so object-bound callbacks can be mass-removed when their object dies.

## Data Structure

`CDelegateBase` stores a `BufferVector_t< INLINE_BLOCK_COUNT - 1, Variant_t >`: raw storage measured in `Variant_t`-sized blocks (a union of scalar/pointer words, giving worst-case scalar alignment). The inline budget is `DELEGATE_INLINE_ALLOCATION_SIZE` (default 64 bytes); one block is deducted for the vector's own count field so the whole delegate stays compact. The implementation objects themselves store: function pointer (+ `MPack` payload), or object pointer + member-function pointer (+ payload), or the moved-in callable (+ payload). An unbound delegate holds an empty storage vector.

## Storage Model

A bound delegate placement-constructs its implementation object into the block storage; the storage stays inline up to the configured budget and spills to the heap only if an implementation object exceeds it. Rebinding sizes the storage to the new implementation and constructs in place; `Clear`/destruction destroys the implementation and releases the storage.

## Ownership and Lifetime

The delegate owns its implementation object (constructed/destroyed in place). It does **not** own the target object of a member binding — the caller must ensure the object outlives the binding or clear it (`ClearIfBoundTo`). Copying a delegate deep-copies the implementation (including stored lambdas/payloads).

## Type Relationships

- Storage: `BufferVector_t` from [CVector](BTL.CVector.md); payloads: `MPack` ([meta](../modules/meta.md)); default returns: `MReturn`.
- Aggregated by [CMulticastDelegate](BTL.CMulticastDelegate.md) as `CDelegate< void, Ts... >` entries.
- A delegate is not memmove-safe; containers relocate delegate elements by move-construct + destroy (see [CVector](BTL.CVector.md) `RelocateColumn`).

## Invariants

`IsBound()` iff the storage vector is non-empty; the storage, when non-empty, always begins with a live object derived from `IDelegateBase`.

## Operations

- Binding: constructors deduce the flavor (member-function pointer / free function / callable); explicit `BindStatic`, `BindMember` (const and non-const, with a `static_assert` against binding a mutating method on a `const` object), `BindLambda`. Binding releases any previous implementation first.
- Invocation: `Execute( args... )` (asserts bound), `ExecuteIfBound` (returns `MReturn< R >::Default()` when unbound). Payload values are appended after the call-time arguments via index-sequence expansion; a null object pointer in `CObjectDelegate` yields the default return instead of a call.
- Lifecycle: copy/move of the whole delegate go through the stored implementation's virtual `CopyConstruct`/`MoveConstruct` into freshly sized storage; `Clear`/`Release` destroys via virtual `Destroy` and frees the storage; queries `IsBound`, `IsBoundTo`, `GetOwner`, `GetSize`.

## Usage

```cpp
BALL_DECLARE_DELEGATE_RET( IntOp_t, int, int );
IntOp_t d( []( int x, int mul ) { return x * mul; }, 3 );  // payload mul = 3
int r = d.Execute( 14 );                                    // 42
```
