#include <stdio.h>
#include <string.h>

#include <bml/bml.h>

int main(void) {
  static const char *const inputs[] = {"x",   "1+2",   "sin(x)", "(x+y)^2", "",     "(",
                                       "x+)", "log()", "x @ y",  "1e",      "cos(x"};
  BML_Context *ctx = bml_context_create();
  if (!ctx)
    return 1;
  for (size_t i = 0; i < sizeof(inputs) / sizeof(*inputs); i++) {
    BML_Error error;
    BML_ParseDiagnostic diagnostic;
    BML_Expr *expr = bml_parse_with_diagnostic(ctx, inputs[i], &error, &diagnostic);
    if (expr) {
      char output[256];
      if (bml_format_infix(bml_simplify(ctx, expr), output, sizeof(output)) !=
          BML_SUCCESS) {
        bml_context_destroy(ctx);
        return 1;
      }
    } else if (error != BML_ERROR_PARSE || diagnostic.offset > strlen(inputs[i])) {
      bml_context_destroy(ctx);
      return 1;
    }
  }
  bml_context_destroy(ctx);
  return 0;
}
