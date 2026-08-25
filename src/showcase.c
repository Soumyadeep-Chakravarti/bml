#include "bml/showcase.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bml/core/arena.h"
#include "bml/core/ast_builder.h"
#include "bml/core/context.h"
#include "bml/core/simplify.h"
#include "bml/internal/expr_utils.h"

typedef struct {
  BML_Context *ctx;
  const char *cursor;
  BML_Error error;
  const char *message;
} Parser;

static void skip_space(Parser *parser) {
  while (isspace((unsigned char)*parser->cursor))
    parser->cursor++;
}

static const char *copy_name(BML_Context *ctx, const char *start, size_t length) {
  char *name = bml_arena_alloc(ctx->arena, length + 1);
  if (!name)
    return NULL;
  memcpy(name, start, length);
  name[length] = '\0';
  return name;
}

static BML_Expr *parse_expression(Parser *parser);

static BML_Expr *parse_primary(Parser *parser) {
  skip_space(parser);
  if (*parser->cursor == '(') {
    parser->cursor++;
    BML_Expr *expr = parse_expression(parser);
    skip_space(parser);
    if (*parser->cursor != ')') {
      parser->error = BML_ERROR_PARSE;
      parser->message = "expected ')'";
      return NULL;
    }
    parser->cursor++;
    return expr;
  }
  if (isdigit((unsigned char)*parser->cursor) || *parser->cursor == '.') {
    char *end;
    double value = strtod(parser->cursor, &end);
    if (end == parser->cursor) {
      parser->error = BML_ERROR_PARSE;
      parser->message = "invalid number";
      return NULL;
    }
    parser->cursor = end;
    return bml_number(parser->ctx, value);
  }
  if (isalpha((unsigned char)*parser->cursor) || *parser->cursor == '_') {
    const char *start = parser->cursor++;
    while (isalnum((unsigned char)*parser->cursor) || *parser->cursor == '_')
      parser->cursor++;
    const char *name = copy_name(parser->ctx, start, (size_t)(parser->cursor - start));
    if (!name) {
      parser->error = BML_ERROR_OUT_OF_MEMORY;
      return NULL;
    }
    skip_space(parser);
    if (*parser->cursor != '(')
      return bml_variable(parser->ctx, name);
    parser->cursor++;
    BML_Expr *argument = parse_expression(parser);
    skip_space(parser);
    if (!argument || *parser->cursor != ')' ||
        (strcmp(name, "sin") && strcmp(name, "cos") && strcmp(name, "exp") &&
         strcmp(name, "log"))) {
      parser->error = BML_ERROR_PARSE;
      parser->message = "expected a supported one-argument function";
      return NULL;
    }
    parser->cursor++;
    return bml_function(parser->ctx, name, argument);
  }
  parser->error = BML_ERROR_PARSE;
  parser->message = "expected a number, identifier, or '('";
  return NULL;
}

static BML_Expr *parse_unary(Parser *parser) {
  skip_space(parser);
  if (*parser->cursor == '-') {
    parser->cursor++;
    BML_Expr *operand = parse_unary(parser);
    return operand ? bml_negate(parser->ctx, operand) : NULL;
  }
  return parse_primary(parser);
}

static BML_Expr *parse_power(Parser *parser) {
  BML_Expr *left = parse_unary(parser);
  skip_space(parser);
  if (left && *parser->cursor == '^') {
    parser->cursor++;
    BML_Expr *right = parse_power(parser);
    return right ? bml_power(parser->ctx, left, right) : NULL;
  }
  return left;
}

static BML_Expr *parse_product(Parser *parser) {
  BML_Expr *left = parse_power(parser);
  while (left) {
    skip_space(parser);
    char op = *parser->cursor;
    if (op != '*' && op != '/')
      break;
    parser->cursor++;
    BML_Expr *right = parse_power(parser);
    if (!right)
      return NULL;
    left = op == '*' ? bml_multiply(parser->ctx, left, right)
                     : bml_divide(parser->ctx, left, right);
  }
  return left;
}

static BML_Expr *parse_expression(Parser *parser) {
  BML_Expr *left = parse_product(parser);
  while (left) {
    skip_space(parser);
    char op = *parser->cursor;
    if (op != '+' && op != '-')
      break;
    parser->cursor++;
    BML_Expr *right = parse_product(parser);
    if (!right)
      return NULL;
    left = op == '+' ? bml_add(parser->ctx, left, right)
                     : bml_subtract(parser->ctx, left, right);
  }
  return left;
}

BML_Expr *bml_parse_with_diagnostic(BML_Context *ctx, const char *input,
                                    BML_Error *error, BML_ParseDiagnostic *diagnostic) {
  Parser parser = {ctx, input, BML_SUCCESS, NULL};
  if (!ctx || !input)
    parser.error = BML_ERROR_INVALID_ARGUMENT;
  BML_Expr *expr = parser.error == BML_SUCCESS ? parse_expression(&parser) : NULL;
  if (parser.error == BML_SUCCESS)
    skip_space(&parser);
  if (!expr || (parser.error == BML_SUCCESS && *parser.cursor)) {
    if (parser.error == BML_SUCCESS) {
      parser.error = BML_ERROR_PARSE;
      parser.message = "unexpected trailing input";
    }
  }
  if (error)
    *error = parser.error;
  if (diagnostic) {
    diagnostic->offset = input ? (size_t)(parser.cursor - input) : 0;
    diagnostic->message =
        parser.message ? parser.message : bml_error_string(parser.error);
  }
  return parser.error == BML_SUCCESS ? expr : NULL;
}

BML_Expr *bml_parse(BML_Context *ctx, const char *input, BML_Error *error) {
  return bml_parse_with_diagnostic(ctx, input, error, NULL);
}

static BML_Error evaluate(const BML_Expr *expr, const BML_Binding *bindings,
                          size_t count, double *result) {
  if (!expr || !result)
    return BML_ERROR_INVALID_ARGUMENT;
  if (expr->type == BML_NODE_NUMBER) {
    *result = expr->value.number;
    return BML_SUCCESS;
  }
  if (expr->type == BML_NODE_VARIABLE) {
    for (size_t i = 0; i < count; i++)
      if (!strcmp(expr->value.identifier, bindings[i].name)) {
        *result = bindings[i].value;
        return BML_SUCCESS;
      }
    return BML_ERROR_INVALID_ARGUMENT;
  }
  double left, right;
  BML_Error error = evaluate(expr->children[0], bindings, count, &left);
  if (error != BML_SUCCESS)
    return error;
  if (expr->type == BML_NODE_NEGATE) {
    *result = -left;
    return BML_SUCCESS;
  }
  if (expr->type == BML_NODE_FUNCTION) {
    if (!strcmp(expr->value.identifier, "sin"))
      *result = sin(left);
    else if (!strcmp(expr->value.identifier, "cos"))
      *result = cos(left);
    else if (!strcmp(expr->value.identifier, "exp"))
      *result = exp(left);
    else if (!strcmp(expr->value.identifier, "log")) {
      if (left <= 0.0)
        return BML_ERROR_DOMAIN;
      *result = log(left);
    } else
      return BML_ERROR_INVALID_ARGUMENT;
    return isfinite(*result) ? BML_SUCCESS : BML_ERROR_DOMAIN;
  }
  error = evaluate(expr->children[1], bindings, count, &right);
  if (error != BML_SUCCESS)
    return error;
  if (expr->type == BML_NODE_ADD)
    *result = left + right;
  else if (expr->type == BML_NODE_SUBTRACT)
    *result = left - right;
  else if (expr->type == BML_NODE_MULTIPLY)
    *result = left * right;
  else if (expr->type == BML_NODE_DIVIDE) {
    if (right == 0.0)
      return BML_ERROR_DOMAIN;
    *result = left / right;
  } else if (expr->type == BML_NODE_POWER)
    *result = pow(left, right);
  else
    return BML_ERROR_INVALID_ARGUMENT;
  return isfinite(*result) ? BML_SUCCESS : BML_ERROR_DOMAIN;
}

BML_Error bml_evaluate(const BML_Expr *expr, const BML_Binding *bindings,
                       size_t binding_count, double *result) {
  if (binding_count && !bindings)
    return BML_ERROR_INVALID_ARGUMENT;
  return evaluate(expr, bindings, binding_count, result);
}

static BML_Expr *copy_or_substitute(BML_Context *ctx, const BML_Expr *expr,
                                    const char *name, const BML_Expr *replacement) {
  if (expr->type == BML_NODE_NUMBER)
    return bml_number(ctx, expr->value.number);
  if (expr->type == BML_NODE_VARIABLE) {
    return !strcmp(expr->value.identifier, name)
               ? (BML_Expr *)replacement
               : bml_variable(ctx, expr->value.identifier);
  }
  BML_Expr *a = copy_or_substitute(ctx, expr->children[0], name, replacement);
  if (!a)
    return NULL;
  if (expr->type == BML_NODE_NEGATE)
    return bml_negate(ctx, a);
  if (expr->type == BML_NODE_FUNCTION)
    return bml_function(ctx, expr->value.identifier, a);
  BML_Expr *b = copy_or_substitute(ctx, expr->children[1], name, replacement);
  if (!b)
    return NULL;
  switch (expr->type) {
  case BML_NODE_ADD:
    return bml_add(ctx, a, b);
  case BML_NODE_SUBTRACT:
    return bml_subtract(ctx, a, b);
  case BML_NODE_MULTIPLY:
    return bml_multiply(ctx, a, b);
  case BML_NODE_DIVIDE:
    return bml_divide(ctx, a, b);
  case BML_NODE_POWER:
    return bml_power(ctx, a, b);
  default:
    return NULL;
  }
}

BML_Expr *bml_substitute(BML_Context *ctx, const BML_Expr *expr, const char *name,
                         const BML_Expr *replacement) {
  return ctx && expr && name && replacement
             ? copy_or_substitute(ctx, expr, name, replacement)
             : NULL;
}

static BML_Expr *differentiate(BML_Context *ctx, const BML_Expr *e, const char *v) {
  if (e->type == BML_NODE_NUMBER)
    return bml_number(ctx, 0);
  if (e->type == BML_NODE_VARIABLE)
    return bml_number(ctx, !strcmp(e->value.identifier, v));
  BML_Expr *a = differentiate(ctx, e->children[0], v);
  if (!a)
    return NULL;
  if (e->type == BML_NODE_NEGATE)
    return bml_negate(ctx, a);
  if (e->type == BML_NODE_FUNCTION) {
    BML_Expr *factor = NULL;
    if (!strcmp(e->value.identifier, "sin"))
      factor = bml_function(ctx, "cos", e->children[0]);
    else if (!strcmp(e->value.identifier, "cos"))
      factor = bml_negate(ctx, bml_function(ctx, "sin", e->children[0]));
    else if (!strcmp(e->value.identifier, "exp"))
      factor = bml_function(ctx, "exp", e->children[0]);
    else if (!strcmp(e->value.identifier, "log"))
      factor = bml_divide(ctx, bml_number(ctx, 1), e->children[0]);
    return factor ? bml_multiply(ctx, factor, a) : NULL;
  }
  BML_Expr *b = differentiate(ctx, e->children[1], v);
  if (!b)
    return NULL;
  if (e->type == BML_NODE_ADD)
    return bml_add(ctx, a, b);
  if (e->type == BML_NODE_SUBTRACT)
    return bml_subtract(ctx, a, b);
  if (e->type == BML_NODE_MULTIPLY)
    return bml_add(ctx, bml_multiply(ctx, a, e->children[1]),
                   bml_multiply(ctx, e->children[0], b));
  if (e->type == BML_NODE_DIVIDE)
    return bml_divide(ctx,
                      bml_subtract(ctx, bml_multiply(ctx, a, e->children[1]),
                                   bml_multiply(ctx, e->children[0], b)),
                      bml_power(ctx, e->children[1], bml_number(ctx, 2)));
  if (e->type == BML_NODE_POWER && e->children[1]->type == BML_NODE_NUMBER)
    return bml_multiply(
        ctx,
        bml_multiply(ctx, bml_number(ctx, e->children[1]->value.number),
                     bml_power(ctx, e->children[0],
                               bml_number(ctx, e->children[1]->value.number - 1))),
        a);
  return NULL;
}

BML_Expr *bml_differentiate(BML_Context *ctx, const BML_Expr *expr,
                            const char *variable) {
  return ctx && expr && variable ? differentiate(ctx, expr, variable) : NULL;
}

typedef struct {
  char *buffer;
  size_t size;
  size_t used;
  BML_Error error;
} Writer;
static void writef(Writer *w, const char *format, ...) {
  if (w->error != BML_SUCCESS)
    return;
  va_list args;
  va_start(args, format);
  int written = vsnprintf(w->buffer + w->used,
                          w->size > w->used ? w->size - w->used : 0, format, args);
  va_end(args);
  if (written < 0 || (size_t)written >= w->size - w->used) {
    w->error = BML_ERROR_OUT_OF_MEMORY;
    return;
  }
  w->used += (size_t)written;
}
static int precedence(const BML_Expr *e) {
  if (e->type == BML_NODE_ADD || e->type == BML_NODE_SUBTRACT)
    return 1;
  if (e->type == BML_NODE_MULTIPLY || e->type == BML_NODE_DIVIDE)
    return 2;
  if (e->type == BML_NODE_NEGATE)
    return 3;
  if (e->type == BML_NODE_POWER)
    return 4;
  return 5;
}
static void format_expr(Writer *w, const BML_Expr *e, int parent, int right,
                        int c_mode) {
  int p = precedence(e), parens = p < parent || (right &&
                                                 (e->type == BML_NODE_SUBTRACT ||
                                                  e->type == BML_NODE_DIVIDE ||
                                                  e->type == BML_NODE_POWER) &&
                                                 p == parent);
  if (parens)
    writef(w, "(");
  if (e->type == BML_NODE_NUMBER)
    writef(w, "%.17g", e->value.number);
  else if (e->type == BML_NODE_VARIABLE)
    writef(w, "%s", e->value.identifier);
  else if (e->type == BML_NODE_NEGATE) {
    writef(w, "-");
    format_expr(w, e->children[0], p, 0, c_mode);
  } else if (e->type == BML_NODE_FUNCTION) {
    writef(w, "%s(", e->value.identifier);
    format_expr(w, e->children[0], 0, 0, c_mode);
    writef(w, ")");
  } else if (e->type == BML_NODE_POWER && c_mode) {
    if (bml_is_number(e->children[1]) && e->children[1]->value.number >= 0.0 &&
        e->children[1]->value.number <= 4.0 &&
        floor(e->children[1]->value.number) == e->children[1]->value.number) {
      unsigned exponent = (unsigned)e->children[1]->value.number;
      if (exponent == 0)
        writef(w, "1.0");
      else {
        writef(w, "(");
        for (unsigned i = 0; i < exponent; i++) {
          if (i)
            writef(w, " * ");
          format_expr(w, e->children[0], p, 0, c_mode);
        }
        writef(w, ")");
      }
    } else {
      writef(w, "pow(");
      format_expr(w, e->children[0], 0, 0, c_mode);
      writef(w, ", ");
      format_expr(w, e->children[1], 0, 0, c_mode);
      writef(w, ")");
    }
  } else if (e->type >= BML_NODE_ADD && e->type <= BML_NODE_POWER) {
    static const char *ops[] = {"+", "-", "*", "/", "^"};
    int index = e->type - BML_NODE_ADD;
    format_expr(w, e->children[0], p, 0, c_mode);
    writef(w, " %s ", ops[index]);
    format_expr(w, e->children[1], p, 1, c_mode);
  } else {
    w->error = BML_ERROR_INVALID_ARGUMENT;
  }
  if (parens)
    writef(w, ")");
}
static BML_Error format(const BML_Expr *e, char *buffer, size_t size, int c_mode) {
  if (!e || !buffer || !size)
    return BML_ERROR_INVALID_ARGUMENT;
  Writer w = {buffer, size, 0, BML_SUCCESS};
  buffer[0] = '\0';
  format_expr(&w, e, 0, 0, c_mode);
  return w.error;
}
BML_Error bml_format_infix(const BML_Expr *e, char *buffer, size_t size) {
  return format(e, buffer, size, 0);
}
BML_Error bml_emit_c(const BML_Expr *e, char *buffer, size_t size) {
  return format(e, buffer, size, 1);
}

/* Replaced by the lowered C IR implementation in core/lower.c. */
#if 0
typedef struct {
  const BML_Expr *expr;
  size_t count;
  size_t temporary;
} CseEntry;

static void count_subtrees(const BML_Expr *expr, CseEntry *entries, size_t *count,
                           size_t capacity) {
  if (!expr)
    return;
  for (size_t i = 0; i < *count; i++)
    if (bml_expr_equal(entries[i].expr, expr)) {
      entries[i].count++;
      goto children;
    }
  if (*count < capacity)
    entries[(*count)++] = (CseEntry){expr, 1, SIZE_MAX};
children:
  for (size_t i = 0; i < expr->child_count; i++)
    count_subtrees(expr->children[i], entries, count, capacity);
}

static void format_with_temps(Writer *writer, const BML_Expr *expr,
                              const CseEntry *entries, size_t count) {
  for (size_t i = 0; i < count; i++)
    if (entries[i].count > 1 && entries[i].expr->child_count &&
        bml_expr_equal(entries[i].expr, expr)) {
      writef(writer, "t%zu", entries[i].temporary);
      return;
    }
  if (expr->type >= BML_NODE_ADD && expr->type <= BML_NODE_POWER) {
    if (expr->type == BML_NODE_POWER) {
      if (bml_is_number(expr->children[1]) && expr->children[1]->value.number >= 0.0 &&
          expr->children[1]->value.number <= 4.0 &&
          floor(expr->children[1]->value.number) == expr->children[1]->value.number) {
        unsigned exponent = (unsigned)expr->children[1]->value.number;
        if (!exponent)
          writef(writer, "1.0");
        else {
          writef(writer, "(");
          for (unsigned i = 0; i < exponent; i++) {
            if (i)
              writef(writer, " * ");
            format_with_temps(writer, expr->children[0], entries, count);
          }
          writef(writer, ")");
        }
      } else {
        writef(writer, "pow(");
        format_with_temps(writer, expr->children[0], entries, count);
        writef(writer, ", ");
        format_with_temps(writer, expr->children[1], entries, count);
        writef(writer, ")");
      }
      return;
    }
    static const char *operators[] = {" + ", " - ", " * ", " / "};
    format_with_temps(writer, expr->children[0], entries, count);
    writef(writer, "%s", operators[expr->type - BML_NODE_ADD]);
    format_with_temps(writer, expr->children[1], entries, count);
    return;
  }
  if (expr->type == BML_NODE_NEGATE) {
    writef(writer, "-");
    format_with_temps(writer, expr->children[0], entries, count);
    return;
  }
  if (expr->type == BML_NODE_FUNCTION) {
    writef(writer, "%s(", expr->value.identifier);
    format_with_temps(writer, expr->children[0], entries, count);
    writef(writer, ")");
    return;
  }
  format_expr(writer, expr, 0, 0, 1);
}

BML_Error bml_emit_c_optimized(const BML_Expr *expr, const char *function_name,
                               const char *const *parameters, size_t parameter_count,
                               char *buffer, size_t buffer_size) {
  if (!expr || !function_name || !buffer || !buffer_size ||
      (parameter_count && !parameters))
    return BML_ERROR_INVALID_ARGUMENT;
  BML_Context *ctx = bml_context_create();
  if (!ctx)
    return BML_ERROR_OUT_OF_MEMORY;
  const BML_Expr *simplified = bml_simplify(ctx, expr);
  if (!simplified) {
    bml_context_destroy(ctx);
    return BML_ERROR_OUT_OF_MEMORY;
  }
  CseEntry entries[128];
  size_t count = 0;
  size_t temporary_count = 0;
  count_subtrees(simplified, entries, &count, 128);
  for (size_t i = 0; i < count; i++)
    if (entries[i].count > 1 && entries[i].expr->child_count)
      entries[i].temporary = temporary_count++;
  Writer writer = {buffer, buffer_size, 0, BML_SUCCESS};
  buffer[0] = '\0';
  writef(&writer, "#include <math.h>\n\ndouble %s(", function_name);
  for (size_t i = 0; i < parameter_count; i++)
    writef(&writer, "%sdouble %s", i ? ", " : "", parameters[i]);
  writef(&writer, ") {\n");
  for (size_t i = 0; i < count; i++)
    if (entries[i].count > 1 && entries[i].expr->child_count) {
      writef(&writer, "  const double t%zu = ", entries[i].temporary);
      format_expr(&writer, entries[i].expr, 0, 0, 1);
      writef(&writer, ";\n");
    }
  writef(&writer, "  return ");
  format_with_temps(&writer, simplified, entries, count);
  writef(&writer, ";\n}\n");
  BML_Error error = writer.error;
  bml_context_destroy(ctx);
  return error;
}
#endif
