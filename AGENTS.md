# Agent Instructions — Ball

Ball is a freestanding C++20 container/utility library (namespace `BTL`, optional public C++20 modules `Ball.Types` / `Ball.Time`). Headers and module interfaces live under `include/ball/`, the few compiled sources under `src/ball/`, tests under `src/ball/types/tests/`.

## Ball Documentation

The authoritative documentation workflow is defined in [`docs/DATA_STRUCTURES.md`](docs/DATA_STRUCTURES.md). Documentation maintenance is mandatory: read that specification before adding or modifying C++ types, and read [docs/README.md](docs/README.md) before major code changes.

When adding, modifying, renaming, moving, or removing a relevant C++ type, update the corresponding documentation under `docs/` in the same change. Inspect the affected Ball modules and source files directly — the source code is authoritative; never document behavior you cannot confirm in the repository.

Preserve exact Ball identifiers and Hungarian notation, including prefixes such as `C`, `I`, `S`, `E`, and `T` when present in the source code (e.g. `CVector`, `CStringView`, `ERBTreeColor`, `m_nCount`).

Before completing a code-related task:

- create documentation for new relevant types (under `docs/types/`, named `BTL.<TypeName>.md`);
- update documentation for changed types;
- update module and global indexes (`docs/modules/*.md`, `docs/README.md`);
- remove references to deleted types;
- validate all relative Markdown links.

A code change is incomplete while its Ball documentation is missing or outdated.

## Code conventions

- No standard C++ library in library headers — Ball ships its own traits, placement `new`, CRT declarations, and assertions (`BALL_ASSERT*`; no exceptions).
- Naming: classes `C*`, meta helpers `M*`, aliases `*_t`, enums `E*`, index type parameter `I`, inline capacity `N`; `I(-1)` is the invalid/NIL index.
- Tabs for indentation, spaces inside parentheses (`Foo( x, y )`), Allman braces — match the surrounding code.
- Doxygen comment conventions are described in the `doxygen-comments` skill (`.claude/skills/doxygen-comments/SKILL.md`).
- SoA containers address columns by type: same-typed columns must be wrapped in distinct `CReflect` tags (`BALL_REFLECT_TAGGED(_TEMPLATE)`); see [docs/architecture.md](docs/architecture.md).

## Build and tests

CMake ≥ 3.20 for header mode and ≥ 3.28 for modules; options `BALL_ENABLE_ASSERT`, `BALL_ENABLE_MODULES`, `BALL_ENABLE_TESTS`. Enabling tests forces module mode and `BALL_TEST_ENABLE_MODULES=ON`. Container tests are assembled into `ball-tests` from the `Ball.Types:Tests.CaseNN` partition interfaces and `Ball.Types` implementation units listed in [cmake/ball/types/tests.cmake](cmake/ball/types/tests.cmake); [src/ball/types/tests/main.cpp](src/ball/types/tests/main.cpp) imports every case partition. New container cases must be added to the CMake module file set and imported by `main.cpp`.
