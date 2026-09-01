# BTL::CAllocator

## Overview

The heap-allocation policy of the owning containers: a stateless wrapper over the library's aligned C allocation functions, in an untyped (`CAllocatorBase`) and a typed (`CAllocator< I, T >`) form.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** classes `CAllocatorBase` and `CAllocator< typename I, typename T >`
- **Declared in:** [include/ball/types/allocator.hpp](../../include/ball/types/allocator.hpp); backing functions `Ball_AllocAlign`/`Ball_ReallocAlign`/`Ball_FreeAlign` declared in [memoryaligned.h](../../include/ball/types/memoryaligned.h) and implemented in [src/ball/types/memory.c](../../src/ball/types/memory.c)

## Purpose

Give containers a swappable allocation seam (the `A` template parameter of the vector stack) while defaulting to aligned CRT allocation.

## Data Structure

Stateless — all members are static. `CAllocatorBase` deals in bytes (`Alloc( nSize, nAligned )`, `Realloc( pMem, nOldSize, nNewSize, nAligned )`, `Free( pMem, nSize, nAligned )`); `CAllocator< I, T >` multiplies element counts by `sizeof( T )` and returns typed pointers, defaulting alignment to `alignof( T )`.

The blocks carry no header of any kind — the page-backed allocator behind them stores nothing next to the memory it hands out — so the size a block was allocated with is not recoverable from the pointer and must be supplied back on every resize and release. That is why `Realloc` takes the old size and `Free` takes the size, and why there is no `Size( pMem )` query.

## Ownership and Lifetime

The allocator owns nothing; containers own the blocks they obtain through it and must pair every `Alloc`/`Realloc` with `Free`, passing the size the block currently holds. `CVectorBase` derives that size from its own capacity (`BlockSize( Capacity() )`), reading the capacity before any operation that migrates the rows back to inline storage.

## Type Relationships

- Empty policy base `A` exclusively of [CVectorBase](BTL.CVector.md). Strings supply their typed allocator to that owning layer; ordinary vectors use `CAllocatorBase`. `CVectorImpl` is allocator-independent.

## Notes

`Realloc` may move the block — the containers treat every capacity change as a potential relocation.
