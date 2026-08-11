# Ball Documentation

Ball is a freestanding C++20 container and utility library. It provides growable and fixed-capacity containers (vectors, structure-of-arrays multi-vectors, a red-black tree, an open-addressing hash map), strings and string views, delegates, a compile-time reflection facility, and the metaprogramming/base-type layers those components are built on.

Ball deliberately avoids the C++ standard library: it declares the few CRT functions it needs itself, ships its own type traits, placement `new`, and assertions, and keeps every container `constexpr`-friendly where feasible. All components live in the `BTL` namespace (the umbrella header [include/ball/types.hpp](../include/ball/types.hpp) includes every public header inside `namespace BTL`). When `BALL_ENABLE_MODULES` is on, the same code is exported as the C++20 module `Ball.Types` (plus `Ball.Time`) via the `.cppm` interface units.

## Design themes

- **Index-type parameterization.** Nearly every container takes its index/size type `I` as a template parameter, and public alias families pin it: `Vector8_t`/`Vector16_t`/`Vector32_t`/ `Vector64_t`, `HashMap32_t`, `RBTree64_t`, and so on. `I(-1)` (all bits set) is the universal `INVALID_INDEX` / `NIL_INDEX` sentinel.
- **Inline buffer + heap overflow.** Containers take an inline capacity `N`; storage lives in an inline buffer until the count exceeds it, then migrates to the heap (and back). `N == 0` still reuses the pointer's own bytes as a tiny inline buffer.
- **Derived capacity.** No capacity member is stored; capacity is always the count rounded up to the next power of two (`BitCeil`), and growth doubles.
- **Structure of arrays (SoA).** Multi-column containers store each column contiguously with one shared row count; the red-black tree and hash map are built as SoA columns over the same substrate.
- **Bit-packed columns.** Types whose fixed-width metadata declares a sub-byte width (1-bit enums such as tree colors and hash-slot states) are stored as packed bit arrays.
- **`constexpr` everywhere practical.** Constant-evaluation-safe paths (union member activation, unified copy loops) let containers and hashing run at compile time with bit-identical results.

## Navigation

Start with [architecture.md](architecture.md) for the layer diagram and dependency rules. Module documents describe a logical component; type documents describe one type family in detail. The authoritative documentation workflow and rules are defined in [DATA_STRUCTURES.md](DATA_STRUCTURES.md) — the Ball Data Structure Documentation Specification.

## Modules

| Module | Contents |
| --- | --- |
| [base](modules/base.md) | C-compatible base layer: architecture/fixed-width integer aliases, character types, platform macros, assertions, CRT imports |
| [meta](modules/meta.md) | Metaprogramming toolkit: type traits, packs, sequences, fixed-width metadata, Fibonacci-hash constants, type info |
| [utilities](modules/utilities.md) | Free-function helpers: element construction/copy/shift, bit operations, integer math, digit writing, `Move`/`Forward`/`Swap`, `Pair_t` |
| [containers](modules/containers.md) | Linear storage: `CArray`, `CView`/`CViewBase`, `CElementsPack`, `CVector`, `CMultiVector`, allocators |
| [associative](modules/associative.md) | Ordered and hashed containers: `CMultiRBTree`/`CRBTree`, `CMultiHashMap`/`CHashMap`, `CFibonacciHash`, `CSlotIterator` |
| [strings](modules/strings.md) | `CStringView`, `CString` and their alias families; number-to-text formatting |
| [reflection](modules/reflection.md) | `CReflect`, `BALL_REFLECT_*` macros, field descriptors, raw serialization |
| [delegates](modules/delegates.md) | Type-erased callables: `CDelegate`, `CMulticastDelegate`, `DelegateHandle_t` |
| [time](modules/time.md) | `Ball.Time`: nanosecond time values (`CTimeNS`) and profiling macros |

## Types

### Linear containers and views
- [BTL::CArray](types/BTL.CArray.md) — fixed-size owning array (plus `CEmptyArray`)
- [BTL::CView](types/BTL.CView.md) — single-column non-owning view with optional inline buffer
- [BTL::CViewBase](types/BTL.CViewBase.md) — multi-column (SoA) view core
- [BTL::CElementsPack](types/BTL.CElementsPack.md) — per-column inline/heap storage substrate
- [BTL::CElementsNode](types/BTL.CElementsNode.md) — internal single-column storage node used by `CElementsPack`
- [BTL::CVector](types/BTL.CVector.md) — growable vector (`CVectorBase`, `CVectorImpl`, `CBufferVector`)
- [BTL::CMultiVector](types/BTL.CMultiVector.md) — growable SoA multi-column vector (`CBufferMultiVector`)

### Associative containers
- [BTL::CMultiRBTree](types/BTL.CMultiRBTree.md) — SoA red-black tree family (`CRBTree`, `CBufferMultiRBTree`)
- [BTL::CMultiHashMap](types/BTL.CMultiHashMap.md) — SoA open-addressing hash map family (`CHashMap`, `CBufferMultiHashMap`)
- [BTL::CFibonacciHash](types/BTL.CFibonacciHash.md) — golden-ratio hashing policy (`MFibonacci`)
- [BTL::CSlotIterator](types/BTL.CSlotIterator.md) — slot iterator shared by tree and hash map

### Strings
- [BTL::CStringView](types/BTL.CStringView.md) — non-owning character range
- [BTL::CString](types/BTL.CString.md) — owning growable string (`CStringImpl`, `CBufferString`)

### Callables
- [BTL::CDelegate](types/BTL.CDelegate.md) — single-cast type-erased delegate and its implementations
- [BTL::CMulticastDelegate](types/BTL.CMulticastDelegate.md) — broadcast delegate list (`DelegateHandle_t`)

### Reflection and value wrappers
- [BTL::CReflect](types/BTL.CReflect.md) — reflected/tagged value wrapper and reflection descriptors

### Support types
- [BTL::CAllocator](types/BTL.CAllocator.md) — aligned heap allocation policy (`CAllocatorBase`)
- [BTL::CFixedBase](types/BTL.CFixedBase.md) — fixed-bit-width integers and packed-storage metadata (`MFixed`, `MFixedMetadata`, `FixedTag_t`)
- [BTL::Pair_t](types/BTL.Pair_t.md) — key/value pair
- [BTL::CTimeNS](types/BTL.CTimeNS.md) — nanosecond time value

## Build

The root [CMakeLists.txt](../CMakeLists.txt) builds a static library `ball` (output name `ball-types`, C 17 / C++ 20) from three C/C++ sources (`memory.c`, `time.c`, `c/assert.cpp`); everything else is header-only. Options: `BALL_ENABLE_ASSERT` (runtime asserts), `BALL_ENABLE_MODULES` (C++20 module file set), `BALL_ENABLE_TESTS` (CTest executable defined in [cmake/ball/types/tests.cmake](../cmake/ball/types/tests.cmake); test cases live in [src/ball/types/tests/](../src/ball/types/tests/)).
