# BTL::CTimeNS

## Overview

A lightweight nanosecond time value with unit conversions — the result type of the `BALL_PROF_BEGIN`/`BALL_PROF_END` profiling macros.

## Declaration

- **Namespace:** `BTL`
- **Module:** [time](../modules/time.md)
- **Kind:** class `CTimeNS`
- **Declared in:** [include/ball/time.hpp](../../include/ball/time.hpp) (module `Ball.Time`)

## Data Structure

A single `timens_t m_nNS` holding nanoseconds; zero means "not set" (`IsValid()` is false).

## Ownership and Lifetime

Trivial value type.

## Type Relationships

Constructed from differences of `BTL::GetTimeNS()` samples; used throughout the [test harness](../../src/ball/types/tests/main.cpp) for reporting case timings.

## Operations

`SetNS`/`AddNS`/`GetNSs`; integer conversions `AsMicros`/`AsMillis`/`AsSeconds`/`AsMinutes`/ `AsHours` (truncating) and floating-point `*F` counterparts.
