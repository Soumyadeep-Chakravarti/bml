# BML Architecture

BML is being developed as an embeddable C11 symbolic optimizer for scalar,
real-valued expressions. The goal is not to recreate a general-purpose computer
algebra system. Its core workflow is to parse or build an expression, validate
it, transform it with explainable rules, and evaluate or emit efficient C code.

The detailed target architecture is maintained in [docs/architecture.md](docs/architecture.md).
The staged delivery plan is in [docs/roadmap.md](docs/roadmap.md).

## Current Implementation

- Context-owned arena allocation with stable allocation addresses.
- Immutable-by-convention AST nodes for numbers, variables, constants, unary
  negation, and binary arithmetic.
- Structural equality, validation, AST printing, and depth-first visitors.
- Bottom-up arithmetic simplification through explicit rewrite rules.
- CTest coverage, sanitizers, formatting, clang-tidy, and Cppcheck targets.

## Design Principles

- The AST is the single expression representation shared by every subsystem.
- Transformations preserve semantics under stated domain assumptions.
- Rewrite rules should be explicit, ordered, testable, and explainable.
- The public API is portable C11 with minimal dependencies.
- Correctness and debuggability take priority over optimization.
