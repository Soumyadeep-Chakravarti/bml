#include "bml/showcase.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "bml/core/context.h"
#include "bml/core/simplify.h"
#include "bml/internal/expr_utils.h"

typedef enum { C_VALUE_SOURCE, C_VALUE_TEMP } CValueKind;
typedef struct {
  CValueKind kind;
  const BML_Expr *source;
  size_t temporary;
} CValue;
typedef struct {
  const BML_Expr *source;
  size_t uses;
  size_t temporary;
} CTemporary;
typedef struct {
  CTemporary entries[128];
  size_t count;
  size_t temporary_count;
} LoweredC;
typedef struct {
  char *buffer;
  size_t size;
  size_t used;
  BML_Error error;
} Writer;

static void writef(Writer *writer, const char *format, ...) {
  if (writer->error != BML_SUCCESS)
    return;
  va_list args;
  va_start(args, format);
  int written = vsnprintf(writer->buffer + writer->used,
                          writer->size > writer->used ? writer->size - writer->used : 0,
                          format, args);
  va_end(args);
  if (written < 0 || (size_t)written >= writer->size - writer->used) {
    writer->error = BML_ERROR_OUT_OF_MEMORY;
    return;
  }
  writer->used += (size_t)written;
}

static void collect(LoweredC *ir, const BML_Expr *expr) {
  for (size_t i = 0; i < ir->count; i++)
    if (bml_expr_equal(ir->entries[i].source, expr)) {
      ir->entries[i].uses++;
      goto children;
    }
  if (ir->count < 128)
    ir->entries[ir->count++] = (CTemporary){expr, 1, SIZE_MAX};
children:
  for (size_t i = 0; i < expr->child_count; i++)
    collect(ir, expr->children[i]);
}

static CValue lower_value(const LoweredC *ir, const BML_Expr *expr, int definition) {
  for (size_t i = 0; i < ir->count; i++)
    if (ir->entries[i].uses > 1 && ir->entries[i].source->child_count &&
        bml_expr_equal(ir->entries[i].source, expr) && !definition)
      return (CValue){C_VALUE_TEMP, NULL, ir->entries[i].temporary};
  return (CValue){C_VALUE_SOURCE, expr, 0};
}

static void print_value(Writer *writer, const LoweredC *ir, CValue value,
                        int definition) {
  if (value.kind == C_VALUE_TEMP) {
    writef(writer, "t%zu", value.temporary);
    return;
  }
  const BML_Expr *expr = value.source;
  if (!expr) {
    writer->error = BML_ERROR_INVALID_ARGUMENT;
    return;
  }
  if (expr->type == BML_NODE_NUMBER)
    writef(writer, "%.17g", expr->value.number);
  else if (expr->type == BML_NODE_VARIABLE)
    writef(writer, "%s", expr->value.identifier);
  else if (expr->type == BML_NODE_NEGATE) {
    writef(writer, "-");
    print_value(writer, ir, lower_value(ir, expr->children[0], 0), 0);
  } else if (expr->type == BML_NODE_FUNCTION) {
    writef(writer, "%s(", expr->value.identifier);
    print_value(writer, ir, lower_value(ir, expr->children[0], 0), 0);
    writef(writer, ")");
  } else if (expr->type == BML_NODE_POWER && bml_is_number(expr->children[1]) &&
             expr->children[1]->value.number >= 2.0 &&
             expr->children[1]->value.number <= 4.0) {
    double exponent_value = expr->children[1]->value.number;
    if (floor(exponent_value) == exponent_value) {
      unsigned exponent = (unsigned)exponent_value;
      writef(writer, "(");
      for (unsigned i = 0; i < exponent; i++) {
        if (i)
          writef(writer, " * ");
        print_value(writer, ir, lower_value(ir, expr->children[0], 0), 0);
      }
      writef(writer, ")");
    } else {
      writef(writer, "pow(");
      print_value(writer, ir, lower_value(ir, expr->children[0], 0), 0);
      writef(writer, ", ");
      print_value(writer, ir, lower_value(ir, expr->children[1], 0), 0);
      writef(writer, ")");
    }
  } else if (expr->type == BML_NODE_POWER) {
    writef(writer, "pow(");
    print_value(writer, ir, lower_value(ir, expr->children[0], 0), 0);
    writef(writer, ", ");
    print_value(writer, ir, lower_value(ir, expr->children[1], 0), 0);
    writef(writer, ")");
  } else {
    static const char *operators[] = {" + ", " - ", " * ", " / "};
    print_value(writer, ir, lower_value(ir, expr->children[0], 0), 0);
    writef(writer, "%s", operators[expr->type - BML_NODE_ADD]);
    print_value(writer, ir, lower_value(ir, expr->children[1], 0), 0);
  }
  (void)definition;
}

BML_Error bml_emit_c_optimized(const BML_Expr *expr, const char *function_name,
                               const char *const *parameters, size_t parameter_count,
                               char *buffer, size_t buffer_size) {
  if (!expr || !function_name || !buffer || !buffer_size ||
      (parameter_count && !parameters))
    return BML_ERROR_INVALID_ARGUMENT;
  BML_Context *ctx = bml_context_create();
  BML_OptimizerResult optimization;
  if (!ctx)
    return BML_ERROR_OUT_OF_MEMORY;
  if (!bml_optimize(ctx, expr, BML_OPTIMIZE_C_RUNTIME, &optimization)) {
    bml_context_destroy(ctx);
    return BML_ERROR_OUT_OF_MEMORY;
  }
  LoweredC ir = {0};
  collect(&ir, optimization.selected);
  for (size_t i = 0; i < ir.count; i++)
    if (ir.entries[i].uses > 1 && ir.entries[i].source->child_count)
      ir.entries[i].temporary = ir.temporary_count++;
  Writer writer = {buffer, buffer_size, 0, BML_SUCCESS};
  buffer[0] = '\0';
  writef(&writer, "#include <math.h>\n\ndouble %s(", function_name);
  for (size_t i = 0; i < parameter_count; i++)
    writef(&writer, "%sdouble %s", i ? ", " : "", parameters[i]);
  writef(&writer, ") {\n");
  for (size_t i = ir.count; i > 0; i--) {
    CTemporary *temporary = &ir.entries[i - 1];
    if (temporary->temporary != SIZE_MAX) {
      writef(&writer, "  const double t%zu = ", temporary->temporary);
      print_value(&writer, &ir, lower_value(&ir, temporary->source, 1), 1);
      writef(&writer, ";\n");
    }
  }
  writef(&writer, "  return ");
  print_value(&writer, &ir, lower_value(&ir, optimization.selected, 0), 0);
  writef(&writer, ";\n}\n");
  BML_Error error = writer.error;
  bml_context_destroy(ctx);
  return error;
}
