typedef enum {
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
} Ast_Flag;

typedef enum {
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
    I64   i64;
    Istr  istr;
  };
} Ast_Node; 

typedef struct {
  Ast_Node* nodes;
  I32  length;
} Ast;

Astid ast_push(Ast* ast, Ast_Node node) {
  I32 index = ast->length;
  ast->nodes[ast->length++] = node;
  Astid astid = { index };
  return astid;
}
Astid ast_push_kind(Ast* ast, Ast_Kind kind) {
  Ast_Node node = { kind, {0} };
  return ast_push(ast, node);
}
Ast_Node ast_pop(Ast* ast) {
  return ast->nodes[--ast->length];
}
Ast_Node* ast_top(Ast ast) {
  return &ast.nodes[ast.length-1];
}
Ast_Node* ast_at(Ast ast, Astid astid) {
  return &ast.nodes[astid.index];
}
B8 ast_is_empty(Ast ast) {
  return ast.length == 0;
}

Cstr cstr_from_ast(Ast ast, C8* buffer) {
  String_Builder sb = string_builder_begin(buffer);
  for (I32 i = 0; i < ast.length; i++) {
    Ast_Node node = ast.nodes[i];
    switch (node.kind) {
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
      string_builder_push_i64(&sb, node.length);
    break;
    case Ast_Kind_record_leave:
      string_builder_push_cstr(&sb, ")r");
    break;
    case Ast_Kind_tuple_enter:
      string_builder_push_cstr(&sb, "t(");
      string_builder_push_i64(&sb, node.length);
    break;
    case Ast_Kind_tuple_leave:
      string_builder_push_cstr(&sb, ")t");
    break;
    case Ast_Kind_call:
      string_builder_push_cstr(&sb, "call");
    break;
    case Ast_Kind_int:
      string_builder_push_i64(&sb, node.i64);
    break;
    case Ast_Kind_name:
      string_builder_push_cstr(&sb, cstr_from_istr(node.istr));
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

void ast_print(Ast ast) {
  C8* buffer = xmalloc(10*ast.length);
  printf("%s\n", cstr_from_ast(ast, buffer));
  free(buffer);
}

Astid slice_ast_insert(Ast* slice, Astid astid, Ast_Node ast) {
  for (I32 i = slice->length; i >= astid.index; i--) {
    slice->nodes[i] = slice->nodes[i-1];
  }
  slice->nodes[astid.index] = ast;
  slice->length++;
  return astid;
}

typedef struct {
  Ast stack;
  Ast final;
  Tokens tokens;
  I32  tok;
} Parser;

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

void parse_stack_transfer_higher_precedence(Parser* parser, Ast_Kind kind) {
  while (!ast_is_empty(parser->stack)) {
    Ast_Node* top_node = ast_top(parser->stack);
    I32 on_stack_precedence  = parse_left_precedence(top_node->kind);
    I32 on_stream_precedence = parse_right_precedence(kind);
    if (on_stack_precedence > on_stream_precedence) {
      Ast_Node transfer = ast_pop(&parser->stack);
      ast_push(&parser->final, transfer);
    }
    else {
      break;
    }
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

void parse_print(Parser parser, Cstr cstr) {
  printf("%s\n", cstr);
  printf("ast final: ");
  ast_print(parser.final);
  printf("ast stack: ");
  ast_print(parser.stack);
  printf("\n");
}

I32 parse_transfer_final_to_stack_from(Parser* parser, I32 mark) {
  I32 result = parser->stack.length;
  for (I32 i = mark; i < parser->final.length; i++) {
    ast_push(&parser->stack, parser->final.nodes[i]);
  }
  parser->final.length = mark;
  return result;
}

I32 parse_transfer_stack_to_final_from(Parser* parser, I32 mark) {
  I32 result = parser->final.length;
  for (I32 i = mark; i < parser->stack.length; i++) {
    ast_push(&parser->final, parser->stack.nodes[i]);
  }
  parser->stack.length = mark;
  return result;
}

Astid parse_stack_push_kind_with_final_length(Parser* parser, Ast_Kind kind) {
  Ast_Node ast = { kind, { .last_at.index = parser->final.length } };
  Astid astid = ast_push(&parser->stack, ast);
  return astid;
}

Ast parse_tokens(Tokens tokens, Ast_Node* final_buffer) {
  Parser parser = {0};
  parser.stack.nodes = xmalloc(sizeof(Ast_Node) * 2*tokens.length);
  parser.final.nodes = final_buffer;
  parser.tokens     = tokens;
  parser.tok        = 0;
  ast_push_kind(&parser.stack, Ast_Kind_source_leave);
  goto label_statement;
  while (!ast_is_empty(parser.stack)) {
    Ast_Node ast = ast_pop(&parser.stack);
    switch (ast.kind) {
    case Ast_Kind_statement: { label_statement:;
      Token token = tokens.base[parser.tok++];
      switch (token.kind) {
      case Token_Kind_source_enter: {
        ast_push_kind(&parser.final, Ast_Kind_source_enter);
      } goto label_statement;
      case Token_Kind_if: {
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_if_do);
      } goto label_expression;
      case Token_Kind_while: {
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_while_do);
      } goto label_expression;
      // case Token_Kind_for:
      case Token_Kind_return: {
        if (token.flag & Token_Flag_willnewline) {
          ast_push_kind(&parser.final, Ast_Kind_return);
        }
        else {
          parse_stack_push_kind_with_final_length(&parser, Ast_Kind_return);
          goto label_expression;
        }
      } break;
      case Token_Kind_break: {
        if (token.flag & Token_Flag_willnewline) {
          ast_push_kind(&parser.final, Ast_Kind_break);
        }
        else {
          parse_stack_push_kind_with_final_length(&parser, Ast_Kind_break);
          goto label_expression;
        }
      } break;
      default: {
        parser.tok--;
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_assign_lhs);
      } goto label_expression;
      }
    } break;
    case Ast_Kind_while_do: {
      parse_expect_token(&parser, Token_Kind_do);
      ast_push_kind(&parser.final, Ast_Kind_while_enter);
      ast_push_kind(&parser.stack, Ast_Kind_while_leave);
    } goto label_statement;
    case Ast_Kind_if_do: {
      parse_expect_token(&parser, Token_Kind_do);
      ast_push_kind(&parser.final, Ast_Kind_if_enter);
      ast_push_kind(&parser.stack, Ast_Kind_if_else);
    } goto label_statement;
    case Ast_Kind_if_else: {
      if (parse_match_token(&parser, Token_Kind_else)) {
        ast_push_kind(&parser.final, Ast_Kind_if_leave_else_enter);
        ast_push_kind(&parser.stack, Ast_Kind_else_leave);
        goto label_statement;
      }
      else {
        ast_push_kind(&parser.final, Ast_Kind_if_leave);
        goto label_infix_or_suffix;
      }
    } break;
    case Ast_Kind_if_value_do: {
      parse_expect_token(&parser, Token_Kind_do);
      ast_push_kind(&parser.final, Ast_Kind_if_enter);
      ast_push_kind(&parser.stack, Ast_Kind_if_value_else);
    } goto label_expression;
    case Ast_Kind_if_value_else: {
      parse_expect_token(&parser, Token_Kind_else);
      ast_push_kind(&parser.final, Ast_Kind_if_leave_else_enter);
      parse_stack_push_kind_with_final_length(&parser, Ast_Kind_else_value_leave);
    } goto label_expression;
    case Ast_Kind_expression: { label_expression:;
      Token token = tokens.base[parser.tok++];
      switch (token.kind) {
      case Token_Kind_minus_prefix: case Token_Kind_minus:
      case Token_Kind_plus_prefix:  case Token_Kind_plus:
      case Token_Kind_at_prefix:    case Token_Kind_at: {
        Ast_Kind kind = Ast_Flag_unary | ((Ast_Kind)token.kind & 0xff);
        ast_push_kind(&parser.stack, kind);
      } goto label_expression;
      case Token_Kind_int: case Token_Kind_name: {
        Ast_Node ast = { .kind = (Ast_Kind)token.kind, .value = token.value };
        ast_push(&parser.final, ast);
      } goto label_infix_or_suffix;
      case Token_Kind_if: {
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_if_value_do);
      } goto label_expression;
      case Token_Kind_while: {
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_while_do);
      } goto label_expression;
      case Token_Kind_paren_open: {
        Ast_Node paren_open = { Ast_Kind_paren_enter, {0} };
        Astid astid_open = ast_push(&parser.final, paren_open);
        Ast_Node paren_close = { .kind = Ast_Kind_paren_leave, .enter_at = astid_open, .last_at.index = parser.final.length };
        ast_push(&parser.stack, paren_close);
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_assign_lhs);
      } goto label_expression;
      case Token_Kind_curly_open: {
        Astid astid_enter = ast_push_kind(&parser.final, Ast_Kind_block_value_enter);
        Ast_Node   leave = { Ast_Kind_block_value_leave, .enter_at = astid_enter };
        ast_push(&parser.stack, leave);
      } goto label_statement;
      case Token_Kind_brace_open:
      case Token_Kind_brace_prefix_open: {
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_array_close);
      } goto label_expression;
      default:
        parser.tok--;
      break;
      }
    } break;
    case Ast_Kind_infix_or_suffix: { label_infix_or_suffix:;
      Token token = tokens.base[parser.tok++];
      switch (token.kind) {
      case Token_Kind_plus: case Token_Kind_minus:
      case Token_Kind_star: {
        Ast_Kind kind = (Ast_Kind)token.kind | Ast_Flag_binary;
        parse_stack_transfer_higher_precedence(&parser, kind);
        ast_push_kind(&parser.stack, kind);
      } goto label_expression;
      case Token_Kind_dot: {
        parse_stack_transfer_higher_precedence(&parser, Ast_Kind_access);
        ast_push_kind(&parser.final, Ast_Kind_access);
      } goto label_expression;
      case Token_Kind_arrow: {
        Ast_Node fun_enter = { Ast_Kind_fun_enter, {0} };
        parse_stack_transfer_higher_precedence(&parser, Ast_Kind_fun_enter);
        Ast_Node* top = ast_top(parser.stack);
        slice_ast_insert(&parser.final, top->last_at, fun_enter);
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_fun_leave);
      } goto label_expression;
      case Token_Kind_brace_open: {
        parse_stack_transfer_higher_precedence(&parser, Ast_Kind_subscript_close);
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_subscript_close);
      } goto label_expression;
      case Token_Kind_at: {
        parse_stack_transfer_higher_precedence(&parser, Ast_Kind_load);
        ast_push_kind(&parser.final, Ast_Kind_load);
      } goto label_infix_or_suffix;
      case Token_Kind_comma: {
        parse_stack_transfer_higher_precedence(&parser, Ast_Kind_tuple_enter);
        Ast_Node* top = ast_top(parser.stack);
        if (top->kind == Ast_Kind_tuple_leave) {
          Ast_Node* open = ast_at(parser.final, top->enter_at);
          open->length++;
        }
        else {
          Ast_Node tuple_enter = { Ast_Kind_tuple_enter, {.length = 1} };
          Astid tuple_enter_astid = slice_ast_insert(&parser.final, top->last_at, tuple_enter);
          Ast_Node tuple_leave = { Ast_Kind_tuple_leave, .enter_at = tuple_enter_astid };
          ast_push(&parser.stack, tuple_leave);
        }
      } goto label_expression;
      default: {
        parser.tok--;
        if ((token.kind & Token_Kind_Flag_call_rhs) && !(token.flag & Token_Flag_wasnewline)) {
          parse_stack_transfer_higher_precedence(&parser, Ast_Kind_call);
          ast_push_kind(&parser.stack, Ast_Kind_call);
          goto label_expression;
        }
      } break;
      }
    } break;
    case Ast_Kind_assign_lhs: {
      Ast_Node* top = ast_top(parser.final);
      if (parse_match_token(&parser, Token_Kind_equal)) {
        Ast_Node* top = ast_top(parser.stack);
        Ast_Node assign = { Ast_Kind_assign_rhs, {0} };
        if (top->kind == Ast_Kind_record_leave ||
            top->kind == Ast_Kind_paren_leave) {
          assign.kind = Ast_Kind_record_assign;
        }
        I32 final_mark = ast.last_at.index;
        I32 stack_mark_length = parse_transfer_final_to_stack_from(&parser, final_mark);
        assign.last_at.index = stack_mark_length;
        ast_push(&parser.stack, assign);
        goto label_expression;
      }
      else if (parse_match_token(&parser, Token_Kind_colon)) {
        Ast_Node* top = ast_top(parser.stack);
        Ast_Node declare = { Ast_Kind_declare_rhs, {0} };
        if (top->kind == Ast_Kind_record_leave ||
            top->kind == Ast_Kind_paren_leave) {
          declare.kind = Ast_Kind_record_declare;
        }
        I32 final_mark = ast.last_at.index;
        I32 stack_mark_length = parse_transfer_final_to_stack_from(&parser, final_mark);
        declare.last_at.index = stack_mark_length;
        ast_push(&parser.stack, declare);
        goto label_expression;
      }
      else if (top->kind == Ast_Kind_block_value_leave) {
        Ast_Node* enter = ast_at(parser.final, top->enter_at);
        enter->kind = Ast_Kind_block_enter;
        top->kind = Ast_Kind_block_leave;
      }
    } break;
    case Ast_Kind_record_declare:
    case Ast_Kind_declare_rhs:
    case Ast_Kind_record_assign:
    case Ast_Kind_assign_rhs: {
      ast_push(&parser.final, ast);
      parse_transfer_stack_to_final_from(&parser, ast.last_at.index);
    } break;
    case Ast_Kind_block_value_leave: {
      Ast_Node ast_close = ast;
      if (parse_match_token(&parser, Token_Kind_curly_close)) {
        Ast_Node* open = ast_at(parser.final, ast_close.enter_at);
        Astid astid_close = ast_push(&parser.final, ast_close);
        open->leave_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        parse_match_token(&parser, Token_Kind_semicolon);
        ast_push(&parser.stack, ast_close);
        goto label_statement;
      }
    } break;
    case Ast_Kind_source_leave: {
      Ast_Node ast_close = ast;
      if (parse_match_token(&parser, Token_Kind_source_leave)) {
        Ast_Node* open = ast_at(parser.final, ast_close.enter_at);
        Astid astid_close = ast_push(&parser.final, ast_close);
        open->leave_at = astid_close;
      }
      else {
        parse_match_token(&parser, Token_Kind_semicolon);
        ast_push(&parser.stack, ast_close);
        goto label_statement;
      }
    } break;
    case Ast_Kind_tuple_leave: {
      Ast_Node ast_close = ast;
      Ast_Node* open = ast_at(parser.final, ast_close.enter_at);
      Astid astid_close = ast_push(&parser.final, ast_close);
      if (astid_close.index != open->leave_at.index) {
        open->length++;
      }
      open->leave_at = astid_close;
    } break;
    case Ast_Kind_record_leave: {
      parse_match_token(&parser, Token_Kind_semicolon);
      Ast_Node ast_close = ast;
      Ast_Node* open = ast_at(parser.final, ast_close.enter_at);
      if (parse_match_token(&parser, Token_Kind_paren_close)) {
        open->length++;
        Astid astid_close = ast_push(&parser.final, ast_close);
        open->leave_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        ast_close.last_at.index = parser.final.length;
        open->length++;
        ast_push(&parser.stack, ast_close);
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_assign_lhs);
        goto label_expression;
      }
    } break;
    case Ast_Kind_paren_leave: {
      Ast_Node ast_close = ast;
      Ast_Node* open = ast_at(parser.final, ast_close.enter_at);
      if (parse_match_token(&parser, Token_Kind_paren_close)) {
        if (parser.final.length == ast_close.enter_at.index) {
          // empty record ()
          open->kind = Ast_Kind_record_enter;
          ast_close.kind = Ast_Kind_record_leave;
        }
        Astid astid_close = ast_push(&parser.final, ast_close);
        open->leave_at = astid_close;
        goto label_infix_or_suffix;
      }
      else {
        open->kind = Ast_Kind_record_enter;
        ast_close.kind = Ast_Kind_record_leave;
        ast_close.last_at.index = parser.final.length;
        ast_push(&parser.stack, ast_close);
        parse_stack_push_kind_with_final_length(&parser, Ast_Kind_assign_lhs);
        goto label_expression;
      }
    } break;
    case Ast_Kind_array_close: {
      parse_expect_token(&parser, Token_Kind_brace_close);
      Ast_Node array = { Ast_Kind_array, {0} };
      ast_push(&parser.stack, array);
    } goto label_expression;
    case Ast_Kind_subscript_close: {
      parse_expect_token(&parser, Token_Kind_brace_close);
      Ast_Node subscript = { Ast_Kind_subscript, {0} };
      ast_push(&parser.stack, subscript);
    } goto label_infix_or_suffix;
    default:
      ast_push(&parser.final, ast);
    break;
    }
  }
  free(parser.stack.nodes);
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
  free(ast.nodes);
  test_at_source(result, expected, file_name, line, source);
  free(buffer);
}

#define test(source, expected) _test_ast(source, expected, __FILE__, __LINE__)

void parse_test(void) {
  test("1 + 2",          "s{ 1 2 add }s ");
  test("1 + -2",         "s{ 1 2 neg add }s ");
  test("1 + 2*3",        "s{ 1 2 3 mul add }s ");
  test("foo 1\n2",       "s{ foo 1 call 2 }s ");
  test("bar 1 2",        "s{ bar 1 call 2 call }s ");
  test("a = 1-2",        "s{ 1 2 sub = a }s ");
  test("a, b = 1, 2",    "s{ t(2 1 2 )t = t(2 a b )t }s ");
  test("x=1; y:2",       "s{ 1 = x 2 : y }s ");
  test("a = (x=1; y:2)", "s{ r(2 1 r= x 2 r: y )r = a }s ");
}

#undef test
