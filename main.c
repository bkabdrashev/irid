// gcc -g -Wall -Wextra -Wno-unused-parameter -pedantic -fsanitize=address,undefined -fanalyzer main.c -o build/main && build/main
// gcc -g -Wall -Wextra -Wno-unused-parameter -pedantic -fsanitize=address,undefined main.c -o build/main && build/main
// /opt/llvm-mingw/bin/x86_64-w64-mingw32-clang -Wall -Wextra -pedantic -gcodeview -Wl,/debug,/pdb:build/main.pdb -o build/main.exe main.c
#include "includes.h"
// #include "irgen.c"
// #include "typeid.c"

int main(void) {
  istr_init(MB(64));
  istr_from_cstr_token_kind("if",     Token_Kind_if);
  istr_from_cstr_token_kind("do",     Token_Kind_do);
  istr_from_cstr_token_kind("el",     Token_Kind_else);
  istr_from_cstr_token_kind("return", Token_Kind_return);
  istr_from_cstr_token_kind("while",  Token_Kind_while);
  istr_from_cstr_token_kind("break",  Token_Kind_break);
  parse_test();
  irgen_test();
  return 0;
}
