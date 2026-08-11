# BTL::CStringView

## Overview

A non-owning character range: [CView](BTL.CView.md) specialized with string semantics — construction from zero-terminated strings and literals, substring/prefix/suffix operations, and three-way comparison. It is also the view base of the owning [CString](BTL.CString.md).

## Declaration

- **Namespace:** `BTL`
- **Module:** [strings](../modules/strings.md)
- **Kind:** class template `CStringView< typename I, typename T, I N = 0 >` derived from `CView< I, T, N >`
- **Declared in:** [include/ball/types/stringview.hpp](../../include/ball/types/stringview.hpp)
- **Aliases:** `StringView_t` (`char_t`), `WStringView_t`, `UTF8/16/32StringView_t`, each with `8/16/32/64` index-width variants; literal operators `""_sv`, `_sv8` … `_sv64`

## Purpose

Length-delimited string handling without allocation, with safe C-string interop in both directions.

## Data Structure

Identical to [CView](BTL.CView.md): a length and a character pointer (in the inline/pointer union). The view does **not** require or preserve zero termination; `Length()` is the logical count. `String()` returns the base pointer or, for an empty view, the address of a static `'\0'` so callers always receive a valid C string.

## Ownership and Lifetime

`CStringView` does not own the referenced character sequence. The caller must ensure that the underlying storage remains valid for the complete lifetime of the view.

## Type Relationships

- Derives from [CView](BTL.CView.md); base of the [CString](BTL.CString.md) stack.
- Its inherited `Index_t`/`Base()`/`Count()` storage interface is accepted by [CFibonacciHash](BTL.CFibonacciHash.md); it is also used for reflection field names ([CReflect](BTL.CReflect.md)).

## Invalidation Rules

Views into a [CString](BTL.CString.md) are invalidated by any growth, shrink, or edit of the string; views over literals or static storage are never invalidated. Slicing operations return new views over the same storage with the same constraints.

## Operations

- Construction: `(length, ptr)`, zero-terminated `const T *` (measured by the `constexpr` `Length( ptr )`, using builtin strlen where available), string literals / C arrays (`CN - 1`, excluding the terminator), `[begin, end)`.
- Slicing: `SubString`, `RemovePrefix`, `RemoveSuffix` (plus the inherited view slicing).
- Comparison: static `Compare( l, r, n )` (constexpr memcmp), member `Compare` (length-aware three-way), `Equals`; the inherited relational operators.
- Classification helper `IsSpaceASCII`; `EmptyView()` / `NullView()` constants.
