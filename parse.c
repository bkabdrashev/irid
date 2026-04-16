typedef enum {
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
  Ast_Flag_list   = 1 << 10,
  Ast_Flag_value  = 1 << 11,
} Ast_Flag;

typedef enum {
  Ast_Mask_remove_value  = ~Ast_Flag_value,
} Ast_Mask;

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
  Ast_Kind_record_open     = Token_Kind_paren_open  | Ast_Flag_list,
  Ast_Kind_record_close    = Token_Kind_paren_close | Ast_Flag_list,
  Ast_Kind_tuple_open      = Token_Kind_comma       | Ast_Flag_list,
  Ast_Kind_tuple_close     = (Token_Kind_comma+1)   | Ast_Flag_list,
  Ast_Kind_call = 123 | Ast_Flag_binary,
  Ast_Kind_block_enter = Token_Kind_curly_open  | Ast_Flag_list,
  Ast_Kind_block_leave = Token_Kind_curly_close | Ast_Flag_list,

  Ast_Kind_else       = Token_Kind_else,
  Ast_Kind_else_value = Token_Kind_else | Ast_Flag_value,
} Ast_Kind;

typedef struct {
  S32 index;
} Astid;

typedef struct {
  Ast_Kind kind;
  union {
    U64   value;
    Astid open_at;
    struct {
      Astid close_at;
      S32 length;
    };
    S64   s64;
    Istr  istr;
  };
} Ast; 

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
Ast* slice_ast_top(Slice_Ast* slice) {
  return &slice->base[slice->length-1];
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
      string_builder_push_s64(&sb, slice->base[i].length);
    break;
    case Ast_Kind_block_leave:
      string_builder_push_cstr(&sb, "}");
    break;
    case Ast_Kind_paren_open:
      string_builder_push_cstr(&sb, "(");
      string_builder_push_s64(&sb, slice->base[i].close_at.index);
    break;
    case Ast_Kind_paren_close:
      string_builder_push_cstr(&sb, ")");
    break;
    case Ast_Kind_record_open:
      string_builder_push_cstr(&sb, "r(");
      string_builder_push_s64(&sb, slice->base[i].length);
      string_builder_push_cstr(&sb, ",");
      string_builder_push_s64(&sb, slice->base[i].close_at.index);
    break;
    case Ast_Kind_record_close:
      string_builder_push_cstr(&sb, ")r");
    break;
    case Ast_Kind_tuple_open:
      string_builder_push_cstr(&sb, "t(");
      string_builder_push_s64(&sb, slice->base[i].length);
    break;
    case Ast_Kind_tuple_close:
      string_builder_push_cstr(&sb, ")t");
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
    case Ast_Kind_else:
      string_builder_push_cstr(&sb, "else");
    break;
    case Ast_Kind_else_value:
      string_builder_push_cstr(&sb, "elsev");
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
  Parse_Kind_statement,
  Parse_Kind_expression,
  Parse_Kind_infix_or_suffix,
  Parse_Kind_assign,
  Parse_Kind_paren_close,
  Parse_Kind_array_close,
  Parse_Kind_subscript_close,
  Parse_Kind_curly_close,
} Parse_Kind;

typedef struct {
  Parse_Kind kind;
  S32        value;
} Parse_State;

typedef struct {
  Parse_State* base;
  S32  length;
} Slice_Parse_State;

S32 slice_parse_state_push(Slice_Parse_State* slice, Parse_State item) {
  S32 index = slice->length;
  slice->base[slice->length++] = item;
  return index;
}
Parse_State slice_parse_state_pop(Slice_Parse_State* slice) {
  return slice->base[--slice->length];
}
Parse_State* slice_parse_state_top(Slice_Parse_State* slice) {
  return &slice->base[slice->length-1];
}
B8 slice_parse_state_is_empty(Slice_Parse_State* slice) {
  return slice->length == 0;
}

typedef struct {
  Slice_Parse_State state_stack;
  Slice_Astid len_stack;
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
  Ast* ast = slice_ast_top(&parser.ast_stack);
  return ast->kind == (Ast_Kind)kind;
}

Ast* parse_ast_stack_top() {
  Ast* ast = slice_ast_top(&parser.ast_stack);
  return ast;
}

void parse_ast_stack_push(Ast ast) {
  slice_ast_push(&parser.ast_stack, ast);
}

Ast parse_ast_stack_pop(void) {
  return slice_ast_pop(&parser.ast_stack);
}

Astid parse_ast_final_push(Ast ast) {
  S32 index = slice_ast_push(&parser.ast_final, ast);
  Astid astid = { index };
  return astid;
}

Ast parse_ast_final_pop(void) {
  return slice_ast_pop(&parser.ast_final);
}

Ast* parse_ast_final_top(void) {
  return slice_ast_top(&parser.ast_final);
}

Ast* parse_ast_final_at(Astid astid) {
  return &parser.ast_final.base[astid.index];
}

Parse_State  parse_state_stack_pop() {
  return slice_parse_state_pop(&parser.state_stack);
}
Parse_State* parse_state_stack_top() {
  return slice_parse_state_top(&parser.state_stack);
}
S32 parse_state_stack_push(Parse_Kind kind, S32 value) {
  Parse_State state = { .kind = kind, .value = value };
  return slice_parse_state_push(&parser.state_stack, state);
}

void parse_transfer_one(void) {
  Ast ast = parse_ast_stack_pop();
  parse_ast_final_push(ast);
}

S32 parse_left_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_array_open:
  case Ast_Kind_subscript_open:
                       return 0;
  case Ast_Kind_tuple_open:
  case Ast_Kind_tuple_close:
                       return 3;
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
  case Ast_Kind_tuple_open:
  case Ast_Kind_tuple_close:
                       return 4;
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
  Ast* stack_ast = slice_ast_top(&parser.ast_stack);
  S32 on_stack_precedence  = parse_left_precedence(stack_ast->kind);
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

Token parse_consume_token(void) {
  Token token = parse_current_token();
  parser.tok++;
  return token;
}

void parse_unconsume_token(void) {
  parser.tok--;
}

B8 parse_match_token(Token_Kind kind) {
  Token token = parse_current_token();
  if (token.kind == kind) {
    parser.tok++;
    return true;
  }
  return false;
}

B8 parse_expect_token(Token_Kind kind) {
  Token token = parse_current_token();
  if (token.kind == kind) {
    parser.tok++;
    return true;
  }
  else {
  }
  return false;
}

void parse_print(void) {
  printf("\nast final: ");
  slice_ast_print(&parser.ast_final);
  printf("ast stack: ");
  slice_ast_print(&parser.ast_stack);
  printf("len stack: ");
  for (S32 i = 0; i < parser.len_stack.length; i++) {
    printf("%i ", parser.len_stack.base[i].index);
  }
  printf("\n");
}

void parse_end_expression(void) {
  while (!parse_ast_stack_is_empty()) {
    Ast ast = parse_ast_stack_pop();
    parse_ast_final_push(ast);
  }
}

void parse_tokens(Slice_Token tokens, Umi source_length) {
  S32 max_ast_length = source_length + 2;
  parser.len_stack.base = xmalloc(sizeof(S32) * max_ast_length);
  parser.state_stack.base = xmalloc(sizeof(Parse_State) * max_ast_length);
  parser.ast_stack.base = xmalloc(sizeof(Ast) * max_ast_length);
  parser.ast_final.base = xmalloc(sizeof(Ast) * max_ast_length);
  parser.tokens = tokens;
  parser.tok = 0;
  Ast block_enter = { Ast_Kind_block_enter, {0} };
  Astid astid_block_enter = parse_ast_final_push(block_enter);
  parse_state_stack_push(Parse_Kind_statement, 0);
  slice_astid_push(&parser.len_stack, astid_block_enter);
  while (!parse_is_current_token(Token_Kind_eof)) {
    Parse_State state = parse_state_stack_pop();
    switch (state.kind) {
    case Parse_Kind_statement: {
      Token token = parse_current_token();
      switch (token.kind) {
      // case Token_Kind_while:
      // case Token_Kind_for:
      // case Token_Kind_for:
      // case Token_Kind_return:
      default:
        parse_state_stack_push(Parse_Kind_assign, 0);
        parse_state_stack_push(Parse_Kind_expression, 0);
      }
    } break;
    case Parse_Kind_assign: {
      Ast* top = parse_ast_final_top();
      top->kind &= Ast_Mask_remove_value;
      Token token = parse_current_token();
      switch (token.kind) {
      case Token_Kind_equal:
      default: parse_state_stack_push(Parse_Kind_expression, 0);
      }
    } break;
    case Parse_Kind_expression: {
      Token token = parse_current_token();
      parse_consume_token();
      switch (token.kind) {
      case Token_Kind_minus_prefix: case Token_Kind_minus:
      case Token_Kind_plus_prefix:  case Token_Kind_plus:
      case Token_Kind_at_prefix:    case Token_Kind_at:
        parse_ast_stack_push_unary();
        parse_state_stack_push(Parse_Kind_expression, 0);
      break;
      case Token_Kind_int: case Token_Kind_name: {
        Ast ast = { .kind = (Ast_Kind)token.kind, .value = token.value };
        parse_ast_final_push(ast);
        parse_state_stack_push(Parse_Kind_infix_or_suffix, 0);
      } break;
      case Token_Kind_paren_open: {
        Ast paren_open = { Ast_Kind_paren_open, {0} };
        Astid astid_open = parse_ast_final_push(paren_open);
        slice_astid_push(&parser.len_stack, astid_open);
        Ast paren_close = { .kind = Ast_Kind_paren_close, .open_at = astid_open };
        parse_ast_stack_push(paren_close);
        parse_state_stack_push(Parse_Kind_paren_close, 0);
        parse_state_stack_push(Parse_Kind_expression, 0);
      } break;
      case Token_Kind_curly_open: {
        Ast   enter = { Ast_Kind_block_enter, {0} };
        Astid astid_enter = parse_ast_final_push(enter);
        slice_astid_push(&parser.len_stack, astid_enter);
        Ast   leave = { Ast_Kind_block_leave, .open_at = astid_enter };
        parse_ast_stack_push(leave);
        parse_state_stack_push(Parse_Kind_curly_close, 0);
        parse_state_stack_push(Parse_Kind_expression, 0);
      } break;
      case Token_Kind_brace_open:
      case Token_Kind_brace_prefix_open: {
        Ast ast = { Ast_Kind_array_open, {0} };
        parse_ast_stack_push(ast);
        parse_state_stack_push(Parse_Kind_array_close, 0);
        parse_state_stack_push(Parse_Kind_expression, 0);
      } break;
      default:
        parse_unconsume_token();
      break;
      }
    } break;
    case Parse_Kind_infix_or_suffix: {
      Token token = parse_current_token();
      parse_consume_token();
      switch (token.kind) {
      case Token_Kind_plus: case Token_Kind_minus:
      case Token_Kind_star: {
        Ast ast = { (Ast_Kind)token.kind | Ast_Flag_binary, {0} };
        while (parse_ast_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_ast_stack_push(ast);
        parse_state_stack_push(Parse_Kind_expression, 0);
      } break;
      case Token_Kind_brace_open: {
        Ast ast = { Ast_Kind_subscript_open, {0} };
        while (parse_ast_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_ast_stack_push(ast);
        parse_state_stack_push(Parse_Kind_subscript_close, 0);
        parse_state_stack_push(Parse_Kind_expression, 0);
      } break;
      case Token_Kind_at: {
        Ast ast = { Ast_Kind_load, {0} };
        while (parse_ast_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_ast_final_push(ast);
        parse_state_stack_push(Parse_Kind_infix_or_suffix, 0);
      } break;
      case Token_Kind_semicolon: {
        while (!parse_ast_stack_is_top(Ast_Kind_paren_close)
           &&  !parse_ast_stack_is_top(Ast_Kind_record_close)
           &&  !parse_ast_stack_is_top(Ast_Kind_tuple_close)
           &&  !parse_ast_stack_is_top(Ast_Kind_block_leave)) {
          parse_transfer_one();
        }
        Ast* close = parse_ast_stack_top();
        Ast* open = parse_ast_final_at(close->open_at);
        close->kind |= Ast_Flag_list;
        open->kind |= Ast_Flag_list;
        open->length++;
        parse_state_stack_push(Parse_Kind_expression, 0);
      } break;
      default: {
        parse_unconsume_token();
        if (token.flag & Token_Flag_call_rhs) {
          Ast ast = { Ast_Kind_call, {0} };
          while (parse_ast_stack_is_top_higher_precedence(ast.kind)) {
            parse_transfer_one();
          }
          parse_ast_stack_push(ast);
          parse_state_stack_push(Parse_Kind_expression, 0);
        }
        else {
          parse_state_stack_push(Parse_Kind_expression, 0);
        }
      } break;
      }
    } break;
    case Parse_Kind_paren_close: {
      parse_expect_token(Token_Kind_paren_close);
      while (!parse_ast_stack_is_top(Ast_Kind_paren_close)
         &&  !parse_ast_stack_is_top(Ast_Kind_record_close)) {
        parse_transfer_one();
      }
      Ast ast_close = parse_ast_stack_pop();
      Ast* open = parse_ast_final_at(ast_close.open_at);
      if (parser.ast_final.length == ast_close.open_at.index) {
        // empty record ()
        open->kind |= Ast_Flag_list;
        ast_close.kind |= Ast_Flag_list;
      }
      else {
        open->length++;
      }
      Astid astid_close = parse_ast_final_push(ast_close);
      open->close_at = astid_close;
      parse_state_stack_push(Parse_Kind_infix_or_suffix, 0);
    } break;
    case Parse_Kind_array_close: {
      parse_expect_token(Token_Kind_brace_close);
      parse_state_stack_push(Parse_Kind_expression, 0);
    } break;
    case Parse_Kind_subscript_close: {
      parse_expect_token(Token_Kind_brace_close);
      while (!parse_ast_stack_is_top(Ast_Kind_subscript_open)) {
        parse_transfer_one();
      }
      parse_ast_stack_pop();
      parse_state_stack_push(Parse_Kind_infix_or_suffix, 0);
    } break;
    case Parse_Kind_curly_close: {
      parse_expect_token(Token_Kind_curly_close);
      while (!parse_ast_stack_is_top(Ast_Kind_block_leave)) {
        parse_transfer_one();
      }
      Ast ast_close = parse_ast_stack_pop();
      Ast* open = parse_ast_final_at(ast_close.open_at);
      if (parser.ast_final.length != ast_close.open_at.index) {
    // BUG: one off bug {a;}
        open->length++;
      }
      Astid astid_close = parse_ast_final_push(ast_close);
      open->close_at = astid_close;
      parse_state_stack_push(Parse_Kind_infix_or_suffix, 0);
    } break;
    }
  }
  parse_end_expression();
  slice_astid_pop(&parser.len_stack);
  Ast leave = { Ast_Kind_block_leave, {0} };
  parse_ast_final_push(leave);
}

void astid_from_source(Cstr source, Cstr path) {
  Umi source_len = strlen(source);
  istr_init(source_len+1);
  Slice_Token tokens = lex_source(source, path);
  printf("%s\n", cstr_from_slice_token(tokens));
  parse_tokens(tokens, source_len);
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
  test("(a+b;c*d)", "{}");
}

#undef test
