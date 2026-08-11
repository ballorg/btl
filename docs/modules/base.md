# Module: base (C base layer)

## Overview

The lowest layer of Ball: plain C headers under [include/ball/types/base/](../../include/ball/types/base/) and [include/ball/types/c/](../../include/ball/types/c/) that define the fundamental integer, character, and pointer aliases, platform/compiler detection, assertion machinery, and the CRT imports the library uses instead of standard headers. Everything above (meta, containers) builds on these names.

## Responsibilities

- Architecture-word aliases ([base/arch/](../../include/ball/types/base/arch/)): `bool_t`, `uchar_t`/`ushort_t`/`uint_t`/`ulong_t`/`ullong_t`, signed counterparts (`schar_t`, …, `ssize_t`), pointer aliases, and a self-contained `size_t` definition ([base/arch/size.h](../../include/ball/types/base/arch/size.h)) that avoids pulling in `<stddef.h>`.
- Fixed-width aliases ([base/fixed/](../../include/ball/types/base/fixed/)): per-data-model headers (win32/win64/ilp32/lp32/lp64/llp64/standard) selected by [base/fixed.h](../../include/ball/types/base/fixed.h). They define `uintN_t`/`sintN_t`/ `intN_t` for **every** N in 1..64 (mapped to the nearest storage word) plus the size aliases `size8_t`/`size16_t`/`size32_t`/`size64_t` used by container alias families.
- Character types ([base/characters.h](../../include/ball/types/base/characters.h)): `char8_t`/`char16_t`/`char32_t`/`wchar_t` fallbacks for toolchains lacking them.
- Platform macros ([c/platform.h](../../include/ball/types/c/platform.h)): `BALL_MSVC`, `BALL_GNUC`, `BALL_CLANG`, `BALL_MINGW`, `BALL_32BITS`/`BALL_64BITS`, `BALL_CXX`.
- Export/linkage macros ([c/macros.h](../../include/ball/types/c/macros.h)): `BALL_EXPORT` (expands to `export` in module builds), DLL import/export, calling-convention helpers.
- Assertions ([c/assert.h](../../include/ball/types/c/assert.h)): `BALL_ASSERT`, `BALL_ASSERT_MESSAGE`, `BALL_ASSERT_IF(_MESSAGE)` (assert-and-branch), `BALL_FATAL`, `BALL_STATIC_ASSERT`; enabled by `BALL_ENABLE_ASSERT` (defaults to on outside `NDEBUG`). The reporting function `Ball_AssertFail` is compiled in [src/ball/types/c/assert.cpp](../../src/ball/types/c/assert.cpp).
- CRT surface ([memory.h](../../include/ball/types/memory.h)): declares `malloc`/`realloc`/ `free`/`_msize` and `memset`/`memcpy`/`memmove` directly, so no standard header is needed. Aligned allocation wrappers (`Ball_AllocAlign`, `Ball_ReallocAlign`, `Ball_FreeAlign`, `Ball_SizeAlign`) are implemented in [src/ball/types/memory.c](../../src/ball/types/memory.c) and consumed by [CAllocator](../types/BTL.CAllocator.md).
- Miscellaneous C helpers under [c/](../../include/ball/types/c/): `debugbreak.h`, `unreachable.h`, `assume.h`, `nouniqueaddress.h` (`BALL_NO_UNIQUE_ADDRESS`), `io.h`, `mmap.h`.

## Public Interface

The aliases and macros above; there are no classes at this layer. When consumed through `<ball/types.hpp>`, the aliases land inside `namespace BTL` (see [architecture.md](../architecture.md)); [base.hpp](../../include/ball/types/base.hpp) additionally exposes the raw C set as `Base`.

## Data Structures

This layer defines aliases and macros rather than data-bearing classes. The fixed-width metadata built on its alias grid is documented at [BTL::CFixedBase](../types/BTL.CFixedBase.md); the allocation policy over its CRT surface at [BTL::CAllocator](../types/BTL.CAllocator.md).

## Dependencies

None; this layer is freestanding by design.

## Relationships

Everything in Ball depends on this layer. The 1..64-bit alias grid is what makes the [fixed-width metadata](../types/BTL.CFixedBase.md) (`MFixed`, `MFixedMetadata`) and packed column storage possible.
