// gcc -g -Wall -Wextra -Wno-unused-parameter -Wshadow -pedantic -fsanitize=address,undefined main.c -o build/main && build/main
// /opt/llvm-mingw/bin/x86_64-w64-mingw32-clang -Wall -Wextra -pedantic -gcodeview -Wl,/debug,/pdb:build/main.pdb -o build/main.exe main.c
#include "includes.h"
// #include "irgen.c"
// #include "typeid.c"

const char *__asan_default_options() {
  return "abort_on_error=1";
}

int main(void) {
  arena_test();
  parse_test();
  irgen_test();
  sem_test();
  return 0;
}
