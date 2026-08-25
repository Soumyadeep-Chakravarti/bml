#include <bml/bml.h>

int main(void) {
  BML_Context *ctx = bml_context_create();
  BML_Error error;
  BML_Expr *expr = bml_parse(ctx, "x + 0", &error);
  if (!ctx || !expr || error != BML_SUCCESS || BML_VERSION_MINOR != 5) return 1;
  bml_context_destroy(ctx);
  return 0;
}
