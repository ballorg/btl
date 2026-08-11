# BTL::CString

## Overview

The owning, growable string: a `CStringImpl` formatting/editing layer over the [vector stack](BTL.CVector.md), whose view base is [CStringView](BTL.CStringView.md). Comes in a heap-backed form (`CString`) and an inline-buffer form (`CBufferString< I, T, N >`), across all character kinds and index widths.

## Declaration

- **Namespace:** `BTL`
- **Module:** [strings](../modules/strings.md)
- **Kind:** class templates
  - `CStringImpl< class B, typename I, typename T >` derived from `CVectorImpl< B, I, T >`
  - `CString< I, T, A >` over `CVectorBase< CStringView< I, T >, I, T, A >`
  - `CBufferString< I, T, N, A >` over the `N`-buffer view
- **Declared in:** [include/ball/types/string.hpp](../../include/ball/types/string.hpp)
- **Aliases:** `String_t`, `String8_t`…`String64_t`; `WString*_t`, `UTF8String*_t`, `UTF16String*_t`, `UTF32String*_t`; `BufferString_t< N >` and the same matrix of buffer variants

## Purpose

Text building without the CRT: value-to-text conversion, in-place editing, and trimming on top of the vector's storage model.

## Data Structure

No members beyond the vector stack: a character count plus the inline-buffer/heap union; capacity derived as `BitCeil( count )`. Characters are **not** stored zero-terminated; `String()` returns the buffer for non-empty strings and a literal `"\0"` when empty, and `Length()` is the count.

## Storage Model

Identical to [CVector](BTL.CVector.md): contiguous characters, inline buffer first (`CBufferString< I, T, N >` keeps up to `N` characters inline), heap block on overflow, power-of-two growth, migration back inline on shrink.

## Ownership and Lifetime

Owns its characters and heap allocation exactly as [CVector](BTL.CVector.md) does; the same growth/migration rules apply.

## Type Relationships

- View base: [CStringView](BTL.CStringView.md) (every string converts to its view types); digit primitives: [utilities module](../modules/utilities.md); allocation: [CAllocator](BTL.CAllocator.md).
- `CString` ↔ `CBufferString` cross-conversions copy contents.

## Invariants

Same as the vector stack: characters `[0, Length())` are live; capacity is `BitCeil( Length() )`.

## Invalidation Rules

Any edit (`Insert`, `Append`, `Set`, `Replace*`, `Trim*`, `Remove*`) may reallocate or migrate storage — pointers from `String()`/`Base()` and [CStringView](BTL.CStringView.md)s over the string are invalidated by subsequent mutation.

## Operations

- Formatting `Insert( i, x )` overloads: characters, views, string literals (length `N - 1`), `bool` (`"true"`/`"false"`), all signed/unsigned integer widths (decimal, with protected base-8/10/16 helpers `InsertUnsigned`/`InsertSigned` writing digits into a single pre-reserved gap), fixed-precision floats (`InsertFloatFixed< P >`), and pointers (`"0x"` + hex). Each returns the running cursor index for chaining.
- Composite operations: `InsertMultiple`, `Append(Multiple)`, `Set(Multiple)`, `operator=`/`operator+=` forwarding to them; `Length()`, `String()`.
- Editing inherited from [CVectorImpl](BTL.CVector.md): `Replace` (range, first, all, per-character), `Remove`, `RemoveAll`.
- Whitespace: `TrimLeft`, `TrimRight`, `Trim` (ASCII space/tab/newline/carriage return).

## Usage

```cpp
BTL::BufferString_t< 128 > s;                 // inline up to 128 chars
s.AppendMultiple( "x = ", 42, ", t = ", 1.5f, " ms" );
puts( s.String() );
```

## Notes

`String()` on a non-empty string is only zero-terminated if the caller appended a terminator (the test harness appends `"\0"` or `"\n"` explicitly).
