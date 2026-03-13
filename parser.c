typedef struct {
  Arena temp;
  s32  token;
  cstr source;
  cstr path;
} Parser;

Parser p = {0};
void parser_init(cstr source, cstr path) {
  p.temp = {0};
  p.source = source;
  p.path = path;
}

Astid parser_parse(Slice_Token tokens) {
  s32 statements = 0;
  while () {
    Astid astid = parser_statement(token);
    statements++;
  }
  Astid block = astid_new_block(statements);
}

Astid astid_from_source(cstr source, cstr path) {
  lexer_init(source, name);
  Slice_Token tokens = lexer_lex();

  parser_init(source, name);
  Astid astid = parser_parse(tokens);
  return astid;
}

cstr source_to_cstr_from_ast(cstr source, cstr name) {
  Astid astid = astid_from_source(source, name);
  return cstr_from_ast(astid);
}

b32 _test_ast(cstr file_name, u32 line, cc8* source, cc8* expected) {
  if (!test_str(source_to_cstr_from_ast(source, cstr_from_source_info(file_name, line)), expected)) {
    printf("%s(%i): at test source: %s\n", file_name, line, source);
    return false;
  }
  return true;
}

b32 _test_ast(cstr expected, cstr file_name, s32 line, cstr source) {
  Astid astid = astid_from_source(source, cstr_from_source_info(file_name, line));
  b32 result = test_at_source(cstr_from_ast(astid), expected, file_name, line, source);
  return result;
}

#define test(source, expected) _test_ast(expected, __FILE__, __LINE__, source)

void parser_test(void) {
  test("a = 1", "{ a = 1; }");
}

#undef test

