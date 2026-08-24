# BTL::CVector_Packed_Iterator

## Overview

`CVector_Packed_Iterator< I, T, CONST >` is the pointer-based random-access iterator used only when a selected vector column has packed storage and therefore cannot expose a `T*` to an individual element. Non-packed columns use `T*` and `const T*` directly. A column is selected explicitly with `begin<T>()`/`end<T>()`; omitting `T` selects the first column.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** class templates
- **Declared in:** [include/ball/types/vectoriterator.hpp](../../include/ball/types/vectoriterator.hpp)
- **Template parameters:** index/difference type `I`, packed element type `T`, constness flag `CONST`

## Data Structure

The iterator stores a pointer to the byte containing the current packed element and its bit offset. It has no container pointer and performs no column lookup. `CVector_Packed_Reference< T >` addresses the same byte/offset pair and performs direct masked bit reads and writes.

## Iterator Operations

`CVector_Packed_Iterator` supports dereference, indexing, increment/decrement, addition/subtraction, distance, and relational comparison. Cursor movement updates the byte pointer and bit offset directly. Mutable packed iterators produce mutable packed references; const iterators produce values. For non-packed storage all corresponding operations are native pointer operations with no iterator wrapper.

## Invalidation

The iterator follows [CVector](BTL.CVector.md) invalidation rules. Any count-changing operation may relocate storage and invalidates existing iterators and pointers.
