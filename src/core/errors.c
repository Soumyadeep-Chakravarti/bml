#include "bml/core/errors.h"

const char *bml_error_string(BML_Error error) {
  switch (error) {
  case BML_SUCCESS:
    return "success";
  case BML_ERROR_OUT_OF_MEMORY:
    return "out of memory";
  case BML_ERROR_INVALID_ARGUMENT:
    return "invalid argument";
  case BML_ERROR_PARSE:
    return "parse error";
  case BML_ERROR_DOMAIN:
    return "domain error";
  case BML_ERROR_INTERNAL:
    return "internal error";
  case BML_ERROR_UNKNOWN:
  default:
    return "unknown error";
  }
}
