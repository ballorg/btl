# BTL::CReflect

## Overview

A value-like wrapper with two jobs: (1) the storage cell of reflected fields declared through the `BALL_REFLECT_*` macros, carrying a hook point for serialization; and (2) — via unique empty tag types — the mechanism that gives otherwise identical types distinct identities so same-typed SoA columns do not alias.

## Declaration

- **Namespace:** `BTL`
- **Module:** [reflection](../modules/reflection.md)
- **Kind:** class template `CReflect< typename T, typename F = MReflectNone >` (`F` is the metadata/tag base); free functions `ReflectAccess`, `ReflectAssign`, `ReflectSerialize`, `ReflectDeserialize`; macros `BALL_REFLECT_TAGGED(_TEMPLATE)`, `BALL_REFLECT_BEGIN/END`, `BALL_REFLECT_FIELD` (alias `BALL_REFLECT`), `BALL_REFLECT_BASE`, `BALL_REFLECT_TYPE`, `BALL_REFLECT_FIELD_SPEC`
- **Declared in:** [include/ball/types/reflect.hpp](../../include/ball/types/reflect.hpp); descriptor machinery under [include/ball/types/meta/](../../include/ball/types/meta/) (`reflectfield.hpp`, `reflectdescriptor.hpp`, `reflecttraits.hpp`, `reflectforeach.hpp`)

## Purpose

Field-level metadata (name, offset, size, accessors) without RTTI or external tooling, plus raw byte serialization of trivially copyable values; and unique column tags for the SoA containers.

## Data Structure

`CReflect` stores exactly one `T m_Value` and inherits from `F` (an empty tag or field-spec type), so it is layout-identical to `T` for empty `F`. It converts implicitly to `T&`, assigns from anything assignable to `T`, exposes `Get()`, member-`operator->` for class payloads, and `Serialize`/`Deserialize` forwarding to the raw byte routines. `ReflectAccess`/`ReflectAssign` are the uniform pass-throughs: they unwrap a `CReflect` and leave plain values untouched, letting generic code handle both.

The descriptor layer (built by the macros, not stored in objects):

- `MFieldSpec` / `MFieldStaticDesc` / `MField` — per-field name ([CStringView](BTL.CStringView.md)), offset provider (`offsetof` for standard-layout owners, measured pointer-to-member otherwise), size/alignment, `Get`/`Set` accessors, and per-field serialize/deserialize. Compile-time bounds checks assert the field lies inside the owner.
- `MClass` — the class descriptor: owner size/align, the `MTuple` of fields, summed `SERIALIZED_SIZE`, whole-object `Serialize`/`Deserialize` (cursor-checked), and layout validation (ordered, non-overlapping) for standard-layout owners.
- `Reflect_t< O >` resolves a type's descriptor; `IS_REFLECTABLE< O >` / concept `Reflectable_t` detect the `ReflectionSentinel_t` + `MakeReflectionDescriptor` entry points the macros emit; `Reflect_ForEach< O >( cb )` visits fields in declaration order, reporting inter-field padding.

## Ownership and Lifetime

`CReflect` owns its payload like a plain member; it adds no allocation and no runtime cost beyond `T` itself.

## Type Relationships

- Tags used by [CRBTree](BTL.CRBTree.md) (`RBTreeKeyColumn_t`, `RBTreeLeft/Right/ParentColumn_t`) and [CHashMap](BTL.CHashMap.md) (`HashKeyColumn_t`); the aliasing problem is described in [architecture.md](../architecture.md#same-type-soa-columns-and-reflect-tags).
- Field names use [CStringView](BTL.CStringView.md); descriptors use `MTuple`, `MIndexSequence`, `MTypeInfo` from [meta](../modules/meta.md).

## Operations

- In-class reflection: `BALL_REFLECT_BEGIN( T )` … `BALL_REFLECT_FIELD( type, name )` … `BALL_REFLECT_END( T )` declares members as `CReflect< type, spec >` and emits `Serialize`/`Deserialize` members. `BALL_REFLECT_BASE` re-exports a field declared in a base class.
- Non-intrusive reflection: `BALL_REFLECT_TYPE( T, BALL_REFLECT_FIELD_SPEC( … ), … )` over existing members.
- Tagging: `BALL_REFLECT_TAGGED( Name, type )` → `Name_t` (a uniquely typed `CReflect< type, MName >`); `BALL_REFLECT_TAGGED_TEMPLATE( Name )` → `Name_t< T >` with the payload deferred.
- Raw serialization: byte-append into any container with `AddToTail`/`Base`/`Count`, cursor-based reads back; restricted to trivially copyable values by `static_assert`.

## Notes

Serialization is raw and unversioned: bytes in declaration order, with `Deserialize` requiring the storage to be consumed exactly. Exercised by [case06_reflection.cpp](../../src/ball/types/tests/case06_reflection.cpp).
