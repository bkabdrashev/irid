typedef enum {
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
  Ast_Flag_list   = 1 << 10,
} Ast_Flag;

typedef enum {
  Ast_Kind_eof  = Token_Kind_eof,
  Ast_Kind_name = Token_Kind_name,
  Ast_Kind_add  = Token_Kind_plus  | Ast_Flag_binary,
  Ast_Kind_sub  = Token_Kind_minus | Ast_Flag_binary,
  Ast_Kind_mul  = Token_Kind_star  | Ast_Flag_binary,
  Ast_Kind_neg  = Token_Kind_minus | Ast_Flag_unary,
  Ast_Kind_pos  = Token_Kind_plus  | Ast_Flag_unary,
  Ast_Kind_ptr  = Token_Kind_at    | Ast_Flag_unary,
  Ast_Kind_load = (Token_Kind_at+1) | Ast_Flag_unary,
  Ast_Kind_arr  = Token_Kind_brace_open | Ast_Flag_binary,
  Ast_Kind_subscript = (Token_Kind_brace_open+1) | Ast_Flag_binary,
  Ast_Kind_assign = Token_Kind_equal | Ast_Flag_binary,
  Ast_Kind_paren_open  = Token_Kind_paren_open | Ast_Flag_list,
  Ast_Kind_paren_close = Token_Kind_paren_close,
  Ast_Kind_call = 123 | Ast_Flag_binary,
  Ast_Kind_block_enter = Token_Kind_curly_open  | Ast_Flag_list,
  Ast_Kind_block_leave = Token_Kind_curly_close,
} Ast_Kind;

typedef struct {
  Ast_Kind kind;
  union {
    u64 value;
    s64 val_s64;
    Istr istr;
  };
} Ast; 

typedef struct {
  s32 index;
} Astid;

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
  String_Builder sb = string_builder_begin(&temp_arena, 10 * slice->length * sizeof(c8));
  for (s32 i = 0; i < slice->length; i++) {
    switch (slice->base[i].kind) {
    case Ast_Kind_add:
      string_builder_push_cstr(&sb, "add");
    break;
    case Ast_Kind_sub:
      string_builder_push_cstr(&sb, "sub");
    break;
    case Ast_Kind_mul:
      string_builder_push_cstr(&sb, "mul");
    break;
    case Ast_Kind_eof:
      string_builder_push_cstr(&sb, "eof");
    break;
    case Ast_Kind_arr:
      string_builder_push_cstr(&sb, "arr");
    break;
    case Ast_Kind_subscript:
      string_builder_push_cstr(&sb, "subscript");
    break;
    case Ast_Kind_ptr:
      string_builder_push_cstr(&sb, "ptr");
    break;
    case Ast_Kind_load:
      string_builder_push_cstr(&sb, "load");
    break;
    case Ast_Kind_assign:
      string_builder_push_cstr(&sb, "=");
    break;
    case Ast_Kind_block_enter:
      string_builder_push_cstr(&sb, "{");
      string_builder_push_s64(&sb, slice->base[i].val_s64);
    break;
    case Ast_Kind_block_leave:
      string_builder_push_cstr(&sb, "}");
    break;
    case Ast_Kind_paren_open:
      string_builder_push_cstr(&sb, "(");
      string_builder_push_s64(&sb, slice->base[i].val_s64);
    break;
    case Ast_Kind_paren_close:
      string_builder_push_cstr(&sb, ")");
    break;
    case Ast_Kind_call:
      string_builder_push_cstr(&sb, "call");
    break;
    case Ast_Kind_name:
      string_builder_push_cstr(&sb, cstr_from_istr(slice->base[i].istr));
    break;
    case Ast_Kind_neg:
      string_builder_push_cstr(&sb, "neg");
    break;
    case Ast_Kind_pos:
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
  Slice_Ast ast_stack;
  Slice_Ast ast_inter;
  Slice_Ast ast_final;
  Slice_Token tokens;
  s32  tok;
  cstr source;
  cstr path;
} Parser;

Parser parser = {0};

Astid parse_ast_push(Slice_Ast* slice, Ast_Kind kind, s32 value) {
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
  Token_Kind kind = token.kind & 0xff;
  Ast ast = { .kind = kind | Ast_Flag_unary, .value = token.value };
  slice_ast_push(&parser.ast_stack, ast);
}

b8 parse_ast_stack_is_empty(void) {
  return slice_ast_empty(&parser.ast_stack);
}

b8 parse_ast_stack_is_top(Ast_Kind kind) {
  Ast ast = slice_ast_top(&parser.ast_stack);
  return ast.kind == (Ast_Kind)kind;
}

Ast parse_ast_stack_top() {
  Ast ast = slice_ast_top(&parser.ast_stack);
  return ast;
}

void parse_ast_stack_push(Ast_Kind kind, s32 value) {
  Ast ast = { .kind = kind, .value = value };
  slice_ast_push(&parser.ast_stack, ast);
}

void parse_ast_stack_push_kind(Ast_Kind kind) {
  Ast ast = { .kind = kind, .value = 0 };
  slice_ast_push(&parser.ast_stack, ast);
}

Ast parse_ast_stack_pop(void) {
  return slice_ast_pop(&parser.ast_stack);
}

s32 parse_left_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_sub:
  case Ast_Kind_add: return 11;
  case Ast_Kind_mul: return 13;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:  return 15;
  case Ast_Kind_load: return 17;
  case Ast_Kind_call: return 19;
  default :          return -1;
  }
}

s32 parse_right_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_sub:
  case Ast_Kind_add: return 12;
  case Ast_Kind_mul: return 14;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:  return 16;
  case Ast_Kind_load: return 18;
  case Ast_Kind_call: return 20;
  default :          return -1;
  }
}

b8 parse_ast_stack_is_top_higher_precedence(Ast_Kind kind) {
  if (slice_ast_empty(&parser.ast_stack)) return false;
  Ast stack_ast = slice_ast_top(&parser.ast_stack);
  s32 on_stack_precedence  = parse_left_precedence(stack_ast.kind);
  s32 on_stream_precedence = parse_right_precedence(kind);
  if (on_stack_precedence <= on_stream_precedence) {
    return false;
  }
  return true;
}

Token parse_current_token(void) {
  return parser.tokens.base[parser.tok];
}

Token parse_next_token(void) {
  return parser.tokens.base[parser.tok+1];
}

Token_Kind parse_current_token_kind(void) {
  return parser.tokens.base[parser.tok].kind;
}

b8 parse_is_current_token(Token_Kind kind) {
  return parser.tokens.base[parser.tok].kind == kind;
}

b8 parse_current_token_is_flag(Token_Flag flag) {
  Token token = parse_current_token();
  return (token.kind & flag) != 0;
}

void parse_consume_token(void) {
  parser.tok++;
}

b8 parse_is_token_separates_expression(void) {
  Token token = parse_current_token();
  return (token.kind & Token_Kind_Flag_separates) != 0;
}

typedef enum {
  Parse_State_expression,
  Parse_State_infix_or_suffix,
} Parse_State;

Parse_State parse_state() {
  if (parser.ast_stack.length == 0) return Parse_State_expression;
  Ast top = parse_ast_stack_top();
  if (top.kind & Ast_Flag_unary)  return Parse_State_expression;
  if (top.kind & Ast_Flag_binary) return Parse_State_expression;
  if (top.kind & Ast_Flag_list)   return Parse_State_expression;
  return Parse_State_infix_or_suffix;
}

void parse_expression(void) {
  while (!parse_is_token_separates_expression()) {
    switch (parse_state()) {
    case Parse_State_expression: {
      Token token = parse_current_token();
      cstr_from_slice_ast(&parser.ast_inter)
      switch (token.kind) {
      case Token_Kind_minus_prefix: case Token_Kind_minus:
      case Token_Kind_plus_prefix:  case Token_Kind_plus:
      case Token_Kind_at_prefix:    case Token_Kind_at:
        parse_ast_stack_push_unary();
        break;
      case Token_Kind_int: case Token_Kind_name:
        parse_ast_push(&parser.ast_inter, (Ast_Kind)token.kind, token.value);
        break;
      case Token_Kind_paren_open: {
        Ast_Kind kind = (Ast_Kind)token.kind | Ast_Flag_list;
        parse_ast_stack_push(kind, token.value);
      } break;
      case Token_Kind_paren_close:
        while (!parse_ast_stack_is_top(Ast_Kind_paren_open)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_stack_pop(); // pops Token_Kind_paren_open
        break;
      case Token_Kind_brace_open:
      case Token_Kind_brace_prefix_open:
        parse_ast_stack_push((Ast_Kind)token.kind, token.value);
        break;
      case Token_Kind_brace_close:
        while (!parse_ast_stack_is_top(Ast_Kind_arr)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_stack_pop(); // pops Token_Kind_brace_open
        parse_ast_stack_push_kind(Ast_Kind_arr);// push Token_Kind_brace_close
        break;
      default: break;
      }
      if (token.flag & Token_Flag_call_lhs) {
        Token token = parse_next_token();
        if (token.flag & Token_Flag_call_rhs) {
          while (parse_ast_stack_is_top_higher_precedence(Ast_Kind_call)) {
            parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
          }
          parse_ast_stack_push_kind(Ast_Kind_call);
        }
      }
    } break;
    case Parse_State_infix_or_suffix: {
      Token token = parse_current_token();
      switch (token.kind) {
      case Token_Kind_plus:
      case Token_Kind_minus:
      case Token_Kind_star:
      case Token_Kind_brace_open: {
        Ast_Kind kind = (Ast_Kind)token.kind | Ast_Flag_binary;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_stack_push(kind, token.value);
      } break;
      case Token_Kind_at: {
        Ast_Kind kind = ((Ast_Kind)token.kind + 1) | Ast_Flag_unary;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_push(&parser.ast_inter, kind, token.value);
      } break;
      case Token_Kind_brace_close:
        while (!parse_ast_stack_is_top(Ast_Kind_arr)) {
          parse_transfer_one(&parser.ast_stack, &parser.ast_inter);
        }
        parse_ast_stack_pop(); // pops Token_Kind_brace_open
        break;
      default: {
      } break;
      }
    } break;
    }
    parse_consume_token();
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
  s32 total_ast_slice_length = 2*tokens.length + 2;
  parser.ast_inter.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.ast_stack.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.ast_final.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.tokens = tokens;
  parser.tok = 0;
  s32 statements = 0;
  Astid block_enter = parse_ast_push(&parser.ast_final, Ast_Kind_block_enter, 0);
  while (!parse_is_current_token(Token_Kind_eof)) {
    parse_expression();
    if (parse_is_current_token(Token_Kind_semicolon)) {
      parse_consume_token();
    }
    else if (parse_is_current_token(Token_Kind_equal)) {
      parse_consume_token();
      // a + b = c * d
      // inter: a, b, +, =, c, d * 
      // final: c, d, *, =, a,b,+
      s32 here = parser.ast_inter.length;
      parse_ast_push(&parser.ast_stack, Ast_Kind_assign, 0);
      parse_expression();
      for (s32 i = here; i < parser.ast_inter.length; i++) {
        Ast ast = parser.ast_inter.base[i];
        slice_ast_push(&parser.ast_final, ast);
      }

      parser.ast_inter.length = here;
      parse_transfer_all(&parser.ast_inter, &parser.ast_final);
    }
    else {
      parse_transfer_all(&parser.ast_inter, &parser.ast_final);
    }
    statements++;
  }
  parser.ast_final.base[block_enter.index].value = statements;
  parse_ast_push(&parser.ast_final, Ast_Kind_block_leave, 0);
}

void astid_from_source(cstr source, cstr path) {
  umi source_len = strlen(source);
  istr_init(source_len+1);
  Slice_Token tokens = lex_source(source, path);
  printf("%s\n", cstr_from_slice_token(tokens));
  parse_tokens(tokens);
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
  test("(a) + (c)", "{}");
}

#undef test

