# Module: reflection

## Overview

A compile-time field-reflection facility: macros declare reflected fields inside a class, the meta layer assembles per-field specifications into a class descriptor, and the descriptor drives enumeration and raw byte serialization. The same `CReflect` wrapper doubles as the **column tag** mechanism that keeps same-typed SoA columns distinct (see [architecture.md](../architecture.md#same-type-soa-columns-and-reflect-tags)).

Entry header: [reflect.hpp](../../include/ball/types/reflect.hpp). Descriptor machinery: [meta/reflectvalue.hpp](../../include/ball/types/meta/reflectvalue.hpp), [meta/reflectfield.hpp](../../include/ball/types/meta/reflectfield.hpp), [meta/reflectdescriptor.hpp](../../include/ball/types/meta/reflectdescriptor.hpp), [meta/reflecttraits.hpp](../../include/ball/types/meta/reflecttraits.hpp), [meta/reflectforeach.hpp](../../include/ball/types/meta/reflectforeach.hpp).

## Responsibilities

- Declare reflected fields and generate their metadata:
  - `BALL_REFLECT_BEGIN( Owner )` / `BALL_REFLECT_FIELD( Type, name )` (alias `BALL_REFLECT`) / `BALL_REFLECT_BASE( DeclaredIn, name )` / `BALL_REFLECT_END( Owner )` — in-class declaration style; fields declared with `BALL_REFLECT_FIELD` are stored as `CReflect< Type, FieldSpec >` members.
  - `BALL_REFLECT_TYPE( Owner, FieldSpecs... )` with `BALL_REFLECT_FIELD_SPEC` — non-intrusive style for already-declared members.
- Unique column tags: `BALL_REFLECT_TAGGED( name, type )` and `BALL_REFLECT_TAGGED_TEMPLATE( name )` emit an empty tag struct plus a `CReflect< T, Tag >` alias whose distinct type keeps a column separate from same-typed columns.
- Field metadata (`MFieldSpec`, `MFieldStaticDesc`, `MField`): name (`CStringView`), offset (offsetof on standard-layout owners, measured pointer-to-member otherwise), size, alignment, uniform `Get`/`Set` accessors, per-field `Serialize`/`Deserialize`.
- Class descriptor (`MClass`): owner size/alignment, field tuple, total `SERIALIZED_SIZE`, whole-object `Serialize`/`Deserialize`, and compile-time layout validation (ordered, non-overlapping fields) for standard-layout owners.
- Detection: `IS_REFLECTABLE< O >`, the C++20 concept `Reflectable_t`, and `Reflect_t< O >` resolving the descriptor.
- Enumeration: `Reflect_ForEach< O >( callback )` visits fields in declaration order and reports the padding before/after each field.
- Raw serialization: `ReflectSerialize`/`ReflectDeserialize` append/read the value's bytes to or from a byte container (anything with `AddToTail`/`Base`/`Count`); restricted by `static_assert` to trivially copyable values.

## Public Interface

The macros above; [BTL::CReflect](../types/BTL.CReflect.md) with `ReflectAccess` / `ReflectAssign`; `Reflect_t`, `IS_REFLECTABLE`, `Reflect_ForEach`.

## Data Structures

- [BTL::CReflect](../types/BTL.CReflect.md) — the value wrapper, tag macros, field descriptors, and raw serialization helpers

## Dependencies

[meta](meta.md) (tuples, sequences, typeinfo, size), [strings](strings.md) (`CStringView` field names).

## Relationships

- [CRBTree](../types/BTL.CRBTree.md) tags its key and link columns; [CHashMap](../types/BTL.CHashMap.md) tags its key column.
- Serialization targets are typically byte vectors from the [containers module](containers.md).
- Exercised by [case06_reflection.cpp](../../src/ball/types/tests/case06_reflection.cpp).
