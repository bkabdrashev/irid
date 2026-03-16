#include <string.h>
#include "common.c"
typedef const char* cstr;
b8 cstr_eq(cstr a, cstr b) {
  return strcmp(a, b) == 0;
}
