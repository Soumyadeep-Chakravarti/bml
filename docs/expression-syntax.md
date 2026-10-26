# Initial expression syntax

The initial parser builds an inspectable expression tree. It does not evaluate
expressions or assign special mathematical meaning to function names.

## Accepted forms

- Decimal and scientific-notation numbers, such as `12`, `.5`, `2.0`, and
  `1.25e-3`. Their source text is retained instead of converted to floating
  point.
- Identifiers beginning with an ASCII letter or underscore, followed by ASCII
  letters, digits, or underscores.
- Binary operators `+`, `-`, `*`, `/`, and `^`.
- Unary `+` and `-`.
- Parentheses or braces for grouping.
- Function calls with comma-separated arguments, such as `sqrt(x^2 + y^2)`.
  Adjacent braced arguments are also accepted for fraction-style calls, such as
  `frac({x + 1}{x - 1})`.
- Equality (`=`) and approach (`->`) relations, such as `k = 1` and `x -> 0`.

Multiplication and division associate left to right. Addition and subtraction
associate left to right. Powers associate right to left. Powers bind more
tightly than a leading unary sign, so `-x^2` parses as `-(x^2)`.

## Common mathematical forms

The following common mathematical forms can be entered directly inside C
strings:

```c
"frac({a + b}{c + d})"
"sqrt({x^2 + y^2})"
"limit({sin(x)/x}, {x -> 0})"
"diff({x^3 + 2*x}, {x})"
"integral({x^2}, {x}, {0}, {1})"
"sum({k}, {k = 1}, {n})"
```

These are parsed as named function calls with expression arguments. The parser
does not yet validate their argument counts or implement their mathematical
meaning. Equality and approach relations are represented structurally.

## Not handled yet

Other relations, implicit multiplication, LaTeX commands, and domain-specific
syntax for sets, matrices, and other mathematical objects are not supported
yet.

The grammar is an initial implementation and may change as BML's mathematical
representations develop.
