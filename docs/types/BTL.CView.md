# BTL::CView

## Overview

A single-column view: `CViewBase` specialized to one element type, with the untyped (`Get( i )`, `operator[]`, `begin`/`end`) surface a caller expects from a span. It doubles as the *state layer* of the owning vector — `CVector` derives from it and reinterprets the same count/pointer as owned storage.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class template `CView< typename I, typename T, I N = 0 >` derived from `CViewBase< I, N, size8_t, T >`
- **Declared in:** [include/ball/types/view.hpp](../../include/ball/types/view.hpp)
- **Aliases:** `View_t< T >` (const elements, `size_t` index), `View8_t` … `View64_t`, `BufferView_t< T, N >` and width variants

## Purpose

Bounds-asserted, non-owning access to a contiguous (or bit-packed) element range, plus the protected mutation hooks (`Set`, `SetTo`, `CopyFrom`, `MoveFrom`) the vector stack builds on.

## Data Structure

Inherited from [CViewBase](BTL.CViewBase.md): a count and a one-column [CElementsPack](BTL.CElementsPack.md) node — i.e. a union of inline buffer and data pointer (plus packed forms). With `N == 0` the inline buffer is the pointer-sized default, so a plain `CView` is effectively `{ count, pointer }`. For packed element types (`IS_PACKED_STORAGE`) element access goes through bit get/set and `Get( i )` returns by value (or a `Ref_t` proxy for assignment) rather than a reference.

## Storage Model

The view borrows storage: either it points at externally managed elements, or (with `N > 0`, or as the base of an owning container) the elements live in its inline buffer. It never allocates and never resizes what it points to.

## Ownership and Lifetime

Non-owning; identical caveats to [CViewBase](BTL.CViewBase.md). The caller must ensure the viewed elements outlive the view.

## Type Relationships

- Base (as `B`) of [CVectorBase](BTL.CVector.md); [CStringView](BTL.CStringView.md) derives from it for character ranges.
- `View_t`/`ConstView_t`/`GrowableView_t< GN >` member aliases thread the view types through the vector stack.

## Invariants

Indices satisfy `0 <= i < Count()` (asserted); `INVALID_INDEX == I(-1)` is the not-found value.

## Invalidation Rules

A view into an owning container is invalidated by any reallocation of that container (growth across a power-of-two boundary, heap/inline migration, shrink) and by destruction of the owner. Slices (`Subview`, `First`, …) alias the same storage and share its fate.

## Operations

- Construction from `(count, pointer)`, a C array, or a `[begin, end)` pointer pair.
- Element access (`Get`, `operator[]`, `Front`, `Back`, `GetValue`), iterators, `Empty`.
- Search: `Find`/`RFind` for a single value and for a subrange needle (batched probing; empty-needle convention returns the clamped start).
- Prefix/suffix tests `StartsWith`/`EndsWith`; lexicographic `==`/`!=`/`<`/`>`/`<=`/`>=`.
- Slicing (`Subview`, `First`, `Last`, `DropFront`, `DropBack`) and `Const()` conversion.
