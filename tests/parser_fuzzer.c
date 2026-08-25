#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <bml/bml.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  char *input = malloc(size + 1);
  if (!input)
    return 0;
  memcpy(input, data, size);
  input[size] = '\0';
  BML_Context *ctx = bml_context_create();
  BML_Error error;
  if (ctx) {
    BML_Expr *expr = bml_parse(ctx, input, &error);
    if (expr)
      bml_simplify(ctx, expr);
    bml_context_destroy(ctx);
  }
  free(input);
  return 0;
}
