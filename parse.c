typedef enum {
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
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
  Ast_Kind_access = Token_Kind_dot | Ast_Flag_binary,
  Ast_Kind_load            = 128 | Ast_Flag_unary,
  Ast_Kind_subscript_close = 129,
  Ast_Kind_subscript       = 130 | Ast_Flag_binary,
  Ast_Kind_array_close     = 131,
  Ast_Kind_array           = 132 | Ast_Flag_binary,
  Ast_Kind_assign_lhs      = 133,
  Ast_Kind_assign_rhs      = 134,
  Ast_Kind_record_assign   = 135,
  Ast_Kind_declare_lhs     = 136,
  Ast_Kind_declare_rhs     = 137,
  Ast_Kind_record_declare  = 138,
  Ast_Kind_paren_enter     = 139,
  Ast_Kind_paren_leave     = 140,
  Ast_Kind_record_enter    = 141,
  Ast_Kind_record_leave    = 142,
  Ast_Kind_tuple_enter     = 143,
  Ast_Kind_tuple_leave     = 144,
  Ast_Kind_source_enter    = 145,
  Ast_Kind_source_leave    = 146,
  Ast_Kind_block_enter     = 147,
  Ast_Kind_block_leave     = 148,
  Ast_Kind_block_value_enter = 149,
  Ast_Kind_block_value_leave = 150,
  Ast_Kind_statement       = 151,
  Ast_Kind_expression      = 152,
  Ast_Kind_infix_or_suffix = 153,
  Ast_Kind_call            = 154 | Ast_Flag_binary,
  Ast_Kind_if_enter            = 155,
  Ast_Kind_if_leave            = 156,
  Ast_Kind_if_do               = 157,
  Ast_Kind_if_else             = 158,
  Ast_Kind_if_leave_else_enter = 159,
  Ast_Kind_else_leave          = 160,
  Ast_Kind_if_value_do         = 161,
  Ast_Kind_if_value_else       = 162,
  Ast_Kind_else_value_leave    = 163,
  Ast_Kind_fun_enter           = 164,
  Ast_Kind_fun_leave           = 165,
  Ast_Kind_return              = 166,
  Ast_Kind_while_enter         = 167,
  Ast_Kind_while_leave         = 168,
  Ast_Kind_while_do            = 169,
  Ast_Kind_break               = 172,
} Ast_Kind;

typedef struct {
  I32 index;
} Astid;

typedef struct {
  Ast_Kind kind;
  union {
    U64   value;
    struct {
      Astid enter_at;
      Astid last_at;
    };
    struct {
      Astid leave_at;
      I32 length;
    };
    I64   s64;
    Istr  istr;
  };
} Ast; 

typedef struct {
  Ast* base;
  I32  length;
} Slice_Ast;

I32 slice_ast_push(Slice_Ast* slice, Ast item) {
  I32 index = slice->length;
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
  for (I32 i = 0; i < slice->length; i++) {
    Ast ast = slice->base[i];
    switch (ast.kind) {
    case Ast_Kind_statement:
      string_builder_push_cstr(&sb, "stm");
    break;
    case Ast_Kind_expression:
      string_builder_push_cstr(&sb, "exp");
    break;
    case Ast_Kind_infix_or_suffix:
      string_builder_push_cstr(&sb, "infix");
    break;
    case Ast_Kind_return:
      string_builder_push_cstr(&sb, "return");
    break;
    case Ast_Kind_while_enter:
      string_builder_push_cstr(&sb, "while(");
    break;
    case Ast_Kind_while_leave:
      string_builder_push_cstr(&sb, ")w");
    break;
    case Ast_Kind_while_do:
      string_builder_push_cstr(&sb, "while_do");
    break;
    case Ast_Kind_break:
      string_builder_push_cstr(&sb, "break");
    break;
    case Ast_Kind_if_enter:
      string_builder_push_cstr(&sb, "if(");
    break;
    case Ast_Kind_if_leave:
      string_builder_push_cstr(&sb, ")fi");
    break;
    case Ast_Kind_if_do:
      string_builder_push_cstr(&sb, "if_do");
    break;
    case Ast_Kind_if_value_do:
      string_builder_push_cstr(&sb, "if_v_do");
    break;
    case Ast_Kind_if_else:
      string_builder_push_cstr(&sb, "if_else");
    break;
    case Ast_Kind_if_value_else:
      string_builder_push_cstr(&sb, "v_if_else");
    break;
    case Ast_Kind_if_leave_else_enter:
      string_builder_push_cstr(&sb, "else(");
    break;
    case Ast_Kind_else_value_leave:
      string_builder_push_cstr(&sb, ")ev");
    break;
    case Ast_Kind_else_leave:
      string_builder_push_cstr(&sb, ")el");
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
    case Ast_Kind_access:
      string_builder_push_cstr(&sb, "access");
    break;
    case Ast_Kind_fun_enter:
      string_builder_push_cstr(&sb, "fun(");
    break;
    case Ast_Kind_fun_leave:
      string_builder_push_cstr(&sb, ")fun");
    break;
    case Ast_Kind_eof:
      string_builder_push_cstr(&sb, "eof");
    break;
    case Ast_Kind_array_close:
      string_builder_push_cstr(&sb, "]a");
    break;
    case Ast_Kind_array:
      string_builder_push_cstr(&sb, "a[]");
    break;
    case Ast_Kind_subscript_close:
      string_builder_push_cstr(&sb, "]s");
    break;
    case Ast_Kind_subscript:
      string_builder_push_cstr(&sb, "s[]");
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
      string_builder_push_cstr(&sb, "=");
    break;
    case Ast_Kind_record_assign:
      string_builder_push_cstr(&sb, "r=");
    break;
    case Ast_Kind_declare_lhs:
      string_builder_push_cstr(&sb, "l:");
    break;
    case Ast_Kind_declare_rhs:
      string_builder_push_cstr(&sb, ":");
    break;
    case Ast_Kind_record_declare:
      string_builder_push_cstr(&sb, "r:");
    break;
    case Ast_Kind_block_enter:
      string_builder_push_cstr(&sb, "{");
    break;
    case Ast_Kind_block_leave:
      string_builder_push_cstr(&sb, "}");
    break;
    case Ast_Kind_block_value_enter:
      string_builder_push_cstr(&sb, "v{");
    break;
    case Ast_Kind_block_value_leave:
      string_builder_push_cstr(&sb, "}v");
    break;
    case Ast_Kind_source_enter:
      string_builder_push_cstr(&sb, "s{");
    break;
    case Ast_Kind_source_leave:
      string_builder_push_cstr(&sb, "}s");
    break;
    case Ast_Kind_paren_enter:
      string_builder_push_cstr(&sb, "(");
    break;
    case Ast_Kind_paren_leave:
      string_builder_push_cstr(&sb, ")");
    break;
    case Ast_Kind_record_enter:
      string_builder_push_cstr(&sb, "r(");
      string_builder_push_s64(&sb, ast.length);
    break;
    case Ast_Kind_record_leave:
      string_builder_push_cstr(&sb, ")r");
    break;
    case Ast_Kind_tuple_enter:
      string_builder_push_cstr(&sb, "t(");
      string_builder_push_s64(&sb, ast.length);
    break;
    case Ast_Kind_tuple_leave:
      string_builder_push_cstr(&sb, ")t");
    break;
    case Ast_Kind_call:
      string_builder_push_cstr(&sb, "call");
    break;
    case Ast_Kind_int:
      string_builder_push_s64(&sb, ast.s64);
    break;
    case Ast_Kind_name:
      string_builder_push_cstr(&sb, cstr_from_istr(ast.istr));
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

typedef struct {
  Slice_Ast stack;
  Slice_Ast final;
  Slice_Token tokens;
  I32  tok;
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
  I32 index = slice_ast_push(&parser.final, ast);
  Astid astid = { index };
  return astid;
}

Astid parse_final_insert(Astid astid, Ast ast) {
  for (I32 i = parser.final.length; i >= astid.index; i--) {
    parser.final.base[i] = parser.final.base[i-1];
  }
  parser.final.base[astid.index] = ast;
  parser.final.length++;
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

I32 parse_right_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_fun_enter:
  case Ast_Kind_fun_leave:
    return 1;
  case Ast_Kind_tuple_enter:
  case Ast_Kind_tuple_leave:
    return 3;
  case Ast_Kind_sub:
  case Ast_Kind_add:
    return 11;
  case Ast_Kind_mul:
    return 13;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:
    return 15;
  case Ast_Kind_array:
  case Ast_Kind_load:
    return 17;
  case Ast_Kind_subscript:
  case Ast_Kind_call:
  case Ast_Kind_access:
    return 19;
  default :
    return -1;
  }
}

I32 parse_left_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_fun_enter:
  case Ast_Kind_fun_leave:
    return 1;
  case Ast_Kind_tuple_enter:
  case Ast_Kind_tuple_leave:
    return 4;
  case Ast_Kind_sub:
  case Ast_Kind_add:
    return 12;
  case Ast_Kind_mul: return 14;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:
    return 16;
  case Ast_Kind_array_close:
  case Ast_Kind_load:
    return 18;
  case Ast_Kind_subscript_close:
  case Ast_Kind_call:
  case Ast_Kind_access:
    return 20;
  default :
    return -1;
  }
}

B8 parse_stack_is_top_higher_precedence(Ast_Kind kind) {
  if (slice_ast_is_empty(&parser.stack)) return false;
  Ast* stack_ast = slice_ast_top(&parser.stack);
  I32 on_stack_precedence  = parse_left_precedence(stack_ast->kind);
  I32 on_stream_precedence = parse_right_precedence(kind);
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
    printf("expected\n");
  }
  return false;
}

void parse_print(Cstr cstr) {
  printf("%s\n", cstr);
  printf("ast final: ");
  slice_ast_print(&parser.final);
  printf("ast stack: ");
  slice_ast_print(&parser.stack);
  printf("\n");
}

I32 parse_transfer_final_to_stack_from(I32 mark) {
  I32 result = parser.stack.length;
  for (I32 i = mark; i < parser.final.length; i++) {
    parse_stack_push(parser.final.base[i]);
  }
  parser.final.length = mark;
  return result;
}

I32 parse_transfer_stack_to_final_from(I32 mark) {
  I32 result = parser.final.length;
  for (I32 i = mark; i < parser.stack.length; i++) {
    parse_final_push(parser.stack.base[i]);
  }
  parser.stack.length = mark;
  return result;
}

void parse_tokens(Slice_Token tokens, Umi source_length) {
  I32 max_ast_length = source_length + 2;
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
    case Ast_Kind_statement: { label_statement:;
      Token token = parse_consume_token();
      switch (token.kind) {
      case Token_Kind_none: {
      } goto label_statement;
      case Token_Kind_if: {
        Ast if_do = { Ast_Kind_if_do, { .last_at.index = parser.final.length } };
        parse_stack_push(if_do);
      } goto label_expression;
      case Token_Kind_while: {
        Ast while_do = { Ast_Kind_while_do, { .last_at.index = parser.final.length } };
        parse_stack_push(while_do);
      } goto label_expression;
      // case Token_Kind_for:
      case Token_Kind_return: {
        Ast ret = { Ast_Kind_return, { .last_at.index = parser.final.length } };
        if (token.flag & Token_Flag_willnewline) {
          parse_final_push(ret);
        }
        else {
          parse_stack_push(ret);
          goto label_expression;
        }
      } break;
      case Token_Kind_break: {
        Ast brk = { Ast_Kind_break, { .last_at.index = parser.final.length } };
        if (token.flag & Token_Flag_willnewline) {
          parse_final_push(brk);
        }
        else {
          parse_stack_push(brk);
          goto label_expression;
        }
      } break;
      default: {
        parse_unconsume_token();
        Ast assign = { Ast_Kind_assign_lhs, { .last_at.index = parser.final.length } };
        parse_stack_push(assign);
      } goto label_expression;
      }
    } break;
    case Ast_Kind_while_do: {
      parse_expect_token(Token_Kind_do);
      Ast while_enter = { Ast_Kind_while_enter, {0} };
      parse_final_push(while_enter);
      Ast while_leave = { Ast_Kind_while_leave, {0} };
      parse_stack_push(while_leave);
    } goto label_statement;
    case Ast_Kind_if_do: {
      parse_expect_token(Token_Kind_do);
      Ast if_enter = { Ast_Kind_if_enter, {0} };
      parse_final_push(if_enter);
      Ast if_else = { Ast_Kind_if_else, {0} };
      parse_stack_push(if_else);
    } goto label_statement;
    case Ast_Kind_if_else: {
      if (parse_match_token(Token_Kind_else)) {
        Ast if_leave_else_enter = { Ast_Kind_if_leave_else_enter, {0} };
        parse_final_push(if_leave_else_enter);
        Ast else_leave = { Ast_Kind_else_leave, {0} };
        parse_stack_push(else_leave);
        goto label_statement;
      }
      else {
        Ast if_leave = { Ast_Kind_if_leave, {0} };
        parse_final_push(if_leave);
        goto label_infix_or_suffix;
      }
    } break;
    case Ast_Kind_if_value_do: {
      parse_expect_token(Token_Kind_do);
      Ast if_enter = { Ast_Kind_if_enter, {0} };
      parse_final_push(if_enter);
      Ast if_else = { Ast_Kind_if_value_else, {0} };
      parse_stack_push(if_else);
    } goto label_expression;
    case Ast_Kind_if_value_else: {
      parse_expect_token(Token_Kind_else);
      Ast if_leave_else_enter = { Ast_Kind_if_leave_else_enter, {0} };
      parse_final_push(if_leave_else_enter);
      Ast else_leave = { Ast_Kind_else_value_leave, { .last_at.index = parser.final.length } };
      parse_stack_push(else_leave);
    } goto label_expression;
    case Ast_Kind_expression: { label_expression:;
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
      case Token_Kind_if: {
        Ast if_do = { Ast_Kind_if_value_do, { .last_at.index = parser.final.length } };
        parse_stack_push(if_do);
      } goto label_expression;
      case Token_Kind_while: {
        Ast while_do = { Ast_Kind_while_do, { .last_at.index = parser.final.length } };
        parse_stack_push(while_do);
      } goto label_expression;
      case Token_Kind_paren_open: {
        Ast paren_open = { Ast_Kind_paren_enter, {0} };
        Astid astid_open = parse_final_push(paren_open);
        Ast paren_close = { .kind = Ast_Kind_paren_leave, .enter_at = astid_open, .last_at.index = parser.final.length };
        parse_stack_push(paren_close);
        Ast assign = { Ast_Kind_assign_lhs, { .last_at.index = parser.final.length } };
        parse_stack_push(assign);
      } goto label_expression;
      case Token_Kind_curly_open: {
        Ast   enter = { Ast_Kind_block_value_enter, {0} };
        Astid astid_enter = parse_final_push(enter);
        Ast   leave = { Ast_Kind_block_value_leave, .enter_at = astid_enter };
        parse_stack_push(leave);
      } goto label_statement;
      case Token_Kind_brace_open:
      case Token_Kind_brace_prefix_open: {
        Ast ast = { Ast_Kind_array_close, { .last_at.index = parser.final.length } };
        parse_stack_push(ast);
      } goto label_expression;
      default:
        parse_unconsume_token();
      break;
      }
    } break;
    case Ast_Kind_infix_or_suffix: { label_infix_or_suffix:;
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
      case Token_Kind_dot: {
        Ast ast = { Ast_Kind_access, {0} };
        while (parse_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        parse_final_push(ast);
      } goto label_expression;
      case Token_Kind_arrow: {
        Ast fun_enter = { Ast_Kind_fun_enter, {0} };
        while (parse_stack_is_top_higher_precedence(fun_enter.kind)) {
          parse_transfer_one();
        }
        Ast* top = parse_stack_top();
        parse_final_insert(top->last_at, fun_enter);
        Ast fun_leave = { Ast_Kind_fun_leave, { .last_at.index = parser.final.length } };
        parse_stack_push(fun_leave);
      } goto label_expression;
      case Token_Kind_brace_open: {
        Ast ast = { Ast_Kind_subscript_close, { .last_at.index = parser.final.length } };
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
        Ast ast = { Ast_Kind_tuple_enter, {0} };
        while (parse_stack_is_top_higher_precedence(ast.kind)) {
          parse_transfer_one();
        }
        Ast* top = parse_stack_top();
        if (top->kind == Ast_Kind_tuple_leave) {
          Ast* open = parse_final_at(top->enter_at);
          open->leave_at.index = parser.final.length;
          open->length++;
        }
        else {
          Ast tuple_enter = { Ast_Kind_tuple_enter, {.length = 1} };
          Astid tuple_enter_astid = parse_final_insert(top->last_at, tuple_enter);
          Ast tuple_leave = { Ast_Kind_tuple_leave, .enter_at = tuple_enter_astid };
          parse_stack_push(tuple_leave);
        }
      } goto label_expression;
      default: {
        parse_unconsume_token();
        if ((token.kind & Token_Kind_Flag_call_rhs) && !(token.flag & Token_Flag_wasnewline)) {
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
      Ast* top = parse_final_top();
      if (parse_match_token(Token_Kind_equal)) {
        Ast* top = parse_stack_top();
        Ast assign = { Ast_Kind_assign_rhs, {0} };
        if (top->kind == Ast_Kind_record_leave ||
            top->kind == Ast_Kind_paren_leave) {
          assign.kind = Ast_Kind_record_assign;
        }
        I32 final_mark = ast.last_at.index;
        I32 stack_mark_length = parse_transfer_final_to_stack_from(final_mark);
        assign.last_at.index = stack_mark_length;
        parse_stack_push(assign);
        goto label_expression;
      }
      else if (parse_match_token(Token_Kind_colon)) {
        Ast* top = parse_stack_top();
        Ast declare = { Ast_Kind_declare_rhs, {0} };
        if (top->kind == Ast_Kind_record_leave ||
            top->kind == Ast_Kind_paren_leave) {
          declare.kind = Ast_Kind_record_declare;
        }
        I32 final_mark = ast.last_at.index;
        I32 stack_mark_length = parse_transfer_final_to_stack_from(final_mark);
        declare.last_at.index = stack_mark_length;
        parse_stack_push(declare);
        goto label_expression;
      }
      else if (top->kind == Ast_Kind_block_value_leave) {
        parse_print("");
        Ast* enter = parse_final_at(top->enter_at);
        enter->kind = Ast_Kind_block_enter;
        top->kind = Ast_Kind_block_leave;
      }
    } break;
    case Ast_Kind_record_declare:
    case Ast_Kind_declare_rhs:
    case Ast_Kind_record_assign:
    case Ast_Kind_assign_rhs: {
      parse_final_push(ast);
      parse_transfer_stack_to_final_from(ast.last_at.index);
    } break;
    case Ast_Kind_block_value_leave: {
      Ast ast_close = ast;
      if (parse_match_token(Token_Kind_curly_close)) {
        Ast* open = parse_final_at(ast_close.enter_at);
        Astid astid_close = parse_final_push(ast_close);
        open->leave_at = astid_close;
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
        Ast* open = parse_final_at(ast_close.enter_at);
        Astid astid_close = parse_final_push(ast_close);
        open->leave_at = astid_close;
      }
      else {
        parse_match_token(Token_Kind_semicolon);
        parse_stack_push(ast_close);
        goto label_statement;
      }
    } break;
    case Ast_Kind_tuple_leave: {
      Ast ast_close = ast;
      Ast* open = parse_final_at(ast_close.enter_at);
      Astid astid_close = parse_final_push(ast_close);
      if (astid_close.index != open->leave_at.index) {
        open->length++;
      }
      open->leave_at = astid_close;
    } break;
    case Ast_Kind_record_leave: {
      parse_match_token(Token_Kind_semicolon);
      Ast ast_close = ast;
      Ast* open = parse_final_at(ast_close.enter_at);
      if (parse_match_token(Token_Kind_paren_close)) {
        open->length++;
        Astid astid_close = parse_final_push(ast_close);
        open->leave_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        ast_close.last_at.index = parser.final.length;
        open->length++;
        parse_stack_push(ast_close);
        Ast assign = { Ast_Kind_assign_lhs, { .last_at.index = parser.final.length } };
        parse_stack_push(assign);
        goto label_expression;
      }
    } break;
    case Ast_Kind_paren_leave: {
      Ast ast_close = ast;
      Ast* open = parse_final_at(ast_close.enter_at);
      if (parse_match_token(Token_Kind_paren_close)) {
        if (parser.final.length == ast_close.enter_at.index) {
          // empty record ()
          open->kind = Ast_Kind_record_enter;
          ast_close.kind = Ast_Kind_record_leave;
        }
        Astid astid_close = parse_final_push(ast_close);
        open->leave_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        open->kind = Ast_Kind_record_enter;
        ast_close.kind = Ast_Kind_record_leave;
        ast_close.last_at.index = parser.final.length;
        parse_stack_push(ast_close);
        Ast assign = { Ast_Kind_assign_lhs, { .last_at.index = parser.final.length } };
        parse_stack_push(assign);
        goto label_expression;
      }
    } break;
    case Ast_Kind_array_close: {
      parse_expect_token(Token_Kind_brace_close);
      Ast array = { Ast_Kind_array, {0} };
      parse_stack_push(array);
    } goto label_expression;
    case Ast_Kind_subscript_close: {
      parse_expect_token(Token_Kind_brace_close);
      Ast subscript = { Ast_Kind_subscript, {0} };
      parse_stack_push(subscript);
    } goto label_infix_or_suffix;
    default:
      parse_final_push(ast);
    break;
    }
  }
}

void astid_from_source(Cstr source, Cstr path) {
  Umi source_len = strlen(source);
  I32  keyword_count = 32;
  istr_init(source_len+1 + keyword_count);
  Slice_Token tokens = lex_source(source, path);
  printf("%s\n", cstr_from_slice_token(tokens));
  parse_tokens(tokens, source_len);
}

Cstr source_to_cstr_from_ast(Cstr source, Cstr name) {
  astid_from_source(source, name);
  return cstr_from_slice_ast(&parser.final);
}

Cstr cstr_from_source_info(Cstr file_name, I32 line) {
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

B8 _test_ast(Cstr expected, Cstr file_name, I32 line, Cstr source) {
  arena_init(&temp_arena, GB(1));
  Cstr resulted = source_to_cstr_from_ast(source, cstr_from_source_info(file_name, line));
  B8 result = test_at_source(resulted, expected, file_name, line, source);
  arena_free_all(&temp_arena);
  return result;
}

#define test(source, expected) _test_ast(expected, __FILE__, __LINE__, source)

void parse_test(void) {
  test("a: b = c", "{}");
}

#undef test
