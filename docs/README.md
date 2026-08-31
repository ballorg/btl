# Ball Documentation

Ball is a freestanding C++20 container and utility library. It provides growable and fixed-capacity containers (vectors, structure-of-arrays vectors, a red-black tree, an open-addressing hash map), strings and string views, delegates, a compile-time reflection facility, and the metaprogramming/base-type layers those components are built on.

Ball deliberately avoids the C++ standard library: it declares the few CRT functions it needs itself, ships its own type traits, placement `new`, and assertions, and keeps every container `constexpr`-friendly where feasible. All components live in the `BTL` namespace. Component headers under `include/ball/types/` are fragments with no includes of their own; the umbrella header [include/ball/types.hpp](../include/ball/types.hpp) is the single header-mode entry point, composing them (plus their macro-only dependencies) in the order [cmake/ball/types/modules.cmake](../cmake/ball/types/modules.cmake) declares. When `BALL_ENABLE_MODULES` is on, the same components are exported through the public C++20 modules `Ball.New`, `Ball.Types`, and `Ball.Time`, generated from that same manifest; `Ball.Types` re-exports generated component partitions and the complete `Meta` metaprogramming partition.

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
| [placement new](modules/new.md) | `Ball.New`: freestanding placement `new` and its matching placement `delete` |
| [base](modules/base.md) | C-compatible base layer: architecture/fixed-width integer aliases, character types, platform macros, assertions, CRT imports |
| [meta](modules/meta.md) | Metaprogramming toolkit: type traits, packs, sequences, fixed-width metadata, Fibonacci-hash constants, type info |
| [utilities](modules/utilities.md) | Free-function helpers: element construction/copy/shift, bit operations, integer math, digit writing, `Move`/`Forward`/`Swap`, `Pair_t` |
| [containers](modules/containers.md) | Linear storage: `CArray`, `CView`/`CViewBase`, `CElementsPack`, variadic `CVector`, allocators |
| [associative](modules/associative.md) | Ordered and hashed containers: `CRBTree`/`CRBTree`, `CHashMap`/`CHashMap`, `CFibonacciHash`, `CSlotIterator` |
| [strings](modules/strings.md) | `CStringView`, `CString` and their alias families; number-to-text formatting |
| [reflection](modules/reflection.md) | `CReflect`, `BALL_REFLECT_*` macros, field descriptors, raw serialization |
| [delegates](modules/delegates.md) | Type-erased callables: `CDelegate`, `CMulticastDelegate`, `DelegateHandle_t` |
| [time](modules/time.md) | `Ball.Time`: nanosecond time values (`CTimeNS`) and profiling macros |

## Types

### Linear containers and views
- [BTL::CArray](types/BTL.CArray.md) — fixed-size owning array (plus `CEmptyArray`)
- [BTL::CView](types/BTL.CView.md) — single-column non-owning view with optional inline buffer
- [BTL::CViewBase](types/BTL.CViewBase.md) — multi-column (SoA) view core
- [BTL::EPrefetchAccess](types/BTL.EPrefetchAccess.md) — read/write mode for row prefetching
- [BTL::CElementsPack](types/BTL.CElementsPack.md) — per-column inline/heap storage substrate
- [BTL::CElementsNode](types/BTL.CElementsNode.md) — internal single-column storage node used by `CElementsPack`
- [BTL::CVector](types/BTL.CVector.md) — growable single-column or SoA vector (`CVectorBase`, `CVectorImpl`, `CBufferVector`)
- [BTL::CVector_Packed_Iterator](types/BTL.CVector_Packed_Iterator.md) — byte-pointer iterator for packed elements; ordinary columns use native pointers

### Associative containers
- [BTL::CRBTree](types/BTL.CRBTree.md) — SoA red-black tree family (`CRBTree`, `CBufferRBTree`)
- [BTL::CHashMap](types/BTL.CHashMap.md) — SoA open-addressing hash map family (`CHashMap`, `CBufferHashMap`)
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

## Build and integration

The root [CMakeLists.txt](../CMakeLists.txt) builds a static library `ball` (alias `Ball::BTL`, output name `ball-types`, C 17 / C++ 20) from three C/C++ sources (`memory.c`, `time.c`, `c/assert.cpp`); everything else is header-only. Options: `BALL_ENABLE_ASSERT` (runtime asserts), `BALL_ENABLE_MODULES` (public `Ball.New`, `Ball.Types`, and `Ball.Time` module file set plus the internal `Ball.Types` partitions), `BALL_ENABLE_TESTS` (CTest executables defined in [cmake/ball/types/tests.cmake](../cmake/ball/types/tests.cmake) and [cmake/ball/types/base/tests.cmake](../cmake/ball/types/base/tests.cmake)). The shared generation functions live in [cmake/modules.cmake](../cmake/modules.cmake). Module builds generate the three public interfaces from [cmake/ball/module.cppm.in](../cmake/ball/module.cppm.in) and [cmake/ball/modules.cmake](../cmake/ball/modules.cmake), and generate component and meta partition interfaces from [cmake/ball/types/module.cppm.in](../cmake/ball/types/module.cppm.in) and [cmake/ball/types/modules.cmake](../cmake/ball/types/modules.cmake). CMake discovers the generated `.cppm` files and the handwritten test partition interfaces with non-recursive globs. The `Meta` partition discovers its headers with `file(GLOB ... CONFIGURE_DEPENDS)`. Enabling tests forces `BALL_ENABLE_MODULES` and `BALL_TEST_ENABLE_MODULES` on. Every container case has a `Ball.Types:Tests.CaseNN` partition interface and one or more `Ball.Types` implementation units; [src/ball/types/tests/main.cpp](../src/ball/types/tests/main.cpp) imports every case partition directly. Each test imports only the component partitions it uses (`:Vector`, `:String`, `:HashMap`, and so on), plus `Meta`, `Ball.New`, and `Ball.Time` where required; there is no umbrella `import Ball.Types;` or header-mode test branch. Module and test builds require CMake 3.28 or newer.

GCC 14 and AppleClang fall back to the supported header-only library build. GCC 14 cannot reliably consume Ball's partition BMIs, while AppleClang does not provide the dependency scanning required by CMake for C++20 modules. Upstream Clang and MSVC build the public modules and module-based test suite directly through CMake's native scanner integration; the macOS CI job installs upstream LLVM through Homebrew for this reason.
