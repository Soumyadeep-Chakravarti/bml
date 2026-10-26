#include "bml/bml.h"

#include <stdio.h>

static const char *kind_name(BML_ExprKind kind) {
    switch (kind) {
    case BML_EXPR_NUMBER: return "number";
    case BML_EXPR_SYMBOL: return "symbol";
    case BML_EXPR_ADD: return "add";
    case BML_EXPR_SUBTRACT: return "subtract";
    case BML_EXPR_MULTIPLY: return "multiply";
    case BML_EXPR_DIVIDE: return "divide";
    case BML_EXPR_POWER: return "power";
    case BML_EXPR_NEGATE: return "negate";
    case BML_EXPR_EQUAL: return "equal";
    case BML_EXPR_APPROACHES: return "approaches";
    case BML_EXPR_CALL: return "call";
    case BML_EXPR_INVALID: return "invalid";
    }
    return "unknown";
}

static void print_tree(const BML_Expr *expr, size_t depth) {
    for (size_t i = 0; i < depth; i++) {
        fputs("  ", stdout);
    }

    printf("%s", kind_name(bml_expr_kind(expr)));
    const char *name = bml_expr_name(expr);
    const char *number = bml_expr_number_text(expr);
    if (name != NULL) {
        printf(" %s", name);
    } else if (number != NULL) {
        printf(" %s", number);
    }
    putchar('\n');

    for (size_t i = 0; i < bml_expr_child_count(expr); i++) {
        print_tree(bml_expr_child(expr, i), depth + 1);
    }
}

int main(void) {
    static const char *const inputs[] = {
        "frac({a + b}{c + d})",
        "sqrt({x^2 + y^2})",
        "limit({sin(x)/x}, {x -> 0})",
        "diff({x^3 + 2*x}, {x})",
        "integral({x^2}, {x}, {0}, {1})",
        "sum({k}, {k = 1}, {n})",
    };

    BML_Context *ctx = bml_context_create();
    if (ctx == NULL) {
        fputs("Could not create BML context.\n", stderr);
        return 1;
    }

    int exit_code = 0;
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        BML_Expr *expr = bml_parse(ctx, inputs[i]);
        if (expr == NULL) {
            BML_Error error = bml_context_error(ctx);
            fprintf(stderr, "Parse failed at byte %zu in: %s\n",
                    error.offset, inputs[i]);
            exit_code = 1;
            break;
        }

        printf("Input: %s\n", inputs[i]);
        print_tree(expr, 1);
        putchar('\n');
    }

    bml_context_destroy(ctx);
    return exit_code;
}
