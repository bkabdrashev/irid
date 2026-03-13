#include "str.c"
#include "lexer.c"
#include "parser.c"
// #include "irgen.c"
// #include "typeid.c"

int main(void) {
  lexer_test();
  parser_test();
  return 0;
}
