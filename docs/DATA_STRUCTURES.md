# Ball Data Structure Documentation Specification

## 1. Purpose

This specification defines the automatic creation and maintenance of human-readable documentation for C++ data structures in the Ball library.

The documentation must describe the actual architecture and data model of Ball without requiring a manually prepared list of modules, files, or types.

An AI agent must analyze the repository directly and automatically populate the `docs/` directory when:

* the documentation is created for the first time;
* a new C++ type is introduced;
* an existing C++ type is changed;
* a type is renamed, moved, or removed;
* ownership, lifetime, storage, or invalidation behavior changes;
* a module interface is modified.

Documentation updates are part of the implementation task and must be committed together with the corresponding source-code changes.

## 2. Project Naming

The project name must always be written as:

```text
Ball
```

The following forms must not be used when referring to the project:

```text
ball
BALL
Ball Library
```

`Ball library` may be used grammatically inside a sentence, but the project name itself must remain `Ball`.

Examples:

```text
Ball is a C++20 module library.
The Ball documentation is stored under docs/.
This type is part of the Ball.Container module.
```

## 3. Source of Truth

The Ball source code is the authoritative source of truth.

The AI agent must derive documentation from:

* C++20 module interface units;
* C++20 module implementation units;
* header files;
* inline implementation files;
* source files;
* templates;
* concepts;
* tests;
* examples;
* CMake configuration;
* module and target dependencies.

Existing documentation may be used for navigation, but it must never override the current implementation.

The agent must not invent behavior or guarantees that cannot be verified from the repository.

## 4. Documentation Location

All documentation must be stored under:

```text
docs/
```

Required structure:

```text
docs/
├── README.md
├── DATA_STRUCTURES.md
├── architecture.md
├── modules/
└── types/
```

Where:

* `docs/README.md` is the main documentation index;
* `docs/DATA_STRUCTURES.md` contains this specification;
* `docs/architecture.md` describes the high-level Ball architecture;
* `docs/modules/` contains module documentation;
* `docs/types/` contains documentation for individual C++ types.

The AI agent may introduce additional subdirectories when the repository becomes large, but documentation must remain reachable from `docs/README.md`.

## 5. AI Agent Integration

The repository-level `AGENTS.md` and `CLAUDE.md` files must reference this specification:

```markdown
[`docs/DATA_STRUCTURES.md`](DATA_STRUCTURES.md)
```

Both files must state that documentation maintenance is mandatory.

They must instruct an AI agent to:

1. Read `docs/DATA_STRUCTURES.md` before adding or modifying C++ types.
2. Inspect the affected Ball modules and source files.
3. Create documentation for every new relevant type.
4. Update documentation for every changed type.
5. Update module and global indexes.
6. Remove outdated documentation for deleted types.
7. Validate documentation before completing the task.

Detailed documentation rules must remain in `docs/DATA_STRUCTURES.md` and must not be duplicated entirely in `AGENTS.md` or `CLAUDE.md`.

## 6. Automatic Documentation Population

The AI agent must automatically compare the current source tree with the documentation tree.

For every relevant C++ type, the agent must determine whether:

* a documentation file already exists;
* the documentation matches the current declaration;
* the documented ownership model is still valid;
* the documented relationships are still valid;
* the type is present in the appropriate indexes.

When a relevant source type has no documentation, the agent must create it automatically.

The agent must not wait for an explicit documentation request.

A code change introducing a relevant undocumented type is incomplete.

## 7. Repository Scan Scope

For an initial documentation task, the AI agent must scan the complete Ball repository.

For a regular implementation task, the agent must scan:

* all modified source files;
* the owning module;
* directly related types;
* affected tests;
* affected documentation indexes.

The scan must identify:

* exported types;
* significant internal types;
* templates;
* concepts;
* aliases;
* ownership relationships;
* inheritance relationships;
* containment relationships;
* module dependencies;
* storage and allocation strategies.

The agent must not rely exclusively on version-control diffs when understanding a type. The complete declaration and relevant implementation must be analyzed.

## 8. Types Requiring Documentation

Standalone documentation is required for relevant types, including:

* classes;
* structures;
* unions;
* enumerations;
* class templates;
* structure templates;
* concepts;
* public type aliases;
* containers;
* allocators;
* buffers;
* strings and string views;
* spans and ranges;
* smart handles;
* resource-owning wrappers;
* public policy types;
* module-level data structures;
* types that represent persistent, serialized, or transferable data.

A private helper type may be documented inside the owning type or module document when it:

* has no independent semantic role;
* is not exported;
* is not reused by other components;
* does not introduce a separate ownership model.

Standalone documentation is not required for:

* trivial local structures used inside a single function;
* implementation-only tags with no architectural meaning;
* compiler-generated types;
* test-only fixtures with no reusable project semantics.

## 9. Hungarian Notation

Ball uses identifier prefixes that may represent type category, ownership, storage, interface role, or other project-specific semantics.

The AI agent must preserve exact identifier spelling.

Examples may include:

```cpp
CVector
CLeanVector
CArray
CStringView
CBufferStringN
IAllocator
EStorageMode
SNode
TValue
```

The agent must not normalize, simplify, or remove prefixes.

Incorrect:

```text
Vector
LeanVector
StringView
BufferString
Allocator
StorageMode
```

Correct:

```text
CVector
CLeanVector
CStringView
CBufferStringN
IAllocator
EStorageMode
```

### 9.1 Prefix Interpretation

A prefix may be explained only when its meaning is confirmed by Ball conventions or repository usage.

Potential prefix meanings must not be guessed.

For example, the agent must not automatically claim that:

* `C` always means `class`;
* `S` always means `struct`;
* `I` always means `interface`;
* `E` always means `enum`;
* `T` always means `template`.

These meanings may be documented only when supported by the codebase.

### 9.2 Exact Type Names

Documentation headings, links, indexes, and filenames must preserve the exact C++ type name.

Example:

```markdown
# CLeanVector
```

Not:

```markdown
# Lean Vector
```

A readable descriptive name may be added in prose:

```text
CLeanVector is a compact dynamically sized contiguous container.
```

### 9.3 Member Names

Member names may be included when they improve traceability.

If Ball uses member prefixes such as:

```cpp
m_pData
m_nSize
m_nCapacity
m_bOwnsMemory
```

the documentation must preserve them exactly.

The documentation must explain their semantics rather than treating the prefix itself as sufficient documentation.

Bad:

```text
m_pData is a pointer.
m_nSize is a number.
```

Good:

```text
`m_pData` refers to the beginning of the contiguous element storage.
`m_nSize` stores the number of currently constructed elements.
`m_nCapacity` stores the number of elements that can be held before
reallocation is required.
```

### 9.4 Templates and Suffixes

Template parameters and compile-time suffixes must remain unchanged.

Examples:

```cpp
CArray<T, N>
CBufferStringN<N>
TAllocator
TElement
```

The documentation must not rename them for stylistic consistency.

## 10. Documentation File Naming

A standalone type document should use the exact type name.

Recommended format:

```text
docs/types/<namespace>.<TypeName>.md
```

Examples:

```text
docs/types/Ball.CVector.md
docs/types/Ball.CLeanVector.md
docs/types/Ball.CStringView.md
docs/types/Ball.CBufferStringN.md
```

For nested types:

```text
docs/types/Ball.CVector.Iterator.md
```

For overloaded names from different modules or namespaces, include enough qualification to avoid ambiguity.

File names must remain stable unless the corresponding C++ entity is renamed or moved.

## 11. Type Documentation Template

Each standalone type document must use the following structure where applicable:

```markdown
# TypeName

## Overview

A concise explanation of the type and its role in Ball.

## Declaration

- **Namespace:** `...`
- **Module:** `...`
- **Kind:** class, structure, enumeration, concept, alias, or template
- **Declared in:** `...`

## Purpose

The responsibility or problem represented by the type.

## Data Structure

A human-readable description of the stored state and its logical
organization.

## Storage Model

How storage is represented, allocated, resized, shared, or borrowed.

## Ownership and Lifetime

Which resources are owned, borrowed, transferred, or referenced, and how
their lifetimes are constrained.

## Type Relationships

Inheritance, composition, aggregation, template relationships, and related
Ball abstractions.

## Invariants

Conditions that must remain valid while the object is in a usable state.

## Invalidation Rules

Operations that may invalidate references, pointers, iterators, views,
handles, or internal addresses.

## Operations

A concise description of the main public operations grouped by purpose.

## Usage

A minimal C++20 example when it materially improves understanding.

## Notes

Relevant limitations, compatibility rules, or implementation constraints.
```

Sections that do not apply must be omitted.

Empty placeholder sections are not allowed.

## 12. Data Structure Description

The `Data Structure` section is mandatory for data-bearing classes and structures.

It must describe:

* what information is stored;
* how the state is logically organized;
* which fields represent primary state;
* which fields contain cached or derived state;
* how empty state is represented;
* how moved-from state is represented when defined;
* how elements or records relate to each other;
* whether storage is contiguous, segmented, linked, inline, or external;
* whether size and capacity are tracked separately;
* whether runtime polymorphism is involved.

The description must not be a raw field dump.

Bad:

```text
The class contains m_pData, m_nSize, and m_nCapacity.
```

Good:

```text
CVector owns a contiguous allocation containing constructed elements.
It tracks the number of active elements independently from the allocation
capacity, allowing insertion without reallocation while unused capacity
remains.
```

## 13. Ownership and Lifetime Documentation

For every pointer, reference, view, handle, allocator, or resource-owning field, the AI agent must determine whether it is:

* owning;
* non-owning;
* borrowed;
* shared;
* transferred;
* optional;
* externally managed.

The agent must document lifetime requirements when they are verifiable.

Examples:

```text
CStringView does not own the referenced character sequence. The caller must
ensure that the underlying storage remains valid for the complete lifetime of
the view.
```

```text
CBuffer owns its allocation and releases it during destruction through the
configured allocator.
```

When ownership cannot be verified, the agent must not guess.

## 14. Invalidation Documentation

Container-like and view-like types must document invalidation behavior.

The AI agent must check whether operations invalidate:

* iterators;
* pointers;
* references;
* views;
* indexes;
* handles;
* cached addresses.

Typical operations requiring analysis include:

* insertion;
* removal;
* reallocation;
* reserve;
* resize;
* clear;
* move construction;
* move assignment;
* swap;
* allocator replacement.

Invalidation guarantees must be based on the implementation, not on behavior of similarly named standard-library types.

## 15. Module Documentation

Each major Ball module must have a document under:

```text
docs/modules/
```

A module document must contain:

```markdown
# ModuleName

## Overview

The purpose of the module.

## Responsibilities

The responsibilities owned by the module.

## Public Interface

Exported types, functions, variables, aliases, and concepts.

## Data Structures

Links to the documented types exposed or implemented by the module.

## Dependencies

Other Ball modules and relevant external dependencies.

## Relationships

How this module interacts with the rest of Ball.
```

The AI agent must preserve the exact module name used in source code.

## 16. Main Documentation Index

`docs/README.md` must contain:

* a concise description of Ball;
* a link to the architecture overview;
* a link to this specification;
* a module index;
* a categorized type index;
* navigation links to all standalone type documents.

Recommended type categories:

* containers;
* strings and text views;
* memory management;
* buffers;
* algorithms;
* utility types;
* interfaces;
* concepts;
* enumerations;
* internal architectural types.

The AI agent may determine categories from the actual Ball repository.

A type must not appear in multiple categories unless this improves navigation and the links remain consistent.

## 17. Automatic Update Workflow

When adding a new relevant C++ type, the AI agent must perform all of the following in the same task:

1. Analyze the complete declaration.
2. Analyze relevant implementation units.
3. Inspect tests and usage sites.
4. Determine the owning Ball module.
5. Determine the exact identifier and namespace.
6. Preserve Hungarian notation.
7. Create the type document under `docs/types/`.
8. Describe the logical data structure.
9. Describe storage, ownership, and lifetime behavior.
10. Describe relationships and invariants.
11. Document invalidation rules when applicable.
12. Add the type to the owning module document.
13. Add the type to `docs/README.md`.
14. Add cross-references to related types.
15. Validate all relative Markdown links.

The implementation task is incomplete until these actions are complete.

## 18. Existing Type Changes

When an existing type changes, the AI agent must check for changes to:

* fields;
* template parameters;
* inheritance;
* ownership;
* borrowing;
* allocation;
* capacity behavior;
* move semantics;
* copy semantics;
* destruction;
* invalidation;
* invariants;
* module ownership;
* exported API;
* serialization;
* binary layout assumptions;
* concurrency behavior;
* exception behavior.

Only affected documentation sections should be changed.

Unrelated documentation must not be rewritten without reason.

## 19. Renamed, Moved, or Removed Types

When a type is renamed:

* rename its documentation file;
* update its heading;
* update all links;
* update module indexes;
* update `docs/README.md`;
* remove the old name unless migration documentation is required.

When a type is moved:

* update its namespace;
* update its owning module;
* update its declaration location;
* update links and indexes.

When a type is removed:

* delete its standalone documentation;
* remove it from indexes;
* remove incoming links;
* update related documents that referenced it.

The documentation must represent the current Ball source tree.

## 20. Human-Readable Writing Requirements

Documentation must be written in clear technical English.

It must:

* explain purpose before implementation;
* use concise paragraphs;
* use stable headings;
* preserve exact C++ identifiers;
* use repository-relative links;
* distinguish ownership from borrowing;
* distinguish API guarantees from implementation details;
* remain understandable without reading every source file.

It must not:

* resemble an AST dump;
* copy declarations without explanation;
* list every private field without semantic value;
* repeat the same description across multiple files;
* use marketing language;
* invent design intent;
* replace technical explanations with vague phrases.

## 21. Code Examples

Code examples are optional.

An example should be included only when it clarifies:

* construction;
* ownership;
* lifetime;
* iteration;
* mutation;
* interaction between Ball types;
* a non-obvious usage constraint.

Examples must:

* use valid C++20 syntax;
* preserve Ball naming conventions;
* use exact current APIs;
* remain minimal;
* avoid unrelated setup code.

## 22. Validation

Before completing a code-related task, the AI agent must verify that:

* every new relevant type is documented;
* every changed type has current documentation;
* exact Hungarian-style names are preserved;
* the project name is written as `Ball`;
* module ownership is correct;
* namespaces are correct;
* declaration paths are correct;
* ownership descriptions match the implementation;
* invalidation descriptions match the implementation;
* removed types are no longer referenced;
* renamed types use their current names;
* all relative Markdown links resolve;
* `docs/README.md` is synchronized;
* module indexes are synchronized;
* no empty template sections remain;
* documentation is readable by a human developer.

## 23. Prohibited Assumptions

The AI agent must not claim any of the following without explicit implementation evidence:

* thread safety;
* lock-free behavior;
* exception guarantees;
* iterator stability;
* pointer stability;
* ABI stability;
* binary compatibility;
* fixed memory layout;
* standard-library compatibility;
* constant-time complexity;
* allocator propagation rules;
* ownership transfer;
* null-termination;
* moved-from state guarantees.

Similarity to a standard-library type is not evidence.

For example, `CVector` must not automatically be documented as behaving identically to `std::vector`.

## 24. Required AGENTS.md Section

`AGENTS.md` must contain an instruction equivalent to:

```markdown

## Ball Documentation

The authoritative documentation workflow is defined in
[`docs/DATA_STRUCTURES.md`](DATA_STRUCTURES.md).

When adding, modifying, renaming, moving, or removing a relevant C++ type,
update the corresponding documentation under `docs/` in the same change.

Preserve exact Ball identifiers and Hungarian notation, including prefixes
such as `C`, `I`, `S`, `E`, and `T` when present in the source code.

Before completing a code-related task:

- create documentation for new relevant types;
- update documentation for changed types;
- update module and global indexes;
- remove references to deleted types;
- validate all relative Markdown links.

A code change is incomplete while its Ball documentation is missing or
outdated.
```

## 25. Required CLAUDE.md Section

`CLAUDE.md` must contain an instruction equivalent to:

```markdown

## Ball Documentation Maintenance

Read [`docs/DATA_STRUCTURES.md`](DATA_STRUCTURES.md) before changing
Ball data structures.

Documentation under `docs/` must be populated and maintained automatically
as part of implementation work.

For every new relevant C++ type:

1. Preserve its exact source identifier and Hungarian notation.
2. Create its documentation under `docs/types/`.
3. Document its purpose, data structure, storage, ownership, lifetime,
   relationships, invariants, and invalidation rules when applicable.
4. Add it to the owning module document.
5. Add it to `docs/README.md`.
6. Validate all links.

Do not wait for a separate documentation request.
```

## 26. Acceptance Criteria

The task is complete when:

1. `docs/DATA_STRUCTURES.md` exists.
2. `AGENTS.md` links to `docs/DATA_STRUCTURES.md`.
3. `CLAUDE.md` links to `docs/DATA_STRUCTURES.md`.
4. `docs/README.md` provides navigation to Ball modules and types.
5. Existing significant Ball data structures are documented.
6. New relevant types are automatically documented during future AI-agent work.
7. Hungarian notation is preserved exactly.
8. The project name is consistently written as `Ball`.
9. Ownership, storage, and invalidation rules match the implementation.
10. Documentation remains concise, technical, and human-readable.
