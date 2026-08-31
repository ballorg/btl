# Module: meta (metaprogramming toolkit)

## Overview

Ball's replacement for `<type_traits>` and friends, under [include/ball/types/meta/](../../include/ball/types/meta/). It provides type traits, value and type packs, index sequences, fixed-width integer metadata, the derived Fibonacci-hash constants, and the descriptor machinery behind reflection. The aggregate header [meta.hpp](../../include/ball/types/meta.hpp) pulls in the commonly used subset; several headers (reflect*, typeinfo, indexsequence, tuple) are included directly by their consumers.

Module builds expose this complete layer through the generated `Ball.Types:Meta` partition. Its headers are discovered from `include/ball/types/meta/*.hpp` with `file(GLOB ... CONFIGURE_DEPENDS)` in [cmake/ball/types/modules.cmake](../../cmake/ball/types/modules.cmake) and rendered through the shared [module.cppm.in](../../cmake/ball/types/module.cppm.in) template.

## Responsibilities

### Type traits (one tiny header each)

`MIsSame`/`IS_SAME`, `IS_INTEGRAL`, `IS_CONST`, `IS_POINTER`, `IS_STANDARD_LAYOUT`, class/union/trivial/copyable/constructible/assignable probes (aggregated by `MTypeInfo`), `RemoveCV_t`, `RemoveReference_t`, `Decay_t`, `Conditional_t`, `EnableIf_t`, `MSelect` (type selector by boolean), `MSigned`/`MUnsigned` (signedness partners), `MFirst`, `MIndexOf` (index of a type in a pack, -1 if absent), `MIndexType` (type at index), `Void_t`.

`TYPE_COUNT< Ts... >` ([typecount.hpp](../../include/ball/types/meta/typecount.hpp)) exposes `sizeof...( Ts )` as a reusable inline variable template. The namespace-scope trait shorthands (`IS_INTEGRAL`, `IS_CLASS`, constructibility/assignability probes, and related values) are inline as well, giving them external linkage required for export from `Ball.Types` without changing their compile-time semantics. `CViewBase` stores the corresponding count locally and owning layers expose that value through inheritance.

Vector composition uses two small detection traits kept beside the other meta helpers: `MVectorViewTypes` ([vectorviewtypes.hpp](../../include/ball/types/meta/vectorviewtypes.hpp)) selects either `B::View_t`/`B::ConstView_t` or the default `CView` pair, while `MVectorAllocatorType` ([vectorallocatortype.hpp](../../include/ball/types/meta/vectorallocatortype.hpp)) unwraps an allocator adapter's `Base_t` when present.

### Value/type packs and sequences

- `MPack< TI, Ts... >` ([pack.hpp](../../include/ball/types/meta/pack.hpp)) — a recursive compile-time value pack storing one value per `Ts`, accessed by type or index through `BaseBy`; `MPointerPack` is the pointer flavor. Used by delegates for stored payloads.
- `MSequence`/`MMakeSequence` ([sequence.hpp](../../include/ball/types/meta/sequence.hpp)) and `MIndexSequence`/`MakeIndexSequence_t` ([indexsequence.hpp](../../include/ball/types/meta/indexsequence.hpp)) — index-sequence machinery for fold expansion.
- `MTuple`/`TupleCat_t` ([tuple.hpp](../../include/ball/types/meta/tuple.hpp)) — a minimal type list used by the reflection descriptors.
- `Get< K >( pack )` / `Get< T >( pack )` ([get.hpp](../../include/ball/types/meta/get.hpp)) — uniform free-function access to anything exposing `BaseBy` (packs, views, vector SoA storage). The vector-specific type, access, search, storage, shift, initialization, and packed-operation shorthands are separated into [get/vector.hpp](../../include/ball/types/meta/get/vector.hpp) and included by `get.hpp`.

### Value utilities

- [xvalue.hpp](../../include/ball/types/meta/xvalue.hpp) — `Move`, `Forward`, `Swap`.
- [constevaluated.hpp](../../include/ball/types/meta/constevaluated.hpp) — `IsConstantEvaluated()`, a `constexpr` wrapper over the compiler builtin; containers use it to pick constant-expression-safe code paths (e.g. union member activation).
- [return.hpp](../../include/ball/types/meta/return.hpp) — `MReturn< R >::Default()` for default-returning unbound delegates.
- [variant.hpp](../../include/ball/types/meta/variant.hpp) — `Variant_t`, a raw union of scalar/pointer words used as the alignment/size block for delegate inline storage.
- [memberfunction.hpp](../../include/ball/types/meta/memberfunction.hpp) — member-function pointer type builders and `IS_MEMBER_FUNCTION_POINTER` / `IS_CONST_MEMBER_FUNCTION_POINTER` covering all cv/ref/noexcept combinations.

### Integer metadata

- `MFixed< T >`, `FixedTag_t`, `MFixedBase`, and `MFixedMetadata< T >` ([fixed.hpp](../../include/ball/types/meta/fixed.hpp)) — base-independent width, normalization, packed-storage, signedness, min/max, and all-bits `INVALID` metadata. The enum-specialization macros live separately in the macro-only [fixed.h](../../include/ball/types/meta/fixed.h), so module partitions and consumers can obtain them without redeclaring the traits. `include/ball/types/fixed.hpp` adds the wrappers backed by Ball's base integer aliases.
- `MFibonacci< U >` ([fibonacci.hpp](../../include/ball/types/meta/fibonacci.hpp)) — the golden-ratio multiplier and FNV-style fold seed for any unsigned width 1..64; see [BTL::CFibonacciHash](../types/BTL.CFibonacciHash.md).
- `IS_MEMMOVE_SAFE< T >` ([ismemmovesafe.hpp](../../include/ball/types/meta/ismemmovesafe.hpp)) — variable-template form of `MIsMemmoveSafe`; `MTypeInfo< T >` ([typeinfo.hpp](../../include/ball/types/meta/typeinfo.hpp)) mirrors it through its `IS_MEMMOVE_SAFE` member, which drives the containers' choice between byte-relocation and move-construct/destroy relocation.

### Reflection descriptors

`reflectvalue.hpp`, `reflectfield.hpp`, `reflectdescriptor.hpp`, `reflecttraits.hpp`, `reflectforeach.hpp`, `reflectcommon.hpp` implement the field-spec/descriptor layer behind the `BALL_REFLECT_*` macros; they are documented with the [reflection module](reflection.md) and [BTL::CReflect](../types/BTL.CReflect.md).

## Data Structures

- [BTL::CFixedBase](../types/BTL.CFixedBase.md) — `MFixed`, `MFixedMetadata`, `CFixedBase`, `FixedTag_t` (fixed-width integer metadata and wrappers)
- [BTL::CFibonacciHash](../types/BTL.CFibonacciHash.md) — `MFibonacci` constants and the hashing policy built on them
- [BTL::CReflect](../types/BTL.CReflect.md) — the reflection descriptor machinery (`MFieldSpec`, `MField`, `MClass`)

## Dependencies

The core traits depend only on other `meta/` headers and built-in C++ types. The wrapper alias grids in `include/ball/types/fixed.hpp` additionally depend on [base](base.md).

## Relationships

Every container header includes parts of this module. The fixed-width metadata cluster (`MFixed`, `MFixedMetadata` in [meta/fixed.hpp](../../include/ball/types/meta/fixed.hpp), wrappers in [types/fixed.hpp](../../include/ball/types/fixed.hpp)) is documented at [BTL::CFixedBase](../types/BTL.CFixedBase.md).
