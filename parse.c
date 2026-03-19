typedef enum {
  AstFlag_unary  = 1 << 8,
  AstFlag_binary = 1 << 9,
} AstFlag;

typedef enum {
  AstKind_eof  = TokenKind_eof,
  AstKind_name = TokenKind_name,
  AstKind_add  = TokenKind_plus  | AstFlag_binary,
  AstKind_sub  = TokenKind_minus | AstFlag_binary,
  AstKind_mul  = TokenKind_star  | AstFlag_binary,
  AstKind_neg  = TokenKind_minus | AstFlag_unary,
  AstKind_arr  = TokenKind_brace_close | AstFlag_binary,
  AstKind_call = 123 | AstFlag_binary,
  AstKind_block = 124 | AstFlag_binary,
} AstKind;

typedef struct {
  AstKind kind;
  union {
    u64 value;
    s64 val_s64;
    Istr istr;
  };
} Ast; 

typedef struct {
  s32 index;
} Astid;

typedef enum {
  Parse_State_expression,
  Parse_State_infix_or_suffix,
} Parse_State;


typedef struct {
  Ast* base;
  umi  length;
} Slice_Ast;

s32 slice_ast_push(Slice_Ast* slice, Ast item) {
  s32 index = slice->length;
  slice->base[slice->length++] = item;
  return index;
}

Ast slice_ast_pop(Slice_Ast* slice) {
  return slice->base[--slice->length];
}
Ast slice_ast_top(Slice_Ast* slice) {
  return slice->base[slice->length-1];
}
b8 slice_ast_empty(Slice_Ast* slice) {
  return slice->length == 0;
}

typedef struct {
  Parse_State* base;
  umi  length;
} Slice_Parse_State;

void slice_parse_state_push(Slice_Parse_State* slice, Parse_State item) {
  slice->base[slice->length++] = item;
}

Parse_State slice_parse_state_pop(Slice_Parse_State* slice) {
  return slice->base[--slice->length];
}

typedef struct {
  Slice_Parse_State state_stack;
  Slice_Ast ast_stack;
  Slice_Ast ast_final;
  Slice_Token tokens;
  s32  tok;
  cstr source;
  cstr path;
} Parser;

Parser parser = {0};

Astid parse_ast_final_push(AstKind kind, s32 value) {
  Ast ast = { .kind = kind, .value = value };
  Astid astid = { .index = slice_ast_push(&parser.ast_final, ast) };
  return astid;
}

void parse_ast_final_push_unary() {
  Token token = slice_token_at(&parser.tokens, parser.tok);
  Ast ast = { .kind = token.kind | AstFlag_unary, .value = token.value };
  slice_ast_push(&parser.ast_final, ast);
}

b8 parse_ast_stack_is_empty() {
  return slice_ast_empty(&parser.ast_stack);
}

void parse_ast_stack_transfer_to_ast_final() {
  Ast ast = slice_ast_pop(&parser.ast_stack);
  slice_ast_push(&parser.ast_final, ast);
}

b8 parse_ast_stack_is_top(TokenKind kind) {
  Ast ast = slice_ast_top(&parser.ast_stack);
  return ast.kind == (AstKind)kind;
}

void parse_ast_stack_push(AstKind kind, s32 value) {
  Ast ast = { .kind = kind, .value = value };
  slice_ast_push(&parser.ast_stack, ast);
}

void parse_ast_stack_push_kind(AstKind kind) {
  Ast ast = { .kind = kind, .value = 0 };
  slice_ast_push(&parser.ast_stack, ast);
}

Ast parse_ast_stack_pop() {
  return slice_ast_pop(&parser.ast_stack);
}

s32 parse_infix_left_precedence(AstKind kind) {
  switch (kind) {
  case AstKind_add: return 11;
  case AstKind_mul:  return 13;
  default :          return -1;
  }
}

s32 parse_infix_right_precedence(AstKind kind) {
  switch (kind) {
  case AstKind_add: return 12;
  case AstKind_mul: return 14;
  default :         return -1;
  }
}

b8 parse_ast_stack_is_top_higher_precedence(AstKind kind) {
  if (slice_ast_empty(&parser.ast_stack)) return false;
  Ast stack_ast = slice_ast_top(&parser.ast_stack);
  s32 on_stack_precedence  = parse_infix_left_precedence(stack_ast.kind);
  s32 on_stream_precedence = parse_infix_right_precedence(kind);
  if (on_stack_precedence <= on_stream_precedence) {
    return false;
  }
  return true;
}

Parse_State parse_state_stack_pop() {
  Parse_State state = slice_parse_state_pop(&parser.state_stack);
  return state;
}

void parse_state_stack_push(Parse_State parse_state) {
  slice_parse_state_push(&parser.state_stack, parse_state);
}

Token parse_current_token() {
  return parser.tokens.base[parser.tok];
}

TokenKind parse_current_token_kind() {
  return parser.tokens.base[parser.tok].kind;
}

b8 parse_is_current_token(TokenKind kind) {
  return parser.tokens.base[parser.tok].kind == kind;
}

b8 parse_current_token_is_flag(TokenFlag flag) {
  Token token = parse_current_token();
  return (token.kind & flag) != 0;
}

void parse_next_token() {
  parser.tok++;
}

b8 parse_is_token_separates_expression() {
  Token token = parse_current_token();
  return (token.kind & TokenFlag_separates) != 0;
}

void parse_expression() {
  // TODO: 1) seprates expresssion is wrong (a+b) does seprate but we should keep going
  //       2) state info is wrong: (a+b) is in infix state when ) is proccessed
  while (!parse_is_token_separates_expression()) {
    switch (parse_state_stack_pop()) {
    case Parse_State_expression: {
      Token token = parse_current_token();
      switch (token.kind) {
      case TokenKind_minus_prefix: case TokenKind_minus:
      case TokenKind_plus_prefix:  case TokenKind_plus:
        parse_ast_final_push_unary();
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_int: case TokenKind_name:
        parse_ast_final_push((AstKind)token.kind, token.value);
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      case TokenKind_paren_open:
        parse_ast_stack_push((AstKind)token.kind, token.value);
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_paren_close:
        while (!parse_ast_stack_is_top(TokenKind_paren_open)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_pop(); // pops TokenKind_paren_open
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      case TokenKind_brace_open:
      case TokenKind_brace_prefix_open:
        parse_ast_stack_push((AstKind)token.kind, token.value);
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_brace_close:
        while (!parse_ast_stack_is_top(TokenKind_brace_open)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_pop(); // pops TokenKind_brace_open
        parse_ast_stack_push_kind(AstKind_arr);// push TokenKind_brace_close
        parse_state_stack_push(Parse_State_expression);
        break;
      default: break;
      }
    } break;
    case Parse_State_infix_or_suffix: {
      Token token = parse_current_token();
      switch (token.kind) {
      case TokenKind_plus:
      case TokenKind_minus:
      case TokenKind_star:
      case TokenKind_brace_open: {
        AstKind kind = (AstKind)token.kind | AstFlag_binary;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_push(kind, token.value);
        parse_state_stack_push(Parse_State_expression);
      } break;
      case TokenKind_at: {
        AstKind kind = (AstKind)token.kind;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_final_push(kind, token.value);
        parse_state_stack_push(Parse_State_infix_or_suffix);
      } break;
      default: {
        AstKind kind = (AstKind)token.kind;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_push_kind(AstKind_call);
        parse_state_stack_push(Parse_State_infix_or_suffix);
      } break;
      }
    } break;
    }
    parse_next_token();
  }
  while (!parse_ast_stack_is_empty()) {
    parse_ast_stack_transfer_to_ast_final();
  }
}

Astid parse_statement() {
  Astid astid = {0};
  return astid;
}

Astid parse_tokens(Slice_Token tokens) {
  parser.ast_final.base = malloc(MB(2));
  parser.ast_stack.base = malloc(MB(2));
  parser.state_stack.base = malloc(MB(2));
  parser.tokens = tokens;
  parser.tok = 0;
  s32 statements = 0;
  while (!parse_is_current_token(TokenKind_eof)) {
    parse_expression();
    statements++;
  }
  Astid block = parse_ast_final_push(AstKind_block, statements);
  return block;
}

Astid astid_from_source(cstr source, cstr path) {
  istr_init();
  Slice_Token tokens = lex_source(source, path);
  // printf("%s\n", cstr_from_slice_token(tokens));
  Astid astid = parse_tokens(tokens);
  return astid;
}

void string_builder_push_s64(String_Builder* sb, s64 val) {
  c8 line_str[20];
  sprintf(line_str, "%li", val);
  string_builder_push_cstr(sb, line_str);
}

void string_builder_push_ast(String_Builder* sb, Ast ast) {
  switch (ast.kind) {
  case AstKind_add:
    string_builder_push_cstr(sb, "+");
  break;
  case AstKind_mul:
    string_builder_push_cstr(sb, "*");
  break;
  case AstKind_name:
    cstr str = cstr_from_istr(ast.istr);
    string_builder_push_cstr(sb, str);
  break;
  case AstKind_block:
    string_builder_push_cstr(sb, "block ");
    string_builder_push_s64(sb, ast.val_s64);
  break;
  case AstKind_eof:
    string_builder_push_cstr(sb, "eof");
  break;
  default: 
    string_builder_push_cstr(sb, "<error>");
  break;
  }
}

cstr cstr_from_slice_ast(Slice_Ast slice) {
  String_Builder sb = string_builder_begin(&temp_arena);
  for (Ast* ast = slice.base; ast < slice.base + slice.length; ast++) {
    string_builder_push_ast(&sb, *ast);
    string_builder_push_cstr(&sb, ", ");
  }
  cstr str = string_builder_end(&sb);
  return str;
}

cstr source_to_cstr_from_ast(cstr source, cstr name) {
  astid_from_source(source, name);
  return cstr_from_slice_ast(parser.ast_final);
}

cstr cstr_from_source_info(cstr file_name, s32 line) {
  umi len1 = strlen(file_name);
  c8  line_str[10];
  sprintf(line_str, "%i", line);
  umi len2 = strlen(line_str);
  c8* buf = arena_alloc(&temp_arena, len1 + len2 + 2 + 1);
  strcat(buf, file_name);
  strcat(buf, "(");
  strcat(buf, line_str);
  strcat(buf, ")");
  return buf;
}

b8 _test_ast(cstr expected, cstr file_name, s32 line, cstr source) {
  arena_init(&temp_arena, MB(2));
  cstr resulted = source_to_cstr_from_ast(source, cstr_from_source_info(file_name, line));
  b8 result = test_at_source(resulted, expected, file_name, line, source);
  return result;
}

#define test(source, expected) _test_ast(expected, __FILE__, __LINE__, source)

void parse_test(void) {
  test("(a+b)", "{ a = 1; }");
}

#undef test

