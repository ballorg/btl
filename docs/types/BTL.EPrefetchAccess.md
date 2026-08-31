# BTL::EPrefetchAccess

## Overview

A scoped compile-time access mode for selecting read or write cache hints.

## Declaration

- **Namespace:** `BTL`
- **Module:** [containers](../modules/containers.md)
- **Kind:** scoped enumeration `enum class EPrefetchAccess : bool`
- **Declared in:** [include/ball/types/prefetch.hpp](../../include/ball/types/prefetch.hpp)

## Values

- `READ` — request a read cache hint.
- `WRITE` — request a write cache hint.

## Relationships

[CViewBase](BTL.CViewBase.md) uses the value as the first template argument of its protected `PrefetchRow< ACCESS, Ks... >( i )` helper, which needs the compiler-specific `BALL_PREFETCH_READ`/`BALL_PREFETCH_WRITE` macros from [c/prefetch.h](../../include/ball/types/c/prefetch.h); in header mode that header reaches all consumers through the umbrella header's global include list, not through `prefetch.hpp` itself.
