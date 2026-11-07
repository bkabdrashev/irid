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
} Token;

typedef struct {
  Source* source;
  Token* base;
  Umi    length;
} Tokens;

typedef struct {
  Cstr    stream;
  Cstr    end;
  Source* source;
  Diags*  diags;
  B8      wasnewline;
  B8      wasspace;
  I16     indent;
} Lexer;

Str_View token_str(Token token) {
  Str_View str_view = str_view_from_loc(token.loc);
  str_view.base   += 1;
  str_view.length -= 2;
  return str_view;
}

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
  loc.source  = lexer.source;
  loc.start = (U32)(begin - lexer.source->data);
  loc.end   = (U32)(end - lexer.source->data);
  return loc;
}

Tokens lex_source(Source* source, Diags* diags) {
  Tokens tokens = {0};
  tokens.base = arena_push_type(source->arena, Token, source->length + 2);
  tokens.source = source;

  lexer.source = source;
  lexer.stream = source->data;
  lexer.end    = source->data + source->length;
  lexer.diags  = diags;
  lexer.wasnewline = true;
  lexer.wasspace   = false;
  lexer.indent    = 0;
  tokens_push(&tokens, (Token){
    .kind = Token_Kind_source_enter,
    .loc  = {source, 0, 0},
  });

  while (lexer.stream < lexer.end && !(diags->limit_hit)) {
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
          if (!invalid_reported) {
            Diag* diag = diag_error(lexer.diags, Diag_Code_invalid_digit, lex_loc(lexer.stream, lexer.stream + 1));
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
            diag_error(lexer.diags, Diag_Code_number_overflow, lex_loc(number_start, lexer.stream + 1));
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
        Diag* diag = diag_error(lexer.diags, Diag_Code_missing_digits, lex_loc(number_start, lexer.stream));
        if (diag) diag->data.text = prefix;
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
              diag_error(lexer.diags, Diag_Code_number_overflow, lex_loc(number_start, lexer.stream + 1)); \
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
        token.kind = Token_Kind_float;
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
      while (lexer.stream < lexer.end && (isalnum((U8)*lexer.stream) || *lexer.stream == '_')) {
        lex_advance();
      }
      Umi length = lexer.stream - start;
      Str_View sv = { .base = start, .length = length };
      if (str_view_is_equal(str_view_from_cstr("while"), sv))  token.kind = Token_Kind_while;
      else if (str_view_is_equal(str_view_from_cstr("if"), sv))     token.kind = Token_Kind_if;
      else if (str_view_is_equal(str_view_from_cstr("else"), sv))   token.kind = Token_Kind_else;
      else if (str_view_is_equal(str_view_from_cstr("do"), sv))     token.kind = Token_Kind_do;
      else if (str_view_is_equal(str_view_from_cstr("return"), sv)) token.kind = Token_Kind_return;
      else if (str_view_is_equal(str_view_from_cstr("break"), sv))  token.kind = Token_Kind_break;
      else if (str_view_is_equal(str_view_from_cstr("use"), sv))    token.kind = Token_Kind_use;
      else if (str_view_is_equal(str_view_from_cstr("import"), sv)) token.kind = Token_Kind_import;
      else {
        token.kind  = Token_Kind_name;
      }
    } break;
    case '|': {
      token.kind = Token_Kind_pipe;
      lex_advance();
      if (*lexer.stream == '|') {
        token.kind = Token_Kind_pipe_pipe;
        lex_advance();
      }
    } break;
    case '^': {
      token.kind = Token_Kind_pike;
      lex_advance();
    } break;
    case '&': {
      token.kind = Token_Kind_amp;
      lex_advance();
      if (*lexer.stream == '&') {
        token.kind = Token_Kind_amp_amp;
        lex_advance();
      }
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
          diag_error(lexer.diags, Diag_Code_unterminated_comment, lex_loc(comment_start, end));
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
      token.kind = Token_Kind_str;
      while (lexer.stream < lexer.end && *lexer.stream != '"') {
        lex_advance();
      }
      if (lexer.stream >= lexer.end || *lexer.stream != '"') {
        if (lexer.diags) {
          diag_error(lexer.diags, Diag_Code_unterminated_string, lex_loc(quote_start, lexer.stream));
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
        Diag* diag = diag_error(lexer.diags, Diag_Code_unknown_character, lex_loc(lexer.stream, lexer.stream + 1));
        if (diag) diag->data.unknown_char = (U8)*lexer.stream;
      }
      token.kind = Token_Kind_error;
      lex_advance();
    } break;
    }
    if (lexer.wasnewline) {
      token.flag |= Token_Flag_wasnewline;
      token.flag |= Token_Flag_wasspace;
      Token* top = tokens_top(&tokens);
      top->flag |= Token_Flag_willspace;
      top->flag |= Token_Flag_willnewline;
    }
    if (lexer.wasspace) {
      token.flag |= Token_Flag_wasspace;
      Token* top = tokens_top(&tokens);
      top->flag |= Token_Flag_willspace;
    }
    token.indent = lexer.indent;
    token.loc    = lex_loc(token_start, lexer.stream);

    // NOTE: wasnewline checks whether rhs should be disabled
    lexer.wasnewline = false;

    // NOTE: wasspace is used to determine operator precedence.
    //       foo (a).x == foo(a.x)
    //       foo(a).x == (foo(a)).x
    lexer.wasspace   = false;
    tokens_push(&tokens, token);
  }
  Token* top = tokens_top(&tokens);
  top->flag |= Token_Flag_willnewline;
  Token token = {
    .kind = Token_Kind_source_leave,
    .loc  = {source, (U32)source->length, (U32)source->length},
  };
  tokens_push(&tokens, token);
  return tokens;
}

Cstr cstr_from_tokens(Arena* arena, Tokens tokens) {
  String_Builder sb = string_builder_begin(arena);
  for (Token* token = tokens.base; token < tokens.base + tokens.length; token++) {
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
    case Token_Kind_if:
    case Token_Kind_do:
    case Token_Kind_else:
    case Token_Kind_return:
    case Token_Kind_while:
    case Token_Kind_break:
    case Token_Kind_sharp:
    case Token_Kind_use:
    case Token_Kind_import:
    case Token_Kind_name:
    case Token_Kind_int:
    case Token_Kind_str:
    case Token_Kind_float: {
      Str_View str_view = str_view_from_loc(token->loc);
      string_builder_push_str_view(&sb, str_view);
    } break;
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
    case Token_Kind_pipe_pipe:
      string_builder_push_cstr(&sb, "||");
    break;
    case Token_Kind_pike:
      string_builder_push_cstr(&sb, "^");
    break;
    case Token_Kind_amp:
      string_builder_push_cstr(&sb, "&");
    break;
    case Token_Kind_amp_amp:
      string_builder_push_cstr(&sb, "&&");
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
    }
    string_builder_push_cstr(&sb, " ");
  }
  Cstr str = string_builder_end(&sb);
  return str;
}
