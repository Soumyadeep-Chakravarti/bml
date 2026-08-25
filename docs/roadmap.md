# Roadmap

## BML 0.3 Baseline

The current implementation keeps the public binary AST and provides named
symbol assumptions, declarative rewrite metadata, bounded fixed-point
simplification, deterministic `+`/`*` canonicalization, structural-hash
memoization, and operation-cost analysis. The optimizer exposes readability,
operation-count, and C-runtime targets with explicit power/repeated-multiply
candidate costs. C output passes through a local lowered IR where CSE
temporaries and powers 2 through 4 are lowered before printing.

The deliberately small rule set only uses `x / x -> 1` and `x ^ 0 -> 1` when
`x` is known nonzero. Distribution, factorization, symbolic cancellation, and
general algebraic identities remain deferred.

## Endgame Demonstration

BML should be able to accept a scalar expression, simplify it with an
explanation trace, differentiate or evaluate it, and emit C code:

```text
input:        (x + y)^2 + sin(x)^2 + cos(x)^2
simplified:   (x + y)^2 + 1
derivative x: 2 * (x + y)
generated C:  return (x + y) * (x + y) + 1.0;
```

## Phase 1: Correct Core

- Define ownership, allocation-failure behavior, and identifier lifetime.
- Complete AST validation and public API consistency.
- Define arithmetic semantics for undefined expressions and non-finite values.
- Add regression, property, and sanitizer tests for every rewrite.

## Phase 2: Usable Scalar Algebra

- Add symbol bindings and numeric evaluation.
- Add substitution and a deterministic arithmetic canonical form.
- Expand the rule set only where semantic preconditions are explicit.
- Add readable infix output.

## Phase 3: Textual Workflow

- Implement lexer, infix parser, spans, and diagnostics.
- Add a CLI for parse, simplify, evaluate, and explain operations.
- Build a corpus of expressions used for parser and simplifier regression tests.

## Phase 4: Symbolic Differentiation

- Support arithmetic and a small set of elementary functions.
- Simplify derivative results using the existing transformation layer.
- Validate transformations by comparing evaluated results over valid sampled
  bindings.

## Next: Explainable Optimization And Output

- Extend the existing transformation trace and operation-count cost model.
- Improve local CSE only where it preserves scalar floating-point evaluation order.
- Emit portable C expressions or functions for more AST node kinds.
- Benchmark source and optimized expressions for node count, operation count,
  and numeric evaluation time.

## Deferred Work

- LaTeX input and output.
- Equality saturation or e-graph optimization.
- Matrices, geometry, units, statistics, probability, and equation solving.
- JIT compilation and target-specific code generation.
