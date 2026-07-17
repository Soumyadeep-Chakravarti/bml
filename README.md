# Buna's Math Library (BML)

> **An experimental symbolic mathematics and optimization engine written in C.**

Buna's Math Library (BML) is an experimental symbolic mathematics and optimization engine written in C. Rather than treating mathematics as numerical computation alone, BML represents expressions as structured syntax trees that can be analyzed, transformed, simplified, optimized, and evaluated.

Most symbolic transformations in BML are intended to be implemented through pattern matching and rewrite rules, allowing mathematically equivalent expressions to be transformed into forms better suited for a given objective.

BML is designed to serve as a general-purpose mathematical engine for applications such as physics engines, graphics, simulations, scientific computing, optimization, robotics, and compiler research.

BML is inspired by symbolic mathematics systems such as Maple, Mathematica, Maxima, and SymPy, but is developed as an experimental and educational project focused on understanding how such systems can be built from first principles in C.

> **Status:** Early experimental design. APIs, architecture, and internal representations are expected to evolve.

---

## Features

### Current

- 🚧 Project architecture
- 🚧 AST design
- 🚧 Parser design
- 🚧 Documentation
- 🚧 Build system

### Planned

- Symbolic Algebra
- Numerical Mathematics
- Calculus
- Linear Algebra
- Geometry
- Trigonometry
- Complex Numbers
- Statistics
- Probability
- Optimization
- Differential Equations
- Coordinate Transformations
- Physics Mathematics
- Units & Dimensional Analysis
- Equation Solving
- Expression Simplification
- Expression Expansion
- Expression Factorization
- Expression Normalization
- Symbolic Optimization
- Code Optimization
- Automatic Code Generation
- Pretty-printing
- LaTeX Import / Export

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

## Long-Term Vision

The ultimate goal of BML is to become a general-purpose symbolic mathematics engine capable of:

- Understanding mathematical notation
- Reasoning about expressions symbolically
- Performing symbolic computation across mathematical disciplines
- Optimizing expressions for different objectives
- Generating efficient executable code
- Serving as the mathematical foundation for scientific software, simulations, graphics, robotics, and physics engines

The project philosophy is simple:

> **If a mathematical expression can be represented, BML should eventually be able to understand it, manipulate it, optimize it, and compute it.**

---

## Expression Input

BML aims to use **LaTeX mathematics as its primary input language**, allowing equations to be copied directly from lecture notes, textbooks, scientific papers, and documentation.

Example:

```latex
\frac{x^2+1}{\sqrt{x}}
```

instead of requiring a custom mathematical syntax.

Initially, BML will support a carefully selected mathematical subset of LaTeX. Support will gradually expand over time.

Advantages include:

- Copy equations directly from notes.
- Reuse mathematical notation already familiar to scientists and engineers.
- Produce publication-quality LaTeX output.
- Share a common language with documentation and research papers.

---

## Planned Architecture

```mermaid
graph TD

    Input([LaTeX Input])

    Lexer[Lexer]
    Parser[Parser]

    AST["Abstract Syntax Tree"]

    Validator[Validator]
    Simplifier[Simplifier]
    Symbolic[Symbolic Engine]
    Numerical[Numerical Engine]
    Solver[Equation Solver]
    Optimizer[Optimizer]
    CodeGen[Code Generator]
    Printer[Pretty Printer]

    Input --> Lexer
    Lexer --> Parser
    Parser --> AST

    AST --> Validator
    AST --> Simplifier
    AST --> Symbolic
    AST --> Numerical
    AST --> Solver
    AST --> Optimizer
    AST --> CodeGen
    AST --> Printer
```

The AST acts as the heart of the library. Every subsystem operates on the same internal mathematical representation.

---

## Planned Modules

- Core Infrastructure
- Expression Trees (AST)
- Memory Management
- Lexer
- Parser
- LaTeX Frontend
- Symbolic Algebra
- Calculus
- Numerical Analysis
- Linear Algebra
- Matrix Mathematics
- Vector Mathematics
- Geometry
- Complex Numbers
- Differential Equations
- Probability
- Statistics
- Optimization
- Physics Utilities
- Coordinate Systems
- Units & Dimensional Analysis
- Random Number Generation
- Code Generation
- Pretty Printer

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

## Roadmap

### Phase 1 — Core Infrastructure

- [ ] Arena Allocator
- [ ] AST
- [ ] Error System
- [ ] Context Management
- [ ] Basic Testing Framework

### Phase 2 — Parsing

- [ ] Lexer
- [ ] Tokenizer
- [ ] LaTeX Parser
- [ ] AST Builder
- [ ] Pretty Printer

### Phase 3 — Symbolic Mathematics

- [ ] Expression Evaluation
- [ ] Simplification
- [ ] Expansion
- [ ] Factorization
- [ ] Substitution

### Phase 4 — Calculus

- [ ] Differentiation
- [ ] Integration
- [ ] Limits
- [ ] Taylor Series

### Phase 5 — Scientific Mathematics

- [ ] Linear Algebra
- [ ] Geometry
- [ ] Numerical Methods
- [ ] Optimization
- [ ] Differential Equations

---

## Non-Goals

At least during the early stages of development, BML is **not** intended to:

- Implement the entirety of the TeX/LaTeX language.
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
