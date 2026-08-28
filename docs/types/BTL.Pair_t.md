# BTL::Pair_t

## Overview

A minimal key/value aggregate — Ball's `std::pair` stand-in, used by the tests and available as a general element type.

## Declaration

- **Namespace:** `BTL`
- **Module:** [utilities](../modules/utilities.md)
- **Kind:** struct template `Pair_t< typename K, typename V >`
- **Declared in:** [include/ball/types/pair.hpp](../../include/ball/types/pair.hpp)

## Data Structure

Two public members, `m_Key` and `m_Value`, with copy/move constructors for every lvalue/rvalue combination and dual accessor spellings (`Key`/`Value`, `First`/`Second`).

## Ownership and Lifetime

Plain aggregate semantics; members are direct subobjects.

## Type Relationships

Used as the element type in the vector test cases ([tests/vector_case.hpp](../../src/ball/types/tests/vector_case.hpp)).

## Operations

Comparisons against a bare key (`==`, `!=`, `<` compare the key only) and against another pair (member-wise; note `!=` and `<` require **both** members to satisfy the relation, which is not a lexicographic ordering).
