# Module: time (Ball.Time)

## Overview

A small, standalone timing component: [include/ball/time.hpp](../../include/ball/time.hpp) (C++ wrapper, module `Ball.Time` via [time.cppm](../../src/ball/time.cppm)) over the C interface [include/ball/time.h](../../include/ball/time.h) implemented in [src/ball/types/time.c](../../src/ball/types/time.c). It is independent of `Ball.Types` (it includes only the base arch aliases). The profiling macros live in [include/ball/time/macros.h](../../include/ball/time/macros.h), because macros are not exported by C++20 module imports.

## Responsibilities

- `BTL::timens_t` / `BTL::GetTimeNS()` — raw nanosecond clock (C layer).
- [BTL::CTimeNS](../types/BTL.CTimeNS.md) — a value type holding nanoseconds with integer and floating-point conversions to micro/milli/seconds/minutes/hours.
- Profiling macros: `BALL_PROF_BEGIN( tag )` captures a start time in a tag-derived local; `BALL_PROF_END( tag )` yields a `CTimeNS` with the elapsed time.

## Public Interface

`CTimeNS`, `GetTimeNS`, `timens_t`, `BALL_PROF_BEGIN` / `BALL_PROF_END`.

## Data Structures

- [BTL::CTimeNS](../types/BTL.CTimeNS.md) — nanosecond time value

## Dependencies

[base](base.md) arch aliases only.

## Relationships

The test harness ([tests/main.cpp](../../src/ball/types/tests/main.cpp)) uses the profiling macros to time every imported test-case module.
