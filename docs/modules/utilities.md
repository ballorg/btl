# Module: utilities (free-function helpers)

## Overview

Header-level free functions that the containers are written in terms of: raw element lifetime and relocation, bit manipulation, integer math, and digit formatting. All are `constexpr` where the underlying operation allows it.

## Element operations — [elements.hpp](../../include/ball/types/elements.hpp)

Raw-memory element helpers used by every owning container:

- `ConstructElement( p, args... )` / `ConstructElements( first, end )` — placement-new construction (uses Ball's own placement `new` from [ball/new.hpp](../../include/ball/new.hpp)).
- `DestructElement` / `DestructElements` — explicit destructor calls over a range.
- `CopyElements( n, dst, src )` — forward byte copy (`memcpy` at run time, an element loop under constant evaluation via `CopyElements_Unified`); asserts against unsafe overlap.
- `CopyElementsFromEnd` — backward copy for overlapping right-shifts.
- `ShiftElementsLeft/Right/Diff/ShiftElements` — overlap-safe range shifts; `ShiftElementsRight` dispatches between a non-overlapping copy, a by-one carry loop (`ShiftElementsRight_ByOne`), and a backward copy, deliberately avoiding one bulk `memmove` that provoked spurious GCC warnings in fixed-buffer callers.
- `CompareElements( n, l, r )` — bytewise `memcmp`-style comparison.

These helpers treat memory as raw storage: callers are responsible for matching construction/destruction to the container's element-lifetime model.

## Bit operations — [bits.hpp](../../include/ball/types/bits.hpp)

- `BitWidth( x )` — index of the highest set bit of `x - 1` plus one, via `_BitScanReverse(64)` / `__builtin_clz(ll)`.
- `BitCeil( x )` — next power of two `>= x` computed from `BitWidth`; this is the library's capacity function (capacity is always `BitCeil( count )`).
- `BitCeil_Unified` / `BitCeil_Const` — portable/consteval bit-spreading variants (saturating).
- `PopCount( x )` — hardware population count; requires an unsigned type. Used by the hash map to recover its live count from the packed state column.

## Integer math — [math.hpp](../../include/ball/types/math.hpp)

`Math_Pow10`, `Math_IsPow2`, `Math_RoundUp` (bitmask alignment rounding, used for SoA block offsets), `Math_Log2_Floor`, `Math_Log2Pow2`, `Math_BitWidth`, `Math_Log_Floor` (any base 2..36 with power-of-two fast paths), `Math_Digits`.

## Digit writing — [number.hpp](../../include/ball/types/number.hpp)

`Num_Digits< I, NS >( u )` (digit count in base NS), `Num_DigitToChar`, and `Num_WriteUnsigned< T, I, NS >( u, out, digits )` which writes exactly `digits` characters back-to-front with base-specialized loops (shifts for power-of-two bases, one division per step otherwise). [CString](../types/BTL.CString.md) builds its integer/float `Insert` overloads on these.

## Value helpers

- `Move`, `Forward`, `Swap` — [meta/xvalue.hpp](../../include/ball/types/meta/xvalue.hpp).
- [BTL::Pair_t](../types/BTL.Pair_t.md) — simple key/value aggregate.

## Data Structures

- [BTL::Pair_t](../types/BTL.Pair_t.md) — key/value aggregate

The remaining helpers are free functions; they are documented above rather than as standalone types.

## Dependencies

[base](base.md), [meta](meta.md).

## Relationships

[containers](containers.md), [associative](associative.md), and [strings](strings.md) are the main consumers; `BitCeil` defines the growth model of every growable container.
