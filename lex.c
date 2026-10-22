typedef enum {
  Token_Flag_wasnewline  = 1 << 0,
  Token_Flag_willnewline = 1 << 1,
  Token_Flag_wasspace    = 1 << 2,
  Token_Flag_willspace   = 1 << 3,
} Token_Flag;

typedef struct {
  Token_Kind kind;
  Token_Flag flag;
  I16 indent;
  Loc loc;
  union {
    U64  bits;
    I64  i64;
    F64  f64;
    Str* str;
  };
} Token;

typedef struct {
  Token* base;
  Umi    length;
} Tokens;

typedef struct {
  Cstr    source;
  Cstr    stream;
  Cstr    end;
  File_Id file;
  Diags*  diags;
  B8      wasnewline;
  B8      wasspace;
  I16     indent;
} Lexer;

void tokens_push(Tokens* tokens, Token item) {
  tokens->base[tokens->length++] = item;
}

Token tokens_pop(Tokens* token) {
  return token->base[token->length--];
}

Token* tokens_at(Tokens* token, I32 at) {
  return &token->base[at];
}

Token* tokens_top(Tokens* token) {
  return &token->base[token->length-1];
}

Lexer lexer = {0};

void lex_advance() {
  if (lexer.stream < lexer.end) {
    lexer.stream++;
  }
}

C8 lex_peek(I32 offset) {
  Cstr at = lexer.stream + offset;
  if (at >= lexer.end) return '\0';
  return *at;
}

Loc lex_loc(Cstr begin, Cstr end) {
  Loc loc = {0};
  loc.file  = lexer.file;
  loc.start = (U32)(begin - lexer.source);
  loc.end   = (U32)(end - lexer.source);
  return loc;
}

Tokens lex_source(Arena* arena, Source_Map* sources, File_Id file, Diags* diags) {
  Tokens slice_token = {0};
  Source_File* source_file = source_get(sources, file);
  assert(source_file);
  Cstr source        = source_file->data;
  Umi  source_length = source_file->length;
  slice_token.base = arena_push(arena, sizeof(Token) * (source_length + 2));
  lexer.source = source;
  lexer.stream = source;
  lexer.end    = source + source_length;
  lexer.file   = file;
  lexer.diags  = diags;
  lexer.wasnewline = true;
  lexer.wasspace   = false;
  lexer.indent    = 0;
  tokens_push(&slice_token, (Token){
    .kind = Token_Kind_source_enter,
    .loc  = {file, 0, 0},
  });

  while (lexer.stream < lexer.end && !(diags && diags->limit_hit)) {
    Cstr token_start = lexer.stream;
    Token token = {0};
    switch (*lexer.stream) {
    case ' ':
      if (lexer.wasnewline) lexer.indent += 1;
      lex_advance();
      lexer.wasspace = true;
    continue;
    case '\t':
      if (lexer.wasnewline) lexer.indent += 8;
      lex_advance();
      lexer.wasspace = true;
    continue;
    case '\n': case '\r':
      lexer.indent = 0;
      lexer.wasnewline = true;
      while (lexer.stream < lexer.end) {
        if (*lexer.stream == '\r') {
          lex_advance();
          if (lexer.stream < lexer.end && *lexer.stream == '\n') {
            lex_advance();
          }
          lexer.indent = 0;
        }
        else if (*lexer.stream == '\n') {
          lex_advance();
          lexer.indent = 0;
        }
        else if (*lexer.stream == ' ') {
          lexer.indent += 1;
          lex_advance();
        }
        else if (*lexer.stream == '\t') {
          lexer.indent += 8;
          lex_advance();
        }
        else {
          break;
        }
      }
    continue;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      token.kind = Token_Kind_int;
      Cstr number_start = lexer.stream;
      I32 base = 10;
      B8 has_prefix = false;
      Cstr prefix = "";
      if (*lexer.stream == '0') {
        lex_advance();
        if (tolower((U8)lex_peek(0)) == 'x') {
          lex_advance();
          base = 16;
          has_prefix = true;
          prefix = "0x";
        }
        else if (tolower((U8)lex_peek(0)) == 'b') {
          lex_advance();
          base = 2;
          has_prefix = true;
          prefix = "0b";
        }
        else if (isdigit((U8)lex_peek(0))) {
          base = 8;
        }
      }
      U64 val = 0;
      B8 saw_digit = !has_prefix;
      B8 invalid_reported = false;
      B8 overflow_reported = false;
      for (;;) {
        if (lexer.stream >= lexer.end) break;
        if (*lexer.stream == '_') {
          lex_advance();
          continue;
        }
        if (base == 10 && (*lexer.stream == 'e' || *lexer.stream == 'E')
            && (isdigit((U8)lex_peek(1))
             || ((lex_peek(1) == '+' || lex_peek(1) == '-') && isdigit((U8)lex_peek(2))))) {
          break;
        }
        I32 digit = 16;
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

        if (digit > 15) break;
        if (base == 10 && digit > 9) break;
        if ((base == 2 || base == 8) && digit > 9) break;
        if (digit >= base) {
          if (!invalid_reported && lexer.diags) {
            Diag* diag = diag_error(lexer.diags, Diag_Code_invalid_digit,
                                    lex_loc(lexer.stream, lexer.stream + 1));
            if (diag) {
              diag->data.invalid_digit.digit = *lexer.stream;
              diag->data.invalid_digit.base  = base;
            }
            invalid_reported = true;
          }
          digit = 0;
        }
        else {
          saw_digit = true;
        }

        if (val > (UINT64_MAX - (U64)digit) / (U64)base) {
          if (!overflow_reported && lexer.diags) {
            diag_error(lexer.diags, Diag_Code_number_overflow,
                       lex_loc(number_start, lexer.stream + 1));
            overflow_reported = true;
          }
          val = UINT64_MAX;
        }
        else {
          val = val * (U64)base + (U64)digit;
        }
        lex_advance();
      }
      if (has_prefix && !saw_digit && lexer.diags) {
        Diag* diag = diag_error(lexer.diags, Diag_Code_missing_digits,
                                lex_loc(number_start, lexer.stream));
        if (diag) diag->data.text.text = prefix;
      }
      B8 is_float = false;
      if (base == 10) {
        if (lexer.stream < lexer.end && *lexer.stream == '.'
            && isdigit((U8)lex_peek(1))) {
          is_float = true;
        }
        else if (lexer.stream < lexer.end
              && (*lexer.stream == 'e' || *lexer.stream == 'E')
              && (isdigit((U8)lex_peek(1))
               || ((lex_peek(1) == '+' || lex_peek(1) == '-') && isdigit((U8)lex_peek(2))))) {
          is_float = true;
        }
      }
      if (is_float) {
        C8 buffer[128];
        I32 length = 0;
        B8 float_too_long = false;
#define lex_float_push(c)                                                     \
        do {                                                                  \
          if (length + 1 < (I32)sizeof(buffer)) {                             \
            buffer[length++] = (c);                                           \
          }                                                                   \
          else if (!float_too_long) {                                         \
            if (!overflow_reported && lexer.diags) {                          \
              diag_error(lexer.diags, Diag_Code_number_overflow,              \
                         lex_loc(number_start, lexer.stream + 1));            \
              overflow_reported = true;                                       \
            }                                                                 \
            float_too_long = true;                                            \
          }                                                                   \
        } while (0)
        for (Cstr at = number_start; at < lexer.stream; at++) {
          if (*at != '_') {
            lex_float_push(*at);
          }
        }
        if (*lexer.stream == '.') {
          lex_float_push(*lexer.stream);
          lex_advance();
          while (lexer.stream < lexer.end
              && (isdigit((U8)*lexer.stream) || *lexer.stream == '_')) {
            if (*lexer.stream != '_') {
              lex_float_push(*lexer.stream);
            }
            lex_advance();
          }
        }
        if (lexer.stream < lexer.end
            && (*lexer.stream == 'e' || *lexer.stream == 'E')) {
          lex_float_push(*lexer.stream);
          lex_advance();
          if (lexer.stream < lexer.end
              && (*lexer.stream == '+' || *lexer.stream == '-')) {
            lex_float_push(*lexer.stream);
            lex_advance();
          }
          while (lexer.stream < lexer.end
              && (isdigit((U8)*lexer.stream) || *lexer.stream == '_')) {
            if (*lexer.stream != '_') {
              lex_float_push(*lexer.stream);
            }
            lex_advance();
          }
        }
#undef lex_float_push
        buffer[length] = '\0';
        token.kind = Token_Kind_float;
        token.f64  = strtod(buffer, 0);
      }
      else {
        token.i64 = (I64)val;
      }
    } break;
    case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': {
      Cstr start = lexer.stream;
      token.kind = Token_Kind_name;
      while (lexer.stream < lexer.end
         && (isalnum((U8)*lexer.stream) || *lexer.stream == '_')) {
        lex_advance();
      }
      token.str = str_from_range(start, lexer.stream);
      token.kind  = (Token_Kind)token.str->kind;
    } break;
    case '|': {
      token.kind = Token_Kind_pipe;
      lex_advance();
    } break;
    case '^': {
      token.kind = Token_Kind_pike;
      lex_advance();
    } break;
    case '&': {
      token.kind = Token_Kind_ampersand;
      lex_advance();
    } break;
    case '+': {
      token.kind = Token_Kind_plus;
      if (lexer.wasnewline && !isspace((U8)lex_peek(1))) {
        token.kind = Token_Kind_plus_prefix;
      }
      lex_advance();
    } break;
    case '-': {
      token.kind = Token_Kind_minus;
      lex_advance();
      if (lexer.wasnewline && !isspace((U8)lex_peek(0))) {
        token.kind = Token_Kind_minus_prefix;
      }
      else if (lexer.stream < lexer.end && *lexer.stream == '>') {
        token.kind = Token_Kind_arrow;
        lex_advance();
      }
    } break;
    case '*': {
      token.kind = Token_Kind_star;
      lex_advance();
    } break;
    case '/': {
      token.kind = Token_Kind_slash;
      lex_advance();
      if (lexer.stream < lexer.end && *lexer.stream == '/') {
        lex_advance();
        while (lexer.stream < lexer.end
            && *lexer.stream != '\n' && *lexer.stream != '\r') {
          lex_advance();
        }
        continue;
      }
      else if (lexer.stream < lexer.end && *lexer.stream == '*') {
        Cstr comment_start = token_start;
        lex_advance();
        I32 level = 1;
        while (lexer.stream < lexer.end && level > 0) {
          if (lexer.stream + 1 < lexer.end
              && lexer.stream[0] == '/' && lexer.stream[1] == '*') {
            level++;
            lexer.stream += 2;
          }
          else if (lexer.stream + 1 < lexer.end
                && lexer.stream[0] == '*' && lexer.stream[1] == '/') {
            level--;
            lexer.stream += 2;
          }
          else {
            if (*lexer.stream == '\n' || *lexer.stream == '\r') {
              lexer.wasnewline = true;
              lexer.indent = 0;
              if (*lexer.stream == '\r') {
                lex_advance();
                if (lexer.stream < lexer.end && *lexer.stream == '\n') {
                  lex_advance();
                }
              }
              else {
                lex_advance();
              }
            }
            else {
              lex_advance();
            }
          }
        }
        if (level > 0 && lexer.diags) {
          Cstr end = comment_start + 2;
          if (end > lexer.end) end = lexer.end;
          diag_error(lexer.diags, Diag_Code_unterminated_comment,
                     lex_loc(comment_start, end));
        }
        continue;
      }
    } break;
    case '%': {
      token.kind = Token_Kind_percent;
      lex_advance();
    } break;
    case '!': {
      token.kind = Token_Kind_bang;
      lex_advance();
      if (*lexer.stream == '=') {
        token.kind = Token_Kind_bang_equal;
        lex_advance();
      }
    } break;
    case '=': {
      token.kind = Token_Kind_equal;
      lex_advance();
      if (*lexer.stream == '=') {
        token.kind = Token_Kind_equal_equal;
        lex_advance();
      }
    } break;
    case '<': {
      token.kind = Token_Kind_less;
      lex_advance();
      if (*lexer.stream == '=') {
        token.kind = Token_Kind_less_equal;
        lex_advance();
      }
    } break;
    case '>': {
      token.kind = Token_Kind_greater;
      lex_advance();
      if (*lexer.stream == '=') {
        token.kind = Token_Kind_greater_equal;
        lex_advance();
      }
    } break;
    case ':': {
      token.kind = Token_Kind_colon;
      lex_advance();
    } break;
    case '.': {
      token.kind = Token_Kind_dot;
      lex_advance();
      if (*lexer.stream == '.') {
        token.kind = Token_Kind_dot_dot;
        lex_advance();
      }
    } break;
    case '(': {
      token.kind = Token_Kind_paren_open;
      lex_advance();
    } break;
    case ')': {
      token.kind = Token_Kind_paren_close;
      lex_advance();
    } break;
    case '[': {
      token.kind = Token_Kind_brace_open;
      if (lexer.wasnewline) {
        token.kind = Token_Kind_brace_prefix_open;
      }
      lex_advance();
    } break;
    case ']': {
      token.kind = Token_Kind_brace_close;
      lex_advance();
    } break;
    case '{': {
      token.kind = Token_Kind_curly_open;
      lex_advance();
    } break;
    case '}': {
      token.kind = Token_Kind_curly_close;
      lex_advance();
    } break;
    case '@': {
      token.kind = Token_Kind_at;
      if (lexer.wasnewline) {
        token.kind = Token_Kind_at_prefix;
      }
      lex_advance();
    } break;
    case ';': {
      token.kind = Token_Kind_semicolon;
      lex_advance();
    } break;
    case ',': {
      token.kind = Token_Kind_comma;
      lex_advance();
    } break;
    case '\\': {
      token.kind = Token_Kind_backslash;
      lex_advance();
    } break;
    case '\'': {
      token.kind = Token_Kind_quote;
      lex_advance();
    } break;
    case '"': {
      Cstr quote_start = lexer.stream;
      lex_advance();
      Cstr start = lexer.stream;
      token.kind = Token_Kind_str;
      while (lexer.stream < lexer.end
          && *lexer.stream != '"'
          && *lexer.stream != '\n' && *lexer.stream != '\r') {
        lex_advance();
      }
      token.str = str_from_range(start, lexer.stream);
      if (lexer.stream >= lexer.end || *lexer.stream != '"') {
        if (lexer.diags) {
          diag_error(lexer.diags, Diag_Code_unterminated_string,
                     lex_loc(quote_start, lexer.stream));
        }
      }
      else {
        lex_advance();
      }
    } break;
    case '#': {
      token.kind = Token_Kind_sharp;
      lex_advance();
    } break;
    default: {
      if (lexer.diags) {
        Diag* diag = diag_error(lexer.diags, Diag_Code_unknown_character,
                                lex_loc(lexer.stream, lexer.stream + 1));
        if (diag) diag->data.unknown_char.byte = (U8)*lexer.stream;
      }
      token.kind = Token_Kind_error;
      lex_advance();
    } break;
    }
    if (lexer.wasnewline) {
      token.flag |= Token_Flag_wasnewline;
      Token* top = tokens_top(&slice_token);
      top->flag |= Token_Flag_willnewline;
    }
    if (lexer.wasspace) {
      token.flag |= Token_Flag_wasspace;
      Token* top = tokens_top(&slice_token);
      top->flag |= Token_Flag_willspace;
    }
    token.indent = lexer.indent;
    token.loc    = lex_loc(token_start, lexer.stream);

    // NOTE: wasnewline checks whether rhs should be disabled
    lexer.wasnewline = false;

    // NOTE: wasspace is used to determine function call precedence.
    //       foo (a).x == foo(a.x)
    //       foo(a).x == (foo(a)).x
    lexer.wasspace   = false;
    tokens_push(&slice_token, token);
  }
  Token* top = tokens_top(&slice_token);
  top->flag |= Token_Flag_willnewline;
  Token token = {
    .kind = Token_Kind_source_leave,
    .loc  = {file, (U32)source_length, (U32)source_length},
  };
  tokens_push(&slice_token, token);
  return slice_token;
}

Cstr cstr_from_slice_token(Arena* arena, Tokens slice) {
  C8* buffer = arena_push(arena, 64 * slice.length);
  String_Builder sb = string_builder_begin(buffer);
  for (Token* token = slice.base; token < slice.base + slice.length; token++) {
    switch (token->kind) {
    case Token_Kind_source_leave:
      string_builder_push_cstr(&sb, "leave");
    break;
    case Token_Kind_source_enter:
      string_builder_push_cstr(&sb, "enter");
    break;
    case Token_Kind_error:
      string_builder_push_cstr(&sb, "error");
    break;
    case Token_Kind_name: {
      string_builder_push_str(&sb, token->str);
    } break;
    case Token_Kind_int:
      string_builder_push_i64(&sb, token->i64);
    break;
    case Token_Kind_float:
      string_builder_push_f64(&sb, token->f64);
    break;
    case Token_Kind_str:
      string_builder_push_str(&sb, token->str);
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
    case Token_Kind_bang:
      string_builder_push_cstr(&sb, "p-");
    break;
    case Token_Kind_star:
      string_builder_push_cstr(&sb, "*");
    break;
    case Token_Kind_slash:
      string_builder_push_cstr(&sb, "/");
    break;
    case Token_Kind_percent:
      string_builder_push_cstr(&sb, "%");
    break;
    case Token_Kind_pipe:
      string_builder_push_cstr(&sb, "|");
    break;
    case Token_Kind_pike:
      string_builder_push_cstr(&sb, "|");
    break;
    case Token_Kind_ampersand:
      string_builder_push_cstr(&sb, "&");
    break;
    case Token_Kind_equal_equal:
      string_builder_push_cstr(&sb, "==");
    break;
    case Token_Kind_bang_equal:
      string_builder_push_cstr(&sb, "!=");
    break;
    case Token_Kind_less_equal:
      string_builder_push_cstr(&sb, "<=");
    break;
    case Token_Kind_greater_equal:
      string_builder_push_cstr(&sb, ">=");
    break;
    case Token_Kind_less:
      string_builder_push_cstr(&sb, "<");
    break;
    case Token_Kind_greater:
      string_builder_push_cstr(&sb, ">");
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
    case Token_Kind_dot_dot:
      string_builder_push_cstr(&sb, "..");
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
    case Token_Kind_quote:
      string_builder_push_cstr(&sb, "'");
    break;
    case Token_Kind_backslash:
      string_builder_push_cstr(&sb, "\\");
    break;
    case Token_Kind_if:
      string_builder_push_cstr(&sb, "if");
    break;
    case Token_Kind_do:
      string_builder_push_cstr(&sb, "do");
    break;
    case Token_Kind_else:
      string_builder_push_cstr(&sb, "el");
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
    case Token_Kind_sharp:
      string_builder_push_cstr(&sb, "#");
    break;
    }
    string_builder_push_cstr(&sb, " ");
  }
  Cstr str = string_builder_end(&sb);
  return str;
}
