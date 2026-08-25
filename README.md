# Buna's Math Library (BML)

> **An experimental C11 symbolic optimizer for scalar real-valued expressions.**

Buna's Math Library (BML) is an experimental symbolic expression engine written
in C. It represents scalar arithmetic as syntax trees that can be validated,
transformed, evaluated, and eventually emitted as efficient C code.

Most symbolic transformations in BML are intended to be implemented through pattern matching and rewrite rules, allowing mathematically equivalent expressions to be transformed into forms better suited for a given objective.

BML is designed as an embeddable systems and compiler research project, not as
a replacement for general-purpose computer algebra systems.

BML is inspired by symbolic mathematics systems such as Maple, Mathematica, Maxima, and SymPy, but is developed as an experimental and educational project focused on understanding how such systems can be built from first principles in C.

> **Status:** BML 0.4 adds definedness-aware scalar rewrites, reusable numerical
> verification, a benchmark corpus, and an experimental bounded equality
> exploration backend. The local optimizer remains the default.

---

## Features

### Current

- Context-owned AST allocation, structural equality, validation, printing, and visitors
- Assumptions attached to symbols (`real`, `finite`, `positive`, `nonnegative`, `integer`, `nonzero`) with safe derived facts
- Bounded fixed-point simplification with stable rule IDs, categories, priorities, and explicit assumption requirements
- Canonical associative `+`/`*`: flattening, deterministic sorting, numeric coefficient/factor folding, and repeated-factor powers
- Infix `+ - * / ^`, unary minus, parentheses, variables, literals, and one-argument `sin`, `cos`, `exp`, `log`
- Evaluation from bindings, substitution, differentiation, canonical infix formatting, structural hash and operation-cost analysis
- Readability, operation-count, and C-runtime optimization targets with public candidate costs/results
- Public definedness classification plus deterministic equivalence and finite-difference derivative verification
- Experimental bounded `egraph` engine with a recorded binomial-square proof step; it is opt-in and intentionally not a general e-graph
- A structural benchmark corpus and `bml_bench` executable; it measures optimizer work, not compiled-runtime timings
- A separate lowered C layer that introduces CSE temporaries and lowers small integer powers before C printing
- CTest, sanitizers, formatting, clang-tidy, and Cppcheck workflows

### Target Workflow

```text
infix input or C API -> AST -> validation -> transformation -> evaluation or C output
```

## CLI

```sh
./build/dev/bml_cli simplify '(x + 0) * 1'
./build/dev/bml_cli eval 'sin(x) + y' x=0 y=2
./build/dev/bml_cli diff 'x^3 + cos(x)' x
./build/dev/bml_cli emit-c 'x^2 + sin(x)'
./build/dev/bml_cli emit-c --optimize '(x + y) * (x + y)'
./build/dev/bml_cli simplify 'x/x' --assume x:nonzero
./build/dev/bml_cli optimize 'x^3' --target=readability
./build/dev/bml_cli optimize '(x + y)^3 + sin(x + y)' --target=c --explain
./build/dev/bml_cli optimize '(x+y)^2 - 2*x*y' --engine=egraph --target=readability --explain
./build/dev/bml_cli verify '(x+y)^2 - 2*x*y' 'x^2+y^2'
./build/dev/bml_cli verify-diff 'sin(x) + x^3' x
./build/dev/bml_cli benchmark '(x+y)^4' --engine=local
./build/dev/bml_bench
./build/dev/bml_cli explain '(2 + 3) * 1'
```

`explain` reports only rewrites that actually fired. Domain-sensitive identities
such as `x - x` and `0 / x` are deliberately retained. `x / x` and `x ^ 0`
require a `nonzero` assumption (numeric nonzero literals are recognized).
`optimize --explain` reports the canonical form, detected CSE count, each candidate's
node/C-operation cost and score, the selected form, and generated C for target `c`.
Canonicalization deliberately does not distribute, factor, cancel general terms,
or apply trigonometric identities. Optimized C emission targets scalar `double`;
its CSE is intentionally local and does not model floating-point reassociation.

The focused endgame is an explainable symbolic optimizer for scalar real
expressions. See [the roadmap](docs/roadmap.md) and
[target architecture](docs/architecture.md).

---

## Design Principles

BML is built around a few core principles:

- Mathematics is represented symbolically before it is evaluated.
- Expressions are immutable trees that can be transformed safely.
- Mathematical equivalence is more important than textual equality.
- Optimization should preserve semantics.
- Algorithms should be modular and extensible.
- New mathematical domains should integrate through the same AST and rewrite framework.
- Minimal external dependencies.
- Portable ISO C.
- Correctness before optimization.

---

## Simplification vs. Optimization

BML distinguishes between simplification and optimization, which serve different goals.

### Simplification

Reduces expressions to a more canonical or concise form.

- `x + 0 → x`
- `sin²(x) + cos²(x) → 1`
- `2*x + 3*x → 5*x`

### Optimization

Transforms expressions to improve computational properties while preserving semantics.

- Common subexpression elimination
- Factorization to reduce FLOPs
- Algorithm selection for numerical stability
- Operation count reduction
- Vectorization-friendly reordering

---

## Philosophy

BML is built around one central idea:

> **Mathematical expressions should be treated as structured objects—not merely evaluated as floating-point numbers.**

For example,

```text
3*x^2 + 2*x + 1
```

should not immediately become a numerical value.

Instead, it should become an Abstract Syntax Tree (AST), allowing operations such as

- Differentiation
- Integration
- Simplification
- Expansion
- Factorization
- Variable substitution
- Domain analysis
- Expression normalization
- Expression comparison
- Equation solving
- Numerical evaluation
- Mathematical optimization
- Code generation
- Mathematical transformations

All of these operations work from the same internal representation.

One of BML's primary goals is to transform mathematically equivalent expressions into forms that are better suited for a specific objective. For example,

```text
x^2 + 2xy + y^2
```

may be transformed into

```text
(x + y)^2
```

because it requires fewer operations. Likewise,

```text
sin(x)^2 + cos(x)^2
```

may be simplified to

```text
1
```

and repeated subexpressions may be identified and computed only once to reduce computational cost.

The "best" representation of an expression depends on the objective. A symbolic optimizer may choose different forms depending on whether the target is numerical stability, computational efficiency, vectorization, GPU execution, code generation, or human readability.

BML therefore aims to become not only a mathematics library, but also a symbolic optimization engine capable of applying mathematical identities, transformations, rewrite rules, and algorithmic improvements to produce equivalent—but more useful—expressions.

---

## Showcase Endgame

The intended end-to-end demonstration is:

```text
input:        (x + y)^2 + sin(x)^2 + cos(x)^2
simplified:   (x + y)^2 + 1
derivative x: 2 * (x + y)
generated C:  return (x + y) * (x + y) + 1.0;
```

Each transformation should be explainable: BML should record the applied rule,
the expression before and after it, and any required domain assumption.

---

## Expression Input

The first textual frontend will be a conventional infix parser. It is smaller,
easier to test, and shares one semantic model with the C API.

Example:

```text
(x^2 + 1) / sqrt(x)
```

LaTeX import and export remain deferred frontends for a constrained subset;
they must produce the same AST and must not introduce separate semantics. See
[docs/parser.md](docs/parser.md).

---

## Documentation

- [Target architecture](docs/architecture.md)
- [Expression model](docs/ast.md)
- [Parser plan](docs/parser.md)
- [Scalar semantics and verification](docs/semantics.md)
- [Roadmap](docs/roadmap.md)

---

## Example API

The exact API is still under development, but the intended workflow looks similar to the following:

```c
#include <bml/bml.h>

int main(void)
{
    BML_Context *ctx = bml_create();

    BML_Expr *expr =
        bml_parse(ctx,
            "\\frac{x^2+1}{\\sqrt{x}}");

    BML_Expr *derivative =
        bml_diff(ctx, expr, "x");

    printf("%s\n",
        bml_to_latex(ctx, derivative));

    bml_destroy(ctx);

    return 0;
}
```

---

## Development

The repository provides a Nix development shell with Clang, CMake, Ninja,
clang-format, clang-tidy, Cppcheck, GDB, and Valgrind.

```sh
direnv allow                 # or: nix develop ./.nix
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
./build/dev/bml_example
```

The `dev` preset creates a debug build and `compile_commands.json` for editor
integration. Use `cmake --preset release` and `cmake --build --preset release`
for an optimized build. Formatting and analysis configuration is provided in
`.clang-format` and `.clang-tidy`.

Run the project quality tools from a configured build directory:

```sh
cmake --build build/dev --target format-check
cmake --build build/dev --target clang-tidy
cmake --build build/dev --target cppcheck
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
```

`format` applies the repository formatting convention. The `sanitize` preset
enables AddressSanitizer and UndefinedBehaviorSanitizer for the core test suite.

---

## Roadmap

The maintained implementation plan, including explicit deferred work, is in
[docs/roadmap.md](docs/roadmap.md).

---

## Non-Goals

At least during the early stages of development, BML is **not** intended to:

- Implement the entirety of the TeX/LaTeX language.
- Become a broad mathematics platform before the scalar symbolic core is complete.
- Maintain ABI compatibility between experimental releases.
- Prioritize maximum performance over correctness.

---

## Why?

BML exists because building mathematical software provides an opportunity to explore many areas of computer science, including:

- C Programming
- Compiler Design
- Parsing
- Abstract Syntax Trees
- Data Structures
- Numerical Methods
- Symbolic Computation
- Algorithms
- Software Architecture
- Scientific Computing

Whether BML eventually becomes a production-quality mathematics framework or remains an educational experiment, the knowledge gained while building it is the primary objective.

---

## Contributing

BML is currently a personal research and learning project.

Contributions, discussions, bug reports, feature requests, and architectural suggestions are always welcome. As the project matures, contribution guidelines and coding standards will be added.

---

## License

This project will be released under an open-source license.

The specific license has not yet been selected.
