# Module: strings

## Overview

Character-sequence types built on the container stack: a non-owning [CStringView](../types/BTL.CStringView.md) (a `CView` of characters with string semantics) and an owning [CString](../types/BTL.CString.md) (a `CStringImpl` layer over the vector stack that adds text formatting and trimming). Both are templated on the index type `I` and character type `T`, with alias families for `char_t`, `wchar_t`, `char8_t`, `char16_t`, `char32_t` crossed with index widths 8/16/32/64 (`String32_t`, `UTF8StringView_t`, `WBufferString_t< N >`, …).

In module builds `CStringView` and the owning `CString` family belong to the independent `Ball.Types:StringView` and `Ball.Types:String` partitions; the public `Ball.Types` module re-exports both.

## Responsibilities

- Length-delimited, non-owning string ranges with C-string interop (`String()` always returns a valid, possibly empty, zero-terminated pointer for views constructed from C strings).
- Owning, growable strings with inline-buffer variants (`CBufferString< I, T, N >`).
- Value formatting without the CRT: `Insert`/`Append`/`Set` overloads for booleans, all integer widths (base 8/10/16 helpers), fixed-precision floats, pointers (`0x…` hexadecimal), views, literals, and variadic `*Multiple` forms.
- Search and edit: `Find`/`RFind` (element and subrange), `Replace`, `ReplaceFirst`, `ReplaceAll`, `TrimLeft`/`TrimRight`/`Trim`.
- String-view literals: `"…"_sv`, `_sv8` … `_sv64` for every character kind.

## Public Interface

| Type | Document |
| --- | --- |
| `CStringView< I, T, N >` + view aliases + `_sv` literals | [BTL.CStringView.md](../types/BTL.CStringView.md) |
| `CStringImpl`, `CString`, `CBufferString` + string aliases | [BTL.CString.md](../types/BTL.CString.md) |

Digit-writing primitives (`Num_Digits`, `Num_WriteUnsigned`) live in the [utilities module](utilities.md).

## Data Structures

- [BTL::CStringView](../types/BTL.CStringView.md) — non-owning character range
- [BTL::CString](../types/BTL.CString.md) — owning growable string (`CStringImpl`, `CBufferString`)

## Dependencies

[containers](containers.md) (vector stack), [utilities](utilities.md) (math/number helpers).

## Relationships

- `CString` uses the ordinary `CVectorBase`/`CVectorImpl` stack over `CStringView`, so allocation remains confined to `CVectorBase` and every string remains implicitly viewable and comparable as a view.
- [CFibonacciHash](../types/BTL.CFibonacciHash.md) accepts the `Base()`/`Count()` storage interface inherited by `CStringView`, making strings usable as hash-map keys.
- Reflection uses `CStringView` as its field-name type.

## Notes

Strings are **not** implicitly zero-terminated in storage; `Length()` is the logical count and `String()` on an empty owning string returns a literal `"\0"`. The formatting `Insert` overloads return the index just past (or of the last) written character, enabling chained insertion at a running cursor.
