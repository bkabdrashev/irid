#include "lex.c"
#include <string.h>
#include "common.c"

typedef enum {
  AstFlag_unary  = 1 << 8,
  AstFlag_binary = 1 << 9,
} AstFlag;

typedef enum {
  AstKind_add = TokenKind_plus  | AstFlag_binary,
  AstKind_sub = TokenKind_minus | AstFlag_binary,
  AstKind_neg = TokenKind_minus | AstFlag_unary,
  AstKind_arr  = TokenKind_brace_close | AstFlag_binary,
  AstKind_call = 123 | AstFlag_binary,
  AstKind_block = 124 | AstFlag_binary,
} AstKind;

typedef struct {
  AstKind kind;
  union {
    u32 value;
  };
} Ast; 

typedef struct {
  s32 index;
} Astid;

typedef enum {
  Parse_State_expression,
  Parse_State_infix_or_suffix,
} Parse_State;


typedef struct {
  Ast* base;
  umi  length;
} Slice_Ast;

s32 slice_ast_push(Slice_Ast slice, Ast item) {
  s32 index = slice.length;
  slice.base[slice.length++] = item;
  return index;
}

Ast slice_ast_pop(Slice_Ast slice) {
  return slice.base[--slice.length];
}
Ast slice_ast_top(Slice_Ast slice) {
  return slice.base[slice.length-1];
}
b8 slice_ast_empty(Slice_Ast slice) {
  return slice.length == 0;
}

typedef struct {
  Parse_State* base;
  umi  length;
} Slice_Parse_State;

void slice_parse_state_push(Slice_Parse_State slice, Parse_State item) {
  slice.base[slice.length++] = item;
}

Parse_State slice_parse_state_pop(Slice_Parse_State slice) {
  return slice.base[slice.length--];
}

typedef struct {
  Slice_Parse_State state_stack;
  Slice_Ast ast_stack;
  Slice_Ast ast_final;
  Slice_Token tokens;
  s32  tok;
  cstr source;
  cstr path;
} Parser;

Parser parser = {0};

Astid parse_ast_final_push() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .kind = token.kind, .value = token.value };
  Astid astid = { .index = slice_ast_push(parser.ast_final, ast) };
  return astid;
}

void parse_ast_final_push_unary() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .kind = token.kind | AstFlag_unary, .value = token.value };
  slice_ast_push(parser.ast_final, ast);
}

void parse_ast_stack_transfer_to_ast_final() {
  Ast ast = slice_ast_pop(parser.ast_stack);
  slice_ast_push(parser.ast_final, ast);
}

b8 parse_ast_stack_top_is(TokenKind kind) {
  Ast ast = slice_ast_top(parser.ast_stack);
  return ast.kind == kind;
}

void parse_ast_stack_push() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  Ast ast = { .kind = token.kind, .value = token.value };
  slice_ast_push(parser.ast_stack, ast);
}

void parse_ast_stack_push_kind(AstKind kind) {
  Ast ast = { .kind = kind, .value = 0 };
  slice_ast_push(parser.ast_stack, ast);
}

Ast parse_ast_stack_pop() {
  return slice_ast_pop(parser.ast_stack);
}

s32 parse_infix_left_precedence(TokenKind kind) {
  switch (kind) {
  case TokenKind_plus: return 11;
  case TokenKind_star: return 13;
  }
}

s32 parse_infix_right_precedence(TokenKind kind) {
  switch (kind) {
  case TokenKind_plus: return 12;
  case TokenKind_star: return 14;
  }
}

b32 parse_ast_stack_top_is_higher_precedence() {
  Token token = slice_token_at(parser.tokens, parser.tok);
  if (slice_ast_empty(parser.ast_stack)) return true;
  Ast stack_ast = slice_ast_top(parser.ast_stack);
  s32 on_stack_precedence  = parse_infix_left_precedence(stack_ast.kind);
  s32 on_stream_precedence = parse_infix_right_precedence(token.kind);
  if (on_stack_precedence <= on_stream_precedence) {
    return false;
  }
  return true;
}

Parse_State parse_state_stack_pop() {
  Parse_State state = slice_parse_state_pop(parser.state_stack);
  return state;
}

void parse_state_stack_push(Parse_State parse_state) {
  slice_parse_state_push(parser.state_stack, parse_state);
}

Token parse_current_token() {
  return parser.tokens.base[parser.tok];
}

TokenKind parse_current_token_kind() {
  return parser.tokens.base[parser.tok].kind;
}

b8 parse_current_token_kind_is(TokenKind kind) {
  return parser.tokens.base[parser.tok].kind == kind;
}

b8 parse_current_token_flag_is(TokenFlag flag) {
  Token token = parse_current_token();
  return (token.kind & flag) != 0;
}

b8 parse_is_token_separates_expression() {
  Token token = parse_current_token();
  return (token.kind & TokenFlag_separates) != 0;
}

Astid parse_expression() {
  while (!parse_is_token_separates_expression()) {
    switch (parse_state_stack_pop()) {
    case Parse_State_expression: {
      switch (parse_current_token_kind()) {
      case TokenKind_minus_prefix: case TokenKind_minus:
      case TokenKind_plus_prefix:  case TokenKind_plus:
        parse_ast_final_push_unary();
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_int: case TokenKind_name:
        parse_ast_final_push();
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      case TokenKind_paren_open:
        parse_ast_stack_push();
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_paren_close:
        while (!parse_ast_stack_top_is(TokenKind_paren_open)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_pop(); // pops TokenKind_paren_open
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      case TokenKind_brace_open:
      case TokenKind_brace_prefix_open:
        parse_ast_stack_push();
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_brace_close:
        while (!parse_ast_stack_top_is(TokenKind_brace_open)) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_pop(); // pops TokenKind_brace_open
        parse_ast_stack_push_kind(AstKind_arr);// push TokenKind_brace_close
        parse_state_stack_push(Parse_State_expression);
        break;
      }
    } break;
    case Parse_State_infix_or_suffix: {
      switch (parse_current_token_kind()) {
      case TokenKind_plus:
      case TokenKind_minus:
      case TokenKind_star:
      case TokenKind_brace_open:
        while (parse_ast_stack_top_is_higher_precedence()) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_push();
        parse_state_stack_push(Parse_State_expression);
        break;
      case TokenKind_at:
        while (parse_ast_stack_top_is_higher_precedence()) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_final_push();
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      default:
        while (parse_ast_stack_top_is_higher_precedence()) {
          parse_ast_stack_transfer_to_ast_final();
        }
        parse_ast_stack_push_kind(AstKind_call);
        parse_state_stack_push(Parse_State_infix_or_suffix);
        break;
      }
    } break;
    }
  }
}

Astid parse_statement() {
  Astid astid = {0};
  return astid;
}

Astid parse_tokens(Slice_Token tokens) {
  s32 statements = 0;
  while (!parse_current_token_kind_is(TokenKind_eof)) {
    Astid astid = parse_statement();
    statements++;
    parser.tok++;
  }
  Astid block = parse_ast_final_push(AstKind_block, statements);
}

Astid astid_from_source(cstr source, cstr path) {
  Slice_Token tokens = lex_source(source, path);
  Astid astid = parse_tokens(tokens);
  return astid;
}

cstr cstr_from_astid(Astid astid) {
}

cstr source_to_cstr_from_ast(cstr source, cstr name) {
  Astid astid = astid_from_source(source, name);
  return cstr_from_astid(astid);
}

cstr cstr_from_source_info(cstr file_name, cstr line) {
  umi len1 = strlen(file_name);
  umi len2 = strlen(line);
  cstr str = arena_alloc(temp_arena, len1 + len2 + 2 + 1);
  strcat(str, file_name);
  strcat(str, "(");
  strcat(str, line);
  strcat(str, ")");
  return str;
}

b8 _test_ast(cstr expected, cstr file_name, s32 line, cstr source) {
  arena_init(&temp_arena);
  Astid astid = astid_from_source(source, cstr_from_source_info(file_name, line));
  b32 result = test_at_source(cstr_from_ast(astid), expected, file_name, line, source);
  return result;
}

#define test(source, expected) _test_ast(expected, __FILE__, __LINE__, source)

void parse_test(void) {
  test("a = 1", "{ a = 1; }");
}

#undef test

