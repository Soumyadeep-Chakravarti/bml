# Scalar Semantics and Verification

BML 0.4 models finite scalar-real evaluation. `bml_expr_definedness` classifies
an expression as `BML_DEFINED`, `BML_DEFINED_UNKNOWN`, or `BML_UNDEFINED` from
its AST and context assumptions. It currently has explicit domain knowledge for
division (nonzero denominator), `log` (positive argument), and powers (positive
base for arbitrary real exponents; nonzero base for exponent zero). The result
is conservative: unknown is not a proof of failure.

Named assumptions are `real`, `finite`, `positive`, `nonnegative`, `integer`,
and `nonzero`. Positive symbols also derive nonnegative and nonzero facts. The
local simplifier consults this API for domain-sensitive rewrites, so `x / x` and
`x ^ 0` require a nonzero proof, while expressions such as `0 * log(x)` retain
the potentially undefined `log(x)`.

`bml_verify_equivalent` is a reusable numerical check for an original/candidate
pair. It accepts one caller-provided binding set or deterministically generates
valid samples from named assumptions. Its result records evaluated trials,
passes, maximum absolute error, and the first failure. `bml_verify_derivative`
uses centered finite differences at the same valid points to compare a symbolic
derivative. These checks are evidence, not a formal proof.

# Experimental Equality Exploration

`BML_OPTIMIZER_EGRAPH` is experimental and is never the default. It is a small,
bounded equality-exploration backend rather than an industrial e-graph: one
iteration, a 64-node limit, and the scalar binomial-square rule
`(a + b)^2 - 2*a*b == a^2 + b^2`. Candidate extraction uses the same existing
target cost selection as the local optimizer. Explanations include the rule
provenance and metrics. The deliberately limited rule set avoids implying that
general equality saturation or floating-point reassociation is supported.

For `BML_OPTIMIZE_OPERATION_COUNT`, every scalar arithmetic operation, power,
and function call scores as one mathematical operation. `BML_OPTIMIZE_C_RUNTIME`
instead uses the separate lowering estimate, where powers and calls cost more;
that target can therefore still prefer repeated multiplication for small powers.
