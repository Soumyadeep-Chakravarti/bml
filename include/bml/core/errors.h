#ifndef BML_CORE_ERRORS_H
#define BML_CORE_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BML_SUCCESS = 0,

  BML_ERROR_UNKNOWN,

  BML_ERROR_OUT_OF_MEMORY,

  BML_ERROR_INVALID_ARGUMENT,

  BML_ERROR_PARSE,

  BML_ERROR_DOMAIN,

  BML_ERROR_INTERNAL

} BML_Error;

const char *bml_error_string(BML_Error error);

#ifdef __cplusplus
}
#endif

#endif
