typedef enum {
  Ast_Flag_none   = 0,
  Ast_Flag_unary  = 1 << 8,
  Ast_Flag_binary = 1 << 9,
  Ast_Flag_list   = 1 << 10,
  Ast_Flag_value = 1 << 11,
} Ast_Flag;

typedef enum Ast_Kind {
  Ast_Kind_none      = 0,
  Ast_Kind_name      = Token_Kind_name & 0xff,
  Ast_Kind_int       = Token_Kind_int & 0xff,
  Ast_Kind_float     = Token_Kind_float & 0xff,
  Ast_Kind_str       = Token_Kind_str & 0xff,

  Ast_Kind_add       = Token_Kind_plus,
  Ast_Kind_sub       = Token_Kind_minus,
  Ast_Kind_mul       = Token_Kind_star,
  Ast_Kind_div       = Token_Kind_slash,
  Ast_Kind_rem       = Token_Kind_percent,

  Ast_Kind_or        = Token_Kind_pipe,
  Ast_Kind_xor       = Token_Kind_pike,
  Ast_Kind_and       = Token_Kind_ampersand,

  Ast_Kind_eq        = Token_Kind_equal_equal,
  Ast_Kind_ne        = Token_Kind_bang_equal,
  Ast_Kind_lt        = Token_Kind_less,
  Ast_Kind_le        = Token_Kind_less_equal,
  Ast_Kind_gt        = Token_Kind_greater,
  Ast_Kind_ge        = Token_Kind_greater_equal,

  Ast_Kind_neg       = Token_Kind_minus+1,
  Ast_Kind_pos       = Token_Kind_plus+1,

  Ast_Kind_ptr       = Token_Kind_at+1,
  Ast_Kind_run       = (Token_Kind_sharp & 0xff) + 1,
  Ast_Kind_load      = Token_Kind_at,
  Ast_Kind_dot       = Token_Kind_dot,
  Ast_Kind_range     = Token_Kind_dot_dot,
  Ast_Kind_join      = Token_Kind_backslash,
  Ast_Kind_meet      = Token_Kind_comma,
  Ast_Kind_subscript = Token_Kind_brace_open,
  Ast_Kind_array     = Token_Kind_brace_open + 1,
  Ast_Kind_assign    = Token_Kind_equal,
  Ast_Kind_declare   = Token_Kind_colon,
  Ast_Kind_block       = (Token_Kind_curly_open & 0xff),
  Ast_Kind_block_value = (Token_Kind_curly_open & 0xff) + 1,
  Ast_Kind_fun         = Token_Kind_arrow,
  Ast_Kind_if          = Token_Kind_if & 0xff,
  Ast_Kind_if_value    = (Token_Kind_if & 0xff) | Ast_Flag_value,
  Ast_Kind_else        = Token_Kind_else,
  Ast_Kind_else_value  = Token_Kind_else | Ast_Flag_value,
  Ast_Kind_return      = Token_Kind_return,
  Ast_Kind_return_value= Token_Kind_return | Ast_Flag_value,
  Ast_Kind_break       = Token_Kind_break,
  Ast_Kind_break_value = Token_Kind_break | Ast_Flag_value,
  Ast_Kind_while       = Token_Kind_while,
  Ast_Kind_record      = Token_Kind_paren_open & 0xff,
  Ast_Kind_call        = 100,
  Ast_Kind_iblock,
  Ast_Kind_span,
  Ast_Kind_error,
} Ast_Kind;

typedef struct Scope_Stack Scope_Stack;
struct Scope_Stack {
  Hash_Map** base;
  I32 length;
};

typedef struct Ir Ir;
typedef struct Ast_Node Ast_Node;
typedef struct Ast_List Ast_List;
struct Ast_List {
  I32 length;
  Ast_Node* base[];
};

typedef enum {
  Symbol_Kind_none      = 0,
  Symbol_Kind_constant  = 1,
  Symbol_Kind_variable  = 2,
  Symbol_Kind_parameter = 3,
} Symbol_Kind;

typedef struct Var Var;
typedef struct Symbol Symbol;
struct Symbol {
  Symbol_Kind kind;
  Ast_Node* ast;
  Ir* ir;
};

typedef struct Ast_Block Ast_Block;
struct Ast_Block {
  Hash_Map* scope;
  Ast_List* list;
};
typedef struct Ast_Named Ast_Named;
struct Ast_Named {
  Str* name;
  Ast_Node* node;
};

struct Ast_Node {
  Ast_Kind kind;
  Loc      loc;
  union {
    U64   bits;
    I64   i64;
    F64   f64;
    Str*  str;
    Ast_Node* unary;
    Ast_List* list;
    struct {
      Ast_Node* lhs;
      Ast_Node* rhs;
    } binary;
    Ast_Named declare;
    Ast_Block block;
  };
};

typedef struct Ast_Decl Ast_Decl;
struct Ast_Decl {
  Str*      name;
  Ast_Node* rhs;
  Loc       loc;
};

typedef struct Ast_Decls Ast_Decls;
struct Ast_Decls {
  I32 length;
  Ast_Decl base[];
};

Ast_Node* ast_new_node(Arena* arena, Ast_Kind kind) {
  Ast_Node* node = arena_push_zero(arena, sizeof(Ast_Node));
  node->kind = kind;
  return node;
}

Ast_Node* ast_new_error(Arena* arena, Loc loc) {
  Ast_Node* node = ast_new_node(arena, Ast_Kind_error);
  node->loc = loc;
  return node;
}

Cstr cstr_from_ast_kind(Ast_Kind ast_kind) {
  Cstr result = "unknown";
  switch (ast_kind & 0xff) {
  case Ast_Kind_none:         result = "none"; break;
  case Ast_Kind_name:         result = "name"; break;
  case Ast_Kind_int:          result = "int"; break;
  case Ast_Kind_float:        result = "float"; break;
  case Ast_Kind_add:          result = "+"; break;
  case Ast_Kind_sub:          result = "-"; break;
  case Ast_Kind_mul:          result = "*"; break;
  case Ast_Kind_div:          result = "/"; break;
  case Ast_Kind_rem:          result = "%"; break;
  case Ast_Kind_neg:          result = "-"; break;
  case Ast_Kind_pos:          result = "+"; break;
  case Ast_Kind_or:           result = "|"; break;
  case Ast_Kind_xor:          result = "^"; break;
  case Ast_Kind_and:          result = "&"; break;
  case Ast_Kind_eq:           result = "=="; break;
  case Ast_Kind_ne:           result = "!="; break;
  case Ast_Kind_lt:           result = "<"; break;
  case Ast_Kind_le:           result = "<="; break;
  case Ast_Kind_gt:           result = ">"; break;
  case Ast_Kind_ge:           result = ">="; break;
  case Ast_Kind_ptr:          result = "@"; break;
  case Ast_Kind_load:         result = "@"; break;
  case Ast_Kind_dot:          result = "."; break;
  case Ast_Kind_range:        result = ".."; break;
  case Ast_Kind_meet:         result = ","; break;
  case Ast_Kind_join:         result = "\\"; break;
  case Ast_Kind_subscript:    result = "s[]"; break;
  case Ast_Kind_array:        result = "a[]"; break;
  case Ast_Kind_assign:       result = "="; break;
  case Ast_Kind_declare:      result = ":"; break;
  case Ast_Kind_block:        result = "block"; break;
  case Ast_Kind_block_value:  result = "block_value"; break;
  case Ast_Kind_fun:          result = "->"; break;
  case Ast_Kind_if:           result = "if"; break;
  case Ast_Kind_if_value:     result = "if_value"; break;
  case Ast_Kind_else:         result = "else"; break;
  case Ast_Kind_else_value:   result = "else_value"; break;
  case Ast_Kind_return:       result = "return"; break;
  case Ast_Kind_return_value: result = "ret_value"; break;
  case Ast_Kind_break:        result = "break"; break;
  case Ast_Kind_while:        result = "while"; break;
  case Ast_Kind_record:       result = "record"; break;
  case Ast_Kind_call:         result = "call"; break;
  case Ast_Kind_error:        result = "error"; break;
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
    string_builder_push_str(sb, node->str);
  break;
  case Ast_Kind_int:
    string_builder_push_i64(sb, node->i64);
  break;
  case Ast_Kind_float:
    string_builder_push_f64(sb, node->f64);
  break;
  case Ast_Kind_str:
    string_builder_push_cstr(sb, "\"");
    string_builder_push_str(sb, node->str);
    string_builder_push_cstr(sb, "\"");
  break;
  case Ast_Kind_while: {
    string_builder_push_cstr(sb, "while ");
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, " do ");
    string_builder_push_ast_node(sb, node->binary.rhs);
  } break;
  case Ast_Kind_if_value: case Ast_Kind_if: {
    string_builder_push_cstr(sb, "if ");
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, " do ");
    string_builder_push_ast_node(sb, node->binary.rhs);
  } break;
  case Ast_Kind_else_value: case Ast_Kind_else: {
    string_builder_push_ast_node(sb, node->binary.lhs);
    string_builder_push_cstr(sb, " else ");
    string_builder_push_ast_node(sb, node->binary.rhs);
  } break;
  case Ast_Kind_block_value: case Ast_Kind_block: {
    string_builder_push_cstr(sb, "{");
    Hash_Map* scope = node->block.scope;
    for (I32 i = 0; i < scope->len; i++) {
      Str* str = scope->list[i];
      string_builder_push_str(sb, str);
      string_builder_push_cstr(sb, " : ");
      Symbol* sym = hash_map_get(scope, str);
      string_builder_push_ast_node(sb, sym->ast);
      string_builder_push_cstr(sb, "; ");
    }
    Ast_List* list = node->block.list;
    for (I32 i = 0; i < list->length; i++) {
      string_builder_push_ast_node(sb, list->base[i]);
      string_builder_push_cstr(sb, ";");
      if (i+1 < list->length) {
        string_builder_push_cstr(sb, " ");
      }
    }
    string_builder_push_cstr(sb, "}");
  } break;
  case Ast_Kind_iblock:
    string_builder_push_cstr(sb, "{");
    for (I32 i = 0; i < node->list->length; i++) {
      string_builder_push_ast_node(sb, node->list->base[i]);
      string_builder_push_cstr(sb, ";");
      if (i+1 < node->list->length) {
        string_builder_push_cstr(sb, " ");
      }
    }
    string_builder_push_cstr(sb, "}");
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
  case Ast_Kind_return:
    string_builder_push_cstr(sb, "return");
  break;
  case Ast_Kind_return_value:
    string_builder_push_cstr(sb, "return ");
    string_builder_push_ast_node(sb, node->unary);
  break;
  case Ast_Kind_break:
    string_builder_push_cstr(sb, "break");
  break;
  case Ast_Kind_break_value:
    string_builder_push_cstr(sb, "break ");
    string_builder_push_ast_node(sb, node->unary);
  break;
  case Ast_Kind_ptr:
  case Ast_Kind_pos:
  case Ast_Kind_neg: {
    Cstr op_cstr = cstr_from_ast_kind(node->kind);
    string_builder_push_cstr(sb, op_cstr);
    string_builder_push_ast_node(sb, node->unary);
  } break;
  case Ast_Kind_load: {
    Cstr op_cstr = cstr_from_ast_kind(node->kind);
    string_builder_push_ast_node(sb, node->unary);
    string_builder_push_cstr(sb, op_cstr);
  } break;
  case Ast_Kind_or: case Ast_Kind_xor: case Ast_Kind_and:
  case Ast_Kind_add: case Ast_Kind_sub:
  case Ast_Kind_eq: case Ast_Kind_ne:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
  case Ast_Kind_div: case Ast_Kind_rem:
  case Ast_Kind_mul:
  case Ast_Kind_dot:
  case Ast_Kind_fun:
  case Ast_Kind_meet: case Ast_Kind_join: case Ast_Kind_range:
    {
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
  case Ast_Kind_span: {
    string_builder_push_cstr(sb, "[]");
    string_builder_push_ast_node(sb, node->unary);
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
  case Ast_Kind_declare: {
    string_builder_push_str(sb, node->declare.name);
    string_builder_push_cstr(sb, ":");
    string_builder_push_ast_node(sb, node->declare.node);
  } break;
  case Ast_Kind_run: {
    string_builder_push_cstr(sb, "(");
    string_builder_push_cstr(sb, "# ");
    string_builder_push_ast_node(sb, node->unary);
    string_builder_push_cstr(sb, ")");
  } break;
  case Ast_Kind_error: {
    string_builder_push_cstr(sb, "<error>");
  } break;
  }
}

Cstr cstr_from_ast(C8* buffer, Ast_Block ast) {
  String_Builder sb = string_builder_begin(buffer);
  Hash_Map* scope = ast.scope;
  for (I32 i = 0; i < scope->len; i++) {
    Str* str = scope->list[i];
    string_builder_push_str(&sb, str);
    string_builder_push_cstr(&sb, " : ");
    Symbol* sym = hash_map_get(scope, str);
    string_builder_push_ast_node(&sb, sym->ast);
    string_builder_push_cstr(&sb, "; ");
  }

  Ast_List* list = ast.list;
  for (I32 i = 0; i < list->length; i++) {
    string_builder_push_ast_node(&sb, list->base[i]);
    if (i+1 < list->length) {
      string_builder_push_cstr(&sb, "; ");
    }
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

typedef struct {
  Arena* perm_arena;
  Arena* list_arena;
  Arena* decls_arena;

  Tokens tokens;
  I32  tok;

  Diags* diags;
  I32    last_error_tok;
  B8     panic;
} Parser;

I32 parse_right_precedence(Ast_Kind kind, I32 token_flags) {
  I32 result = 0;
  switch (kind) {
  case Ast_Kind_fun:
    result = 3;
  break;
  case Ast_Kind_meet:
    result = 6;
  break;
  case Ast_Kind_or:
    result = 8;
  break;
  case Ast_Kind_xor:
    result = 10;
  break;
  case Ast_Kind_and:
    result = 12;
  break;
  case Ast_Kind_ne: case Ast_Kind_eq:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
    result = 14;
  break;
  case Ast_Kind_join:
  case Ast_Kind_range:
    result = 16;
  break;
  case Ast_Kind_sub:
  case Ast_Kind_add:
    result = 20;
  break;
  case Ast_Kind_mul:
  case Ast_Kind_div: case Ast_Kind_rem:
    result = 22;
  break;
  case Ast_Kind_ptr:
  case Ast_Kind_neg:
  case Ast_Kind_pos:
  case Ast_Kind_array:
  case Ast_Kind_span:
    result = 24;
  break;
  case Ast_Kind_load:
    result = 26;
  break;
  case Ast_Kind_subscript:
  case Ast_Kind_run:
  case Ast_Kind_call:
  case Ast_Kind_dot:
    result = 28;
  break;
  default :
    result = 0;
  break;
  }
  if (!has_flag(token_flags, Token_Flag_willspace)) {
    result *= 32;
  }
  return result;
}

I32 parse_left_precedence(Ast_Kind kind, I32 token_flags) {
  I32 result = 0;
  switch (kind) {
  case Ast_Kind_fun:
    result = 3;
  break;
  case Ast_Kind_meet:
    result = 5;
  break;
  case Ast_Kind_or:
    result = 7;
  break;
  case Ast_Kind_xor:
    result = 9;
  break;
  case Ast_Kind_and:
    result = 11;
  break;
  case Ast_Kind_ne: case Ast_Kind_eq:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
    result = 13;
  break;
  case Ast_Kind_join:
  case Ast_Kind_range:
    result = 15;
  break;
  case Ast_Kind_sub:
  case Ast_Kind_add:
    result = 19;
  break;
  case Ast_Kind_mul:
  case Ast_Kind_div: case Ast_Kind_rem:
    result = 21;
  break;
  case Ast_Kind_load:
    result = 25;
  break;
  case Ast_Kind_subscript:
  case Ast_Kind_run:
  case Ast_Kind_call:
  case Ast_Kind_dot:
    result = 27;
  break;
  default :
    result = 0;
  break;
  }
  if (!has_flag(token_flags, Token_Flag_wasspace)) {
    result *= 32;
  }
  return result;
}

Token* parse_peek(Parser* parser) {
  if (parser->tokens.length == 0) {
    static Token eof = { .kind = Token_Kind_source_leave };
    return &eof;
  }
  if (parser->tok < 1) parser->tok = 1;
  if ((Umi)parser->tok >= parser->tokens.length) {
    parser->tok = parser->tokens.length - 1;
  }
  return &parser->tokens.base[parser->tok];
}

B8 parse_at_end(Parser* parser) {
  return parse_peek(parser)->kind == Token_Kind_source_leave;
}

void parse_advance(Parser* parser) {
  if (!parse_at_end(parser)) {
    parser->tok++;
  }
}

void parse_retreat(Parser* parser) {
  if (parser->tok > 1) {
    parser->tok--;
  }
}

B8 parse_match_token(Parser* parser, Token_Kind kind) {
  Token token = *parse_peek(parser);
  if (token.kind == kind) {
    parse_advance(parser);
    return true;
  }
  return false;
}

B8 parse_is_token(Parser* parser, Token_Kind kind) {
  Token token = *parse_peek(parser);
  if (token.kind == kind) {
    return true;
  }
  return false;
}

B8 parse_diags_blocked(Parser* parser) {
  return !parser->diags || parser->diags->limit_hit;
}

Diag* parse_diag(Parser* parser, Diag_Code code, Loc loc) {
  if (parser->panic || parser->last_error_tok == parser->tok
      || parse_diags_blocked(parser)) {
    return NULL;
  }
  parser->last_error_tok = parser->tok;
  return diag_error(parser->diags, code, loc);
}

B8 parse_expected(Parser* parser, Token at, Token_Kind expected) {
  Diag* diag = parse_diag(parser, Diag_Code_expected_token, at.loc);
  if (diag) {
    diag->data.expected.expected = expected;
    diag->data.expected.found    = at.kind;
  }
  parser->panic = true;
  return false;
}

B8 parse_expected_expression(Parser* parser, Token at) {
  Diag* diag = parse_diag(parser, Diag_Code_expected_expression, at.loc);
  if (diag) {
    diag->data.expected.found = at.kind;
  }
  parser->panic = true;
  return false;
}

void parse_expected_expression_eol(Parser* parser, Loc loc) {
  Diag* diag = parse_diag(parser, Diag_Code_expected_expression, loc);
  if (diag) {
    diag->data.expected.end_of_line = true;
    parser->last_error_tok = parser->tok - 1;
  }
  parser->panic = true;
}

B8 parse_expression_is_available(Parser* parser) {
  Token next = *parse_peek(parser);
  if (next.kind == Token_Kind_source_leave) return false;
  return true;
}

void parse_unexpected(Parser* parser, Token at) {
  Diag* diag = parse_diag(parser, Diag_Code_unexpected_token, at.loc);
  if (diag) {
    diag->data.expected.found = at.kind;
  }
}

void parse_unmatched(Parser* parser, Token at) {
  Diag* diag = parse_diag(parser, Diag_Code_unmatched_delimiter, at.loc);
  if (diag) {
    diag->data.delimiter.open = at.kind;
  }
}

void parse_unclosed(Parser* parser, Token open, Token_Kind close, Loc at) {
  Diag* diag = parse_diag(parser, Diag_Code_unclosed_delimiter, at);
  if (!diag) return;
  diag->data.delimiter.open  = open.kind;
  diag->data.delimiter.close = close;
  I32 parent = (I32)(diag - parser->diags->base);
  Diag* note = diag_note(parser->diags, parent, Diag_Code_opened_here, open.loc);
  if (note) {
    note->data.delimiter.open = open.kind;
  }
}

B8 parse_token_starts_statement(Token_Kind kind) {
  switch (kind) {
  case Token_Kind_source_leave:
  case Token_Kind_semicolon:
  case Token_Kind_comma:
  case Token_Kind_colon:
  case Token_Kind_paren_close:
  case Token_Kind_brace_close:
  case Token_Kind_curly_close:
  case Token_Kind_equal:
  case Token_Kind_equal_equal:
  case Token_Kind_bang_equal:
  case Token_Kind_less:
  case Token_Kind_less_equal:
  case Token_Kind_greater:
  case Token_Kind_greater_equal:
  case Token_Kind_plus:
  case Token_Kind_minus:
  case Token_Kind_star:
  case Token_Kind_slash:
  case Token_Kind_percent:
  case Token_Kind_pipe:
  case Token_Kind_pike:
  case Token_Kind_ampersand:
  case Token_Kind_dot:
  case Token_Kind_dot_dot:
  case Token_Kind_arrow:
  case Token_Kind_quote:
  case Token_Kind_backslash:
    return false;
  default:
    return true;
  }
}

B8 parse_is_sync(Parser* parser, I16 indent) {
  Token token = *parse_peek(parser);
  if (token.kind == Token_Kind_source_leave) return true;
  if (token.kind == Token_Kind_semicolon) return true;
  if (token.kind == Token_Kind_curly_close) return true;
  if (has_flag(token.flag, Token_Flag_wasnewline)
      && token.indent <= indent
      && parse_token_starts_statement(token.kind)) {
    return true;
  }
  return false;
}

void parse_sync(Parser* parser, I16 indent) {
  while (!parse_at_end(parser) && !parse_is_sync(parser, indent)) {
    parse_advance(parser);
  }
  parser->panic = false;
}

void parse_recover_delimiter(Parser* parser, Token open, Token_Kind close, I16 open_indent) {
  I32 depth = 1;
  while (!parse_at_end(parser)) {
    Token token = *parse_peek(parser);
    if (token.kind == close) {
      depth -= 1;
      if (depth == 0) {
        parse_advance(parser);
        return;
      }
    }
    else if (token.kind == open.kind) {
      depth += 1;
    }
    else if (token.kind == Token_Kind_curly_close) {
      return;
    }
    else if (has_flag(token.flag, Token_Flag_wasnewline)
          && token.indent <= open_indent) {
      return;
    }
    parse_advance(parser);
  }
}

B8 parse_expect_close(Parser* parser, Token open, Token_Kind close, I16 open_indent) {
  Token token = *parse_peek(parser);
  if (token.kind == close) {
    parse_advance(parser);
    return true;
  }
  parse_unclosed(parser, open, close, token.loc);
  parser->panic = true;
  parse_recover_delimiter(parser, open, close, open_indent);
  parser->panic = false;
  return false;
}

B8 parse_expect_token(Parser* parser, Token_Kind kind) {
  Token token = *parse_peek(parser);
  if (token.kind == kind) {
    parse_advance(parser);
    return true;
  }
  return parse_expected(parser, token, kind);
}

Ast_Node* parse_error_node(Parser* parser, Loc loc) {
  return ast_new_error(parser->perm_arena, loc);
}

void parse_list_end(Parser* parser, Ast_List* list) {
  arena_release_mark(parser->list_arena, list);
}

void parse_list_push(Parser* parser, Ast_List* list, Ast_Node* node) {
  arena_extend(parser->list_arena, list, sizeof(Ast_List) + (list->length+1)*sizeof(Ast_Node*));
  fa_add(list, node);
}

Ast_List* parse_list_perm(Parser* parser, Ast_List* temp_list) {
  I32 size = sizeof(Ast_List) + temp_list->length * sizeof(Ast_Node*);
  Ast_List* perm_list = arena_push(parser->perm_arena, size);
  memcpy(perm_list, temp_list, size);
  arena_release_mark(parser->list_arena, temp_list);
  return perm_list;
}

void parse_decls_push(Parser* parser, Ast_Decls* decls, Str* name, Ast_Node* rhs, Loc loc) {
  for (I32 i = 0; i < decls->length; i++) {
    Ast_Decl* previous = &decls->base[i];
    if (previous->name == name) {
      Diag* diag = diag_error(parser->diags, Diag_Code_duplicate_declaration, loc);
      if (diag) {
        diag->data.name.name = name;
        I32 parent = (I32)(diag - parser->diags->base);
        Diag* note = diag_note(parser->diags, parent,
                               Diag_Code_previous_declaration, previous->loc);
        if (note) note->data.name.name = name;
      }
      return;
    }
  }
  Ast_Decl decl = { name, rhs, loc };
  fa_extend(parser->decls_arena, decls, decl);
}

Hash_Map* parse_decls_perm(Parser* parser, Ast_Decls* temp_list) {
  Hash_Map* map = arena_push(parser->perm_arena, sizeof(Hash_Map));
  *map = hash_map_init(parser->perm_arena, temp_list->length+1);
  for (I32 i = 0; i < temp_list->length; i++) {
    Ast_Decl decl = temp_list->base[i];
    Symbol* sym = hash_map_get(map, decl.name);
    if (sym) continue;
    sym = arena_push_zero(parser->perm_arena, sizeof(Symbol));
    sym->ast = decl.rhs;
    hash_map_put(map, decl.name, sym);
  }
  arena_release_mark(parser->decls_arena, temp_list);
  return map;
}

Ast_Node* parse_new_expression(Parser* parser, I32 precedence_to_match);
Ast_Node* parse_statement(Parser* parser);
Ast_Block parse_scope(Parser* parser, Token_Kind end_token_kind, Token open);

Ast_Node* parse_new_unary(Parser* parser, Ast_Kind kind, Ast_Node* lhs) {
  Ast_Node* node = ast_new_node(parser->perm_arena, kind);
  node->unary = lhs;
  if (lhs) node->loc = lhs->loc;
  return node;
}

Ast_Node* parse_new_infix(Parser* parser, Ast_Kind kind, Ast_Node* lhs, I32 token_flags) {
  I32 right_precedence = parse_right_precedence(kind, token_flags);
  Ast_Node* rhs = parse_new_expression(parser, right_precedence);
  if (rhs && rhs->kind == Ast_Kind_error) {
    return rhs;
  }
  Ast_Node* node = ast_new_node(parser->perm_arena, kind);
  node->binary.lhs = lhs;
  node->binary.rhs = rhs;
  node->loc = loc_join(lhs ? lhs->loc : LOC_NONE, rhs ? rhs->loc : LOC_NONE);
  return node;
}

Ast_Node* parse_infix_or_suffix(Parser* parser, Ast_Node* lhs, I32 precedence_to_match) {
  Ast_Node* node = lhs;
  if (node && node->kind == Ast_Kind_error) return node;
  while (true) {
    if (node && node->kind == Ast_Kind_error) return node;
    if (parse_at_end(parser)) return node;
    Token token = *parse_peek(parser);
    parse_advance(parser);
    switch (token.kind) {
    case Token_Kind_arrow:
    case Token_Kind_dot:
    case Token_Kind_comma: case Token_Kind_backslash: case Token_Kind_dot_dot:
    case Token_Kind_bang_equal: case Token_Kind_equal_equal:
    case Token_Kind_less_equal: case Token_Kind_less:
    case Token_Kind_greater_equal: case Token_Kind_greater:
    case Token_Kind_pipe: case Token_Kind_pike: case Token_Kind_ampersand:
    case Token_Kind_plus: case Token_Kind_minus: case Token_Kind_star: case Token_Kind_slash: case Token_Kind_percent:
    {
      Ast_Kind kind = (Ast_Kind)token.kind;
      I32 precedence = parse_left_precedence(kind, token.flag);
      if (precedence >= precedence_to_match) {
        lhs = node;
        node = parse_new_infix(parser, kind, lhs, token.flag);
      }
      else {
        parse_retreat(parser);
        return node;
      }
    } break;
    case Token_Kind_quote: {
      Ast_Kind kind = Ast_Kind_call;
      I32 precedence = parse_left_precedence(kind, token.flag);
      if (precedence >= precedence_to_match) {
        lhs = node;
        I32 right_precedence = parse_right_precedence(kind, token.flag);
        Ast_Node* rhs = parse_new_expression(parser, right_precedence);
        if (rhs && rhs->kind == Ast_Kind_error) return rhs;
        node = ast_new_node(parser->perm_arena, kind);
        node->binary.lhs = rhs;
        node->binary.rhs = lhs;
        node->loc = loc_join(rhs ? rhs->loc : LOC_NONE, lhs ? lhs->loc : LOC_NONE);
      }
      else {
        parse_retreat(parser);
        return node;
      }
    } break;
    case Token_Kind_brace_open: {
      Ast_Kind kind = Ast_Kind_subscript;
      I32 precedence = parse_left_precedence(kind, token.flag);
      if (precedence >= precedence_to_match) {
        lhs = node;
        node = parse_new_infix(parser, kind, lhs, token.flag);
        if (node->kind == Ast_Kind_error) return node;
        parse_expect_close(parser, token, Token_Kind_brace_close, token.indent);
      }
      else {
        parse_retreat(parser);
        return node;
      }
    } break;
    case Token_Kind_at: {
      Ast_Kind kind = Ast_Kind_load;
      I32 precedence = parse_left_precedence(kind, token.flag);
      if (precedence >= precedence_to_match) {
        lhs = node;
        node = parse_new_unary(parser, kind, lhs);
      }
      else {
        parse_retreat(parser);
        return node;
      }
    } break;
    default: {
      parse_retreat(parser);
      if (has_flag(token.kind, Token_Kind_Flag_call_rhs) && !has_flag(token.flag, Token_Flag_wasnewline)) {
        Ast_Kind kind = Ast_Kind_call;
        I32 precedence = parse_left_precedence(kind, token.flag);
        if (precedence >= precedence_to_match) {
          lhs = node;
          Token left_token = parser->tokens.base[parser->tok-1];
          I32 right_precedence = parse_right_precedence(kind, left_token.flag);
          Ast_Node* rhs = parse_new_expression(parser, right_precedence);
          if (rhs && rhs->kind == Ast_Kind_error) return rhs;
          node = ast_new_node(parser->perm_arena, Ast_Kind_call);
          node->binary.lhs = lhs;
          node->binary.rhs = rhs;
          node->loc = loc_join(lhs ? lhs->loc : LOC_NONE, rhs ? rhs->loc : LOC_NONE);
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

Ast_Node* parse_indented_block(Parser* parser, I16 indent) {
  if (parse_at_end(parser)) {
    Token token = *parse_peek(parser);
    parse_diag(parser, Diag_Code_expected_block, token.loc);
    parser->panic = true;
    return parse_error_node(parser, token.loc);
  }

  Token next_token = *parse_peek(parser);
  if ((next_token.flag & Token_Flag_wasnewline) == 0) {
    return parse_statement(parser);
  }

  Ast_List* fa_temp(parser->list_arena, temp);
  while (!parse_at_end(parser)) {
    parser->panic = false;
    next_token = *parse_peek(parser);
    if (has_flag(next_token.flag, Token_Flag_wasnewline)
        && next_token.indent <= indent) {
      break;
    }
    if (next_token.kind == Token_Kind_curly_close) {
      break;
    }
    if (next_token.kind == Token_Kind_semicolon) {
      parse_advance(parser);
      continue;
    }
    I32 before = parser->tok;
    Ast_Node* node = parse_statement(parser);
    if (node) {
      parse_list_push(parser, temp, node);
    }
    if (parser->tok == before) {
      parse_sync(parser, next_token.indent);
      if (parser->tok == before) {
        parse_advance(parser);
      }
    }
    while (parse_match_token(parser, Token_Kind_semicolon)) {}
  }
  if (temp->length == 0) {
    Token token = *parse_peek(parser);
    parse_diag(parser, Diag_Code_expected_block, token.loc);
    parser->panic = true;
    return parse_error_node(parser, token.loc);
  }
  Ast_Node* node = ast_new_node(parser->perm_arena, Ast_Kind_iblock);
  node->list = parse_list_perm(parser, temp);
  return node;
}

Ast_Node* parse_prefix_or_atom(Parser* parser) {
  if (parse_at_end(parser)) return NULL;
  Token token = *parse_peek(parser);
  parse_advance(parser);
  Ast_Node* node = NULL;
  switch (token.kind) {
  case Token_Kind_error: {
    node = ast_new_error(parser->perm_arena, token.loc);
  } break;
  case Token_Kind_sharp: {
    Ast_Kind kind = Ast_Kind_run;
    I32 precedence = parse_right_precedence(kind, token.flag);
    Ast_Node* unary = parse_new_expression(parser, precedence);
    if (unary && unary->kind == Ast_Kind_error) return unary;
    node = ast_new_node(parser->perm_arena, kind);
    node->unary = unary;
    node->loc = loc_join(token.loc, unary ? unary->loc : LOC_NONE);
  } break;
  case Token_Kind_minus_prefix: case Token_Kind_minus:
  case Token_Kind_plus_prefix:  case Token_Kind_plus:
  case Token_Kind_at_prefix:    case Token_Kind_at:
  {
    Ast_Kind kind = ((Ast_Kind)token.kind+1) & 0xff;
    I32 precedence = parse_right_precedence(kind, token.flag);
    Ast_Node* unary = parse_new_expression(parser, precedence);
    if (unary && unary->kind == Ast_Kind_error) return unary;
    node = ast_new_node(parser->perm_arena, kind);
    node->unary = unary;
    node->loc = loc_join(token.loc, unary ? unary->loc : LOC_NONE);
  } break;
  case Token_Kind_str: case Token_Kind_int: case Token_Kind_float: case Token_Kind_name:
  {
    Ast_Kind kind = (Ast_Kind)token.kind & 0xff;
    node = ast_new_node(parser->perm_arena, kind);
    node->bits = token.bits;
    node->loc  = token.loc;
  } break;
  case Token_Kind_backslash: {
    node = parse_new_expression(parser, 0);
  } break;
  case Token_Kind_if: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* if_value = parse_new_expression(parser, 0);
    node = ast_new_node(parser->perm_arena, Ast_Kind_if_value);
    node->binary.lhs = cond;
    node->binary.rhs = if_value;
    node->loc = loc_join(token.loc,
                         loc_join(cond ? cond->loc : LOC_NONE,
                                  if_value ? if_value->loc : LOC_NONE));
    if (parse_match_token(parser, Token_Kind_else)) {
      Ast_Node* if_node = node;
      Ast_Node* else_value = parse_new_expression(parser, 0);
      node = ast_new_node(parser->perm_arena, Ast_Kind_else_value);
      node->binary.lhs = if_node;
      node->binary.rhs = else_value;
      node->loc = loc_join(if_node->loc, else_value ? else_value->loc : LOC_NONE);
    }
  } break;
  case Token_Kind_while: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* while_block = parse_indented_block(parser, token.indent);
    node = ast_new_node(parser->perm_arena, Ast_Kind_while);
    node->binary.lhs = cond;
    node->binary.rhs = while_block;
    node->loc = loc_join(token.loc,
                         loc_join(cond ? cond->loc : LOC_NONE,
                                  while_block ? while_block->loc : LOC_NONE));
  } break;
  case Token_Kind_paren_open: {
    Ast_List* fa_temp(parser->list_arena, temp);
    B8 is_record = false;
    B8 closed = false;
    while (!parse_at_end(parser)) {
      if (parse_match_token(parser, Token_Kind_paren_close)) {
        closed = true;
        break;
      }
      if (parse_is_token(parser, Token_Kind_curly_close)) break;
      I32 before = parser->tok;
      Ast_Node* exp = parse_new_expression(parser, 0);
      if (parse_match_token(parser, Token_Kind_colon)) {
        Ast_Node* lhs = exp;
        Ast_Node* rhs = parse_new_expression(parser, 0);
        if (lhs && lhs->kind == Ast_Kind_name) {
          exp = ast_new_node(parser->perm_arena, Ast_Kind_declare);
          exp->declare.name = lhs->str;
          exp->declare.node = rhs;
          exp->loc = loc_join(lhs->loc, rhs ? rhs->loc : LOC_NONE);
        }
        else {
          Diag* diag = parse_diag(parser, Diag_Code_bad_declaration_target,
                                  lhs ? lhs->loc : token.loc);
          (void)diag;
          exp = parse_error_node(parser, lhs ? lhs->loc : token.loc);
        }
      }
      if (parse_match_token(parser, Token_Kind_semicolon)) {
        is_record = true;
      }
      if (exp) {
        parse_list_push(parser, temp, exp);
      }
      if (parser->tok == before) {
        if (parse_is_token(parser, Token_Kind_paren_close)
            || parse_is_token(parser, Token_Kind_curly_close)) {
          break;
        }
        parse_advance(parser);
      }
    }
    if (!closed) {
      parse_expect_close(parser, token, Token_Kind_paren_close, token.indent);
    }
    if (!is_record && temp->length == 1) {
      node = temp->base[0];
      parse_list_end(parser, temp);
    }
    else {
      node = ast_new_node(parser->perm_arena, Ast_Kind_record);
      node->list = parse_list_perm(parser, temp);
      node->loc  = token.loc;
    }
  } break;
  case Token_Kind_curly_open: {
    Ast_Block block = parse_scope(parser, Token_Kind_curly_close, token);
    node = ast_new_node(parser->perm_arena, Ast_Kind_block_value);
    node->block = block;
    node->loc   = token.loc;
  } break;
  case Token_Kind_brace_open:
  case Token_Kind_brace_prefix_open: {
    Ast_Kind kind = Ast_Kind_array;
    if (parse_match_token(parser, Token_Kind_brace_close)) {
      I32 right_precedence = parse_right_precedence(Ast_Kind_span, token.flag);
      Ast_Node* rhs = parse_new_expression(parser, right_precedence);
      if (rhs && rhs->kind == Ast_Kind_error) return rhs;
      node = ast_new_node(parser->perm_arena, Ast_Kind_span);
      node->unary = rhs;
      node->loc = loc_join(token.loc, rhs ? rhs->loc : LOC_NONE);
    }
    else {
      Ast_Node* lhs = parse_new_expression(parser, 0);
      B8 closed = parse_expect_close(parser, token, Token_Kind_brace_close, token.indent);
      if (!closed) return parse_error_node(parser, token.loc);
      if (lhs && lhs->kind == Ast_Kind_error) return lhs;
      I32 right_precedence = parse_right_precedence(kind, token.flag);
      Ast_Node* rhs = parse_new_expression(parser, right_precedence);
      if (rhs && rhs->kind == Ast_Kind_error) return rhs;
      node = ast_new_node(parser->perm_arena, kind);
      node->binary.lhs = lhs;
      node->binary.rhs = rhs;
      node->loc = loc_join(token.loc,
                           loc_join(lhs ? lhs->loc : LOC_NONE,
                                    rhs ? rhs->loc : LOC_NONE));
    }
  } break;
  default:
    parse_retreat(parser);
  break;
  }
  return node;
}

Ast_Node* parse_new_expression(Parser* parser, I32 precedence_to_match) {
  Ast_Node* prefix_or_atom = parse_prefix_or_atom(parser);
  if (!prefix_or_atom) {
    Token token = *parse_peek(parser);
    parse_expected_expression(parser, token);
    return parse_error_node(parser, token.loc);
  }
  if (prefix_or_atom->kind == Ast_Kind_error) {
    return prefix_or_atom;
  }
  Ast_Node* node = parse_infix_or_suffix(parser, prefix_or_atom, precedence_to_match);
  return node;
}

Ast_Node* parse_statement(Parser* parser) {
  if (parse_at_end(parser)) return NULL;
  Ast_Node* node = NULL;
  Token token = *parse_peek(parser);
  parse_advance(parser);
  switch (token.kind) {
  case Token_Kind_if: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* if_block = parse_indented_block(parser, token.indent);
    node = ast_new_node(parser->perm_arena, Ast_Kind_if);
    node->binary.lhs = cond;
    node->binary.rhs = if_block;
    node->loc = loc_join(token.loc,
                         loc_join(cond ? cond->loc : LOC_NONE,
                                  if_block ? if_block->loc : LOC_NONE));
    if (parse_match_token(parser, Token_Kind_else)) {
      Ast_Node* if_node = node;
      Ast_Node* else_block = parse_indented_block(parser, token.indent);
      node = ast_new_node(parser->perm_arena, Ast_Kind_else);
      node->binary.lhs = if_node;
      node->binary.rhs = else_block;
      node->loc = loc_join(if_node->loc, else_block ? else_block->loc : LOC_NONE);
    }
  } break;
  case Token_Kind_while: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* while_block = parse_indented_block(parser, token.indent);
    node = ast_new_node(parser->perm_arena, Ast_Kind_while);
    node->binary.lhs = cond;
    node->binary.rhs = while_block;
    node->loc = loc_join(token.loc, loc_join(cond ? cond->loc : LOC_NONE, while_block ? while_block->loc : LOC_NONE));
  } break;
  // case Token_Kind_for:
  case Token_Kind_return: {
    if (token.flag & Token_Flag_willnewline) {
      node = ast_new_node(parser->perm_arena, Ast_Kind_return);
      node->loc = token.loc;
    }
    else {
      Ast_Node* unary = parse_new_expression(parser, 0);
      if (unary && unary->kind == Ast_Kind_error) return unary;
      node = parse_new_unary(parser, Ast_Kind_return_value, unary);
      node->loc = loc_join(token.loc, unary ? unary->loc : LOC_NONE);
    }
  } break;
  case Token_Kind_break: {
    if (token.flag & Token_Flag_willnewline) {
      node = ast_new_node(parser->perm_arena, Ast_Kind_break);
      node->loc = token.loc;
    }
    else {
      Ast_Node* unary = parse_new_expression(parser, 0);
      if (unary && unary->kind == Ast_Kind_error) return unary;
      node = parse_new_unary(parser, Ast_Kind_break_value, unary);
      node->loc = loc_join(token.loc, unary ? unary->loc : LOC_NONE);
    }
  } break;
  case Token_Kind_curly_open: {
    Ast_Block block = parse_scope(parser, Token_Kind_curly_close, token);
    node = ast_new_node(parser->perm_arena, Ast_Kind_block);
    node->block = block;
    node->loc   = token.loc;
  } break;
  default: {
    parse_retreat(parser);
    node = parse_new_expression(parser, 0);
    if (node && node->kind == Ast_Kind_error) return node;
    if (parse_is_token(parser, Token_Kind_equal)) {
      Token equal = *parse_peek(parser);
      parse_advance(parser);
      if (!parse_expression_is_available(parser)) {
        parse_expected_expression_eol(parser, equal.loc);
        return parse_error_node(parser, equal.loc);
      }
      Ast_Node* lhs = node;
      Ast_Node* rhs = parse_new_expression(parser, 0);
      if (rhs && rhs->kind == Ast_Kind_error) return rhs;
      node = ast_new_node(parser->perm_arena, Ast_Kind_assign);
      node->binary.lhs = lhs;
      node->binary.rhs = rhs;
      node->loc = loc_join(lhs ? lhs->loc : LOC_NONE, rhs ? rhs->loc : LOC_NONE);
    }
  }
  }
  return node;
}

Ast_Block parse_scope(Parser* parser, Token_Kind end_token_kind, Token open) {
  Ast_Block block = {};
  Ast_List* fa_temp(parser->list_arena, temp);
  Ast_Decls* fa_temp(parser->decls_arena, temp_map);
  while (true) {
    parser->panic = false;
    if (parse_at_end(parser)) {
      if (end_token_kind != Token_Kind_source_leave) {
        parse_unclosed(parser, open, end_token_kind, parse_peek(parser)->loc);
      }
      break;
    }

    Token current = *parse_peek(parser);
    if (current.kind == end_token_kind) {
      parse_advance(parser);
      break;
    }
    else if (current.kind == Token_Kind_curly_close
         || current.kind == Token_Kind_paren_close
         || current.kind == Token_Kind_brace_close) {
      parse_unmatched(parser, current);
      parse_advance(parser);
      continue;
    }
    else if (current.kind == Token_Kind_semicolon) {
      parse_advance(parser);
      continue;
    }
    else if (current.kind == Token_Kind_else || current.kind == Token_Kind_do) {
      parse_unexpected(parser, current);
      parse_advance(parser);
      continue;
    }

    I32 before = parser->tok;
    Ast_Node* node = parse_statement(parser);
    if (!node) {
      parse_sync(parser, current.indent);
      if (parser->tok == before) {
        parse_advance(parser);
      }
      continue;
    }
    if (parser->tok == before) {
      parse_sync(parser, current.indent);
      if (parser->tok == before) {
        parse_advance(parser);
      }
      continue;
    }

    if (parse_is_token(parser, Token_Kind_colon)) {
      Token colon = *parse_peek(parser);
      parse_advance(parser);
      if (!parse_expression_is_available(parser)) {
        parse_expected_expression_eol(parser, colon.loc);
        node = parse_error_node(parser, current.loc);
      }
      else if (node->kind == Ast_Kind_name) {
        if (parse_is_token(parser, Token_Kind_equal)) {
          // name : = exp
          Token equal = *parse_peek(parser);
          parse_advance(parser);
          if (!parse_expression_is_available(parser)) {
            parse_expected_expression_eol(parser, equal.loc);
            node = parse_error_node(parser, equal.loc);
          }
          else {
            Ast_Node* lhs = node;
            Ast_Node* rhs = parse_new_expression(parser, 0);
            if (rhs && rhs->kind == Ast_Kind_error) {
              node = rhs;
            }
            else {
              parse_decls_push(parser, temp_map, lhs->str, rhs, lhs->loc);
              node = ast_new_node(parser->perm_arena, Ast_Kind_assign);
              node->binary.lhs = lhs;
              node->binary.rhs = rhs;
              node->loc = loc_join(lhs->loc, rhs ? rhs->loc : LOC_NONE);
              parse_list_push(parser, temp, node);
            }
          }
        }
        else {
          // name : exp
          Ast_Node* decl_rhs = parse_new_expression(parser, 0);
          if (decl_rhs && decl_rhs->kind == Ast_Kind_error) {
            node = decl_rhs;
          }
          else {
            parse_decls_push(parser, temp_map, node->str, decl_rhs, node->loc);
            if (parse_is_token(parser, Token_Kind_equal)) {
              // name : exp = exp
              Token equal = *parse_peek(parser);
              parse_advance(parser);
              if (!parse_expression_is_available(parser)) {
                parse_expected_expression_eol(parser, equal.loc);
                node = parse_error_node(parser, equal.loc);
              }
              else {
                Ast_Node* lhs = node;
                Ast_Node* rhs = parse_new_expression(parser, 0);
                if (rhs && rhs->kind == Ast_Kind_error) {
                  node = rhs;
                }
                else {
                  node = ast_new_node(parser->perm_arena, Ast_Kind_assign);
                  node->binary.lhs = lhs;
                  node->binary.rhs = rhs;
                  node->loc = loc_join(lhs->loc, rhs ? rhs->loc : LOC_NONE);
                  parse_list_push(parser, temp, node);
                }
              }
            }
            else {
              node = NULL;
            }
          }
        }
      }
      else {
        parse_diag(parser, Diag_Code_bad_declaration_target, node->loc);
        parse_sync(parser, current.indent);
        node = parse_error_node(parser, node->loc);
      }
    }
    else {
      parse_list_push(parser, temp, node);
    }

    while (parse_match_token(parser, Token_Kind_semicolon)) {}
  }
  block.list = parse_list_perm(parser, temp);
  block.scope = parse_decls_perm(parser, temp_map);
  return block;
}

Ast_Block parse_tokens(Arena* perm_arena, Tokens tokens, Diags* diags) {
  Arena temp_ast_arena  = arena_init(perm_arena->capacity);
  Arena temp_map_arena  = arena_init(perm_arena->capacity);
  Parser parser = {0};
  parser.perm_arena = perm_arena;
  parser.list_arena = &temp_ast_arena;
  parser.decls_arena = &temp_map_arena;
  parser.tokens = tokens;
  parser.tok    = 1;
  parser.diags  = diags;
  parser.last_error_tok = -1;
  Ast_Block block = parse_scope(&parser, Token_Kind_source_leave, (Token){0});
  arena_free(&temp_ast_arena);
  arena_free(&temp_map_arena);
  return block;
}

void _test_ast(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  I32 source_length = strlen(source) + 2;
  Arena arena = arena_init(KB(1) * source_length);
  str_init(&arena, 2*source_length);
  Source_Map sources = {0};
  File_Id   file   = source_add(&sources, &arena, file_name, source, strlen(source));
  Diags     diags  = diags_init(&arena, 256);
  Tokens    tokens = lex_source(&arena, &sources, file, &diags);
  Ast_Block ast    = parse_tokens(&arena, tokens, &diags);
  C8*       buffer = arena_push(&arena, 4*source_length);
  Cstr      result = cstr_from_ast(buffer, ast);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_ast(source, expected, __FILE__, __LINE__)

void _test_diag(Cstr source, I32 expected_errors, Diag_Code expected_code,
                B8 expect_note, Cstr file_name, I32 line) {
  I32 source_length = strlen(source) + 2;
  Arena arena = arena_init(KB(1) * source_length);
  str_init(&arena, 2*source_length);
  Source_Map sources = {0};
  File_Id file = source_add(&sources, &arena, file_name, source, strlen(source));
  Diags diags = diags_init(&arena, 256);
  Tokens tokens = lex_source(&arena, &sources, file, &diags);
  parse_tokens(&arena, tokens, &diags);

  B8 found_code = false;
  B8 found_note = false;
  for (I32 i = 0; i < diags.length; i++) {
    Diag* diag = &diags.base[i];
    if (diag->severity == Diag_Error && diag->code == expected_code) {
      found_code = true;
    }
    if (diag->severity == Diag_Note) {
      found_note = true;
    }
  }

  if (diags.error_count != expected_errors
  || (expected_errors > 0 && !found_code)
  || (expect_note && !found_note)) {
    printf("diagnostic test failed at %s:%d for source: %s\n", file_name, line, source);
    printf("expected %d error(s) with code %s%s, got %d error(s)\n",
           expected_errors, cstr_from_diag_code(expected_code),
           expect_note ? " and a note" : "", diags.error_count);
    for (I32 i = 0; i < diags.length; i++) {
      Diag* diag = &diags.base[i];
      printf("  %s[%s] at %d:%d\n",
             diag->severity == Diag_Error ? "error" :
             diag->severity == Diag_Warning ? "warning" : "note",
             cstr_from_diag_code(diag->code),
             source_line_index(source_get(&sources, diag->loc.file), diag->loc.start),
             source_line_column(source_get(&sources, diag->loc.file), diag->loc.start));
    }
    fflush(stdout);
    assert(0);
  }
  arena_free(&arena);
}

#define test_diag(source, errors, code, note) \
  _test_diag(source, errors, code, note, __FILE__, __LINE__)

void _test_diag_render(void) {
  Cstr source = "a : 1\na : 2";
  I32 source_length = strlen(source) + 2;
  Arena arena = arena_init(KB(1) * source_length);
  str_init(&arena, 2*source_length);
  Source_Map sources = {0};
  File_Id file = source_add(&sources, &arena, "test.i", source, strlen(source));
  Diags diags = diags_init(&arena, 256);
  Tokens tokens = lex_source(&arena, &sources, file, &diags);
  parse_tokens(&arena, tokens, &diags);

  FILE* out = tmpfile();
  assert(out);
  diag_print_all(&diags, &sources, out, false);
  fflush(out);
  rewind(out);
  C8 buffer[4096];
  Umi length = fread(buffer, 1, sizeof(buffer) - 1, out);
  buffer[length] = '\0';
  fclose(out);

  Cstr expected =
    "error[P006]: duplicate declaration of 'a'\n"
    " --> test.i:2:1\n"
    "  |\n"
    "2 | a : 2\n"
    "  | ^\n"
    "note: 'a' was first declared here\n"
    " --> test.i:1:1\n"
    "  |\n"
    "1 | a : 1\n"
    "  | ^\n"
    "1 error, 0 warnings\n";
  if (strcmp(buffer, expected) != 0) {
    printf("diagnostic render test failed\nexpected:\n%s\nactual:\n%s\n",
           expected, buffer);
    fflush(stdout);
    assert(0);
  }
  arena_free(&arena);
}

void diag_test(void) {
  test_diag("a : 1 $ 2", 1, Diag_Code_unknown_character, false);
  test_diag("a : 1 $ 2", 1, Diag_Code_unknown_character, false);
  test_diag("a : 0x", 1, Diag_Code_missing_digits, false);
  test_diag("a : 0b102", 1, Diag_Code_invalid_digit, false);
  test_diag("a : \"unterminated", 1, Diag_Code_unterminated_string, false);
  test_diag("/* unterminated", 1, Diag_Code_unterminated_comment, false);
  test_diag("a : ,", 1, Diag_Code_expected_expression, false);
  test_diag("a :", 1, Diag_Code_expected_expression, false);
  test_diag("a =", 1, Diag_Code_expected_expression, false);
  test_diag("a : =\nb : 2", 1, Diag_Code_expected_expression, false);
  test_diag("1 : 2", 1, Diag_Code_bad_declaration_target, false);
  test_diag("a : (1", 1, Diag_Code_unclosed_delimiter, true);
  test_diag("if 1\na : 2", 1, Diag_Code_expected_block, false);
  test_diag("}", 1, Diag_Code_unmatched_delimiter, false);
  test_diag("a : 1\na : 2", 1, Diag_Code_duplicate_declaration, true);
  test_diag("#1", 0, Diag_Code_none, false);
  _test_diag_render();
}

void parse_test(void) {
  return;
  test("a b->c",        "(a (b -> c))");
  test("foo \"Hi\"",    "(foo \"Hi\")");
  test("[]1",           "[]1");
  test("bits 32",       "(bits 32)");
  test("bits 32 (0..9)", "((bits 32) (0 .. 9))");

  test("a:1",            "a : 1; ");

  test("(a:1)->a",            "(a:1 -> a)");

  test("1",              "1");
  test("1*2+3",          "((1 * 2) + 3)");
  test("1 + -2",         "(1 + -2)");
  test("1 + 2*3",        "(1 + (2 * 3))");
  test("1 + 2",          "(1 + 2)");
  test("1*(2+3)",        "(1 * (2 + 3))");
  test("(1 + 2)*3",      "((1 + 2) * 3)");

  test("(1)",            "1");
  test("(1\n)",          "1");
  test("(1;)",           "(1;)");
  test("(1; 2)",         "(1; 2;)");
  test("(1; 2;)",        "(1; 2;)");
  test("(x:1;)",         "(x:1;)");
  test("(x:1)",          "x:1");
  test("(x:1; y:2)",     "(x:1; y:2;)");
  test("1,2",            "(1, 2)");

  test("a'b",     "(b a)");
  test("[2]\\ 2+3",     "[2](2 + 3)");
  test("if 2 break 3",     "if 2 do break 3");
  test("a.b@.c",     "((a . b)@ . c)");
  test("a, b -> 1, 2",     "((a, b) -> (1, 2))");
  test("while 1 do 2",        "while 1 do 2");
  test("(if 1 do 2)",      "if 1 do 2");
  test("(if 1 do 2 else 3)", "if 1 do 2 else 3");
  test("if 1 do 2",      "if 1 do 2");
  test("if 1 do 2 else 3", "if 1 do 2 else 3");

  test("{1; 2}",         "{1; 2;}");

  test("c+b[1]",         "(c + b[1])");
  test("(1\n 2)",        "(1; 2;)");
  test("a, (b,c), d = 1", "(a, (b, c), d) = 1");
  test("a = 1",          "a = 1");
  test("a = 1 + 2",      "a = (1 + 2)");
  test("a, b = 1",       "(a, b) = 1");
  test("a = 1, 2",       "a = (1, 2)");
  test("a+b = 1*2",      "(a + b) = (1 * 2)");

  test("(1,2;3)",        "((1, 2); 3;)");

  test("b[1+2]",         "b[(1 + 2)]");
  test("b[1]",           "b[1]");
  test("b[1+2]*c",       "(b[(1 + 2)] * c)");
  test("[1]b",           "[1]b");
  test("c+[1]b",         "(c + [1]b)");
  test("1 * [2+3]b",     "(1 * [(2 + 3)]b)");
  test("[1+2]b",         "[(1 + 2)]b");

  test("foo 1\n2",       "(foo 1); 2");
  test("bar 1 2",        "((bar 1) 2)");

  test("return 1",        "return 1");
  test("return\n1",        "return; 1");
  return;
}

#undef test
