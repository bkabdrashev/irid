B8 _test_ir(Cstr source, Cstr expected, Cstr file_name, I32 line) {
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void irgen_test(void) {
  test("a: b = c", "{}");
}

#undef test

