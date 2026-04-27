typedef enum {
  Ast_Flag_none   = 0,
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
  Ast_Flag_list   = 1 << 10,
  Ast_Flag_enter = 1 << 11,
  Ast_Flag_leave = 1 << 12,
  Ast_Flag_split = 1 << 13,
} Ast_Flag;

enum {
  Ast_Kind_add       = Token_Kind_plus,
  Ast_Kind_sub       = Token_Kind_minus,
  Ast_Kind_mul       = Token_Kind_star,
  Ast_Kind_neg       = Token_Kind_minus+1,
  Ast_Kind_pos       = Token_Kind_plus+1,
  Ast_Kind_ptr       = Token_Kind_at,
  Ast_Kind_load      = Token_Kind_at+1,
  Ast_Kind_dot       = Token_Kind_dot,
  Ast_Kind_subscript = Token_Kind_brace_open,
  Ast_Kind_array     = Token_Kind_brace_open + 1,
  Ast_Kind_assign    = Token_Kind_equal,
  Ast_Kind_declare   = Token_Kind_colon,
  Ast_Kind_block       = (Token_Kind_curly_open & 0xff),
  Ast_Kind_block_value = (Token_Kind_curly_open & 0xff) + 1,
  Ast_Kind_source      = Token_Kind_source_enter,
  Ast_Kind_tuple       = Token_Kind_comma,
  Ast_Kind_assign_tuple= Token_Kind_comma+1,
  Ast_Kind_fun         = Token_Kind_arrow,
  Ast_Kind_if          = Token_Kind_if & 0xff,
  Ast_Kind_if_value    = (Token_Kind_if & 0xff) +1,
  Ast_Kind_else        = Token_Kind_else,
  Ast_Kind_else_value  = Token_Kind_else+1,
  Ast_Kind_return      = Token_Kind_return,
  Ast_Kind_return_void = Token_Kind_return+1,
  Ast_Kind_break       = Token_Kind_break,
  Ast_Kind_break_void  = Token_Kind_break+1,
  Ast_Kind_while       = Token_Kind_while,
  Ast_Kind_record      = Token_Kind_paren_open & 0xff,
  Ast_Kind_call,
};

typedef enum {
  Ast_Kind_none = 0,
  Ast_Kind_name         = Token_Kind_name & 0xff,
  Ast_Kind_int          = Token_Kind_int & 0xff,
  Ast_Kind_add_enter    = Ast_Kind_add | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_add_split    = Ast_Kind_add | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_add_leave    = Ast_Kind_add | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_sub_enter    = Ast_Kind_sub | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_sub_split    = Ast_Kind_sub | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_sub_leave    = Ast_Kind_sub | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_mul_enter    = Ast_Kind_mul | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_mul_split    = Ast_Kind_mul | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_mul_leave    = Ast_Kind_mul | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_neg_enter    = Ast_Kind_neg | Ast_Flag_unary  | Ast_Flag_enter,
  Ast_Kind_neg_leave    = Ast_Kind_neg | Ast_Flag_unary  | Ast_Flag_leave,
  Ast_Kind_pos_enter    = Ast_Kind_pos | Ast_Flag_unary  | Ast_Flag_enter,
  Ast_Kind_pos_leave    = Ast_Kind_pos | Ast_Flag_unary  | Ast_Flag_leave,
  Ast_Kind_ptr_enter    = Ast_Kind_ptr | Ast_Flag_unary  | Ast_Flag_enter,
  Ast_Kind_ptr_leave    = Ast_Kind_ptr | Ast_Flag_unary  | Ast_Flag_leave,
  Ast_Kind_dot_enter    = Ast_Kind_dot | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_dot_split    = Ast_Kind_dot | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_dot_leave    = Ast_Kind_dot | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_load_enter   = Ast_Kind_load| Ast_Flag_unary | Ast_Flag_enter,
  Ast_Kind_load_leave   = Ast_Kind_load| Ast_Flag_unary | Ast_Flag_leave,
  Ast_Kind_subscript_enter = Ast_Kind_subscript | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_subscript_split = Ast_Kind_subscript | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_subscript_leave = Ast_Kind_subscript | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_array_enter     = Ast_Kind_array     | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_array_split     = Ast_Kind_array     | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_array_leave     = Ast_Kind_array     | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_assign_enter    = Ast_Kind_assign    | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_assign_split    = Ast_Kind_assign    | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_assign_leave    = Ast_Kind_assign    | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_declare_enter   = Ast_Kind_declare   | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_declare_split   = Ast_Kind_declare   | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_declare_leave   = Ast_Kind_declare   | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_record_enter    = Ast_Kind_record    | Ast_Flag_list | Ast_Flag_enter,
  Ast_Kind_record_split    = Ast_Kind_record    | Ast_Flag_list | Ast_Flag_split,
  Ast_Kind_record_leave    = Ast_Kind_record    | Ast_Flag_list | Ast_Flag_leave,
  Ast_Kind_tuple_enter     = Ast_Kind_tuple     | Ast_Flag_list | Ast_Flag_enter,
  Ast_Kind_tuple_split     = Ast_Kind_tuple     | Ast_Flag_list | Ast_Flag_split,
  Ast_Kind_tuple_leave     = Ast_Kind_tuple     | Ast_Flag_list | Ast_Flag_leave,
  Ast_Kind_assign_tuple_enter = Ast_Kind_assign_tuple | Ast_Flag_list | Ast_Flag_enter,
  Ast_Kind_assign_tuple_split = Ast_Kind_assign_tuple | Ast_Flag_list | Ast_Flag_split,
  Ast_Kind_assign_tuple_leave = Ast_Kind_assign_tuple | Ast_Flag_list | Ast_Flag_leave,
  Ast_Kind_source_enter       = Ast_Kind_source | Ast_Flag_list | Ast_Flag_enter,
  Ast_Kind_source_split       = Ast_Kind_source | Ast_Flag_list | Ast_Flag_split,
  Ast_Kind_source_leave       = Ast_Kind_source | Ast_Flag_list | Ast_Flag_leave,
  Ast_Kind_block_enter        = Ast_Kind_block | Ast_Flag_list | Ast_Flag_enter,
  Ast_Kind_block_split        = Ast_Kind_block | Ast_Flag_list | Ast_Flag_split,
  Ast_Kind_block_leave        = Ast_Kind_block | Ast_Flag_list | Ast_Flag_leave,
  Ast_Kind_block_value_enter  = Ast_Kind_block_value | Ast_Flag_list | Ast_Flag_enter,
  Ast_Kind_block_vlaue_split  = Ast_Kind_block_value | Ast_Flag_list | Ast_Flag_split,
  Ast_Kind_block_value_leave  = Ast_Kind_block_value | Ast_Flag_list | Ast_Flag_leave,
  Ast_Kind_call_enter         = Ast_Kind_call | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_call_split         = Ast_Kind_call | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_call_leave         = Ast_Kind_call | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_fun_enter          = Ast_Kind_fun  | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_fun_split          = Ast_Kind_fun  | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_fun_leave          = Ast_Kind_fun  | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_return_enter        = Ast_Kind_return | Ast_Flag_unary | Ast_Flag_enter,
  Ast_Kind_return_leave        = Ast_Kind_return| Ast_Flag_unary | Ast_Flag_leave,
  // Ast_Kind_return_void         = Ast_Kind_return,
  Ast_Kind_break_enter         = Ast_Kind_break | Ast_Flag_unary | Ast_Flag_enter,
  Ast_Kind_break_leave         = Ast_Kind_break | Ast_Flag_unary | Ast_Flag_leave,
  // Ast_Kind_break_void          = Ast_Kind_break ,
  Ast_Kind_while_enter         = Ast_Kind_while | Ast_Flag_binary | Ast_Flag_enter,
  Ast_Kind_while_split         = Ast_Kind_while | Ast_Flag_binary | Ast_Flag_split,
  Ast_Kind_while_leave         = Ast_Kind_while | Ast_Flag_binary | Ast_Flag_leave,
  Ast_Kind_if_enter,
  Ast_Kind_if_split,
  Ast_Kind_if_leave,
  Ast_Kind_if_leave_else_enter,
  Ast_Kind_else_leave,
  Ast_Kind_if_value_leave,
  Ast_Kind_else_value_leave,
} Ast_Kind;

typedef I32 Astid;

typedef struct {
  Ast_Kind kind;
  union {
    U64   bits;
    I64   i64;
    Istr  istr;
    struct {
      I32   position;
    } split;
    struct {
      Astid length;
      Astid leave;
    } list;
    struct {
      Astid two;
      Astid leave;
    } binary;
    struct {
      Astid one;
      Astid leave;
    } unary;
  };
} Ast_Node; 

typedef struct {
  Ast_Node* base;
  I32       length;
} Ast;

Cstr cstr_from_ast_kind(Ast_Kind ast_kind) {
  Cstr result = "unknown";
  switch (ast_kind & 0xff) {
  case Ast_Kind_none:         result = "none"; break;
  case Ast_Kind_name:         result = "name"; break;
  case Ast_Kind_int:          result = "int"; break;
  case Ast_Kind_add:          result = "add"; break;
  case Ast_Kind_sub:          result = "sub"; break;
  case Ast_Kind_mul:          result = "mul"; break;
  case Ast_Kind_neg:          result = "neg"; break;
  case Ast_Kind_pos:          result = "pos"; break;
  case Ast_Kind_ptr:          result = "ptr"; break;
  case Ast_Kind_load:         result = "load"; break;
  case Ast_Kind_dot:          result = "dot"; break;
  case Ast_Kind_subscript:    result = "subscript"; break;
  case Ast_Kind_array:        result = "array"; break;
  case Ast_Kind_assign:       result = "assign"; break;
  case Ast_Kind_declare:      result = "declare"; break;
  case Ast_Kind_block:        result = "block"; break;
  case Ast_Kind_block_value:  result = "block_value"; break;
  case Ast_Kind_source:       result = "source"; break;
  case Ast_Kind_tuple:        result = "tuple"; break;
  case Ast_Kind_assign_tuple: result = "assign_tuple"; break;
  case Ast_Kind_fun:          result = "fun"; break;
  case Ast_Kind_if:           result = "if"; break;
  case Ast_Kind_if_value:     result = "if_value"; break;
  case Ast_Kind_else:         result = "else"; break;
  case Ast_Kind_else_value:   result = "else_value"; break;
  case Ast_Kind_return:       result = "return"; break;
  case Ast_Kind_return_void:  result = "return_void"; break;
  case Ast_Kind_break:        result = "break"; break;
  case Ast_Kind_break_void:   result = "break_void"; break;
  case Ast_Kind_while:        result = "while"; break;
  case Ast_Kind_record:       result = "record"; break;
  case Ast_Kind_call:         result = "call"; break;
  }
  return result;
}

Cstr cstr_from_ast_kind_enter(Ast_Kind ast_kind) {
  Cstr result = "<unknown>";
  switch (ast_kind & 0xff) {
  case Ast_Kind_block:        result = "b{"; break;
  case Ast_Kind_block_value:  result = "v{"; break;
  case Ast_Kind_source:       result = "s{"; break;
  case Ast_Kind_tuple:        result = "t("; break;
  case Ast_Kind_assign_tuple: result = "a("; break;
  case Ast_Kind_if:           result = "if"; break;
  case Ast_Kind_if_value:     result = "if"; break;
  case Ast_Kind_else:         result = "el"; break;
  case Ast_Kind_else_value:   result = "el"; break;
  case Ast_Kind_while:        result = "wh"; break;
  case Ast_Kind_record:       result = "r("; break;
  case Ast_Kind_neg:          result = "(-"; break;
  default: result = "(";
  }
  return result;
}

Cstr cstr_from_ast_kind_leave(Ast_Kind ast_kind) {
  Cstr result = "<unknown>";
  switch (ast_kind & 0xff) {
  case Ast_Kind_block:        result = "}b"; break;
  case Ast_Kind_block_value:  result = "}v"; break;
  case Ast_Kind_source:       result = "}s"; break;
  case Ast_Kind_tuple:        result = ")t"; break;
  case Ast_Kind_assign_tuple: result = ")a"; break;
  case Ast_Kind_record:       result = ")r"; break;
  case Ast_Kind_if:           result = "do"; break;
  case Ast_Kind_if_value:     result = "do"; break;
  case Ast_Kind_while:        result = "do"; break;
  default: result = ")";
  }
  return result;
}

Cstr cstr_from_ast_kind_split(Ast_Kind ast_kind) {
  Cstr result = "<unknown>";
  switch (ast_kind & 0xff) {
  case Ast_Kind_call:         result = " "; break;
  case Ast_Kind_add:          result = "+"; break;
  case Ast_Kind_sub:          result = "-"; break;
  case Ast_Kind_mul:          result = "*"; break;
  case Ast_Kind_dot:          result = "."; break;
  case Ast_Kind_subscript:    result = "[]"; break;
  case Ast_Kind_array:        result = "[]"; break;
  case Ast_Kind_assign:       result = "="; break;
  case Ast_Kind_declare:      result = ":"; break;
  case Ast_Kind_block:        result = ";"; break;
  case Ast_Kind_block_value:  result = ";"; break;
  case Ast_Kind_source:       result = ";"; break;
  case Ast_Kind_tuple:        result = ","; break;
  case Ast_Kind_assign_tuple: result = ","; break;
  case Ast_Kind_fun:          result = "->"; break;
  case Ast_Kind_if:           result = "do"; break;
  case Ast_Kind_if_value:     result = "do"; break;
  case Ast_Kind_while:        result = "do"; break;
  case Ast_Kind_record:       result = ";"; break;
  default: result = cstr_from_ast_kind(ast_kind);
  }
  return result;
}

Cstr cstr_from_ast(Ast ast, C8* buffer) {
  String_Builder sb = string_builder_begin(buffer);
  for (I32 i = 0; i < ast.length; i++) {
    Ast_Node node = ast.base[i];
    switch (node.kind) {
    case Ast_Kind_name:
      string_builder_push_istr(&sb, node.istr);
      break;
    case Ast_Kind_int:
      string_builder_push_i64(&sb, node.i64);
      break;
    default: {
      if (node.kind & Ast_Flag_enter) {
        Cstr cstr = cstr_from_ast_kind_enter(node.kind);
        string_builder_push_cstr(&sb, cstr);
      }
      else if (node.kind & Ast_Flag_split) {
        Cstr cstr = cstr_from_ast_kind_split(node.kind);
        string_builder_push_cstr(&sb, cstr);
      }
      else if (node.kind & Ast_Flag_leave) {
        Cstr cstr = cstr_from_ast_kind_leave(node.kind);
        string_builder_push_cstr(&sb, cstr);
      }
    } break;
    }
    string_builder_push_cstr(&sb, " ");
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

void ast_print(Ast ast) {
  C8* buffer = xmalloc(10*ast.length);
  printf("%s\n", cstr_from_ast(ast, buffer));
  free(buffer);
}

typedef struct {
  Ast stack;
  Ast final;
  Ast_Kind* kinds;
  Tokens tokens;
  I32  tok;
} Parser;

Astid parse_final_push(Parser* parser, Ast_Node node) {
  Astid astid = push(parser->final, node);
  return astid;
}

Astid parse_final_push_kind(Parser* parser, Ast_Kind kind) {
  Ast_Node node = { .kind = kind, {0} };
  return parse_final_push(parser, node);
}

Astid parse_final_insert(Parser* parser, Astid astid, Ast_Node node) {
  for (I32 i = parser->final.length; i >= astid; i--) {
    parser->final.base[i] = parser->final.base[i-1];
  }
  parser->final.base[astid] = node;

  parser->final.length++;

  return astid;
}

I32 parse_right_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_fun_enter:
  case Ast_Kind_fun_leave:
    return 1;
  case Ast_Kind_tuple_enter:
  case Ast_Kind_tuple_leave:
    return 3;
  case Ast_Kind_sub_leave:
  case Ast_Kind_add_leave:
    return 11;
  case Ast_Kind_mul_leave:
    return 13;
  case Ast_Kind_ptr_leave:
  case Ast_Kind_neg_leave:
  case Ast_Kind_pos_leave:
    return 15;
  case Ast_Kind_load_leave:
    return 17;
  case Ast_Kind_subscript_leave:
  case Ast_Kind_call_leave:
  case Ast_Kind_dot_leave:
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
    return 3;
  case Ast_Kind_sub_leave:
  case Ast_Kind_add_leave:
    return 12;
  case Ast_Kind_mul_leave:
    return 14;
  case Ast_Kind_ptr_leave:
  case Ast_Kind_neg_leave:
  case Ast_Kind_pos_leave:
    return 16;
  case Ast_Kind_array_leave:
  case Ast_Kind_load_leave:
    return 18;
  case Ast_Kind_subscript_leave:
  case Ast_Kind_call_leave:
  case Ast_Kind_dot_leave:
    return 20;
  default :
    return -1;
  }
}
void parse_stack_transfer_to_final(Parser* parser) {
  Ast_Node transfer = pop(parser->stack);
  parse_final_push(parser, transfer);
}

void parse_stack_transfer_to_final_higher_precedence(Parser* parser, Ast_Kind kind) {
  while (!empty(parser->stack)) {
    Ast_Node* top_node = &top(parser->stack);
    I32 on_stack_precedence  = parse_left_precedence(top_node->kind);
    I32 on_stream_precedence = parse_right_precedence(kind);
    if (on_stack_precedence > on_stream_precedence) {
      Ast_Node transfer = pop(parser->stack);
      parse_final_push(parser, transfer);
    }
    else {
      break;
    }
  }
}

void parse_stack_transfer_to_final_is_not(Parser* parser, Ast_Kind kind) {
  while (top(parser->stack).kind != Ast_Kind_array_enter) {
    Ast_Node node = pop(parser->stack);
    parse_final_push(parser, node);
  }
}

B8 parse_match_token(Parser* parser, Token_Kind kind) {
  Token token = parser->tokens.base[parser->tok];
  if (token.kind == kind) {
    parser->tok++;
    return true;
  }
  return false;
}

B8 parse_expect_token(Parser* parser, Token_Kind kind) {
  Token token = parser->tokens.base[parser->tok];
  if (token.kind == kind) {
    parser->tok++;
    return true;
  }
  else {
    printf("expected\n");
  }
  return false;
}

void parse_print(Parser* parser, Cstr cstr) {
  printf("%s\n", cstr);
  printf("ast final: ");
  ast_print(parser->final);
  printf("ast stack: ");
  ast_print(parser->stack);
  printf("\n");
}

I32 parse_transfer_final_to_stack_from(Parser* parser, I32 mark) {
  I32 result = parser->stack.length;
  for (I32 i = mark; i < parser->final.length; i++) {
    Ast_Node node = get(parser->final, i);
    add(parser->stack, node);
  }
  parser->final.length = mark;
  return result;
}

I32 parse_transfer_stack_to_final_from(Parser* parser, I32 mark) {
  I32 result = parser->final.length;
  for (I32 i = mark; i < parser->stack.length; i++) {
    parse_final_push(parser, get(parser->stack, i));
  }
  parser->stack.length = mark;
  return result;
}

Astid parse_stack_push_kind(Parser* parser, Ast_Kind kind) {
  Ast_Node ast = { kind, {0} };
  Astid astid = push(parser->stack, ast);
  return astid;
}

void parse_expression_enter(Parser* parser);
void parse_statement(Parser* parser);

void parse_expression(Parser* parser) {
  I32 final_length = parser->final.length;
  parse_stack_push_kind(parser, Ast_Kind_none);
  parse_expression_enter(parser);
  del(parser->stack);
  if (parse_match_token(parser, Token_Kind_comma)) {
    Ast_Node tuple_enter = { Ast_Kind_tuple_enter, .list.length = 0 };
    Astid tuple_enter_astid = parse_final_insert(parser, final_length, tuple_enter);
    Ast_Node* enter = &get(parser->final, tuple_enter_astid);
    do {
      Ast_Node split = { Ast_Kind_tuple_split, .split.position = enter->list.length };
      parse_final_push(parser, split);
      enter->list.length++;
      parse_expression_enter(parser);
    } while (parse_match_token(parser, Token_Kind_comma));

    Ast_Node top = top(parser->final);
    if (top.kind != Ast_Kind_tuple_split) {
      Ast_Node split = { Ast_Kind_tuple_split, .split.position = enter->list.length };
      parse_final_push(parser, split);
      enter->list.length++;
    }
    parse_final_push_kind(parser, Ast_Kind_tuple_leave);
  }
  else {
  }
}

void parse_infix_or_suffix(Parser* parser) {
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_plus: case Token_Kind_minus:
  case Token_Kind_star: case Token_Kind_dot: {
    Ast_Kind kind = (Ast_Kind)token.kind | Ast_Flag_binary;
    parse_stack_transfer_to_final_higher_precedence(parser, kind | Ast_Flag_leave);
    parse_final_push_kind(parser, kind | Ast_Flag_split);
    parse_stack_push_kind(parser, kind | Ast_Flag_leave);
    parse_expression_enter(parser);
  } break;
  case Token_Kind_arrow: {
    // TODO: fun_enter insert
    // Ast_Node fun_enter = { Ast_Kind_fun_enter, {0} };
    // parse_stack_transfer_higher_precedence(&parser, Ast_Kind_fun_enter);
    // Ast_Node* top = &top(parser.stack);
    // parse_final_insert(&parser, top->last_at, fun_enter);
    // parse_stack_push_kind(&parser, Ast_Kind_fun_leave);
    assert(0);
  } break;
  case Token_Kind_brace_open: {
    parse_stack_transfer_to_final_higher_precedence(parser, Ast_Kind_subscript);
    parse_expression(parser);
    parse_expect_token(parser, Token_Kind_brace_close);
    parse_stack_push_kind(parser, Ast_Kind_subscript);
    parse_infix_or_suffix(parser);
  } break;
  case Token_Kind_at: {
    parse_stack_transfer_to_final_higher_precedence(parser, Ast_Kind_load);
    parse_final_push_kind(parser, Ast_Kind_load);
    parse_infix_or_suffix(parser);
  } break;
  default: {
    parser->tok--;
    if ((token.kind & Token_Kind_Flag_call_rhs) && !(token.flag & Token_Flag_wasnewline)) {
      parse_stack_transfer_to_final_higher_precedence(parser, Ast_Kind_call_leave);
      parse_stack_push_kind(parser, Ast_Kind_call_leave);
      parse_expression_enter(parser);
    }
  } break;
  }
}

void parse_expression_enter(Parser* parser) {
  I32 stack_length = parser->stack.length;
  Astid exp = parse_final_push_kind(parser, Ast_Kind_none);
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_minus_prefix: case Token_Kind_minus:
  case Token_Kind_plus_prefix:  case Token_Kind_plus:
  case Token_Kind_at_prefix:    case Token_Kind_at: {
    Ast_Kind kind = Ast_Flag_unary | (((Ast_Kind)token.kind+1) & 0xff);
    parse_stack_push_kind(parser, kind | Ast_Flag_leave);
    parse_expression_enter(parser);
  } break;
  case Token_Kind_int: case Token_Kind_name: {
    Ast_Kind kind = (Ast_Kind)token.kind & 0xff;
    Ast_Node ast = { .kind = kind, .bits = token.value };
    parse_final_push(parser, ast);
  } break;
  case Token_Kind_if: {
    parse_expression(parser);
    parse_expect_token(parser, Token_Kind_do);
    parse_final_push_kind(parser, Ast_Kind_if_enter);
    parse_expression(parser);
    if (parse_match_token(parser, Token_Kind_else)) {
      parse_final_push_kind(parser, Ast_Kind_if_leave_else_enter);
      parse_expression_enter(parser);
      parse_final_push_kind(parser, Ast_Kind_else_value_leave);
    }
    else {
      parse_final_push_kind(parser, Ast_Kind_if_value_leave);
    }
  } break;
  case Token_Kind_while: {
  // TODO: consider while value
    parse_expression(parser);
    parse_expect_token(parser, Token_Kind_do);
    parse_final_push_kind(parser, Ast_Kind_while_enter);
    parse_statement(parser);
    parse_final_push_kind(parser, Ast_Kind_while_leave);
  } break;
  case Token_Kind_paren_open: {
    Astid final_length = parser->final.length;
    Astid record_length = 0;
    B8 is_semicolon = false;
    while (!parse_match_token(parser, Token_Kind_paren_close)) {
      parse_expression(parser);
      record_length++;
      if (parse_match_token(parser, Token_Kind_semicolon)) {
        is_semicolon = true;
      }
      else if (parse_match_token(parser, Token_Kind_equal)) {
      // TODO: equal/colon
        assert(0);
      }
    }
    if (is_semicolon || record_length != 1) {
      Ast_Node record_enter = { Ast_Kind_record_enter, .list.length = record_length };
      parse_final_insert(parser, final_length, record_enter);
      parse_final_push_kind(parser, Ast_Kind_record_leave);
    }
  } break;
  case Token_Kind_curly_open: {
    while (!parse_match_token(parser, Token_Kind_curly_close)) {
      parse_statement(parser);
      parse_match_token(parser, Token_Kind_semicolon);
      parse_final_push_kind(parser, Ast_Kind_block_split);
    }
    parse_final_push_kind(parser, Ast_Kind_block_value_leave);
  } break;
  case Token_Kind_brace_open:
  case Token_Kind_brace_prefix_open: {
    parse_expression(parser);
    parse_expect_token(parser, Token_Kind_brace_close);
    parse_stack_push_kind(parser, Ast_Kind_array);
    parse_expression_enter(parser);
  } break;
  default:
    parser->tok--;
  break;
  }
  parse_infix_or_suffix(parser);

  while (parser->stack.length > stack_length) {
    Ast_Node node = pop(parser->stack);
    parse_final_push(parser, node);
  }

  Ast_Kind kind = top(parser->final).kind;
  if (kind == Ast_Kind_name || kind == Ast_Kind_int) {
    get(parser->final, exp) = pop(parser->final);
  }
  else {
    get(parser->final, exp).kind = (kind & ~Ast_Flag_leave) | Ast_Flag_enter;
    get(parser->final, exp).binary.leave = parser->final.length-1;
  }
}

Astid parse_convert_to_pattern(Parser* parser, Astid astid) {
  Ast_Node node = get(parser->final, astid);
  Ast_Kind kind = node.kind;
  switch (node.kind) {
  case Ast_Kind_tuple_enter: {
    kind = Ast_Kind_assign_tuple_enter;
    parser->final.base[astid++].kind = kind;
    while (node.kind != Ast_Kind_tuple_leave) {
      astid = parse_convert_to_pattern(parser, astid);
      node = get(parser->final, astid);
    }
    kind = Ast_Kind_assign_tuple_leave;
    parser->final.base[astid++].kind = kind;
  } break;
  case Ast_Kind_tuple_split: {
    kind = Ast_Kind_assign_tuple_split;
  } break;
  default: {
    if (node.kind & Ast_Flag_binary) {
    }
  } break;
  }
  parser->final.base[astid].kind = kind;
  return astid+1;
}

void parse_statement(Parser* parser) {
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_source_enter: {
    parse_final_push_kind(parser, Ast_Kind_source_enter);
    while (!parse_match_token(parser, Token_Kind_source_leave)) {
      parse_statement(parser);
      parse_match_token(parser, Token_Kind_semicolon);
      parse_final_push_kind(parser, Ast_Kind_source_split);
    }
    parse_final_push_kind(parser, Ast_Kind_source_leave);
  } break;
  case Token_Kind_if: {
    parse_expression(parser);
    parse_final_push_kind(parser, Ast_Kind_if_enter);

    parse_expect_token(parser, Token_Kind_do);
    parse_statement(parser);
    if (parse_match_token(parser, Token_Kind_else)) {
      parse_final_push_kind(parser, Ast_Kind_if_leave_else_enter);
      parse_statement(parser);
      parse_final_push_kind(parser, Ast_Kind_else_leave);
    }
    else {
      parse_final_push_kind(parser, Ast_Kind_if_leave);
    }
  } break;
  case Token_Kind_while: {
    parse_expression(parser);
    parse_final_push_kind(parser, Ast_Kind_while_enter);
    parse_expect_token(parser, Token_Kind_do);
    parse_statement(parser);
    parse_final_push_kind(parser, Ast_Kind_while_leave);
  } break;
  // case Token_Kind_for:
  case Token_Kind_return: {
    if (token.flag & Token_Flag_willnewline) {
      parse_final_push_kind(parser, Ast_Kind_return);
    }
    else {
      parse_expression_enter(parser);
      parse_final_push_kind(parser, Ast_Kind_return);
    }
  } break;
  case Token_Kind_break: {
    if (token.flag & Token_Flag_willnewline) {
      parse_final_push_kind(parser, Ast_Kind_break);
    }
    else {
      parse_expression_enter(parser);
      parse_final_push_kind(parser, Ast_Kind_break);
    }
  } break;
  default: {
    parser->tok--;
    Astid enter = parser->final.length;
    parse_expression(parser);
    if (parse_match_token(parser, Token_Kind_equal)) {
      parse_convert_to_pattern(parser, enter);
      Astid leave = parse_transfer_final_to_stack_from(parser, enter);
      parse_stack_push_kind(parser, Ast_Kind_assign_leave);
      parse_expression(parser);
      parse_transfer_stack_to_final_from(parser, leave);
    }
  }
  }
}

Ast parse_tokens(Tokens tokens, Ast_Node* final_buffer) {
  Parser parser = {0};
  parser.stack.base = xmalloc(sizeof(Ast_Node) * 2*tokens.length);
  parser.kinds      = xmalloc(sizeof(Ast_Kind) * 2*tokens.length);
  parser.final.base = final_buffer;
  parser.tokens     = tokens;
  parser.tok        = 0;
  parse_statement(&parser);
  free(parser.stack.base);
  free(parser.kinds);
  return parser.final;
}

Ast ast_from_source(Cstr source, Ast_Node* final) {
  Umi source_length   = strlen(source);
  I32 max_length      = source_length + 2;
  Token* token_buffer = xmalloc(sizeof(Token) * max_length);
  Tokens tokens  = lex_source(source, token_buffer);
  // C8* tokens_string   = xmalloc(tokens.length * 6 + 1);
  // cstr_from_slice_token(tokens, tokens_string);
  // printf("%s\n", tokens_string);
  // free((void*)tokens_string);
  Ast ast = parse_tokens(tokens, final);
  free(tokens.base);
  return ast;
}

void _test_ast(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source);
  I32 max_ast_length   = source_length + 2;
  Ast_Node* ast_buffer = xmalloc(sizeof(Ast_Node) * 2*max_ast_length);
  Ast ast = ast_from_source(source, ast_buffer);
  C8* buffer = xmalloc(10 * (source_length+2));
  Cstr result = cstr_from_ast(ast, buffer);
  free(ast_buffer);
  test_at_source(result, expected, file_name, line, source);
  free(buffer);
}

#define test(source, expected) _test_ast(source, expected, __FILE__, __LINE__)

void parse_test(void) {
  test("1 + 2*3",        "s{ ( 1 + ( 2 * 3 ) ) ; }s ");
  test("1",              "s{ 1 ; }s ");
  test("1 + 2",          "s{ ( 1 + 2 ) ; }s ");
  test("1 + -2",         "s{ ( 1 + (- 2 ) ) ; }s ");
  test("1*(2+3)",        "s{ ( 1 * ( ( 2 + 3 ) ) ; }s ");
  test("(1 + 2)*3",      "s{ ( ( 1 + 2 ) * 3 ) ; }s ");
  test("foo 1\n2",       "s{ ( foo 1 ) ; 2 ; }s ");
  return;
  test("bar 1 2",        "s{ bar 1 call 2 call }s ");
  test("(1)",            "s{ 1 }s ");
  test("(1\n)",          "s{ 1 }s ");
  test("(1;)",           "s{ r(1 1 )r }s ");
  test("(1\n 2)",        "s{ r(2 1 2 )r }s ");
  test("(1; 2)",         "s{ r(2 1 2 )r }s ");
  test("(1; 2;)",        "s{ r(2 1 2 )r }s ");
  test("b[1+2]*c",       "s{ b 1 2 add s[] c mul }s ");
  test("1 * [2+3]b",     "s{ 1 2 3 add b a[] mul }s ");
  test("[1+2]b",         "s{ 1 2 add b a[] }s ");
  test("[1]b+c",         "s{ 1 b a[] c add }s ");
  test("[1]b",           "s{ 1 b a[] }s ");
  test("c+b[1]",         "s{ c b 1 s[] add }s ");
  test("b[1+2]",         "s{ b 1 2 add s[] }s ");
  test("b[1+2]*c",       "s{ b 1 2 add s[] c mul }s ");
  test("b[1]",           "s{ b 1 s[] }s ");
  test("1,2",            "s{ t( 1 , 2 , )t }s ");
  test("a, b, c = 1",    "s{ 1 a( a , b , c , )a ) }s ");
  test("a, b = 1",       "s{ 1 =t(2 a =, b =, )t= )= }s ");
  test("a + (b,c), d = 1", "s{ 1 =t(2 a t(2 b , c , )t add =, d =, )t= )= }s ");
  test("a, (b,c), d = 1", "s{ 1 =t(3 a =, =t(2 b =, c =, )t= =, d =, )t= )= }s ");
  test("a, b = 1, 2",    "s{ t(2 1 , 2 , )t =t(2 a =, b =, )t= )= }s ");
  test("a+b = 1-2",      "s{ 1 2 sub a b add )= }s ");
  test("a = 1",          "s{ 1 a )= }s ");
  test("a = 1 + 2",      "s{ 1 2 add a )= }s ");
  test("wh 1 do 2",        "s{ 1 wh( 2 )hw }s ");
  test("(if 1 do 2)",      "s{ 1 if( 2 )vi }s ");
  test("(if 1 do 2 el 3)", "s{ 1 if( 2 )fi el( 3 )ve }s ");
  test("if 1 do 2",      "s{ 1 if( 2 )fi }s ");
  test("if 1 do 2 el 3", "s{ 1 if( 2 )fi el( 3 )le }s ");
  test("(1,2;3)",        "s{ r(2 t(2 1 , 2 , )t 3 )r }s ");
}

#undef test
