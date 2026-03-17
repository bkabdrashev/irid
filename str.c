typedef const char* cstr;
b8 cstr_eq(cstr a, cstr b) {
  return strcmp(a, b) == 0;
}

b8 test_at_source(cstr testee, cstr expected, cstr file_name, s32 line, cstr source) {
  if (cstr_eq(testee, expected)) {
    return true;
  }
  else {
    printf("%s(%i): at test source: %s\n", file_name, line, source);
    return false;
  }
}

