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
  AstKind_pos  = TokenKind_plus  | AstFlag_unary,
  AstKind_ptr  = TokenKind_at    | AstFlag_unary,
  AstKind_assign = TokenKind_equal | AstFlag_binary,
  AstKind_load = (TokenKind_at+1) | AstFlag_unary,
  AstKind_arr  = TokenKind_brace_close | AstFlag_binary,
  AstKind_call = 123 | AstFlag_binary,
  AstKind_block_enter = 124,
  AstKind_block_leave = 125,
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
  s32  length;
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

cstr cstr_from_slice_ast(Slice_Ast* slice) {
  String_Builder sb = string_builder_begin(&temp_arena, 6 * slice->length * sizeof(c8));
  for (s32 i = 0; i < slice->length; i++) {
    switch (slice->base[i].kind) {
    case AstKind_add:
      string_builder_push_cstr(&sb, "add");
    break;
    case AstKind_sub:
      string_builder_push_cstr(&sb, "sub");
    break;
    case AstKind_mul:
      string_builder_push_cstr(&sb, "mul");
    break;
    case AstKind_eof:
      string_builder_push_cstr(&sb, "eof");
    break;
    case AstKind_arr:
      string_builder_push_cstr(&sb, "arr");
    break;
    case AstKind_ptr:
      string_builder_push_cstr(&sb, "ptr");
    break;
    case AstKind_load:
      string_builder_push_cstr(&sb, "load");
    break;
    case AstKind_assign:
      string_builder_push_cstr(&sb, "=");
    break;
    case AstKind_block_enter:
      string_builder_push_cstr(&sb, "{");
      string_builder_push_s64(&sb, slice->base[i].val_s64);
    break;
    case AstKind_block_leave:
      string_builder_push_cstr(&sb, " }");
    break;
    case AstKind_call:
      string_builder_push_cstr(&sb, "call");
    break;
    case AstKind_name:
      string_builder_push_cstr(&sb, cstr_from_istr(slice->base[i].istr));
    break;
    case AstKind_neg:
      string_builder_push_cstr(&sb, "neg");
    break;
    case AstKind_pos:
      string_builder_push_cstr(&sb, "pos");
    break;
    }
    string_builder_push_cstr(&sb, " ");
  }
  cstr result = string_builder_end(&sb);
  return result;
}

void slice_ast_print(Slice_Ast* slice) {
  printf("%s\n", cstr_from_slice_ast(slice));
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
  Slice_Ast ast_inter;
  Slice_Ast ast_final;
  Slice_Token tokens;
  s32  tok;
  cstr source;
  cstr path;
} Parser;

Parser parser = {0};

Astid parse_ast_push(Slice_Ast* slice, AstKind kind, s32 value) {
  Ast ast = { .kind = kind, .value = value };
  Astid astid = { .index = slice_ast_push(slice, ast) };
  return astid;
}
void parse_ast_set_value(Slice_Ast slice, Astid astid, s32 value) {
  slice.base[astid.index].value = value;
}
void parse_transfer_all(Slice_Ast* from, Slice_Ast* to) {
  for (s32 i = 0; i < from->length; i++) {
    Ast ast = from->base[i];
    slice_ast_push(to, ast);
  }
  from->length = 0;
}

void parse_transfer_one(Slice_Ast* from, Slice_Ast* to) {
  Ast ast = slice_ast_pop(from);
  slice_ast_push(to, ast);
}

b8 parse_ast_inter_is_empty(void) {
  return slice_ast_empty(&parser.ast_inter);
}

void parse_ast_stack_push_unary(void) {
  Token token = slice_token_at(&parser.tokens, parser.tok);
  TokenKind kind = token.kind & 0xff;
  Ast ast = { .kind = kind | AstFlag_unary, .value = token.value };
  slice_ast_push(&parser.ast_stack, ast);
}

b8 parse_ast_stack_is_empty(void) {
  return slice_ast_empty(&parser.ast_stack);
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

Ast parse_ast_stack_pop(void) {
  return slice_ast_pop(&parser.ast_stack);
}

s32 parse_left_precedence(AstKind kind) {
  switch (kind) {
  case AstKind_sub:
  case AstKind_add: return 11;
  case AstKind_mul: return 13;
  case AstKind_ptr:
  case AstKind_neg:
  case AstKind_pos:  return 15;
  case AstKind_load: return 17;
  default :          return -1;
  }
}

s32 parse_right_precedence(AstKind kind) {
  switch (kind) {
  case AstKind_sub:
  case AstKind_add: return 12;
  case AstKind_mul: return 14;
  case AstKind_ptr:
  case AstKind_neg:
  case AstKind_pos:  return 16;
  case AstKind_load: return 18;
  default :          return -1;
  }
}

b8 parse_ast_stack_is_top_higher_precedence(AstKind kind) {
  if (slice_ast_empty(&parser.ast_stack)) return false;
  Ast stack_ast = slice_ast_top(&parser.ast_stack);
  s32 on_stack_precedence  = parse_left_precedence(stack_ast.kind);
  s32 on_stream_precedence = parse_right_precedence(kind);
  if (on_stack_precedence <= on_stream_precedence) {
    return false;
  }
  return true;
}

Parse_State parse_state_stack_pop(void) {
  Parse_State state = slice_parse_state_pop(&parser.state_stack);
  return state;
}

void parse_state_stack_push(Parse_State parse_state) {
  slice_parse_state_push(&parser.state_stack, parse_state);
}

Token parse_current_token(void) {
  return parser.tokens.base[parser.tok];
}

TokenKind parse_current_token_kind(void) {
  return parser.tokens.base[parser.tok].kind;
}

b8 parse_is_current_token(TokenKind kind) {
  return parser.tokens.base[parser.tok].kind == kind;
}

b8 parse_current_token_is_flag(TokenFlag flag) {
  Token token = parse_current_token();
  return (token.kind & flag) != 0;
}

void parse_next_token(void) {
  parser.tok++;
}

b8 parse_is_token_separates_expression(void) {
  Token token = parse_current_token();
  return (token.kind & TokenFlag_separates) != 0;
}

void parse_expression(void) {
  while (!parse_is_token_separates_expression()) {
    switch (parse_state_stack_pop()) {
    case Parse_State_expression: {
      Token token = parse_current_token();
      switch (token.kind) {
      case TokenKind_minus_prefix: case TokenKind_minus:
      case TokenKind_plus_prefix:  case TokenKind_plus:
      case TokenKind_at_prefix:    case TokenKind_at:
        parse_ast_stack_push_unary();
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_int: case TokenKind_name:
        parse_ast_push(&parser.ast_inter, (AstKind)token.kind, token.value);
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      case TokenKind_paren_open:
        parse_ast_stack_push((AstKind)token.kind, token.value);
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_paren_close:
        while (!parse_ast_stack_is_top(TokenKind_paren_open)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
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
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
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
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_stack_push(kind, token.value);
        parse_state_stack_push(Parse_State_expression);
      } break;
      case TokenKind_at: {
        AstKind kind = ((AstKind)token.kind + 1) | AstFlag_unary;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_push(&parser.ast_inter, kind, token.value);
        parse_state_stack_push(Parse_State_infix_or_suffix);
      } break;
      case TokenKind_paren_close:
        while (!parse_ast_stack_is_top(TokenKind_paren_open)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_stack_pop(); // pops TokenKind_paren_open
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      default: {
        if (token.kind & TokenFlag_call_rhs) {
          AstKind kind = (AstKind)token.kind;
          while (parse_ast_stack_is_top_higher_precedence(kind)) {
            parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
          }
          parse_ast_stack_push_kind(AstKind_call);
          parse_state_stack_push(Parse_State_infix_or_suffix);
        }
        else {
        }
      } break;
      }
    } break;
    }
    parse_next_token();
  }
  while (!parse_ast_stack_is_empty()) {
    parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
  }
}

Astid parse_statement(void) {
  Astid astid = {0};
  return astid;
}

void parse_tokens(Slice_Token tokens) {
  s32 total_ast_slice_length = tokens.length + 2;
  parser.ast_inter.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.ast_stack.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.ast_final.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.state_stack.base = xmalloc(sizeof(Parse_State) * total_ast_slice_length);
  parser.tokens = tokens;
  parser.tok = 0;
  s32 statements = 0;
  Astid block_enter = parse_ast_push(&parser.ast_final, AstKind_block_enter, 0);
  while (!parse_is_current_token(TokenKind_eof)) {
    parse_state_stack_push(Parse_State_expression);
    parse_expression();
    if (parse_is_current_token(TokenKind_semicolon)) {
      parse_next_token();
    }
    else if (parse_is_current_token(TokenKind_equal)) {
      parse_next_token();
      // a + b = c * d
      // inter: a, b, +, =, c, d * 
      // final: c, d, *, =, a,b,+
      s32 here = parser.ast_inter.length;
      parse_state_stack_push(Parse_State_expression);
      parse_expression();
      for (s32 i = here; i < parser.ast_inter.length; i++) {
        Ast ast = parser.ast_inter.base[i];
        slice_ast_push(&parser.ast_final, ast);
      }
      parse_ast_push(&parser.ast_final, AstKind_assign, 0);

      parser.ast_inter.length = here;
      parse_transfer_all(&parser.ast_inter, &parser.ast_final);
      printf("Ast_inter: ");
      slice_ast_print(&parser.ast_inter);
    }
    else {
      parse_transfer_all(&parser.ast_inter, &parser.ast_final);
    }
    statements++;
  }
  parser.ast_final.base[block_enter.index].value = statements;
  parse_ast_push(&parser.ast_final, AstKind_block_leave, 0);
}

void astid_from_source(cstr source, cstr path) {
  umi source_len = strlen(source);
  istr_init(source_len+1);
  Slice_Token tokens = lex_source(source, path);
  printf("%s\n", cstr_from_slice_token(tokens));
  parse_tokens(tokens);
  slice_ast_print(&parser.ast_final);
}

cstr source_to_cstr_from_ast(cstr source, cstr name) {
  astid_from_source(source, name);
  return cstr_from_slice_ast(&parser.ast_final);
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
  arena_init(&temp_arena, GB(1));
  cstr resulted = source_to_cstr_from_ast(source, cstr_from_source_info(file_name, line));
  b8 result = test_at_source(resulted, expected, file_name, line, source);
  arena_free_all(&temp_arena);
  return result;
}

#define test(source, expected) _test_ast(expected, __FILE__, __LINE__, source)

void parse_test(void) {
  test("a = b", "{}");
}

#undef test

