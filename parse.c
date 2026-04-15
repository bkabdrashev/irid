typedef enum {
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
  Ast_Flag_list   = 1 << 10,
} Ast_Flag;

typedef enum {
  Ast_Kind_eof  = Token_Kind_eof,
  Ast_Kind_name = Token_Kind_name,
  Ast_Kind_int  = Token_Kind_int,
  Ast_Kind_add  = Token_Kind_plus  | Ast_Flag_binary,
  Ast_Kind_sub  = Token_Kind_minus | Ast_Flag_binary,
  Ast_Kind_mul  = Token_Kind_star  | Ast_Flag_binary,
  Ast_Kind_neg  = Token_Kind_minus | Ast_Flag_unary,
  Ast_Kind_pos  = Token_Kind_plus  | Ast_Flag_unary,
  Ast_Kind_ptr  = Token_Kind_at    | Ast_Flag_unary,
  Ast_Kind_load = (Token_Kind_at+1) | Ast_Flag_unary,
  Ast_Kind_subscript_open  = Token_Kind_brace_open      | Ast_Flag_binary,
  Ast_Kind_subscript_close = Token_Kind_brace_close     | Ast_Flag_binary,
  Ast_Kind_array_open      = (Token_Kind_brace_open+1)  | Ast_Flag_binary,
  Ast_Kind_array_close     = (Token_Kind_brace_close+1) | Ast_Flag_binary,
  Ast_Kind_assign          = Token_Kind_equal           | Ast_Flag_binary,
  Ast_Kind_paren_open      = Token_Kind_paren_open,
  Ast_Kind_paren_close     = Token_Kind_paren_close,
  Ast_Kind_record_open     = (Token_Kind_paren_open+1) | Ast_Flag_list,
  Ast_Kind_record_close    = (Token_Kind_paren_close+1),
  Ast_Kind_call = 123 | Ast_Flag_binary,
  Ast_Kind_block_enter = Token_Kind_curly_open  | Ast_Flag_list,
  Ast_Kind_block_leave = Token_Kind_curly_close,
} Ast_Kind;

typedef struct {
  Ast_Kind kind;
  union {
    U64 value;
    S64 s64;
    Istr istr;
  };
} Ast; 

typedef struct {
  S32 index;
} Astid;

typedef struct {
  Ast* base;
  S32  length;
} Slice_Ast;

S32 slice_ast_push(Slice_Ast* slice, Ast item) {
  S32 index = slice->length;
  slice->base[slice->length++] = item;
  return index;
}
Ast slice_ast_pop(Slice_Ast* slice) {
  return slice->base[--slice->length];
}
Ast slice_ast_top(Slice_Ast* slice) {
  return slice->base[slice->length-1];
}
B8 slice_ast_is_empty(Slice_Ast* slice) {
  return slice->length == 0;
}

typedef struct {
  Astid* base;
  S32  length;
} Slice_Astid;

S32 slice_astid_push(Slice_Astid* slice, Astid item) {
  S32 index = slice->length;
  slice->base[slice->length++] = item;
  return index;
}
Astid slice_astid_pop(Slice_Astid* slice) {
  return slice->base[--slice->length];
}
Astid slice_astid_top(Slice_Astid* slice) {
  return slice->base[slice->length-1];
}
B8 slice_astid_is_empty(Slice_Astid* slice) {
  return slice->length == 0;
}

Cstr cstr_from_slice_ast(Slice_Ast* slice) {
  String_Builder sb = string_builder_begin(&temp_arena, 10 * slice->length * sizeof(C8));
  for (S32 i = 0; i < slice->length; i++) {
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
    case Ast_Kind_array_open:
      string_builder_push_cstr(&sb, "array[");
    break;
    case Ast_Kind_array_close:
      string_builder_push_cstr(&sb, "array]");
    break;
    case Ast_Kind_subscript_open:
      string_builder_push_cstr(&sb, "subscript[");
    break;
    case Ast_Kind_subscript_close:
      string_builder_push_cstr(&sb, "subscript]");
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
      string_builder_push_s64(&sb, slice->base[i].s64);
    break;
    case Ast_Kind_block_leave:
      string_builder_push_cstr(&sb, "}");
    break;
    case Ast_Kind_paren_open:
      string_builder_push_cstr(&sb, "(");
      string_builder_push_s64(&sb, slice->base[i].s64);
    break;
    case Ast_Kind_paren_close:
      string_builder_push_cstr(&sb, ")");
    break;
    case Ast_Kind_record_open:
      string_builder_push_cstr(&sb, "r(");
      string_builder_push_s64(&sb, slice->base[i].s64);
    break;
    case Ast_Kind_record_close:
      string_builder_push_cstr(&sb, ")r");
    break;
    case Ast_Kind_call:
      string_builder_push_cstr(&sb, "call");
    break;
    case Ast_Kind_int:
      string_builder_push_s64(&sb, slice->base[i].s64);
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
  Cstr result = string_builder_end(&sb);
  return result;
}

void slice_ast_print(Slice_Ast* slice) {
  printf("%s\n", cstr_from_slice_ast(slice));
}

typedef enum {
  Parse_State_expression,
  Parse_State_infix_or_suffix,
} Parse_State;

typedef struct {
  Parse_State state;
  Slice_Astid len_stack;
  Slice_S32 pos_stack;
  Slice_Ast ast_stack;
  Slice_Ast ast_final;
  Slice_Token tokens;
  S32  tok;
  Cstr source;
  Cstr path;
} Parser;

Parser parser = {0};

void parse_ast_set_value(Slice_Ast slice, Astid astid, S32 value) {
  slice.base[astid.index].value = value;
}
void parse_transfer_all(Slice_Ast* from, Slice_Ast* to) {
  for (S32 i = 0; i < from->length; i++) {
    Ast ast = from->base[i];
    slice_ast_push(to, ast);
  }
  from->length = 0;
}

void parse_ast_stack_push_unary(void) {
  Token token = slice_token_at(&parser.tokens, parser.tok);
  Token_Kind kind = token.kind & 0xff;
  Ast ast = { .kind = kind | Ast_Flag_unary, .value = token.value };
  slice_ast_push(&parser.ast_stack, ast);
}

B8 parse_ast_stack_is_empty(void) {
  return slice_ast_is_empty(&parser.ast_stack);
}

B8 parse_ast_stack_is_top(Ast_Kind kind) {
  Ast ast = slice_ast_top(&parser.ast_stack);
  return ast.kind == (Ast_Kind)kind;
}

Ast parse_ast_stack_top() {
  Ast ast = slice_ast_top(&parser.ast_stack);
  return ast;
}

void parse_ast_stack_push(Ast_Kind kind, S32 value) {
  Ast ast = { .kind = kind, .value = value };
  slice_ast_push(&parser.ast_stack, ast);
}

Ast parse_ast_stack_pop(void) {
  return slice_ast_pop(&parser.ast_stack);
}

Astid parse_ast_final_push(Ast_Kind kind, S32 value) {
  Ast ast = { .kind = kind, .value = value };
  S32 index = slice_ast_push(&parser.ast_final, ast);
  Astid astid = { index };
  return astid;
}

Ast parse_ast_final_pop(void) {
  return slice_ast_pop(&parser.ast_final);
}

void parse_transfer_one(void) {
  Ast ast = parse_ast_stack_pop();
  parse_ast_final_push(ast.kind, ast.value);
}

S32 parse_left_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_array_open:
  case Ast_Kind_subscript_open:
                       return 0;
  case Ast_Kind_sub:
  case Ast_Kind_add: return 11;
  case Ast_Kind_mul: return 13;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:  return 15;
  case Ast_Kind_array_close:
  case Ast_Kind_load: return 17;
  case Ast_Kind_subscript_close:
  case Ast_Kind_call: return 19;
  default :          return -1;
  }
}

S32 parse_right_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_sub:
  case Ast_Kind_add: return 12;
  case Ast_Kind_mul: return 14;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:  return 16;
  case Ast_Kind_array_open:
  case Ast_Kind_load: return 18;
  case Ast_Kind_subscript_open:
  case Ast_Kind_call: return 20;
  default :          return -1;
  }
}

B8 parse_ast_stack_is_top_higher_precedence(Ast_Kind kind) {
  if (slice_ast_is_empty(&parser.ast_stack)) return false;
  Ast stack_ast = slice_ast_top(&parser.ast_stack);
  S32 on_stack_precedence  = parse_left_precedence(stack_ast.kind);
  S32 on_stream_precedence = parse_right_precedence(kind);
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

B8 parse_is_current_token(Token_Kind kind) {
  return parser.tokens.base[parser.tok].kind == kind;
}

B8 parse_current_token_is_flag(Token_Flag flag) {
  Token token = parse_current_token();
  return (token.kind & flag) != 0;
}

void parse_consume_token(void) {
  parser.tok++;
}

B8 parse_is_token_separates_expression(void) {
  Token token = parse_current_token();
  return (token.kind & Token_Kind_Flag_separates) != 0;
}

Parse_State parse_state(void) {
  return parser.state;
}

void parse_start_expression(void) {
  S32 pos = parser.ast_final.length;
  slice_s32_push(&parser.pos_stack, pos);
}
void parse_end_expression(void) {
  S32 rhs_pos = slice_s32_pop(&parser.pos_stack);
  while (!parse_ast_stack_is_empty()) {
    Ast ast = parse_ast_stack_pop();
    if (ast.kind == Ast_Kind_assign) {
      S32 lhs_pos = slice_s32_pop(&parser.pos_stack);
      for (S32 i = rhs_pos; i < parser.ast_final.length; i++) {
        slice_ast_push(&parser.ast_stack, parser.ast_final.base[i]);
      }
      parser.ast_final.length = rhs_pos;
      slice_ast_push(&parser.ast_stack, ast);
      for (S32 i = lhs_pos; i < parser.ast_final.length; i++) {
        slice_ast_push(&parser.ast_stack, parser.ast_final.base[i]);
      }
      parser.ast_final.length = lhs_pos;
      parse_transfer_all(&parser.ast_stack, &parser.ast_final);
    }
    else {
      parse_ast_final_push(ast.kind, ast.value);
    }
  }
}

void parse_tokens(Slice_Token tokens) {
  S32 total_ast_slice_length = 2*tokens.length + 2;
  parser.len_stack.base = xmalloc(sizeof(S32) * total_ast_slice_length);
  parser.pos_stack.base = xmalloc(sizeof(S32) * total_ast_slice_length);
  parser.ast_stack.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.ast_final.base = xmalloc(sizeof(Ast) * total_ast_slice_length);
  parser.tokens = tokens;
  parser.tok = 0;
  Astid block_enter = parse_ast_final_push(Ast_Kind_block_enter, 0);
  parser.state = Parse_State_expression;
  slice_astid_push(&parser.len_stack, block_enter);
  parse_start_expression();
  while (!parse_is_token_separates_expression()) {
    switch (parse_state()) {
    case Parse_State_expression: {
      Token token = parse_current_token();
      switch (token.kind) {
      case Token_Kind_minus_prefix: case Token_Kind_minus:
      case Token_Kind_plus_prefix:  case Token_Kind_plus:
      case Token_Kind_at_prefix:    case Token_Kind_at:
        parse_ast_stack_push_unary();
        parser.state = Parse_State_expression;
      break;
      case Token_Kind_int: case Token_Kind_name:
        parse_ast_final_push((Ast_Kind)token.kind, token.value);
        parser.state = Parse_State_infix_or_suffix;
      break;
      case Token_Kind_paren_open: {
        Ast_Kind kind = Ast_Kind_paren_open;
        Astid astid = parse_ast_final_push(kind, 0);
        slice_astid_push(&parser.len_stack, astid);
        parse_ast_stack_push(kind, 0);
        parser.state = Parse_State_expression;
        parse_start_expression();
      } break;
      case Token_Kind_paren_close: {
        Astid astid = slice_astid_pop(&parser.len_stack);
        while (!parse_ast_stack_is_top(Ast_Kind_paren_open)) {
          parse_transfer_one();
        }
        parse_ast_stack_pop(); // pops Ast_Kind_paren_open
        if (parser.ast_final.base[astid.index].kind == Ast_Kind_paren_open) {
          parse_ast_final_push(Ast_Kind_paren_close, 0);
        }
        else {
          parse_ast_final_push(Ast_Kind_record_close, 0);
        }
        parser.state = Parse_State_infix_or_suffix;
        parse_end_expression();
      } break;
      case Token_Kind_curly_open: {
        Ast_Kind kind = Ast_Kind_block_enter;
        Astid astid = parse_ast_final_push(kind, 0);
        slice_astid_push(&parser.len_stack, astid);
        parse_ast_final_push(kind, 0);
        parser.state = Parse_State_expression;
        parse_start_expression();
      } break;
      case Token_Kind_curly_close:
        while (!parse_ast_stack_is_top(Ast_Kind_block_enter)) {
          parse_transfer_one();
        }
        parse_ast_stack_pop(); // pops Ast_Kind_block_enter
        parse_ast_final_push(Ast_Kind_block_leave, 0);
        parse_end_expression();
        parser.state = Parse_State_infix_or_suffix;
      break;
      case Token_Kind_brace_open:
      case Token_Kind_brace_prefix_open: {
        Ast_Kind kind = Ast_Kind_array_open;
        parse_ast_stack_push(kind, 0);
        parser.state = Parse_State_expression;
        parse_start_expression();
      } break;
      case Token_Kind_brace_close:
        while (!parse_ast_stack_is_top(Ast_Kind_array_open)) {
          parse_transfer_one();
        }
        parse_ast_stack_push(Ast_Kind_array_close, 0);
        parse_end_expression();
        parser.state = Parse_State_infix_or_suffix;
      break;
      default: break;
      }
      parse_consume_token();
    } break;
    case Parse_State_infix_or_suffix: {
      Token token = parse_current_token();
      switch (token.kind) {
      case Token_Kind_plus:
      case Token_Kind_minus:
      case Token_Kind_star:
      case Token_Kind_brace_open: {
        parse_consume_token();
        Ast_Kind kind = (Ast_Kind)token.kind | Ast_Flag_binary;

        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_transfer_one();
        }
        parse_ast_stack_push(kind, 0);
        parser.state = Parse_State_expression;
      } break;
      case Token_Kind_at: {
        parse_consume_token();
        Ast_Kind kind = Ast_Kind_load;
        while (parse_ast_stack_is_top_higher_precedence(kind)) {
          parse_transfer_one();
        }
        parse_ast_final_push(kind, 0);
        parser.state = Parse_State_infix_or_suffix;
      } break;
      case Token_Kind_brace_close: {
        parse_consume_token();
        while (!parse_ast_stack_is_top(Ast_Kind_array_open) && !parse_ast_stack_is_top(Ast_Kind_subscript_open)) {
          parse_transfer_one();
        }
        Ast top = parse_ast_stack_top();
        if (top.kind == Ast_Kind_subscript_open) {
          parse_ast_stack_pop(); // pops Ast_Kind_paren_open
          parse_ast_final_push(Ast_Kind_subscript_close, 0);
          parser.state = Parse_State_infix_or_suffix;
        }
        else if (top.kind == Ast_Kind_array_open) {
          parse_ast_stack_pop(); // pops Ast_Kind_paren_open
          parse_ast_stack_push(Ast_Kind_array_close, 0);
          parser.state = Parse_State_expression;
        }
      } break;
      case Token_Kind_paren_close: {
        parse_consume_token();
        Astid astid = slice_astid_pop(&parser.len_stack);
        parser.ast_final.base[astid.index].s64++;
        while (!parse_ast_stack_is_top(Ast_Kind_paren_open)) {
          parse_transfer_one();
        }
        parse_ast_stack_pop(); // pops Ast_Kind_paren_open
        if (parser.ast_final.base[astid.index].kind == Ast_Kind_paren_open) {
          parse_ast_final_push(Ast_Kind_paren_close, 0);
        }
        else {
          parse_ast_final_push(Ast_Kind_record_close, 0);
        }
        parser.state = Parse_State_infix_or_suffix;
        slice_s32_pop(&parser.pos_stack);
      } break;
      case Token_Kind_curly_close:
        parse_consume_token();
        Astid astid = slice_astid_pop(&parser.len_stack);
        parser.ast_final.base[astid.index].s64++;
        while (!parse_ast_stack_is_top(Ast_Kind_block_enter)) {
          parse_transfer_one();
        }
        parse_ast_stack_pop(); // pops Ast_Kind_block_enter
        parse_ast_final_push(Ast_Kind_block_leave, 0);
        parser.state = Parse_State_infix_or_suffix;
        parse_end_expression();
      break;
      case Token_Kind_semicolon:
        parse_consume_token();
        if (!slice_astid_is_empty(&parser.len_stack)) {
          Astid top = slice_astid_top(&parser.len_stack);
          parser.ast_final.base[top.index].s64++;
          parser.ast_final.base[top.index].kind |= Ast_Flag_list;
        }
        parser.state = Parse_State_expression;
        parse_end_expression();
        parse_start_expression();
      break;
      case Token_Kind_equal: {
        parse_consume_token();
        while (parse_ast_stack_is_top_higher_precedence(Ast_Kind_assign)) {
          parse_transfer_one();
        }
        parse_ast_stack_push(Ast_Kind_assign, 0);
        parse_start_expression();
        parser.state = Parse_State_expression;
      } break;

      default: {
        if (token.flag & Token_Flag_call_rhs) {
          while (parse_ast_stack_is_top_higher_precedence(Ast_Kind_call)) {
            parse_transfer_one();
          }
          parse_ast_stack_push(Ast_Kind_call, 0);
          parser.state = Parse_State_expression;
        }
        else {
          if (!slice_astid_is_empty(&parser.len_stack)) {
            Astid top = slice_astid_top(&parser.len_stack);
            parser.ast_final.base[top.index].s64++;
            parser.ast_final.base[top.index].kind |= Ast_Flag_list;
          }
          parser.state = Parse_State_expression;
        }
      } break;
      }
    } break;
    }
  }
  parse_end_expression();
  if (parser.state == Parse_State_infix_or_suffix) {
    parser.ast_final.base[block_enter.index].s64++;
  }
  slice_astid_pop(&parser.len_stack);
  parse_ast_final_push(Ast_Kind_block_leave, 0);
}

void astid_from_source(Cstr source, Cstr path) {
  Umi source_len = strlen(source);
  istr_init(source_len+1);
  Slice_Token tokens = lex_source(source, path);
  printf("%s\n", cstr_from_slice_token(tokens));
  parse_tokens(tokens);
}

Cstr source_to_cstr_from_ast(Cstr source, Cstr name) {
  astid_from_source(source, name);
  return cstr_from_slice_ast(&parser.ast_final);
}

Cstr cstr_from_source_info(Cstr file_name, S32 line) {
  Umi len1 = strlen(file_name);
  C8  line_str[10];
  sprintf(line_str, "%i", line);
  Umi len2 = strlen(line_str);
  C8* buf = arena_alloc(&temp_arena, len1 + len2 + 2 + 1);
  strcat(buf, file_name);
  strcat(buf, "(");
  strcat(buf, line_str);
  strcat(buf, ")");
  return buf;
}

B8 _test_ast(Cstr expected, Cstr file_name, S32 line, Cstr source) {
  arena_init(&temp_arena, GB(1));
  Cstr resulted = source_to_cstr_from_ast(source, cstr_from_source_info(file_name, line));
  B8 result = test_at_source(resulted, expected, file_name, line, source);
  arena_free_all(&temp_arena);
  return result;
}

#define test(source, expected) _test_ast(expected, __FILE__, __LINE__, source)

void parse_test(void) {
  // t( a b c )t
  // posit: 1, 5
  // lengt: 1
  // stack: 
  // final: { t(3 a b + e c d * )t }
  test("a+b,e,c*d", "{}");
}

#undef test
