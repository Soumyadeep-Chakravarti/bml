#!/usr/bin/env python3
"""Optional differential checks; exits 77 when SymPy is not installed."""
import argparse
import math
import subprocess
import sys

try:
    import sympy
except ImportError:
    print("SKIP: SymPy is unavailable")
    sys.exit(77)

parser = argparse.ArgumentParser()
parser.add_argument("--cli", required=True)
args = parser.parse_args()
corpus = [("x^2 + sin(x)", "x", 0.25), ("exp(x) + x^3", "x", -0.5), ("log(x) + x^2", "x", 1.5)]
for expression, variable, value in corpus:
    derivative = subprocess.check_output([args.cli, "diff", expression, variable], text=True).strip()
    bml_value = float(subprocess.check_output([args.cli, "eval", derivative, f"{variable}={value}"], text=True))
    symbolic = sympy.diff(sympy.sympify(expression.replace("^", "**")), sympy.Symbol(variable))
    expected = float(symbolic.subs(variable, value))
    if not math.isclose(bml_value, expected, rel_tol=1e-9, abs_tol=1e-9):
        raise SystemExit(f"mismatch for {expression}: {bml_value} != {expected}")
print("SymPy differential corpus passed")
