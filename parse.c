typedef enum {
  Ast_Flag_none   = 0,
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
  Ast_Flag_list   = 1 << 10,
  Ast_Flag_enter = 1 << 11,
  Ast_Flag_leave = 1 << 12,
  Ast_Flag_split = 1 << 13,
} Ast_Flag;

typedef enum Ast_Kind {
  Ast_Kind_none      = 0,
  Ast_Kind_name      = Token_Kind_name & 0xff,
  Ast_Kind_int       = Token_Kind_int & 0xff,
  Ast_Kind_add       = Token_Kind_plus,
  Ast_Kind_sub       = Token_Kind_minus,
  Ast_Kind_mul       = Token_Kind_star,
  Ast_Kind_eq        = Token_Kind_equal_equal,
  Ast_Kind_ne        = Token_Kind_bang_equal,
  Ast_Kind_lt        = Token_Kind_less,
  Ast_Kind_le        = Token_Kind_less_equal,
  Ast_Kind_gt        = Token_Kind_greater,
  Ast_Kind_ge        = Token_Kind_greater_equal,
  Ast_Kind_neg       = Token_Kind_minus+1,
  Ast_Kind_pos       = Token_Kind_plus+1,
  Ast_Kind_ptr       = Token_Kind_at+1,
  Ast_Kind_load      = Token_Kind_at,
  Ast_Kind_dot       = Token_Kind_dot,
  Ast_Kind_join      = Token_Kind_backslash,
  Ast_Kind_subscript = Token_Kind_brace_open,
  Ast_Kind_array     = Token_Kind_brace_open + 1,
  Ast_Kind_assign    = Token_Kind_equal,
  Ast_Kind_declare   = Token_Kind_colon,
  Ast_Kind_block       = (Token_Kind_curly_open & 0xff),
  Ast_Kind_block_value = (Token_Kind_curly_open & 0xff) + 1,
  Ast_Kind_source      = Token_Kind_source_enter,
  Ast_Kind_tuple       = Token_Kind_comma,
  Ast_Kind_fun         = Token_Kind_arrow,
  Ast_Kind_if          = Token_Kind_if & 0xff,
  Ast_Kind_if_value    = (Token_Kind_if & 0xff) +1,
  Ast_Kind_else        = Token_Kind_else,
  Ast_Kind_else_value  = Token_Kind_else+1,
  Ast_Kind_return      = Token_Kind_return,
  Ast_Kind_break       = Token_Kind_break,
  Ast_Kind_while       = Token_Kind_while,
  Ast_Kind_record      = Token_Kind_paren_open & 0xff,
  Ast_Kind_call,
} Ast_Kind;

typedef struct {
  I32* base;
  I32 length;
} I32s;

typedef struct Scopes Scopes;
struct Scopes {
  Hash_Map* base;
  I32 length;
};

typedef struct Scope_Stack Scope_Stack;
struct Scope_Stack {
  Hash_Map** base;
  I32 length;
};

typedef struct Ast_Node Ast_Node;
typedef struct Ast Ast;
struct Ast {
  I32 length;
  Ast_Node* base[];
};

struct Ast_Node {
  Ast_Kind kind;
  union {
    U64   bits;
    I64   i64;
    Istr  istr;
    struct {
      Ast_Node* lhs;
      Ast_Node* rhs;
    } binary;
    Ast_Node* unary;
    Ast* list;
  };
};

Cstr cstr_from_ast_kind(Ast_Kind ast_kind) {
  Cstr result = "unknown";
  switch (ast_kind & 0xff) {
  case Ast_Kind_none:         result = "none"; break;
  case Ast_Kind_name:         result = "name"; break;
  case Ast_Kind_int:          result = "int"; break;
  case Ast_Kind_add:          result = "+"; break;
  case Ast_Kind_sub:          result = "sub"; break;
  case Ast_Kind_mul:          result = "*"; break;
  case Ast_Kind_neg:          result = "-"; break;
  case Ast_Kind_pos:          result = "pos"; break;
  case Ast_Kind_ptr:          result = "ptr"; break;
  case Ast_Kind_load:         result = "load"; break;
  case Ast_Kind_dot:          result = "dot"; break;
  case Ast_Kind_join:         result = "join"; break;
  case Ast_Kind_subscript:    result = "s[]"; break;
  case Ast_Kind_array:        result = "a[]"; break;
  case Ast_Kind_assign:       result = "="; break;
  case Ast_Kind_declare:      result = "declare"; break;
  case Ast_Kind_block:        result = "block"; break;
  case Ast_Kind_block_value:  result = "block_value"; break;
  case Ast_Kind_source:       result = "source"; break;
  case Ast_Kind_tuple:        result = "tuple"; break;
  case Ast_Kind_fun:          result = "fun"; break;
  case Ast_Kind_if:           result = "if"; break;
  case Ast_Kind_if_value:     result = "if_value"; break;
  case Ast_Kind_else:         result = "else"; break;
  case Ast_Kind_else_value:   result = "else_value"; break;
  case Ast_Kind_return:       result = "return"; break;
  case Ast_Kind_break:        result = "break"; break;
  case Ast_Kind_while:        result = "while"; break;
  case Ast_Kind_record:       result = "record"; break;
  case Ast_Kind_call:         result = "call"; break;
  }
  return result;
}

void string_builder_push_ast_node(String_Builder* sb, Ast_Node* node) {
  switch (node->kind) {
  case Ast_Kind_none: {
    Cstr cstr = cstr_from_ast_kind(node->kind);
    string_builder_push_cstr(sb, cstr);
  } break;
  case Ast_Kind_name:
    string_builder_push_istr(sb, node->istr);
  break;
  case Ast_Kind_int:
    string_builder_push_i64(sb, node->i64);
  break;
  case Ast_Kind_tuple:
    string_builder_push_cstr(sb, "(");
    for (I32 i = 0; i < node->list->length; i++) {
      string_builder_push_ast_node(sb, node->list->base[i]);
      if (i+1 < node->list->length) {
        string_builder_push_cstr(sb, ", ");
      }
    }
    string_builder_push_cstr(sb, ")");
  break;
  case Ast_Kind_record:
    string_builder_push_cstr(sb, "(");
    for (I32 i = 0; i < node->list->length; i++) {
      string_builder_push_ast_node(sb, node->list->base[i]);
      string_builder_push_cstr(sb, ";");
      if (i+1 < node->list->length) {
        string_builder_push_cstr(sb, " ");
      }
    }
    string_builder_push_cstr(sb, ")");
  break;
  case Ast_Kind_pos:
  case Ast_Kind_neg: {
    Cstr op_cstr = cstr_from_ast_kind(node->kind);
    string_builder_push_cstr(sb, op_cstr);
    string_builder_push_ast_node(sb, node->unary);
  } break;
  case Ast_Kind_add: case Ast_Kind_sub:
  case Ast_Kind_eq: case Ast_Kind_ne:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
  case Ast_Kind_mul: {
    string_builder_push_cstr(sb, "(");
    string_builder_push_ast_node(sb, node->binary.lhs);
    Cstr op_cstr = cstr_from_ast_kind(node->kind);
    string_builder_push_cstr(sb, " ");
    string_builder_push_cstr(sb, op_cstr);
    string_builder_push_cstr(sb, " ");
    string_builder_push_ast_node(sb, node->binary.rhs);
    string_builder_push_cstr(sb, ")");
  } break;
  case Ast_Kind_call: {
    string_builder_push_cstr(sb, "(");
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, " ");
    string_builder_push_ast_node(sb, node->binary.rhs);
    string_builder_push_cstr(sb, ")");
  } break;
  case Ast_Kind_array: {
    string_builder_push_cstr(sb, "[");
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, "]");
    string_builder_push_ast_node(sb, node->binary.rhs);
  } break;
  case Ast_Kind_subscript: {
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, "[");
    string_builder_push_ast_node(sb, node->binary.rhs);
    string_builder_push_cstr(sb, "]");
  } break;
  case Ast_Kind_assign: {
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, " = ");
    string_builder_push_ast_node(sb, node->binary.rhs);
  } break;
  }
}

Cstr cstr_from_ast(C8* buffer, Ast* ast) {
  String_Builder sb = string_builder_begin(buffer);
  for (I32 i = 0; i < ast->length; i++) {
    string_builder_push_ast_node(&sb, ast->base[i]);
    if (i+1 < ast->length) {
      string_builder_push_cstr(&sb, "; ");
    }
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

typedef struct {
  Arena* perm_arena;
  Arena* temp_arena;

  Ast* final;

  Scopes scopes;
  Scope_Stack scope_stack;

  Tokens tokens;
  I32  tok;
} Parser;

I32 parse_right_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_fun:
    return 1;
  case Ast_Kind_tuple:
    return 3;
  case Ast_Kind_eq:
    return 6;
  case Ast_Kind_join:
    return 8;
  case Ast_Kind_sub:
  case Ast_Kind_add:
    return 12;
  case Ast_Kind_mul:
    return 14;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:
    return 16;
  case Ast_Kind_load:
    return 18;
  case Ast_Kind_subscript:
  case Ast_Kind_call:
  case Ast_Kind_dot:
    return 20;
  default :
    return -1;
  }
}

I32 parse_left_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_fun:
    return 1;
  case Ast_Kind_tuple:
    return 3;
  case Ast_Kind_eq:
    return 5;
  case Ast_Kind_join:
    return 7;
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
  case Ast_Kind_dot:
    return 19;
  default :
    return -1;
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

void parse_list_end(Parser* parser, Ast* list) {
  arena_release_mark(parser->temp_arena, list);
}

void parse_list_push(Parser* parser, Ast* list, Ast_Node* node) {
  arena_push(parser->temp_arena, sizeof(Ast_Node));
  list->base[list->length++] = node;
}

Ast* parse_list_temp(Parser* parser) {
  return arena_push_zero(parser->temp_arena, sizeof(Ast));
}

Ast* parse_list_perm(Parser* parser, Ast* temp_list) {
  I32 size = sizeof(Ast) + temp_list->length * sizeof(Ast_Node);
  Ast* perm_list = arena_push(parser->perm_arena, size);
  memcpy(perm_list, temp_list, size);
  arena_release_mark(parser->temp_arena, temp_list);
  return perm_list;
}

Ast_Node* parse_new_expression(Parser* parser, I32 precedence_to_beat);
Ast_Node* parse_statement(Parser* parser);

Ast_Node* parse_tuple_or_expression(Parser* parser) {
  Ast_Node* node = arena_push(parser->perm_arena, sizeof(Ast_Node));
  Ast* temp = parse_list_temp(parser);
  do {
    Ast_Node* exp = parse_new_expression(parser, 0);
    parse_list_push(parser, temp, exp);
  } while (parse_match_token(parser, Token_Kind_comma));
  if (temp->length == 1) {
    node = temp->base[0];
    parse_list_end(parser, temp);
  }
  else {
    node->kind = Ast_Kind_tuple;
    node->list = parse_list_perm(parser, temp);
  }
  return node;
}

Ast_Node* parse_infix_or_suffix(Parser* parser, Ast_Node* lhs, I32 precedence_to_beat) {
  Ast_Node* node = lhs;
  while (true) {
    Token token = parser->tokens.base[parser->tok++];
    switch (token.kind) {
    case Token_Kind_bang_equal:
    case Token_Kind_equal_equal:
    case Token_Kind_less_equal:
    case Token_Kind_less:
    case Token_Kind_greater_equal:
    case Token_Kind_greater:
    case Token_Kind_backslash:
    case Token_Kind_plus: case Token_Kind_minus:
    case Token_Kind_star: case Token_Kind_dot: {
      Ast_Kind kind = (Ast_Kind)token.kind;
      I32 precedence = parse_left_precedence(kind);
      if (precedence > precedence_to_beat) {
        I32 right_precedence = parse_right_precedence(kind);
        Ast_Node* rhs = parse_new_expression(parser, right_precedence);
        lhs = node;
        node = arena_push(parser->perm_arena, sizeof(Ast_Node));
        node->kind = kind;
        node->binary.lhs = lhs;
        node->binary.rhs = rhs;
      }
      else {
        parser->tok--;
        return node;
      }
    } break;
    case Token_Kind_arrow: {
      assert(0);
    } break;
    case Token_Kind_brace_open: {
      Ast_Kind kind = Ast_Kind_subscript;
      I32 precedence = parse_left_precedence(kind);
      if (precedence > precedence_to_beat) {
        Ast_Node* rhs = parse_new_expression(parser, 0);
        parse_expect_token(parser, Token_Kind_brace_close);
        lhs = node;
        node = arena_push(parser->perm_arena, sizeof(Ast_Node));
        node->kind = kind;
        node->binary.lhs = lhs;
        node->binary.rhs = rhs;
      }
      else {
        parser->tok--;
        return node;
      }
    } break;
    case Token_Kind_at: {
      Ast_Kind kind = Ast_Kind_load;
      I32 precedence = parse_left_precedence(kind);
      if (precedence > precedence_to_beat) {
        lhs = node;
        node = arena_push(parser->perm_arena, sizeof(Ast_Node));
        node->kind = kind;
        node->unary = lhs;
      }
      else {
        parser->tok--;
        return node;
      }
    } break;
    default: {
      parser->tok--;
      if ((token.kind & Token_Kind_Flag_call_rhs) && !(token.flag & Token_Flag_wasnewline)) {
        Ast_Kind kind = Ast_Kind_call;
        I32 precedence = parse_left_precedence(kind);
        if (precedence > precedence_to_beat) {
          I32 right_precedence = parse_right_precedence(kind);
          Ast_Node* rhs = parse_new_expression(parser, right_precedence);
          lhs = node;
          node = arena_push(parser->perm_arena, sizeof(Ast_Node));
          node->kind = kind;
          node->binary.lhs = lhs;
          node->binary.rhs = rhs;
        }
        else {
          return node;
        }
      }
      else {
        return node;
      }
    } break;
    }
  }
}

Ast_Node* parse_prefix_or_atom(Parser* parser) {
  Ast_Node* node;
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_minus_prefix: case Token_Kind_minus:
  case Token_Kind_plus_prefix:  case Token_Kind_plus:
  case Token_Kind_at_prefix:    case Token_Kind_at: {
    Ast_Kind kind = ((Ast_Kind)token.kind+1) & 0xff;
    I32 precedence = parse_right_precedence(kind);
    Ast_Node* unary = parse_new_expression(parser, precedence);
    node = arena_push(parser->perm_arena, sizeof(Ast_Node));
    node->kind = kind;
    node->unary = unary;
  } break;
  case Token_Kind_int: case Token_Kind_name: {
    Ast_Kind kind = (Ast_Kind)token.kind & 0xff;
    node = arena_push(parser->perm_arena, sizeof(Ast_Node));
    node->kind = kind;
    node->bits = token.bits;
    // parse_final_push(parser, node);
  } break;
  case Token_Kind_if: {
    assert(0);
  } break;
  case Token_Kind_while: {
    assert(0);
  } break;
  case Token_Kind_paren_open: {
    node = arena_push(parser->perm_arena, sizeof(Ast_Node));
    Ast* temp = parse_list_temp(parser);
    while (!parse_match_token(parser, Token_Kind_paren_close)) {
      Ast_Node* exp = parse_tuple_or_expression(parser);
      if (parse_match_token(parser, Token_Kind_semicolon)) {
        node->kind = Ast_Kind_record;
      }
      else if (parse_match_token(parser, Token_Kind_equal)) {
        node->kind = Ast_Kind_record;
      }
      parse_list_push(parser, temp, exp);
    }
    if (node->kind != Ast_Kind_record && temp->length == 1) {
      node = temp->base[0];
      parse_list_end(parser, temp);
    }
    else {
      node->kind = Ast_Kind_record;
      node->list = parse_list_perm(parser, temp);
    }
  } break;
  case Token_Kind_curly_open: {
    assert(0);
  } break;
  case Token_Kind_brace_open:
  case Token_Kind_brace_prefix_open: {
    Ast_Kind kind = Ast_Kind_array;
    Ast_Node* lhs = parse_new_expression(parser, 0);
    parse_expect_token(parser, Token_Kind_brace_close);
    I32 right_precedence = parse_right_precedence(kind);
    Ast_Node* rhs = parse_new_expression(parser, right_precedence);
    node = arena_push(parser->perm_arena, sizeof(Ast_Node));
    node->kind = kind;
    node->binary.lhs = lhs;
    node->binary.rhs = rhs;
  } break;
  default:
    parser->tok--;
  break;
  }
  return node;
}

Ast_Node* parse_new_expression(Parser* parser, I32 precedence_to_beat) {
  Ast_Node* prefix_or_atom = parse_prefix_or_atom(parser);
  Ast_Node* node = parse_infix_or_suffix(parser, prefix_or_atom, precedence_to_beat);
  return node;
}

void parse_indented_block(Parser* parser, I16 indent) {
  while (true) {
    parse_statement(parser);
    Token next_token = get(parser->tokens, parser->tok);
    if (next_token.indent <= indent) {
      break;
    }
  }
}

Ast_Node* parse_statement(Parser* parser) {
  Ast_Node* node;
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_if: {
    assert(0);
  } break;
  case Token_Kind_while: {
    assert(0);
  } break;
  // case Token_Kind_for:
  case Token_Kind_return: {
    assert(0);
    if (token.flag & Token_Flag_willnewline) {
    }
    else {
    }
  } break;
  case Token_Kind_break: {
    assert(0);
    if (token.flag & Token_Flag_willnewline) {
    }
    else {
    }
  } break;
  default: {
    parser->tok--;
    node = parse_tuple_or_expression(parser);
    if (parse_match_token(parser, Token_Kind_equal)) {
      Ast_Node* lhs = node;
      Ast_Node* rhs = parse_tuple_or_expression(parser);
      node = arena_push(parser->perm_arena, sizeof(Ast_Node));
      node->kind = Ast_Kind_assign;
      node->binary.lhs = lhs;
      node->binary.rhs = rhs;
    }
    else if (parse_match_token(parser, Token_Kind_colon)) {
      assert(0);
    }
  }
  }
  return node;
}

Ast* parse_tokens(Arena* perm_arena, Tokens tokens) {
  Arena temp_arena = arena_init(KB(4) * perm_arena->capacity);
  Parser parser = {0};
  parser.perm_arena = perm_arena;
  parser.temp_arena = &temp_arena;
  parser.tokens = tokens;
  parser.tok    = 1;
  Ast* temp_ast = parse_list_temp(&parser);
  while (!parse_match_token(&parser, Token_Kind_source_leave)) {
    Ast_Node* node = parse_statement(&parser);
    parse_match_token(&parser, Token_Kind_semicolon);
    parse_list_push(&parser, temp_ast, node);
  }
  parser.final = parse_list_perm(&parser, temp_ast);
  arena_free(&temp_arena);
  return parser.final;
}

void _test_ast(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  I32 source_length = strlen(source) + 2;
  Arena arena   = arena_init(KB(4) * source_length);
  Tokens tokens = lex_source(&arena, source);
  Ast* ast      = parse_tokens(&arena, tokens);
  C8* buffer    = arena_push(&arena, 4*source_length);
  Cstr result   = cstr_from_ast(buffer, ast);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_ast(source, expected, __FILE__, __LINE__)

void parse_test(void) {
  test("(1\n 2)",        "(1; 2;)");
  test("a, (b,c), d = 1", "(a, (b, c), d) = 1");
  test("a = 1",          "a = 1");
  test("a = 1 + 2",      "a = (1 + 2)");
  test("a, b = 1",       "(a, b) = 1");
  test("a = 1, 2",       "a = (1, 2)");
  test("a+b = 1*2",      "(a + b) = (1 * 2)");

  test("1,2",            "(1, 2)");
  test("(1,2;3)",        "((1, 2); 3;)");

  test("c+b[1]",         "(c + b[1])");
  test("b[1+2]",         "b[(1 + 2)]");
  test("b[1]",           "b[1]");
  test("b[1+2]*c",       "(b[(1 + 2)] * c)");
  test("[1]b",           "[1]b");
  test("c+[1]b",         "(c + [1]b)");
  test("1 * [2+3]b",     "(1 * [(2 + 3)]b)");
  test("[1+2]b",         "[(1 + 2)]b");

  test("(1)",            "1");
  test("(1\n)",          "1");
  test("(1;)",           "(1;)");
  test("(1; 2)",         "(1; 2;)");
  test("(1; 2;)",        "(1; 2;)");

  test("1*2+3",        "((1 * 2) + 3)");
  test("1 + -2",         "(1 + -2)");
  test("1 + 2*3",        "(1 + (2 * 3))");
  test("1",              "1");
  test("1 + 2",          "(1 + 2)");
  test("1*(2+3)",        "(1 * (2 + 3))");
  test("(1 + 2)*3",      "((1 + 2) * 3)");

  test("foo 1\n2",       "(foo 1); 2");
  test("bar 1 2",        "((bar 1) 2)");
  return;

  test("wh 1 do 2",      "s{ 1 wh_do 2 hw ; }s ");
  test("(if 1 do 2)",      "s{ 1 if_do 2 vi ; }s ");
  test("(if 1 do 2 el 3)", "s{ 1 if_do 2 fi_el 3 ve ; }s ");
  test("if 1 do 2",      "s{ 1 if_do 2 fi ; }s ");
  test("if 1 do 2 el 3", "s{ 1 if_do 2 fi_el 3 le ; }s ");
  return;
}

#undef test
