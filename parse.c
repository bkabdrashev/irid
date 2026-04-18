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
  Ast_Kind_load            = 128 | Ast_Flag_unary,
  Ast_Kind_subscript_open  = 129 | Ast_Flag_binary,
  Ast_Kind_subscript_close = 130 | Ast_Flag_binary,
  Ast_Kind_array_open      = 131 | Ast_Flag_binary,
  Ast_Kind_array_close     = 132 | Ast_Flag_binary,
  Ast_Kind_assign_lhs      = 133,
  Ast_Kind_assign_rhs      = 134,
  Ast_Kind_paren_open      = 135,
  Ast_Kind_paren_close     = 136,
  Ast_Kind_record_open     = 135 | Ast_Flag_list,
  Ast_Kind_record_close    = 136 | Ast_Flag_list,
  Ast_Kind_tuple_open      = 139 | Ast_Flag_list,
  Ast_Kind_tuple_close     = 140 | Ast_Flag_list,
  Ast_Kind_source_enter    = 141 | Ast_Flag_list,
  Ast_Kind_source_leave    = 142 | Ast_Flag_list,
  Ast_Kind_block_enter     = 143 | Ast_Flag_list,
  Ast_Kind_block_leave     = 144 | Ast_Flag_list,
  Ast_Kind_else            = 145,
  Ast_Kind_else_value      = 146 | Ast_Flag_value,
  Ast_Kind_statement       = 147,
  Ast_Kind_expression      = 148,
  Ast_Kind_infix_or_suffix = 149,
  Ast_Kind_call            = 150 | Ast_Flag_binary,
} Ast_Kind;

typedef struct {
  S32 index;
} Astid;

typedef struct {
  Ast_Kind kind;
  union {
    U64   value;
    struct {
      Astid open_at;
      Astid last_at;
    };
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

Cstr cstr_from_slice_ast(Slice_Ast* slice) {
  String_Builder sb = string_builder_begin(&temp_arena, 10 * slice->length * sizeof(C8));
  for (S32 i = 0; i < slice->length; i++) {
    switch (slice->base[i].kind) {
    case Ast_Kind_statement:
      string_builder_push_cstr(&sb, "stm");
    break;
    case Ast_Kind_expression:
      string_builder_push_cstr(&sb, "exp");
    break;
    case Ast_Kind_infix_or_suffix:
      string_builder_push_cstr(&sb, "exp2");
    break;
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
      string_builder_push_cstr(&sb, "a[");
    break;
    case Ast_Kind_array_close:
      string_builder_push_cstr(&sb, "]a");
    break;
    case Ast_Kind_subscript_open:
      string_builder_push_cstr(&sb, "s[");
    break;
    case Ast_Kind_subscript_close:
      string_builder_push_cstr(&sb, "]s");
    break;
    case Ast_Kind_ptr:
      string_builder_push_cstr(&sb, "ptr");
    break;
    case Ast_Kind_load:
      string_builder_push_cstr(&sb, "load");
    break;
    case Ast_Kind_assign_lhs:
      string_builder_push_cstr(&sb, "l=");
    break;
    case Ast_Kind_assign_rhs:
      string_builder_push_cstr(&sb, "r=");
    break;
    case Ast_Kind_block_enter:
      string_builder_push_cstr(&sb, "{");
      string_builder_push_cstr(&sb, ",");
      string_builder_push_s64(&sb, slice->base[i].close_at.index);
    break;
    case Ast_Kind_block_leave:
      string_builder_push_cstr(&sb, "}");
    break;
    case Ast_Kind_source_enter:
      string_builder_push_cstr(&sb, "s{");
      string_builder_push_cstr(&sb, ",");
      string_builder_push_s64(&sb, slice->base[i].close_at.index);
    break;
    case Ast_Kind_source_leave:
      string_builder_push_cstr(&sb, "}s");
    break;
    case Ast_Kind_paren_open:
      string_builder_push_cstr(&sb, "(,");
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
      string_builder_push_cstr(&sb, ",");
      string_builder_push_s64(&sb, slice->base[i].close_at.index);
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

typedef struct {
  Slice_Ast stack;
  Slice_Ast final;
  Slice_Token tokens;
  S32  tok;
  Cstr source;
  Cstr path;
} Parser;

Parser parser = {0};

B8 parse_stack_is_empty(void) {
  return slice_ast_is_empty(&parser.stack);
}

B8 parse_stack_is_top(Ast_Kind kind) {
  Ast* ast = slice_ast_top(&parser.stack);
  return ast->kind == (Ast_Kind)kind;
}

Ast* parse_stack_top() {
  Ast* ast = slice_ast_top(&parser.stack);
  return ast;
}

void parse_stack_push(Ast ast) {
  slice_ast_push(&parser.stack, ast);
}

Ast parse_stack_pop(void) {
  return slice_ast_pop(&parser.stack);
}

Astid parse_final_push(Ast ast) {
  S32 index = slice_ast_push(&parser.final, ast);
  Astid astid = { index };
  return astid;
}

Ast parse_final_pop(void) {
  return slice_ast_pop(&parser.final);
}

Ast* parse_final_top(void) {
  return slice_ast_top(&parser.final);
}

Ast* parse_final_at(Astid astid) {
  return &parser.final.base[astid.index];
}

void parse_transfer_one(void) {
  Ast ast = parse_stack_pop();
  parse_final_push(ast);
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

B8 parse_stack_is_top_higher_precedence(Ast_Kind kind) {
  if (slice_ast_is_empty(&parser.stack)) return false;
  Ast* stack_ast = slice_ast_top(&parser.stack);
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

Token parse_peek_token(void) {
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
  slice_ast_print(&parser.final);
  printf("ast stack: ");
  slice_ast_print(&parser.stack);
  printf("\n");
}

S32 parse_transfer_final_to_stack_from(S32 mark) {
  S32 result = parser.stack.length;
  for (S32 i = mark; i < parser.final.length; i++) {
    parse_stack_push(parser.final.base[i]);
  }
  parser.final.length = mark;
  return result;
}

S32 parse_transfer_stack_to_final_from(S32 mark) {
  S32 result = parser.final.length;
  for (S32 i = mark; i < parser.stack.length; i++) {
    parse_final_push(parser.stack.base[i]);
  }
  parser.stack.length = mark;
  return result;
}

void parse_tokens(Slice_Token tokens, Umi source_length) {
  S32 max_ast_length = source_length + 2;
  parser.stack.base = xmalloc(sizeof(Ast) * 2*max_ast_length);
  parser.final.base = xmalloc(sizeof(Ast) * 2*max_ast_length);
  parser.tokens = tokens;
  parser.tok = 0;
  Ast source_enter = { Ast_Kind_source_enter, {0} };
  parse_final_push(source_enter);
  Ast source_leave = { Ast_Kind_source_leave, {0} };
  parse_stack_push(source_leave);
  Ast statement = { Ast_Kind_statement, {0} };
  parse_stack_push(statement);
  while (!parse_stack_is_empty()) {
    Ast ast = parse_stack_pop();
    switch (ast.kind) {
    case Ast_Kind_statement: {
    label_statement:;
      Token token = parse_current_token();
      switch (token.kind) {
      // case Token_Kind_while:
      // case Token_Kind_for:
      // case Token_Kind_return:
      default: {
        Ast assign = { Ast_Kind_assign_lhs, { .last_at.index = parser.final.length } };
        parse_stack_push(assign);
      } goto label_expression;
      }
    } break;
    case Ast_Kind_expression: {
    label_expression:;
      Token token = parse_consume_token();
      switch (token.kind) {
      case Token_Kind_minus_prefix: case Token_Kind_minus:
      case Token_Kind_plus_prefix:  case Token_Kind_plus:
      case Token_Kind_at_prefix:    case Token_Kind_at: {
        Ast_Kind kind = (Ast_Kind)token.kind & 0xff;
        Ast ast = { .kind = kind | Ast_Flag_unary, .value = token.value };
        slice_ast_push(&parser.stack, ast);
      } goto label_expression;
      case Token_Kind_int: case Token_Kind_name: {
        Ast ast = { .kind = (Ast_Kind)token.kind, .value = token.value };
        parse_final_push(ast);
      } goto label_infix_or_suffix;
      case Token_Kind_paren_open: {
        Ast paren_open = { Ast_Kind_paren_open, {0} };
        Astid astid_open = parse_final_push(paren_open);
        Ast paren_close = { .kind = Ast_Kind_paren_close, .open_at = astid_open, .last_at.index = parser.final.length };
        parse_stack_push(paren_close);
      } goto label_expression;
      case Token_Kind_curly_open: {
        Ast   enter = { Ast_Kind_block_enter, {0} };
        Astid astid_enter = parse_final_push(enter);
        Ast   leave = { Ast_Kind_block_leave, .open_at = astid_enter };
        parse_stack_push(leave);
      } goto label_statement;
      case Token_Kind_brace_open:
      case Token_Kind_brace_prefix_open: {
        Ast ast = { Ast_Kind_array_open, { .last_at.index = parser.final.length } };
        parse_stack_push(ast);
      } goto label_expression;
      default:
        parse_unconsume_token();
      break;
      }
    } break;
    case Ast_Kind_infix_or_suffix: {
    label_infix_or_suffix:;
      Token token = parse_consume_token();
      switch (token.kind) {
      case Token_Kind_plus: case Token_Kind_minus:
      case Token_Kind_star: {
        Ast ast = { (Ast_Kind)token.kind | Ast_Flag_binary, {0} };
        while (parse_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_stack_push(ast);
      } goto label_expression;
      case Token_Kind_brace_open: {
        Ast ast = { Ast_Kind_subscript_open, { .last_at.index = parser.final.length } };
        while (parse_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_stack_push(ast);
      } goto label_expression;
      case Token_Kind_at: {
        Ast ast = { Ast_Kind_load, {0} };
        while (parse_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_final_push(ast);
      } goto label_infix_or_suffix;
      case Token_Kind_comma: {
        Ast ast = { Ast_Kind_tuple_open, {0} };
        while (parse_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        Ast* top = parse_stack_top();
        if (top->kind == Ast_Kind_assign_lhs || top->kind == Ast_Kind_assign_rhs ||
            top->kind == Ast_Kind_paren_close || top->kind == Ast_Kind_record_close ||
            top->kind == Ast_Kind_array_open || top->kind == Ast_Kind_subscript_open) {
          S32 stack_mark_length = parse_transfer_final_to_stack_from(top->last_at.index);
          Ast tuple_open = { Ast_Kind_tuple_open, {.length = 1} };
          Astid tuple_open_astid = parse_final_push(tuple_open);
          parse_transfer_stack_to_final_from(stack_mark_length);
          Ast tuple_close = { Ast_Kind_tuple_close, .open_at = tuple_open_astid };
          parse_stack_push(tuple_close);
        }
        else {
          Ast* open = parse_final_at(top->open_at);
          open->close_at.index = parser.final.length;
          open->length++;
        }
      } goto label_expression;
      default: {
        parse_unconsume_token();
        if (token.flag & Token_Flag_call_rhs) {
          Ast ast = { Ast_Kind_call, {0} };
          while (parse_stack_is_top_higher_precedence(ast.kind)) {
            parse_transfer_one();
          }
          parse_stack_push(ast);
          goto label_expression;
        }
      } break;
      }
    } break;
    case Ast_Kind_assign_lhs: {
      if (parse_match_token(Token_Kind_equal)) {
        S32 final_mark = ast.last_at.index;
        parse_final_push(ast);
        S32 stack_mark_length = parse_transfer_final_to_stack_from(final_mark);
        Ast assign = { Ast_Kind_assign_rhs, { .last_at.index = stack_mark_length } };
        parse_stack_push(assign);
        goto label_expression;
      }
    } break;
    case Ast_Kind_assign_rhs: {
      parse_final_push(ast);
      parse_transfer_stack_to_final_from(ast.last_at.index);
    } break;
    case Ast_Kind_block_leave: {
      Ast ast_close = ast;
      if (parse_match_token(Token_Kind_curly_close)) {
        Ast* open = parse_final_at(ast_close.open_at);
        Astid astid_close = parse_final_push(ast_close);
        open->close_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        parse_match_token(Token_Kind_semicolon);
        parse_stack_push(ast_close);
        goto label_statement;
      }
    } break;
    case Ast_Kind_source_leave: {
      Ast ast_close = ast;
      if (parse_match_token(Token_Kind_eof)) {
        Ast* open = parse_final_at(ast_close.open_at);
        Astid astid_close = parse_final_push(ast_close);
        open->close_at = astid_close;
      }
      else {
        parse_match_token(Token_Kind_semicolon);
        parse_stack_push(ast_close);
        goto label_statement;
      }
    } break;
    case Ast_Kind_tuple_close: {
      Ast ast_close = ast;
      Ast* open = parse_final_at(ast_close.open_at);
      Astid astid_close = parse_final_push(ast_close);
      if (astid_close.index != open->close_at.index) {
        open->length++;
      }
      open->close_at = astid_close;
    } break;
    case Ast_Kind_record_close: {
      parse_match_token(Token_Kind_semicolon);
      Ast ast_close = ast;
      Ast* open = parse_final_at(ast_close.open_at);
      if (parse_match_token(Token_Kind_paren_close)) {
        open->length++;
        Astid astid_close = parse_final_push(ast_close);
        open->close_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        ast_close.last_at.index = parser.final.length;
        open->length++;
        parse_stack_push(ast_close);
        goto label_expression;
      }
    } break;
    case Ast_Kind_paren_close: {
      Ast ast_close = ast;
      Ast* open = parse_final_at(ast_close.open_at);
      if (parse_match_token(Token_Kind_paren_close)) {
        if (parser.final.length == ast_close.open_at.index) {
          // empty record ()
          open->kind |= Ast_Flag_list;
          ast_close.kind |= Ast_Flag_list;
        }
        Astid astid_close = parse_final_push(ast_close);
        open->close_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        open->kind |= Ast_Flag_list;
        ast_close.kind |= Ast_Flag_list;
        ast_close.last_at.index = parser.final.length;
        parse_stack_push(ast_close);
        goto label_expression;
      }
    } break;
    case Ast_Kind_array_open: {
      parse_expect_token(Token_Kind_brace_close);
      Ast ast = { Ast_Kind_array_close, {0} };
      parse_stack_push(ast);
    } goto label_expression;
    case Ast_Kind_subscript_open: {
      parse_expect_token(Token_Kind_brace_close);
      Ast ast = { Ast_Kind_subscript_close, {0} };
      parse_stack_push(ast);
    } goto label_infix_or_suffix;
    default:
      parse_final_push(ast);
    break;
    }
  }
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
  return cstr_from_slice_ast(&parser.final);
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
  test("(-a, b)", "{}");
}

#undef test
