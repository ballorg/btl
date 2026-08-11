# CLAUDE.md

Follow [AGENTS.md](AGENTS.md) — it contains the Ball project conventions and the binding documentation rules.

## Ball Documentation Maintenance

Read [`docs/DATA_STRUCTURES.md`](docs/DATA_STRUCTURES.md) before changing Ball data structures. Documentation maintenance is mandatory.

Documentation under `docs/` must be populated and maintained automatically as part of implementation work.

For every new relevant C++ type:

1. Preserve its exact source identifier and Hungarian notation.
2. Create its documentation under `docs/types/` (named `BTL.<TypeName>.md`).
3. Document its purpose, data structure, storage, ownership, lifetime, relationships, invariants, and invalidation rules when applicable.
4. Add it to the owning module document under `docs/modules/`.
5. Add it to `docs/README.md`.
6. Validate all links.

For changed types, update the affected documentation sections in the same change; for renamed, moved, or removed types, rename or delete the documents and fix all indexes and incoming links. Do not wait for a separate documentation request.

## Project notes

- The project name is written `Ball` (never `ball`, `BALL`, or `Ball Library`); the C++ namespace is `BTL` and the C++20 modules are `Ball.Types` / `Ball.Time`.
- Ball is freestanding C++20 (no std headers in library code), assertion-based (no exceptions), tab-indented with spaces inside parentheses.
