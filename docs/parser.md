# Parser Plan

## First Frontend

The first parser should accept a conventional infix language, not general
LaTeX. This makes expression semantics, error reporting, and testing tractable
before adding notation-specific complexity.

Example input:

```text
(x + y)^2 + sin(x)^2 + cos(x)^2
```

## Initial Grammar

```text
expression  := sum
sum         := product (("+" | "-") product)*
product     := power (("*" | "/") power)*
power       := unary ("^" power)?
unary       := "-" unary | primary
primary     := number | identifier | identifier "(" expression ")" | "(" expression ")"
```

The parser must define precedence and associativity explicitly: exponentiation
is right-associative and has higher precedence than multiplication; unary minus
must be tested against exponentiation edge cases.

## Error Reporting

Errors should include a stable error kind and source span. The parser should
not create partially valid public ASTs after an error. Error recovery is not a
first-version requirement.

## LaTeX Later

LaTeX support should begin with a constrained subset: numbers, identifiers,
parentheses, `+`, `-`, powers, `\frac`, `\sqrt`, and named functions. It must
produce the same AST as the infix parser and must not become a second semantic
implementation.
