# BTL::CAllocator

## Overview

The heap-allocation policy of the owning containers: a stateless wrapper over the library's aligned C allocation functions, in an untyped (`CAllocatorBase`) and a typed (`CAllocator< I, T >`) form.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** classes `CAllocatorBase` and `CAllocator< typename I, typename T >`
- **Declared in:** [include/ball/types/allocator.hpp](../../include/ball/types/allocator.hpp); backing functions `Ball_AllocAlign`/`Ball_ReallocAlign`/`Ball_FreeAlign`/`Ball_SizeAlign` declared in [memoryaligned.h](../../include/ball/types/memoryaligned.h) and implemented in [src/ball/types/memory.c](../../src/ball/types/memory.c)

## Purpose

Give containers a swappable allocation seam (the `A` template parameter of the vector stack) while defaulting to aligned CRT allocation.

## Data Structure

Stateless — all members are static. `CAllocatorBase` deals in bytes (`Alloc/Realloc/Free/Size` with an explicit alignment); `CAllocator< I, T >` multiplies element counts by `sizeof( T )` and returns typed pointers, defaulting alignment to `alignof( T )`.

## Ownership and Lifetime

The allocator owns nothing; containers own the blocks they obtain through it and must pair every `Alloc`/`Realloc` with `Free`.

## Type Relationships

- Empty policy base `A` exclusively of [CVectorBase](BTL.CVector.md). Strings supply their typed allocator to that owning layer; ordinary vectors use `CAllocatorBase`. `CVectorImpl` is allocator-independent.

## Notes

`Realloc` may move the block — the containers treat every capacity change as a potential relocation.
