# Architecture

Ball is organized as a strict layering of headers under [include/ball/](../include/ball/). Higher layers include lower ones; nothing includes upward.

```
┌────────────────────────────────────────────────────────────────────┐
│ facades      CString  CRBTree/CMultiRBTree  CHashMap/CMultiHashMap │
│              CDelegate/CMulticastDelegate                          │
├────────────────────────────────────────────────────────────────────┤
│ containers   CVector  CMultiVector  CView  CViewBase               │
│              CElementsPack  CArray  CSlotIterator                  │
├────────────────────────────────────────────────────────────────────┤
│ policies     CAllocator  CFibonacciHash  CRBTreeLess  CReflect     │
├────────────────────────────────────────────────────────────────────┤
│ utilities    elements.hpp  bits.hpp  math.hpp  number.hpp          │
│              fixed.hpp  pair.hpp  hash.hpp                         │
├────────────────────────────────────────────────────────────────────┤
│ meta/        traits, MPack, sequences, MFixed, MFixedMetadata,     │
│              MFibonacci, MTypeInfo, reflect descriptors            │
├────────────────────────────────────────────────────────────────────┤
│ base/, c/    arch & fixed-width typedefs, characters, platform     │
│              macros, asserts, CRT imports (memory.h)               │
└────────────────────────────────────────────────────────────────────┘
```

## Namespace and module composition

Public headers do not open a namespace themselves. The umbrella header [include/ball/types.hpp](../include/ball/types.hpp) includes every component **inside** `BALL_EXPORT namespace BTL { ... }`, so all types documented here are `BTL::` types when consumed through `<ball/types.hpp>` (or `import Ball.Types;`). Only `c/macros.h` and `memory.h` are included before the namespace so `size_t` and the CRT imports stay global.

With `BALL_ENABLE_MODULES`, the `.cppm` interface units (`ball/types.cppm`, `ball/time.cppm`, `ball/new.cppm`, and the `ball/types/base/*.cppm` partitions) wrap the same headers as C++20 modules `Ball.Types` and `Ball.Time`; `BALL_EXPORT` expands to `export` in that build.

## The storage stack

The linear containers are assembled by stacking CRTP-style layers, each adding one concern:

1. **[CElementsPack](types/BTL.CElementsPack.md)** — typed collection of [CElementsNode](types/BTL.CElementsNode.md) unions, stored in `MPack`; each node provides an inline `CArray`, an external pointer, and their bit-packed counterparts.
2. **[CViewBase](types/BTL.CViewBase.md)** — adds the shared row count, typed column accessors, packed bit get/set/shift, batched find, subviews, and copy/move/swap of the view state.
3. **[CView](types/BTL.CView.md)** — single-column convenience wrapper (element access, iterators, find/rfind, comparisons); also the base of `CStringView`.
4. **[CVectorBase / CVectorImpl / CVector](types/BTL.CVector.md)** — adds ownership: allocator policy, `EnsureCapacity` growth/migration, insert/remove/replace, element lifetime.
5. **[CMultiVectorBase / CMultiVectorImpl / CMultiVector](types/BTL.CMultiVector.md)** — the multi-column (SoA) equivalent; all columns share a single heap block and a single count.
6. **Facades** — [CString](types/BTL.CString.md) over the vector stack; [CMultiRBTree](types/BTL.CMultiRBTree.md) and [CMultiHashMap](types/BTL.CMultiHashMap.md) over `CBufferMultiVector`, adding their metadata columns (color/links, slot state) in front of the payload columns.

Every level of the stack exists in two capacity flavors selected by the inline count `N`: a heap-backed default (`N == 0`) and a `Buffer*` form that keeps up to `N` elements inline. Cross-capacity copy/move conversions rebuild content rather than stealing pointers.

## Shared conventions

- `Index_t I` is a template parameter; alias suffixes `8/16/32/64` pin it to `size8_t`…`size64_t` (defined in [base/fixed/](../include/ball/types/base/fixed/)).
- `I(-1)` is `INVALID_INDEX`/`NIL_INDEX`; `IsValidIndex` tests against it.
- Classes are `C*`, metaprogramming helpers are `M*`, aliases end in `_t`, enum types are `E*`.
- Errors are contract violations checked by `BALL_ASSERT*` (from [c/assert.h](../include/ball/types/c/assert.h)); the library does not throw exceptions.
- Nothing in the library is thread-safe; callers synchronize externally.

## Same-type SoA columns and reflect tags

The SoA substrate addresses columns **by type**. Two columns of the same type would resolve to the same storage and alias each other, so every column of a multi-column container must have a distinct type. The [reflection module](modules/reflection.md) provides `BALL_REFLECT_TAGGED(_TEMPLATE)` which wraps a payload type in a uniquely-tagged [CReflect](types/BTL.CReflect.md); the tree tags its key and link columns (`RBTreeKeyColumn_t`, `RBTreeLeftColumn_t`, …) and the hash map tags its key (`HashKeyColumn_t`) this way. User value columns that share a type must be tagged by the user.

## Dependency summary

| Component | Depends on |
| --- | --- |
| base/, c/ | nothing (freestanding C headers) |
| meta/ | base/ |
| utilities | base/, c/, meta/ |
| containers | utilities, meta/, allocator, memory.h |
| associative | containers, hash policy, reflection (column tags), slot iterator |
| strings | containers (vector stack), number/math |
| reflection | meta/reflect*, strings (name views) |
| delegates | containers (`CMultiVector`, `BufferVector_t`), meta/ |
| time | base/ only (`Ball.Time` is independent of `Ball.Types`) |
