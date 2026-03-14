typedef struct {
  Slice_Ast stack;
  Slice_Ast asts;
  Slice_Token tokens;
  s32  tok;
  cstr source;
  cstr path;
} Parser;

Parser parser = {0};
// a = 1
// 
// a
// 1
// assign
// 
// a = 1+2

Astid parse_push_ast() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .tag = token.tag, .value = token.value };
  slice_ast_push(parser.asts, ast);
}

void parse_stack_transfer() {
  Ast ast = slice_ast_pop(parser.stack):
  slice_ast_push(parser.asts, ast);
}

void parse_stack_push() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .tag = token.tag, .value = token.value };
  slice_ast_push(parser.stack, ast);
}

void parse_infix_left_precedence(TokenTag tag) {
  switch (tag) {
  case TokenTag_plus: return 11;
  case TokenTag_star: return 13;
  }
}

void parse_infix_right_precedence(TokenTag tag) {
  switch (tag) {
  case TokenTag_plus: return 12;
  case TokenTag_star: return 14;
  }
}

b32 parse_stack_not_have_lower_precedence() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  if (slice_ast_empty(parser.stack)) return true;
  Ast stack_ast = slice_ast_top(parser.stack);
  s32 on_stack_precedence  = parse_infix_left_precedence(ast.tag);
  s32 on_stream_precedence = parse_infix_right_precedence(token.tag);
  1 + 2 * 3
  1,2,3
  +*
  if (on_stack_precedence <= on_stream_precedence) {
    return false;
  }
  return true;
}

Astid parse_expression() {
  if (parse_match_token(TokenTag_int)) {
    parse_ast_push()
  }
  else if (parse_match_token(TokenTag_plus)) {
    while (parse_stack_has_not_lower_precedence()) {
      parse_stack_transfer();
    }
    parse_stack_push();
  }
  else if (parse_match_token(TokenTag_star)) {
    while (parse_stack_has_not_lower_precedence()) {
      parse_stack_transfer_ast();
    }
    parse_stack_push();
  }
}

Astid parse_statement() {
  Astid astid = {0};
  if (parse_match_token(Token_do)) {
    astid = parse_statement(p);
  }
  return astid;
}

Astid parse_tokens(Slice_Token tokens) {
  s32 statements = 0;
  while (lex_is_not_eof(slice_token_at(tokens, parser.tok))) {
    Astid astid = parse_statement();
    statements++;
    parser.tok++;
  }
  Astid block = astid_new_block(statements);
}

Astid astid_from_source(cstr source, cstr path) {
  Slice_Token tokens = lex_source(source, path);
  Astid astid = parse_tokens(tokens);
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

void parse_test(void) {
  test("a = 1", "{ a = 1; }");
}

#undef test

