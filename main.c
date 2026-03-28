// gcc -g -Wall -Wextra -pedantic -fsanitize=address -fanalyzer main.c && .build/a.out
// gcc -g -Wall -Wextra -pedantic -fsanitize=address main.c && .build/a.out
// /opt/llvm-mingw/bin/x86_64-w64-mingw32-clang -Wall -Wextra -pedantic -gcodeview -Wl,/debug,/pdb:main.pdb -o main.exe main.c
#include "includes.h"
// #include "irgen.c"
// #include "typeid.c"

int main(void) {
  parse_test();
  return 0;
}
