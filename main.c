// gcc -g -Wall -Wextra -pedantic -fsanitize=address -fanalyzer main.c -o build/main && build/main
// gcc -g -Wall -Wextra -pedantic -fsanitize=address main.c -o build/main && build/main
// /opt/llvm-mingw/bin/x86_64-w64-mingw32-clang -Wall -Wextra -pedantic -gcodeview -Wl,/debug,/pdb:build/main.pdb -o build/main.exe main.c
#include "includes.h"
// #include "irgen.c"
// #include "typeid.c"

int main(void) {
  parse_test();
  return 0;
}
