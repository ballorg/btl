# BTL::CFixedBase

## Overview

The cluster that gives BTL integers of *any* logical bit width 1..64 and, more importantly, tells containers which types can be stored as packed bit arrays. It spans two headers: [meta/fixed.hpp](../../include/ball/types/meta/fixed.hpp) contains the base-independent tags and metadata, while [fixed.hpp](../../include/ball/types/fixed.hpp) adds the `CFixedBase` wrapper and the 1..64 alias grids backed by Ball's base integer types. The macro-only [fixed.h](../../include/ball/types/fixed.h) exposes `BALL_FIXED_FOR_EACH_BITS` to both header and module consumers: header-mode consumers get it from the umbrella header's global include list, module consumers from the `Fixed` partition's global module fragment.

## Declaration

- **Namespace:** `BTL`
- **Module:** [meta](../modules/meta.md)
- **Kind:**
  - `MFixed< T >` — metadata for any integral type: `BITS`, `BYTES`, signed/unsigned partner types, `MIN`/`MAX`, and the all-bits `INVALID` — the source of every container's `INVALID_INDEX`.
  - `FixedTag_t` — signedness policy tag (`FIXED_SIGNED`, `FIXED_UNSIGNED`, `FIXED_UNCERTAIN`).
  - `MFixedBase< T, BITS, TAG >` / `CFixedBase< T, BITS, TAG >` — normalization policy and a trivial value wrapper storing an always-normalized `T` (mask to `BITS`, sign-extend for signed tags). Alias grids `FixedSignedN_t` / `FixedUnsigedN_t` / `FixedUncertainN_t` for N in 1..64.
  - `MFixedPacked_Base< T >` / `MFixedMetadataBase` / `MFixedMetadata< T >` — per-type packed storage metadata and operations: logical `BITS`, `STORAGE_BITS`, `IS_PACKED` (`BITS < STORAGE_BITS`), `BYTES`, `MASK`, raw/signed/unsigned partner types, packed cursor offsets, and direct packed reads/writes. Specialized for `bool` (1 bit) and extensible to enums.
  - Enum registration macros: `BALL_FIXED_(UN)SIGNED_ENUM(_CLASS)` declare an enum and its trait together; `BALL_FIXED_(UN)SIGNED_ENUM_TRAIT( E, bits )` register an existing enum (e.g. `BALL_FIXED_UNSIGNED_ENUM_TRAIT( ERBTreeColor, 1 )`).

## Purpose

Deterministic sub-word integer semantics, and the compile-time switch that turns a container column into a bit array: any column type whose `MFixedMetadata` reports `IS_PACKED` is stored and accessed bit-packed by [CViewBase](BTL.CViewBase.md)/[CElementsPack](BTL.CElementsPack.md).

## Data Structure

`CFixedBase` stores one backing word `m_Value`, always kept canonical by `Normalize` on construction and assignment (low-bit mask, then sign extension from bit `BITS - 1` for signed policies); it converts implicitly to the backing type and compares by normalized payload. All the `M*` metadata types are stateless.

## Type Relationships

- Consumed by [CViewBase](BTL.CViewBase.md) (`Packed_Traits_t`, `PACKED_BITS`), [CElementsPack](BTL.CElementsPack.md) (packed buffers and sizes), and the associative containers' 1-bit metadata columns (`ERBTreeColor`, `EHashSlotState`).
- `MFixed` underlies `INVALID_INDEX`/`NIL_INDEX` throughout, and [MFibonacci](BTL.CFibonacciHash.md) uses `MFixedMetadata< U >` for logical width and signedness.

## Invariants

Stored `CFixedBase` values are always canonical for their declared width; re-normalization is a no-op. `MFixedMetadata< T >::BYTES` is the packed byte footprint (`ceil( BITS / 8 )`) when packed, `sizeof( T )` otherwise.

## Operations

- `MFixedBase::Normalize` — canonicalize any input to the (`BITS`, `TAG`) domain; idempotent.
- `MFixedMetadata` — answer `IS_PACKED`/`BITS`/`BYTES`/`MASK`; calculate packed bit counts, byte offsets and bit shifts; read/write values with `Packed_Get`/`Packed_Set`. Enums registered via the trait macros remain the user-facing column type while `Raw_t` is their declared underlying integral type.
