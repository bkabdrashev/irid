typedef struct {
  Slice_ParseState state_stack;
  Slice_Ast ast_stack;
  Slice_Ast ast_final;
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

void parse_ast_final_push() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .tag = token.tag, .value = token.value };
  slice_ast_push(parser.ast_final, ast);
}

void parse_ast_final_push_unary() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .tag = token.tag | AstFlag_unary, .value = token.value };
  slice_ast_push(parser.ast_final, ast);
}

void parse_ast_stack_transfer_to_ast_final() {
  Ast ast = slice_ast_pop(parser.ast_stack):
  slice_ast_push(parser.ast_final, ast);
}

void parse_ast_stack_push() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .tag = token.tag, .value = token.value };
  slice_ast_push(parser.stack, ast);
}

void parse_ast_stack_push_binary() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .tag = token.tag | AstFlag_binary, .value = token.value };
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

b32 parse_ast_stack_top_is_higher_precedence() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  if (slice_ast_empty(parser.stack)) return true;
  Ast stack_ast = slice_ast_top(parser.stack);
  s32 on_stack_precedence  = parse_infix_left_precedence(ast.tag);
  s32 on_stream_precedence = parse_infix_right_precedence(token.tag);
  if (on_stack_precedence <= on_stream_precedence) {
    return false;
  }
  return true;
}

ParseState parse_state_stack_pop() {
  ParseState state = slice_ast_pop(parser.state_stack);
  return state;
}

ParseState parse_state_stack_push() {
  ParseState state = slice_ast_push(parser.state_stack);
  return state;
}

Astid parse_expression() {
  while (!parse_is_token_separates_expression()) {
    switch (parse_state_stack_pop()) {
    case ParseState_expression: {
      if (parse_match_token_flag(TokenFlag_prefix)) {
        parse_ast_final_push_unary()
        parse_state_stack_push(ParseState_expression);
      }
      else if (parse_match_token_flag(TokenFlag_atom)) {
        parse_ast_final_push()
        parse_state_stack_push(ParseState_infix_or_suffix);
      }
      else if (parse_match_token(TokenTag_paren_open)) {
        parse_ast_stack_push();
        parse_state_stack_push(ParseState_expression);
      }
      else if (parse_match_token(TokenTag_paren_close)) {
        while (!parse_ast_stack_top_is(TokenTag_paren_open)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_pop(); // pops TokenTag_paren_open
        parse_state_stack_push(ParseState_infix_of_suffix);
      }
      else if (parse_match_token(TokenTag_brace_open)) {
        parse_ast_stack_push();
        parse_state_stack_push(ParseState_expression);
      }
      else if (parse_match_token(TokenTag_brace_close)) {
        while (!parse_ast_stack_top_is(TokenTag_brace_open)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_pop(); // pops TokenTag_brace_open
        parse_ast_stack_push_binary();// push TokenTag_brace_close
        parse_state_stack_push(ParseState_expression);
      }
    } break;
    case ParseState_infix_or_suffix: {
      if (parse_match_token_flag(TokenFlag_infix)) {
        while (parse_ast_stack_top_is_higher_precedence()) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_push_binary();
        parse_state_stack_push(ParseState_expression);
      }
      else if (parse_match_token_flag(TokenFlag_suffix)) {
        while (parse_ast_stack_top_is_higher_precedence()) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_final_push_unary()
        parse_state_stack_push(ParseState_infix_or_suffix);
      }
      else if (parse_match_token_flag(TokenFlag_call_rhs)) {
        while (parse_ast_stack_top_is_higher_precedence()) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_push_call()
        parse_state_stack_push(ParseState_infix_or_suffix);
      }
    } break;
    }
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
  while (!parse_is_token_eof(slice_token_at(tokens, parser.tok))) {
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

