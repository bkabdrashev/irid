typedef enum {
  TokenFlag_separates = 1 << 8,
} TokenFlag;

typedef enum {
  TokenKind_eof                = 0 | TokenFlag_separates,
  TokenKind_name               = 1,
  TokenKind_int                = 2,
  TokenKind_plus               = 3,
  TokenKind_plus_prefix        = 4,
  TokenKind_minus              = 5,
  TokenKind_minus_prefix       = 6,
  TokenKind_star               = 7,
  TokenKind_at                 = 8,
  TokenKind_brace_open         = 9,
  TokenKind_brace_prefix_open  = 10,
  TokenKind_brace_close        = 11 | TokenFlag_separates,
  TokenKind_paren_open         = 12,
  TokenKind_paren_close        = 13 | TokenFlag_separates,
} TokenKind;

typedef struct {
  TokenKind kind;
  union {
    s64 s64_val;
    u64 value;
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
  Slice_Token slice_token = {0};
  slice_token.base = malloc(MB(2));
  lexer.source = source;
  lexer.stream = source;
  lexer.file_path = file_path;
  lexer.wasnewline = true;
  lexer.wasspace   = true;

  Token token = {0};
  while (*lexer.stream) {
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
      token.kind = TokenKind_name;
      while (isalnum(*lexer.stream)) {
        lexer.stream++;
      }
    } break;
    case '+': {
      token.kind = TokenKind_plus;
      if (lexer.wasnewline && !isspace(lexer.stream[1])) {
        token.kind = TokenKind_plus_prefix;
      }
      lexer.stream++;
    } break;
    case '-': {
      token.kind = TokenKind_minus;
      if (lexer.wasnewline && !isspace(lexer.stream[1])) {
       token.kind = TokenKind_minus_prefix;
      }
    } break;
    case '(': {
      token.kind = TokenKind_paren_open;
      lexer.stream++;
    } break;
    case '[': {
      token.kind = TokenKind_brace_open;
      if (lexer.wasnewline) {
        token.kind = TokenKind_brace_prefix_open;
      }
      lexer.stream++;
    } break;
    case '\0': {
      token.kind = TokenKind_eof;
    } break;
    default: {
      lexer.stream++;
    } break;
    }

    slice_token_push(&slice_token, token);
  }
  token.kind = TokenKind_eof;
  slice_token_push(&slice_token, token);
  return slice_token;
}


