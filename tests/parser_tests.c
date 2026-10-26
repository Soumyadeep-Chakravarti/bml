#include "bml/bml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                                \
            exit(EXIT_FAILURE);                                                \
        }                                                                       \
    } while (0)

typedef struct {
    const char *source;
    const char *name;
    size_t argument_count;
} CallCase;

static const BML_Expr *parse(BML_Context *ctx, const char *source) {
    BML_Expr *expr = bml_parse(ctx, source);
    if (expr == NULL) {
        BML_Error error = bml_context_error(ctx);
        fprintf(stderr, "Could not parse '%s' (error %d at byte %zu)\n",
                source, (int)error.code, error.offset);
    }
    CHECK(expr != NULL);
    return expr;
}

static void test_showcase_inputs(BML_Context *ctx) {
    static const CallCase cases[] = {
        {"frac({a + b}{c + d})", "frac", 2},
        {"sqrt({x^2 + y^2})", "sqrt", 1},
        {"limit({sin(x)/x}, {x -> 0})", "limit", 2},
        {"diff({x^3 + 2*x}, {x})", "diff", 2},
        {"integral({x^2}, {x}, {0}, {1})", "integral", 4},
        {"sum({k}, {k = 1}, {n})", "sum", 3},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const BML_Expr *expr = parse(ctx, cases[i].source);
        CHECK(bml_expr_kind(expr) == BML_EXPR_CALL);
        CHECK(strcmp(bml_expr_name(expr), cases[i].name) == 0);
        CHECK(bml_expr_child_count(expr) == cases[i].argument_count);
    }

    const BML_Expr *limit = parse(ctx, cases[2].source);
    const BML_Expr *approach = bml_expr_child(limit, 1);
    CHECK(bml_expr_kind(approach) == BML_EXPR_APPROACHES);
    CHECK(bml_expr_kind(bml_expr_child(approach, 0)) == BML_EXPR_SYMBOL);
    CHECK(strcmp(bml_expr_name(bml_expr_child(approach, 0)), "x") == 0);
    CHECK(bml_expr_kind(bml_expr_child(approach, 1)) == BML_EXPR_NUMBER);
    CHECK(strcmp(bml_expr_number_text(bml_expr_child(approach, 1)), "0") == 0);

    const BML_Expr *sum = parse(ctx, cases[5].source);
    const BML_Expr *lower_bound = bml_expr_child(sum, 1);
    CHECK(bml_expr_kind(lower_bound) == BML_EXPR_EQUAL);
    CHECK(strcmp(bml_expr_name(bml_expr_child(lower_bound, 0)), "k") == 0);
}

static void test_precedence_and_associativity(BML_Context *ctx) {
    const BML_Expr *expr = parse(ctx, "x^2 + 2*x + 1");
    CHECK(bml_expr_kind(expr) == BML_EXPR_ADD);
    CHECK(bml_expr_kind(bml_expr_child(expr, 0)) == BML_EXPR_ADD);

    expr = parse(ctx, "a^b^c");
    CHECK(bml_expr_kind(expr) == BML_EXPR_POWER);
    CHECK(bml_expr_kind(bml_expr_child(expr, 1)) == BML_EXPR_POWER);

    expr = parse(ctx, "-x^2");
    CHECK(bml_expr_kind(expr) == BML_EXPR_NEGATE);
    CHECK(bml_expr_kind(bml_expr_child(expr, 0)) == BML_EXPR_POWER);

    expr = parse(ctx, "1-2-3");
    CHECK(bml_expr_kind(expr) == BML_EXPR_SUBTRACT);
    CHECK(bml_expr_kind(bml_expr_child(expr, 0)) == BML_EXPR_SUBTRACT);
}

static void test_literal_text_and_error_state(BML_Context *ctx) {
    const BML_Expr *expr = parse(ctx, ".125e+2");
    CHECK(bml_expr_kind(expr) == BML_EXPR_NUMBER);
    CHECK(strcmp(bml_expr_number_text(expr), ".125e+2") == 0);

    CHECK(bml_parse(ctx, "x +") == NULL);
    BML_Error error = bml_context_error(ctx);
    CHECK(error.code == BML_ERROR_PARSE);
    CHECK(error.offset == 3);

    CHECK(bml_parse(ctx, "x @ y") == NULL);
    error = bml_context_error(ctx);
    CHECK(error.code == BML_ERROR_PARSE);
    CHECK(error.offset == 2);

    expr = parse(ctx, "x");
    CHECK(bml_expr_kind(expr) == BML_EXPR_SYMBOL);
    CHECK(bml_context_error(ctx).code == BML_ERROR_NONE);

    CHECK(bml_parse(ctx, NULL) == NULL);
    CHECK(bml_context_error(ctx).code == BML_ERROR_INVALID_ARGUMENT);
}

int main(void) {
    BML_Context *ctx = bml_context_create();
    CHECK(ctx != NULL);

    test_showcase_inputs(ctx);
    test_precedence_and_associativity(ctx);
    test_literal_text_and_error_state(ctx);

    bml_context_destroy(ctx);
    puts("All parser tests passed.");
    return 0;
}
