// gcc -g -Wall -Wextra -Wno-unused-parameter -Wshadow -pedantic -fsanitize=address,undefined main.c -o build/main && build/main
#include "includes.h"

const char *__asan_default_options() {
  return "abort_on_error=1";
}

int main(void) {
  arena_test();
  parse_test();
  irgen_test();
  sem_test();
  llvm_test();
  irid_run_path("test.i");
  return 0;
}
