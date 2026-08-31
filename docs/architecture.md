# Architecture

Ball is organized as a strict layering of headers under [include/ball/](../include/ball/). Higher layers include lower ones; nothing includes upward.

```
┌────────────────────────────────────────────────────────────────────┐
│ facades      CString  CRBTree/CRBTree  CHashMap/CHashMap │
│              CDelegate/CMulticastDelegate                          │
├────────────────────────────────────────────────────────────────────┤
│ containers   CVector (single/SoA)  CView  CViewBase                │
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

Public headers do not open a namespace themselves. The umbrella header [include/ball/types.hpp](../include/ball/types.hpp) includes every component **inside** `BALL_EXPORT namespace BTL { ... }`, so all types documented here are `BTL::` types when consumed through `<ball/types.hpp>` (or `import Ball.Types;`). Only `new.hpp` and `memory.h` are included before the namespace, keeping placement `new`, `size_t`, and the CRT imports global.

With `BALL_ENABLE_MODULES`, the generated `Ball.New` interface exports the global placement allocation functions from [new.hpp](../include/ball/new.hpp). The generated `Ball.Types` interface re-exports `Ball.New`, the complete generated `Meta` partition, and every generated component partition. `Meta` owns the base aliases, traits, type inspection, and reflection descriptors.

The shared module-generation functions live in [cmake/modules.cmake](../cmake/modules.cmake). All three public interfaces are generated into the build tree from [module.cppm.in](../cmake/ball/module.cppm.in) and the declarative [modules.cmake](../cmake/ball/modules.cmake) manifest. That manifest records the headers and global-fragment dependencies exported by `Ball.New` and `Ball.Time`, plus the ordered partition re-exports of `Ball.Types`.

Component interfaces are generated into the build tree from the shared [module.cppm.in](../cmake/ball/types/module.cppm.in) template and the declarative [modules.cmake](../cmake/ball/types/modules.cmake) manifest. The `Meta` header set is discovered automatically with `file(GLOB ... CONFIGURE_DEPENDS)`; the manifest records only the partition name, owning headers, global-fragment headers, and extra partition imports for `Allocator`, `Array`, `Bits`, `Elements`, `ElementsPack`, `Fixed`, `Hash`, `Math`, `Number`, `Pair`, `Prefetch`, `Reflect`, `SlotIterator`, `StringView`, `VectorIterator`, `ViewBase`, `View`, `Vector`, `String`, `RBTree`, `HashMap`, and `Delegate`. The generator supplies the common global module fragment, `Ball.New` and `Meta` imports, module-mode configuration, exported `BTL` namespace, and header inclusion. The macro-only [module.h](../include/ball/types/module.h) controls module-mode header dependencies. These partitions are implementation details; ordinary consumers import `Ball.Types`. The generated `Ball.Time` module independently exports the timing layer.

Placement `new` remains in the global namespace, but module consumers obtain it from `Ball.New` directly or through `Ball.Types`. Header-mode consumers continue to use [new.hpp](../include/ball/new.hpp). Generated component interfaces place the required C runtime declarations in their global module fragments.

## The storage stack

The linear containers are assembled by stacking CRTP-style layers, each adding one concern:

1. **[CElementsPack](types/BTL.CElementsPack.md)** — typed collection of [CElementsNode](types/BTL.CElementsNode.md) unions, stored in `MPack`; each node provides an inline `CArray`, an external pointer, and their bit-packed counterparts.
2. **[CViewBase](types/BTL.CViewBase.md)** — adds the shared row count, typed column accessors, packed bit get/set/shift, batched find, subviews, and copy/move/swap of the view state.
3. **[CView](types/BTL.CView.md)** — single-column convenience wrapper (element access, iterators, find/rfind, comparisons); also the base of `CStringView`.
4. **[CVectorBase / CVectorImpl / CVector](types/BTL.CVector.md)** — one common SoA ownership layer over `CViewBase` for every arity; `CVector< I, T >` is its single-column case, while optional types after `T` add columns to the same shared block and count.
5. **Facades** — [CString](types/BTL.CString.md) over the single-column vector stack; [CRBTree](types/BTL.CRBTree.md) and [CHashMap](types/BTL.CHashMap.md) over variadic `CBufferVector`, adding their metadata columns (color/links, slot state) in front of the payload columns.

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
| delegates | containers (variadic `CVector`, `BufferVector_t`), meta/ |
| time | base/ only (`Ball.Time` is independent of `Ball.Types`) |
