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
  Ast_Kind_tuple       = Token_Kind_comma,
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
  Ast_Kind_call        = 50,
  Ast_Kind_declare_field,
  Ast_Kind_assign_field,
  Ast_Kind_iblock,
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

typedef struct Var Var;
typedef struct Symbol Symbol;
struct Symbol {
  Ast_Node* ast;
  Var* var;
};

typedef struct Ast_Block Ast_Block;
struct Ast_Block {
  Hash_Map* scope;
  Ast_List* list;
};

struct Ast_Node {
  Ast_Kind kind;
  union {
    U64   bits;
    I64   i64;
    Str*  str;
    Ast_Node* unary;
    Ast_List* list;
    struct {
      Ast_Node* lhs;
      Ast_Node* rhs;
    } binary;
    struct {
      Str* str;
      Ast_Node* rhs;
    } declare;
    Ast_Block block;
  };
};

typedef struct Ast_Decl Ast_Decl;
struct Ast_Decl {
  Str*      name;
  Ast_Node* rhs;
};

typedef struct Ast_Decls Ast_Decls;
struct Ast_Decls {
  I32 length;
  Ast_Decl base[];
};

Cstr cstr_from_ast_kind(Ast_Kind ast_kind) {
  Cstr result = "unknown";
  switch (ast_kind & 0xff) {
  case Ast_Kind_none:         result = "none"; break;
  case Ast_Kind_name:         result = "name"; break;
  case Ast_Kind_int:          result = "int"; break;
  case Ast_Kind_add:          result = "+"; break;
  case Ast_Kind_sub:          result = "-"; break;
  case Ast_Kind_mul:          result = "*"; break;
  case Ast_Kind_neg:          result = "-"; break;
  case Ast_Kind_pos:          result = "+"; break;
  case Ast_Kind_eq:           result = "=="; break;
  case Ast_Kind_ne:           result = "!="; break;
  case Ast_Kind_lt:           result = "<"; break;
  case Ast_Kind_le:           result = "<="; break;
  case Ast_Kind_gt:           result = ">"; break;
  case Ast_Kind_ge:           result = ">="; break;
  case Ast_Kind_ptr:          result = "@"; break;
  case Ast_Kind_load:         result = "@"; break;
  case Ast_Kind_dot:          result = "."; break;
  case Ast_Kind_join:         result = "\\"; break;
  case Ast_Kind_subscript:    result = "s[]"; break;
  case Ast_Kind_array:        result = "a[]"; break;
  case Ast_Kind_assign:       result = "="; break;
  case Ast_Kind_declare:      result = ":"; break;
  case Ast_Kind_block:        result = "block"; break;
  case Ast_Kind_block_value:  result = "block_value"; break;
  case Ast_Kind_tuple:        result = "tuple"; break;
  case Ast_Kind_fun:          result = "->"; break;
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
    string_builder_push_str(sb, node->str);
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
      Str* str = scope->keys[i];
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
  case Ast_Kind_add: case Ast_Kind_sub:
  case Ast_Kind_eq: case Ast_Kind_ne:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
  case Ast_Kind_mul:
  case Ast_Kind_dot:
  case Ast_Kind_fun:
  case Ast_Kind_join:
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
  case Ast_Kind_assign_field: {
    string_builder_push_str(sb, node->declare.str);
    string_builder_push_cstr(sb, "=");
    string_builder_push_ast_node(sb, node->declare.rhs);
  } break;
  case Ast_Kind_declare_field: {
    string_builder_push_str(sb, node->declare.str);
    string_builder_push_cstr(sb, ":");
    string_builder_push_ast_node(sb, node->declare.rhs);
  } break;
  case Ast_Kind_declare: {
    string_builder_push_str(sb, node->declare.str);
    string_builder_push_cstr(sb, " : ");
    string_builder_push_ast_node(sb, node->declare.rhs);
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
  Arena* map_arena;

  Tokens tokens;
  I32  tok;
} Parser;

I32 parse_right_precedence(Ast_Kind kind) {
  switch (kind) {
  case Ast_Kind_fun:
    return 1;
  case Ast_Kind_tuple:
    return 3;
  case Ast_Kind_ne: case Ast_Kind_eq:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
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
  case Ast_Kind_array:
    return 16;
  case Ast_Kind_load:
    return 18;
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
  case Ast_Kind_ne: case Ast_Kind_eq:
  case Ast_Kind_lt: case Ast_Kind_le:
  case Ast_Kind_gt: case Ast_Kind_ge:
    return 5;
  case Ast_Kind_join:
    return 7;
  case Ast_Kind_sub:
  case Ast_Kind_add:
    return 11;
  case Ast_Kind_mul:
    return 13;
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

B8 parse_is_token(Parser* parser, Token_Kind kind) {
  Token token = parser->tokens.base[parser->tok];
  if (token.kind == kind) {
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

void parse_list_end(Parser* parser, Ast_List* list) {
  arena_release_mark(parser->list_arena, list);
}

void parse_list_push(Parser* parser, Ast_List* list, Ast_Node* node) {
  arena_push(parser->list_arena, sizeof(Ast_Node*));
  list->base[list->length++] = node;
}

Ast_List* parse_list_temp(Parser* parser) {
  return arena_push_zero(parser->list_arena, sizeof(Ast_List));
}

Ast_List* parse_list_perm(Parser* parser, Ast_List* temp_list) {
  I32 size = sizeof(Ast_List) + temp_list->length * sizeof(Ast_Node*);
  Ast_List* perm_list = arena_push(parser->perm_arena, size);
  memcpy(perm_list, temp_list, size);
  arena_release_mark(parser->list_arena, temp_list);
  return perm_list;
}

Ast_Decls* parse_map_temp(Parser* parser) {
  return arena_push_zero(parser->map_arena, sizeof(Ast_Decls));
}

void parse_map_push(Parser* parser, Ast_Decls* temp_map, Str* name, Ast_Node* rhs) {
  arena_push(parser->map_arena, sizeof(Ast_Decl));
  Ast_Decl decl = { name, rhs };
  fa_add(temp_map, decl);
}

Hash_Map* parse_map_perm(Parser* parser, Ast_Decls* temp_list) {
  Hash_Map* map = arena_push(parser->perm_arena, sizeof(Hash_Map));
  *map = hash_map_init(parser->perm_arena, temp_list->length+1);
  for (I32 i = 0; i < temp_list->length; i++) {
    Ast_Decl decl = temp_list->base[i];
    {
      Symbol* sym = hash_map_get(map, decl.name);
      if (sym) assert(0);
    }
    Symbol* sym = arena_push(parser->perm_arena, sizeof(Symbol));
    sym->ast = decl.rhs;
    hash_map_put(map, decl.name, sym);
  }
  arena_release_mark(parser->map_arena, temp_list);
  return map;
}

Ast_Node* parse_new_expression(Parser* parser, I32 precedence_to_beat);
Ast_Node* parse_statement(Parser* parser);
Ast_Block parse_scope(Parser* parser, Token_Kind end_token_kind);

Ast_Node* parse_new_node(Parser* parser, Ast_Kind kind) {
  Ast_Node* node = arena_push(parser->perm_arena, sizeof(Ast_Node));
  node->kind = kind;
  return node;
}

Ast_Node* parse_new_unary(Parser* parser, Ast_Kind kind, Ast_Node* lhs) {
  Ast_Node* node = parse_new_node(parser, kind);
  node->unary = lhs;
  return node;
}

Ast_Node* parse_new_infix(Parser* parser, Ast_Kind kind, Ast_Node* lhs) {
  I32 right_precedence = parse_right_precedence(kind);
  Ast_Node* rhs = parse_new_expression(parser, right_precedence);
  Ast_Node* node = parse_new_node(parser, kind);
  node->binary.lhs = lhs;
  node->binary.rhs = rhs;
  return node;
}

Ast_Node* parse_tuple_or_exp(Parser* parser) {
  Ast_Node* node;
  Ast_List* temp = parse_list_temp(parser);
  do {
    Ast_Node* exp = parse_new_expression(parser, 0);
    parse_list_push(parser, temp, exp);
  } while (parse_match_token(parser, Token_Kind_comma));
  if (temp->length == 1) {
    node = temp->base[0];
    parse_list_end(parser, temp);
  }
  else {
    node = parse_new_node(parser, Ast_Kind_tuple);
    node->list = parse_list_perm(parser, temp);
  }
  return node;
}

Ast_Node* parse_fun_tuple_or_exp(Parser* parser) {
  Ast_Node* lhs = parse_tuple_or_exp(parser);
  if (parse_match_token(parser, Token_Kind_arrow)) {
    Ast_Node* rhs = parse_tuple_or_exp(parser);
    Ast_Node* fun = parse_new_node(parser, Ast_Kind_fun);
    fun->binary.lhs = lhs;
    fun->binary.rhs = rhs;
    return fun;
  }
  else {
    return lhs;
  }
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
        lhs = node;
        node = parse_new_infix(parser, kind, lhs);
      }
      else {
        parser->tok--;
        return node;
      }
    } break;
    case Token_Kind_quote: {
      Ast_Kind kind = Ast_Kind_call;
      I32 precedence = parse_left_precedence(kind);
      if (precedence > precedence_to_beat) {
        lhs = node;
        I32 right_precedence = parse_right_precedence(kind);
        Ast_Node* rhs = parse_new_expression(parser, right_precedence);
        node = parse_new_node(parser, kind);
        node->binary.lhs = rhs;
        node->binary.rhs = lhs;
      }
      else {
        parser->tok--;
        return node;
      }
    } break;
    case Token_Kind_brace_open: {
      Ast_Kind kind = Ast_Kind_subscript;
      I32 precedence = parse_left_precedence(kind);
      if (precedence > precedence_to_beat) {
        lhs = node;
        node = parse_new_infix(parser, kind, lhs);
        parse_expect_token(parser, Token_Kind_brace_close);
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
        node = parse_new_unary(parser, kind, lhs);
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
          lhs = node;
          node = parse_new_infix(parser, kind, lhs);
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
  {
    Token next_token = get(parser->tokens, parser->tok);
    if ((next_token.flag & Token_Flag_wasnewline) == 0) {
      Ast_Node* node = parse_statement(parser);
      return node;
    }
  }
  Ast_List* temp = parse_list_temp(parser);
  while (true) {
    Ast_Node* node = parse_statement(parser);
    parse_list_push(parser, temp, node);
    Token next_token = get(parser->tokens, parser->tok);
    if (next_token.indent <= indent) {
      break;
    }
  }
  Ast_Node* node = parse_new_node(parser, Ast_Kind_iblock);
  node->list = parse_list_perm(parser, temp);
  return node;
}

Ast_Node* parse_prefix_or_atom(Parser* parser) {
  Ast_Node* node = 0;
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
  case Token_Kind_backslash: {
    node = parse_new_expression(parser, 0);
  } break;
  case Token_Kind_if: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* if_value = parse_new_expression(parser, 0);
    node = parse_new_node(parser, Ast_Kind_if_value);
    node->binary.lhs = cond;
    node->binary.rhs = if_value;
    if (parse_match_token(parser, Token_Kind_else)) {
      Ast_Node* if_node = node;
      Ast_Node* else_value = parse_new_expression(parser, 0);
      node = parse_new_node(parser, Ast_Kind_else_value);
      node->binary.lhs = if_node;
      node->binary.rhs = else_value;
    }
  } break;
  case Token_Kind_while: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* while_block = parse_indented_block(parser, token.indent);
    node = parse_new_node(parser, Ast_Kind_while);
    node->binary.lhs = cond;
    node->binary.rhs = while_block;
  } break;
  case Token_Kind_paren_open: {
    Ast_List* temp = parse_list_temp(parser);
    B8 is_record = false;
    while (!parse_match_token(parser, Token_Kind_paren_close)) {
      Ast_Node* exp = parse_fun_tuple_or_exp(parser);
      if (parse_match_token(parser, Token_Kind_colon)) {
        is_record = true;
        Ast_Node* lhs = exp;
        if (lhs->kind == Ast_Kind_name) {
          Ast_Node* rhs = parse_fun_tuple_or_exp(parser);
          exp = parse_new_node(parser, Ast_Kind_declare_field);
          exp->declare.str = lhs->str;
          exp->declare.rhs = rhs;
        }
        else {
          assert(0);
        }
      }
      else if (parse_match_token(parser, Token_Kind_equal)) {
        is_record = true;
        Ast_Node* lhs = exp;
        if (lhs->kind == Ast_Kind_name) {
          Ast_Node* rhs = parse_fun_tuple_or_exp(parser);
          exp = parse_new_node(parser, Ast_Kind_assign_field);
          exp->declare.str = lhs->str;
          exp->declare.rhs = rhs;
        }
        else {
          assert(0);
        }
      }
      if (parse_match_token(parser, Token_Kind_semicolon)) {
        is_record = true;
      }
      parse_list_push(parser, temp, exp);
    }
    if (!is_record && temp->length == 1) {
      node = temp->base[0];
      parse_list_end(parser, temp);
    }
    else {
      node = parse_new_node(parser, Ast_Kind_record);
      node->list = parse_list_perm(parser, temp);
    }
  } break;
  case Token_Kind_curly_open: {
    Ast_Block block = parse_scope(parser, Token_Kind_curly_close);
    node = parse_new_node(parser, Ast_Kind_block_value);
    node->block = block;
  } break;
  case Token_Kind_brace_open:
  case Token_Kind_brace_prefix_open: {
    Ast_Kind kind = Ast_Kind_array;
    Ast_Node* lhs = parse_new_expression(parser, 0);
    parse_expect_token(parser, Token_Kind_brace_close);
    I32 right_precedence = parse_right_precedence(kind);
    Ast_Node* rhs = parse_new_expression(parser, right_precedence);
    node = parse_new_node(parser, kind);
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

Ast_Node* parse_statement(Parser* parser) {
  Ast_Node* node = 0;
  Token token = parser->tokens.base[parser->tok++];
  switch (token.kind) {
  case Token_Kind_if: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* if_block = parse_indented_block(parser, token.indent);
    node = parse_new_node(parser, Ast_Kind_if);
    node->binary.lhs = cond;
    node->binary.rhs = if_block;
    if (parse_match_token(parser, Token_Kind_else)) {
      Ast_Node* if_node = node;
      Ast_Node* else_block = parse_indented_block(parser, token.indent);
      node = parse_new_node(parser, Ast_Kind_else);
      node->binary.lhs = if_node;
      node->binary.rhs = else_block;
    }
  } break;
  case Token_Kind_while: {
    Ast_Node* cond = parse_new_expression(parser, 0);
    parse_match_token(parser, Token_Kind_do);
    Ast_Node* while_block = parse_indented_block(parser, token.indent);
    node = parse_new_node(parser, Ast_Kind_while);
    node->binary.lhs = cond;
    node->binary.rhs = while_block;
  } break;
  // case Token_Kind_for:
  case Token_Kind_return: {
    if (token.flag & Token_Flag_willnewline) {
      node = parse_new_node(parser, Ast_Kind_return);
    }
    else {
      Ast_Node* unary = parse_new_expression(parser, 0);
      node = parse_new_unary(parser, Ast_Kind_return_value, unary);
    }
  } break;
  case Token_Kind_break: {
    if (token.flag & Token_Flag_willnewline) {
      node = parse_new_node(parser, Ast_Kind_break);
    }
    else {
      Ast_Node* unary = parse_new_expression(parser, 0);
      node = parse_new_unary(parser, Ast_Kind_break_value, unary);
    }
  } break;
  case Token_Kind_curly_open: {
    Ast_Block block = parse_scope(parser, Token_Kind_curly_close);
    node = parse_new_node(parser, Ast_Kind_block);
    node->block = block;
  } break;
  default: {
    parser->tok--;
    node = parse_fun_tuple_or_exp(parser);
    if (parse_match_token(parser, Token_Kind_equal)) {
      Ast_Node* lhs = node;
      Ast_Node* rhs = parse_fun_tuple_or_exp(parser);
      node = parse_new_node(parser, Ast_Kind_assign);
      node->binary.lhs = lhs;
      node->binary.rhs = rhs;
    }
  }
  }
  return node;
}

Ast_Block parse_scope(Parser* parser, Token_Kind end_token_kind) {
  Ast_Block block = {};
  Ast_List* temp_ast = parse_list_temp(parser);
  Ast_Decls* temp_map = parse_map_temp(parser);
  while (!parse_match_token(parser, end_token_kind)) {
    Ast_Node* node = parse_statement(parser);
    if (parse_match_token(parser, Token_Kind_colon)) {
      if (node->kind == Ast_Kind_name) {
        if (parse_match_token(parser, Token_Kind_equal)) {
          // name : = exp
          Ast_Node* lhs = node;
          Ast_Node* rhs = parse_fun_tuple_or_exp(parser);
          parse_map_push(parser, temp_map, lhs->str, rhs);
          node = parse_new_node(parser, Ast_Kind_assign);
          node->binary.lhs = lhs;
          node->binary.rhs = rhs;
          parse_list_push(parser, temp_ast, node);
        }
        else {
          // name : exp
          Ast_Node* decl_rhs = parse_fun_tuple_or_exp(parser);
          parse_map_push(parser, temp_map, node->str, decl_rhs);
          if (parse_match_token(parser, Token_Kind_equal)) {
            // name : exp = exp
            Ast_Node* lhs = node;
            Ast_Node* rhs = parse_fun_tuple_or_exp(parser);
            node = parse_new_node(parser, Ast_Kind_assign);
            node->binary.lhs = lhs;
            node->binary.rhs = rhs;
            parse_list_push(parser, temp_ast, node);
          }
          else {
            node = 0;
          }
        }
      }
      else {
        assert(0);
      }
    }
    else {
      parse_list_push(parser, temp_ast, node);
    }
    parse_match_token(parser, Token_Kind_semicolon);
  }
  block.list = parse_list_perm(parser, temp_ast);
  block.scope = parse_map_perm(parser, temp_map);
  return block;
}

Ast_Block parse_tokens(Arena* perm_arena, Tokens tokens) {
  Arena temp_ast_arena  = arena_init(KB(1) * perm_arena->capacity);
  Arena temp_map_arena = arena_init(KB(1) * perm_arena->capacity);
  Parser parser = {0};
  parser.perm_arena = perm_arena;
  parser.list_arena = &temp_ast_arena;
  parser.map_arena = &temp_map_arena;
  parser.tokens = tokens;
  parser.tok    = 1;
  Ast_Block block = parse_scope(&parser, Token_Kind_source_leave);
  arena_free(&temp_ast_arena);
  arena_free(&temp_map_arena);
  return block;
}

void _test_ast(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  I32 source_length = strlen(source) + 2;
  Arena arena   = arena_init(KB(1) * source_length);
  str_init(&arena, 2*source_length);
  Tokens tokens = lex_source(&arena, source);
  Ast_Block ast = parse_tokens(&arena, tokens);
  C8* buffer    = arena_push(&arena, 4*source_length);
  Cstr result   = cstr_from_ast(buffer, ast);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_ast(source, expected, __FILE__, __LINE__)

void parse_test(void) {
  test("[2]\\ 2+3",     "[2](2 + 3)");
  test("a'b",     "(b a)");
  test("if 2 br 3",     "if 2 do break 3");
  test("a.b@.c",     "((a . b)@ . c)");
  test("a, b -> 1, 2",     "((a, b) -> (1, 2))");
  test("wh 1 do 2",        "while 1 do 2");
  test("(if 1 do 2)",      "if 1 do 2");
  test("(if 1 do 2 el 3)", "if 1 do 2 else 3");
  test("if 1 do 2",      "if 1 do 2");
  test("if 1 do 2 el 3", "if 1 do 2 else 3");

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

  test("(1)",            "1");
  test("(1\n)",          "1");
  test("(1;)",           "(1;)");
  test("(1; 2)",         "(1; 2;)");
  test("(1; 2;)",        "(1; 2;)");
  test("(x=1)",          "(x=1;)");
  test("(x:1; y:2)",     "(x:1; y:2;)");
  test("1,2",            "(1, 2)");

  test("1",              "1");
  test("1*2+3",        "((1 * 2) + 3)");
  test("1 + -2",         "(1 + -2)");
  test("1 + 2*3",        "(1 + (2 * 3))");
  test("1 + 2",          "(1 + 2)");
  test("1*(2+3)",        "(1 * (2 + 3))");
  test("(1 + 2)*3",      "((1 + 2) * 3)");

  test("foo 1\n2",       "(foo 1); 2");
  test("bar 1 2",        "((bar 1) 2)");

  test("re 1",        "return 1");
  test("re\n1",        "return; 1");
  return;
}

#undef test
