# Expression Model

## Scope

The initial expression model represents scalar real-valued arithmetic:

```text
number | symbol | constant | -expr | expr + expr | expr - expr
       | expr * expr | expr / expr | expr ^ expr | function(expr...)
```

Relations, vectors, matrices, calculus nodes, and piecewise expressions should
not be treated as complete merely because corresponding enum values exist.
Each requires explicit payload, validation, evaluation, and transformation
semantics before it becomes a supported domain.

## Ownership

A `BML_Context` owns the arena that allocates expression nodes. Expressions
remain valid until their owning context is destroyed. Individual expressions
are not freed; `bml_expr_destroy` should not be used as a normal ownership API
for context-allocated nodes.

The arena uses stable blocks rather than relocating allocations, so references
between AST nodes remain valid as the arena grows.

## Invariants

- Number, variable, and constant nodes have no children.
- Negation has exactly one child.
- Arithmetic binary operators have exactly two children.
- Every non-leaf child pointer is non-null.
- Nodes are not mutated after construction.
- Identifier storage must outlive the expression until symbol interning or
  context-owned string allocation is introduced.

## Equality

`bml_expr_equal` performs structural equality. It does not use commutativity,
associativity, or algebraic identities, so `x + y` and `y + x` are distinct.
Canonicalization is responsible for creating comparable normal forms.

## Definedness

Structural validity is separate from mathematical validity. For example,
`x / y` is structurally valid even when no assumption establishes `y != 0`.
Rules that require definedness must either preserve it exactly or require and
record the necessary assumptions.
