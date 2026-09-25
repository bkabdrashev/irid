// gcc -g -Wall -Wextra -Wshadow -Wno-unused-parameter -pedantic -fsanitize=address,undefined $(llvm-config --cflags --libs core analysis bitwriter passes native support target) main.c -o build/main && build/main
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
  // irid_run_path("test.i");
  return 0;
}
