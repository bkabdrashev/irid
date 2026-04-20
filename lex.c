typedef enum {
  Token_Flag_wasnewline  = 1 << 0,
  Token_Flag_willnewline = 1 << 1,
} Token_Flag;
typedef struct {
  Token_Kind kind;
  Token_Flag flag;
  union {
    I64  s64;
    U64  value;
    Istr istr;
  };
} Token;

typedef struct {
  Token* base;
  Umi    length;
} Slice_Token;

typedef struct {
  Cstr source;
  Cstr stream;
  Cstr file_path;
  B8   wasnewline;
} Lexer;

void slice_token_push(Slice_Token* slice, Token item) {
  slice->base[slice->length++] = item;
}

Token slice_token_pop(Slice_Token* slice) {
  return slice->base[slice->length--];
}

Token* slice_token_at(Slice_Token* slice, I32 at) {
  return &slice->base[at];
}

Token* slice_token_top(Slice_Token* slice) {
  return &slice->base[slice->length-1];
}

Lexer lexer = {0};

Slice_Token lex_source(Cstr source, Cstr file_path) {
  Umi source_len = strlen(source);
  Slice_Token slice_token = {0};
  slice_token.base = xmalloc(sizeof(Token) * (source_len+2));
  lexer.source = source;
  lexer.stream = source;
  lexer.file_path = file_path;
  lexer.wasnewline = true;
  slice_token_push(&slice_token, (Token){Token_Kind_none, 0, {0}});

  istr_from_cstr_token_kind("if", Token_Kind_if);
  istr_from_cstr_token_kind("do", Token_Kind_do);
  istr_from_cstr_token_kind("else", Token_Kind_else);
  istr_from_cstr_token_kind("return", Token_Kind_return);
  istr_from_cstr_token_kind("while", Token_Kind_while);
  istr_from_cstr_token_kind("break", Token_Kind_break);

  while (*lexer.stream) {
    Token token = {0};
    switch (*lexer.stream) {
    case ' ': case '\t': case '\v' :
      lexer.stream++;
    continue;
    case '\n': case '\r': 
      lexer.stream++;
      lexer.wasnewline = true;
    continue;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      token.kind = Token_Kind_int;
      I32 base = 10;
      if (*lexer.stream == '0') {
        lexer.stream++;
        if (tolower(*lexer.stream) == 'x') {
          lexer.stream++;
          base = 16;
        }
        else if (tolower(*lexer.stream) == 'b') {
          lexer.stream++;
          base = 2;
        }
        else if (isdigit(*lexer.stream)) {
          base = 8;
        }
      }
      U64 val = 0;
      for (;;) {
        if (*lexer.stream == '_') {
          lexer.stream++;
          continue;
        }
        I32 digit = 0;
        switch (*lexer.stream) {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
          digit = *lexer.stream - '0';
        break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
          digit = *lexer.stream - 'a' + 10;
        break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          digit = *lexer.stream - 'A' + 10;
        break;
        default:
          digit = 16;
        break;
        }

        if (digit == 16 && *lexer.stream != '0') {
          break;
        }
        if (digit >= base) {
          printf("Digit '%c' out of range for base %d", *lexer.stream, base);
          digit = 0;
        }
        val = val*base + digit;
        lexer.stream++;
      }
      token.s64 = val;
    } break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': {
      Cstr start = lexer.stream;
      token.kind = Token_Kind_name;
      while (isalnum(*lexer.stream)) {
        lexer.stream++;
      }
      token.istr = istr_from_range(start, lexer.stream);
      token.kind = token_kind_from_istr(token.istr);
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
      lexer.stream++;
      if (lexer.wasnewline && !isspace(lexer.stream[1])) {
        token.kind = Token_Kind_minus_prefix;
      }
      else if (*lexer.stream == '>') {
        token.kind = Token_Kind_arrow;
        lexer.stream++;
      }
    } break;
    case '*': {
      token.kind = Token_Kind_star;
      lexer.stream++;
    } break;
    case '=': {
      token.kind = Token_Kind_equal;
      lexer.stream++;
    } break;
    case ':': {
      token.kind = Token_Kind_colon;
      lexer.stream++;
    } break;
    case '.': {
      token.kind = Token_Kind_dot;
      lexer.stream++;
    } break;
    case '(': {
      token.kind = Token_Kind_paren_open;
      lexer.stream++;
    } break;
    case ')': {
      token.kind = Token_Kind_paren_close;
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
      lexer.stream++;
    } break;
    case '{': {
      token.kind = Token_Kind_curly_open;
      lexer.stream++;
    } break;
    case '}': {
      token.kind = Token_Kind_curly_close;
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
    if (lexer.wasnewline) {
      token.flag |= Token_Flag_wasnewline;
      Token* top = slice_token_top(&slice_token);
      top->flag |= Token_Flag_willnewline;
    }

    // NOTE: checks whether rhs should be disabled
    lexer.wasnewline = false;
    slice_token_push(&slice_token, token);
  }
  Token token = { .kind = Token_Kind_eof };
  slice_token_push(&slice_token, token);
  return slice_token;
}

Cstr cstr_from_slice_token(Slice_Token slice) {
  String_Builder sb = string_builder_begin(&temp_arena, slice.length * 3 + 1);
  for (Token* token = slice.base; token < slice.base + slice.length; token++) {
    switch (token->kind) {
    case Token_Kind_eof:
      string_builder_push_cstr(&sb, "eof");
    break;
    case Token_Kind_none:
      string_builder_push_cstr(&sb, "none");
    break;
    case Token_Kind_name: {
      Cstr str =  cstr_from_istr(token->istr);
      string_builder_push_cstr(&sb, str);
    } break;
    case Token_Kind_int:
      string_builder_push_s64(&sb, token->s64);
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
    case Token_Kind_colon:
      string_builder_push_cstr(&sb, ":");
    break;
    case Token_Kind_dot:
      string_builder_push_cstr(&sb, ".");
    break;
    case Token_Kind_at:
      string_builder_push_cstr(&sb, "@");
    break;
    case Token_Kind_at_prefix:
      string_builder_push_cstr(&sb, "p@");
    break;
    case Token_Kind_arrow:
      string_builder_push_cstr(&sb, "->");
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
      string_builder_push_cstr(&sb, ",");
    break;
    case Token_Kind_if:
      string_builder_push_cstr(&sb, "if");
    break;
    case Token_Kind_do:
      string_builder_push_cstr(&sb, "do");
    break;
    case Token_Kind_else:
      string_builder_push_cstr(&sb, "else");
    break;
    case Token_Kind_return:
      string_builder_push_cstr(&sb, "return");
    break;
    case Token_Kind_while:
      string_builder_push_cstr(&sb, "while");
    break;
    case Token_Kind_break:
      string_builder_push_cstr(&sb, "break");
    break;
    }
    string_builder_push_cstr(&sb, " ");
  }
  Cstr str = string_builder_end(&sb);
  return str;
}


