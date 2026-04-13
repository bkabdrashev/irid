typedef enum {
  Token_Kind_Flag_separates = 1 << 8,
  Token_Kind_Flag_prefix    = 1 << 9,
} Token_Kind_Flag;

typedef enum {
  Token_Flag_call_lhs  = 1 << 0,
  Token_Flag_call_rhs  = 1 << 1,
} Token_Flag;

typedef enum {
  Token_Kind_eof                = 0 | Token_Kind_Flag_separates,
  Token_Kind_name               = 1,
  Token_Kind_int                = 2,
  Token_Kind_plus               = 3,
  Token_Kind_plus_prefix        = 3 | Token_Kind_Flag_prefix,
  Token_Kind_minus              = 5,
  Token_Kind_minus_prefix       = 5 | Token_Kind_Flag_prefix,
  Token_Kind_star               = 7,
  Token_Kind_at                 = 8,
  Token_Kind_at_prefix          = 8 | Token_Kind_Flag_prefix,
  Token_Kind_equal              = 10 | Token_Kind_Flag_separates,
  Token_Kind_brace_open         = 11,
  Token_Kind_brace_prefix_open  = 12,
  Token_Kind_brace_close        = 13,
  Token_Kind_paren_open         = 14,
  Token_Kind_paren_close        = 15,
  Token_Kind_curly_open         = 16,
  Token_Kind_curly_close        = 17,
  Token_Kind_semicolon          = 18 | Token_Kind_Flag_separates,
  Token_Kind_comma              = 19,
} Token_Kind;

typedef struct {
  Token_Kind kind;
  Token_Flag flag;
  union {
    s64 s64_val;
    u64 value;
    Istr istr;
  };
} Token;

typedef struct {
  Token* base;
  umi    length;
} Slice_Token;

void slice_token_push(Slice_Token* slice, Token item) {
  slice->base[slice->length++] = item;
}

Token slice_token_pop(Slice_Token* slice) {
  return slice->base[slice->length--];
}

Token slice_token_at(Slice_Token* slice, s32 at) {
  return slice->base[at];
}


typedef struct {
  cstr source;
  cstr stream;
  cstr file_path;
  b8   wasnewline;
  b8   wasspace;
} Lexer;

Lexer lexer = {0};

Slice_Token lex_source(cstr source, cstr file_path) {
  umi source_len = strlen(source);
  Slice_Token slice_token = {0};
  slice_token.base = xmalloc(sizeof(Token) * (source_len+1));
  lexer.source = source;
  lexer.stream = source;
  lexer.file_path = file_path;
  lexer.wasnewline = true;
  lexer.wasspace   = true;

  while (*lexer.stream) {
    Token token = {0};
    switch (*lexer.stream) {
    case ' ': case '\t': case '\v' :
      lexer.stream++;
      lexer.wasspace = true;
      continue;
    case '\n': case '\r': 
      lexer.stream++;
      lexer.wasspace = true;
      lexer.wasnewline = true;
      continue;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    {
      cstr start = lexer.stream;
      token.kind = Token_Kind_name;
      token.flag = Token_Flag_call_lhs;
      if (!lexer.wasnewline) {
        token.flag |= Token_Flag_call_rhs;
      }
      while (isalnum(*lexer.stream)) {
        lexer.stream++;
      }
      token.istr = istr_from_range(start, lexer.stream);
    } break;
    case '+': {
      token.kind = Token_Kind_plus;
      if (lexer.wasnewline && !isspace(lexer.stream[1])) {
        token.kind = Token_Kind_plus_prefix;
      }
      lexer.stream++;
    } break;
    case '-': {
      token.kind = Token_Kind_minus;
      if (lexer.wasnewline && !isspace(lexer.stream[1])) {
       token.kind = Token_Kind_minus_prefix;
      }
      lexer.stream++;
    } break;
    case '*': {
      token.kind = Token_Kind_star;
      lexer.stream++;
    } break;
    case '=': {
      token.kind = Token_Kind_equal;
      lexer.stream++;
    } break;
    case '(': {
      token.kind = Token_Kind_paren_open;
      if (!lexer.wasnewline) {
        token.flag = Token_Flag_call_rhs;
      }
      lexer.stream++;
    } break;
    case ')': {
      token.kind = Token_Kind_paren_close;
      token.flag = Token_Flag_call_lhs;
      lexer.stream++;
    } break;
    case '[': {
      token.kind = Token_Kind_brace_open;
      if (lexer.wasnewline) {
        token.kind = Token_Kind_brace_prefix_open;
      }
      lexer.stream++;
    } break;
    case ']': {
      token.kind = Token_Kind_brace_close;
      token.flag = Token_Flag_call_lhs;
      lexer.stream++;
    } break;
    case '{': {
      token.kind = Token_Kind_curly_open;
      if (!lexer.wasnewline) {
        token.flag = Token_Flag_call_rhs;
      }
      lexer.stream++;
    } break;
    case '}': {
      token.kind = Token_Kind_curly_close;
      token.flag = Token_Flag_call_lhs;
      lexer.stream++;
    } break;
    case '@': {
      token.kind = Token_Kind_at;
      if (lexer.wasnewline) {
        token.kind = Token_Kind_at_prefix;
      }
      lexer.stream++;
    } break;
    case ';': {
      token.kind = Token_Kind_semicolon;
      lexer.stream++;
    } break;
    case ',': {
      token.kind = Token_Kind_comma;
      lexer.stream++;
    } break;
    case '\0': {
      token.kind = Token_Kind_eof;
    } break;
    default: {
      lexer.stream++;
    } break;
    }

    // NOTE: checks whether rhs should be disabled
    lexer.wasnewline = false;
    lexer.wasspace   = false;
    slice_token_push(&slice_token, token);
  }
  Token token = { .kind = Token_Kind_eof };
  slice_token_push(&slice_token, token);
  return slice_token;
}

cstr cstr_from_slice_token(Slice_Token slice) {
  String_Builder sb = string_builder_begin(&temp_arena, slice.length * 3 + 1);
  for (Token* token = slice.base; token < slice.base + slice.length; token++) {
    switch (token->kind) {
    case Token_Kind_eof:
      string_builder_push_cstr(&sb, "eof");
      break;
    case Token_Kind_name: {
      cstr str =  cstr_from_istr(token->istr);
      string_builder_push_cstr(&sb, str);
    } break;
    case Token_Kind_int:
      string_builder_push_cstr(&sb, "int");
      break;
    case Token_Kind_plus:
      string_builder_push_cstr(&sb, "+");
      break;
    case Token_Kind_plus_prefix:
      string_builder_push_cstr(&sb, "p+");
      break;
    case Token_Kind_minus:
      string_builder_push_cstr(&sb, "-");
      break;
    case Token_Kind_minus_prefix:
      string_builder_push_cstr(&sb, "p-");
      break;
    case Token_Kind_star:
      string_builder_push_cstr(&sb, "*");
      break;
    case Token_Kind_equal:
      string_builder_push_cstr(&sb, "=");
      break;
    case Token_Kind_at:
      string_builder_push_cstr(&sb, "@");
      break;
    case Token_Kind_at_prefix:
      string_builder_push_cstr(&sb, "p@");
      break;
    case Token_Kind_brace_open:
      string_builder_push_cstr(&sb, "[");
      break;
    case Token_Kind_brace_prefix_open:
      string_builder_push_cstr(&sb, "p[");
      break;
    case Token_Kind_brace_close:
      string_builder_push_cstr(&sb, "]");
      break;
    case Token_Kind_paren_open:
      string_builder_push_cstr(&sb, "(");
      break;
    case Token_Kind_paren_close:
      string_builder_push_cstr(&sb, ")");
      break;
    case Token_Kind_curly_open:
      string_builder_push_cstr(&sb, "{");
      break;
    case Token_Kind_curly_close:
      string_builder_push_cstr(&sb, "}");
      break;
    case Token_Kind_semicolon:
      string_builder_push_cstr(&sb, ";");
      break;
    case Token_Kind_comma:
      string_builder_push_cstr(&sb, ";");
      break;
    }
    string_builder_push_cstr(&sb, " ");
  }
  cstr str = string_builder_end(&sb);
  return str;
}


