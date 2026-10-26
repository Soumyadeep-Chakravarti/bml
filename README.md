# BML — Buna's Math Library

**A mathematics library written from scratch in C, built to explore mathematics beyond numerical computation.**

BML is an experimental project to build a general-purpose mathematical system as a C library. Its goal is to represent, manipulate, explore, and reason about mathematical objects and relationships—not merely evaluate expressions and return numerical answers.

The project begins with a simple interface: mathematical expressions supplied as strings. Behind that interface, BML aims to develop its own mathematical representations, algorithms, and computational machinery, with the long-term ambition of connecting diverse areas of mathematics within a coherent system.

BML is being restarted from scratch. Its current focus is establishing a sound foundation rather than claiming broad mathematical capabilities prematurely.

## Philosophy

Mathematics is more than computation.

A mathematical expression has structure. A function has a domain. An equation expresses a relationship. A limit depends on how a function behaves near a point. An algebraic transformation may require assumptions to be valid. A numerical result alone does not capture these distinctions.

BML aims to make such mathematical structure a first-class part of computation.

The project is guided by a few principles:

- **Mathematics first.** Numerical computation is one capability, not the definition of the entire library.
- **Structure over strings.** Input expressions should become meaningful mathematical objects that can be inspected and transformed.
- **Conditions matter.** Domains, assumptions, constraints, and mathematical validity must not be silently discarded.
- **Exactness where possible.** Preserve exact mathematical representations when appropriate, distinguishing them from approximations.
- **Explicit reasoning.** Transformations should be understandable, and their validity should be distinguishable from heuristic or experimental results.
- **Build from first principles.** Implement the underlying machinery in C rather than wrapping an existing computer algebra system or relying on an external mathematical engine.

## Scope and ambition

BML's intended scope is mathematics in its breadth. Its long-term direction includes, but is not limited to:

- **Algebra:** expressions, identities, equations, inequalities, polynomials, and algebraic transformations.
- **Calculus and analysis:** limits, continuity, differentiation, integration, convergence, and related concepts.
- **Linear algebra:** vectors, matrices, linear maps, systems of equations, and their underlying structures.
- **Multivariable and vector calculus:** partial derivatives, gradients, divergence, curl, and multiple integrals.
- **Discrete mathematics:** combinatorics, sequences, recurrence relations, and discrete structures.
- **Abstract mathematics:** sets, relations, functions, groups, rings, fields, and other mathematical structures.
- **Further mathematical domains:** differential equations, topology, geometry, and other areas as the underlying foundations develop.

These are long-term areas of exploration, not a declaration that they are already implemented or that every problem in these fields will be solvable.

The central ambition is to connect these domains through shared mathematical representations and principles instead of treating each as an unrelated collection of functions.

## Expression interface

BML is a C library intended to be included and used by other C programs.

Mathematical expressions are supplied as C strings using a dedicated mathematical expression syntax. The syntax is intended to retain familiar mathematical notation while using explicit function-style constructs where useful.

For example, the intended notation may include expressions such as:

```
x^2 + 2*x + 1
frac({x^2 - 1}{x - 1})
sqrt({x^2 + y^2})
limit({sin(x)/x}, {x -> 0})
```

These examples illustrate the intended direction; the grammar and public API are not yet finalized.

BML will parse expressions into its own internal representations. Those representations should support operations such as inspection, transformation, symbolic manipulation, and evaluation without requiring the original expression to remain an opaque string.

The input syntax is an interface to the mathematical system, not the mathematical system itself.

## Implementation principles

- **Language:** C, with no C++ implementation layer.
- **Independence:** Build the mathematical machinery in the project rather than delegating core functionality to external mathematical engines.
- **Modularity:** Keep mathematical representations, parsing, transformations, and evaluation conceptually distinct.
- **Correctness:** Test mathematical behavior, edge cases, domain restrictions, and invalid inputs—not only successful numerical examples.
- **Documentation:** Document the mathematical meaning, assumptions, limitations, and behavior of implemented operations.
- **Incremental development:** Introduce capabilities through small, testable milestones, establishing a reliable foundation before expanding into more advanced mathematics.

Ordinary development tools, compilers, build systems, debuggers, and testing tools are part of the development process. Building from scratch refers to BML's own mathematical machinery, not reinventing the tools used to develop software.

## Build and showcase

Enter the development environment and build the library, tests, and showcase:

```sh
nix develop ./.nix
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/bml_showcase
```

The showcase parses the example strings and prints their expression trees. The
current grammar and its limits are described in [the expression syntax
guide](docs/expression-syntax.md).

## Project status

**Early development — foundational implementation underway.**

BML is being rebuilt from scratch. Its eventual scope is intentionally ambitious, but the project will distinguish implemented capabilities from planned work and theoretical goals.

The initial C library foundation now includes context-owned expression trees and a parser for numeric literals, identifiers, arithmetic operators, equality and approach relations, grouping, and generic function calls. Numeric literals retain their source text. The parser does not evaluate expressions or interpret function names; its current syntax limits are documented in [the expression syntax guide](docs/expression-syntax.md).

There is no claim of complete computer algebra, general theorem proving, universal equation solving, or comprehensive support for every mathematical domain.

The immediate objective is to establish a reliable core on which those capabilities can be developed and evaluated over time.

## Long-term vision

BML aims to become a mathematical engine that can serve as a foundation for experimentation, exploration, and deeper computational work in mathematics.

The ambition is not simply to accumulate more operations. It is to build a system in which mathematical objects, their relationships, the conditions under which statements hold, and the transformations applied to them can be represented coherently.

Mathematics is vast. BML is an attempt to explore that vastness through software, one sound foundation at a time.

---

*BML — Buna's Math Library. Written in C, built from first principles, and developed in pursuit of mathematics itself.*
