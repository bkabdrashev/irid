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

typedef struct Ast_Node Ast_Node;
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
  };
};

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

Cstr cstr_from_ast(C8* buffer, Ast_Node* node) {
  String_Builder sb = string_builder_begin(buffer);
  switch (node->kind) {
  case Ast_Kind_none: {
    Cstr cstr = cstr_from_ast_kind(node->kind);
    string_builder_push_cstr(&sb, cstr);
  } break;
  case Ast_Kind_name:
    string_builder_push_istr(&sb, node->istr);
  break;
  case Ast_Kind_int:
    string_builder_push_i64(&sb, node->i64);
  break;
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

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

typedef struct {
  Ast final;

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

I32 parse_left_precedence(Ast_Kind kind) {
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
  case Ast_Kind_array:
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
  ast_print(parser->arena, parser->final);
  printf("ast stack: ");
  ast_print(parser->arena, parser->stack);
  printf("\n");
}

Ast_Node* parse_new_expression(Parser* parser, I32 precedence_to_beat);
Ast_Node* parse_statement(Parser* parser);

Ast_Node* parse_infix_or_suffix(Parser* parser, Ast_Node* lhs, I32 precedence_to_beat) {
  Ast_Node* node = lhs;
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
    Ast_Kind kind = (Ast_Kind)token.kind | Ast_Flag_binary;
    I32 precedence = parse_left_precedence(kind);
    if (precedence > precedence_to_beat) {
      I32 right_precedence = parse_right_precedence(kind);
      Ast_Node* rhs = parse_new_expression(parser, right_precedence);
      node = arena_push(parser->arena, sizeof(Ast_Node));
      node->kind = kind;
      node->binary.lhs = lhs;
      node->binary.rhs = rhs;
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
      node = arena_push(parser->arena, sizeof(Ast_Node));
      node->kind = kind;
      node->binary.lhs = lhs;
      node->binary.rhs = rhs;
    }
  } break;
  case Token_Kind_at: {
    Ast_Kind kind = Ast_Kind_load;
    I32 precedence = parse_left_precedence(kind);
    if (precedence > precedence_to_beat) {
      node = arena_push(parser->arena, sizeof(Ast_Node));
      node->kind = kind;
      node->unary = lhs;
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
        node = arena_push(parser->arena, sizeof(Ast_Node));
        node->kind = kind;
        node->binary.lhs = lhs;
        node->binary.rhs = rhs;
      }
    }
  } break;
  }
  return node;
}

Ast_Node* parse_prefix_or_atom(Parser* parser) {
  Ast_Node* node;
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_minus_prefix: case Token_Kind_minus:
  case Token_Kind_plus_prefix:  case Token_Kind_plus:
  case Token_Kind_at_prefix:    case Token_Kind_at: {
    Ast_Kind kind = Ast_Flag_unary | (((Ast_Kind)token.kind+1) & 0xff);
    I32 precedence = parse_right_precedence(kind);
    Ast_Node* unary = parse_new_expression(parser, precedence);
    node = arena_push(parser->arena, sizeof(Ast_Node));
    node->kind = kind;
    node->unary = unary;
  } break;
  case Token_Kind_int: case Token_Kind_name: {
    Ast_Kind kind = (Ast_Kind)token.kind & 0xff;
    Ast_Node node = { .kind = kind, .bits = token.value };
    // parse_final_push(parser, node);
  } break;
  case Token_Kind_if: {
    assert(0);
  } break;
  case Token_Kind_while: {
    assert(0);
  } break;
  case Token_Kind_paren_open: {
    assert(0);
    while (!parse_match_token(parser, Token_Kind_paren_close)) {
    }
  } break;
  case Token_Kind_curly_open: {
    assert(0);
  } break;
  case Token_Kind_brace_open:
  case Token_Kind_brace_prefix_open: {
    assert(0);
  } break;
  default:
    parser->tok--;
  break;
  }
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
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_source_enter: {
    // parse_final_push_kind(parser, Ast_Kind_source_enter);
    // parse_final_push_kind(parser, Ast_Kind_source_leave);
  } break;
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
    Ast_Node* node = parse_new_expression(parser, 0);
    if (parse_match_token(parser, Token_Kind_equal)) {
      assert(0);
    }
    else if (parse_match_token(parser, Token_Kind_colon)) {
      assert(0);
    }
  }
  }
}

Ast_Node* parse_tokens(Arena* arena, Tokens tokens) {
  Parser parser = {0};
  parser.arena  = arena;
  parser.tokens = tokens;
  parser.tok    = 0;
  while (!parse_match_token(&parser, Token_Kind_source_leave)) {
    Ast_Node* node = parse_statement(&parser);
    parse_match_token(&parser, Token_Kind_semicolon);
  }
  return node;
}

Ast ast_from_source(Arena* arena, Cstr source) {
  Tokens tokens  = lex_source(arena, source);
  Ast ast        = parse_tokens(arena, tokens);
  return ast;
}

void _test_ast(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Arena arena = arena_init(KB(4) * strlen(source));
  Ast ast = ast_from_source(&arena, source);
  Cstr result = cstr_from_ast(&arena, ast);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_ast(source, expected, __FILE__, __LINE__)

void parse_test(void) {
  test("1",              "s{ 1 ; }s ");
  return;

  test("1 * [2+3]b",     "s{ 1 2 3 add b a[] mul ; }s ");
  test("c+[1]b",         "s{ c 1 b a[] add ; }s ");
  test("c+b[1]",         "s{ c b 1 s[] add ; }s ");
  test("[1+2]b",         "s{ 1 2 add b a[] ; }s ");
  test("[1]b",           "s{ 1 b a[] ; }s ");
  test("b[1+2]",         "s{ b 1 2 add s[] ; }s ");
  test("b[1]",           "s{ b 1 s[] ; }s ");
  test("b[1+2]*c",       "s{ b 1 2 add s[] c mul ; }s ");
  test("1 + 2*3",        "s{ 1 2 3 mul add ; }s ");
  test("1",              "s{ 1 ; }s ");
  test("1 + 2",          "s{ 1 2 add ; }s ");
  test("1 + -2",         "s{ 1 2 neg add ; }s ");
  test("1*(2+3)",        "s{ 1 2 3 add mul ; }s ");
  test("(1 + 2)*3",      "s{ 1 2 add 3 mul ; }s ");
  test("foo 1\n2",       "s{ foo 1 call ; 2 ; }s ");
  test("bar 1 2",        "s{ bar 1 call 2 call ; }s ");
  test("(1)",            "s{ 1 ; }s ");
  test("(1\n)",          "s{ 1 ; }s ");
  test("1,2",            "s{ t( 1 , 2 , )t ; }s ");
  test("a = 1",          "s{ 1 a = ; }s ");
  test("a = 1 + 2",      "s{ 1 2 add a = ; }s ");
  test("a, b = 1",       "s{ 1 a( a , b , )a = ; }s ");
  test("a, b, c = 1",    "s{ 1 a( a , b , c , )a = ; }s ");
  test("a, b = 1, 2",    "s{ t( 1 , 2 , )t a( a , b , )a = ; }s ");
  test("a+b = 1-2",      "s{ 1 2 sub a b add = ; }s ");
  test("a, (b,c), d = 1", "s{ 1 a( a , a( b , c , )a , d , )a = ; }s ");
  test("b + (c,d), e = 1", "s{ 1 a( b t( c , d , )t add , e , )a = ; }s ");
  test("(1;)",           "s{ r( 1 ; )r ; }s ");
  test("(1\n 2)",        "s{ r( 1 ; 2 ; )r ; }s ");
  test("(1; 2)",         "s{ r( 1 ; 2 ; )r ; }s ");
  test("(1; 2;)",        "s{ r( 1 ; 2 ; )r ; }s ");
  test("(1,2;3)",        "s{ r( t( 1 , 2 , )t ; 3 ; )r ; }s ");
  test("wh 1 do 2",      "s{ 1 wh_do 2 hw ; }s ");
  test("(if 1 do 2)",      "s{ 1 if_do 2 vi ; }s ");
  test("(if 1 do 2 el 3)", "s{ 1 if_do 2 fi_el 3 ve ; }s ");
  test("if 1 do 2",      "s{ 1 if_do 2 fi ; }s ");
  test("if 1 do 2 el 3", "s{ 1 if_do 2 fi_el 3 le ; }s ");
  return;
}

#undef test
