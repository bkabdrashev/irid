typedef enum Token_Kind_Flag {
  Token_Kind_Flag_prefix   = 1 << 8,
  Token_Kind_Flag_call_rhs = 1 << 9,
} Token_Kind_Flag;

typedef enum Token_Kind {
  Token_Kind_source_leave       = 0,
  Token_Kind_source_enter       = 1,
  Token_Kind_error              = 2,
  Token_Kind_int                = 3 | Token_Kind_Flag_call_rhs,
  Token_Kind_float              = 12 | Token_Kind_Flag_call_rhs,
  Token_Kind_plus               = 4,
  Token_Kind_plus_prefix        = 4 | Token_Kind_Flag_prefix,
  Token_Kind_minus              = 6,
  Token_Kind_minus_prefix       = 6 | Token_Kind_Flag_prefix,
  Token_Kind_star               = 8,
  Token_Kind_at                 = 9,
  Token_Kind_at_prefix          = 9 | Token_Kind_Flag_prefix,
  Token_Kind_dot                = 11,
  Token_Kind_equal              = 13,
  Token_Kind_colon              = 15,
  Token_Kind_brace_open         = 17,
  Token_Kind_brace_prefix_open  = 17 | Token_Kind_Flag_prefix,
  Token_Kind_brace_close        = 19,
  Token_Kind_paren_open         = 20 | Token_Kind_Flag_call_rhs,
  Token_Kind_paren_close        = 22,
  Token_Kind_curly_open         = 23 | Token_Kind_Flag_call_rhs,
  Token_Kind_curly_close        = 24,
  Token_Kind_semicolon          = 25,
  Token_Kind_comma              = 26,
  Token_Kind_arrow              = 33,
  Token_Kind_bang               = 35 | Token_Kind_Flag_call_rhs,
  Token_Kind_bang_equal         = 40,
  Token_Kind_equal_equal        = 41,
  Token_Kind_less_equal         = 42,
  Token_Kind_less               = 43,
  Token_Kind_greater_equal      = 44,
  Token_Kind_greater            = 45,
  Token_Kind_backslash          = 46,
  Token_Kind_quote              = 47,
  Token_Kind_str                = 48 | Token_Kind_Flag_call_rhs,
  Token_Kind_sharp              = 49 | Token_Kind_Flag_call_rhs,
  Token_Kind_slash              = 53,
  Token_Kind_percent            = 55,
  Token_Kind_dot_dot            = 57,
  Token_Kind_pipe               = 59,
  Token_Kind_pike               = 61,
  Token_Kind_ampersand          = 63,

  Token_Kind_name               = String_Kind_name | Token_Kind_Flag_call_rhs,
  Token_Kind_if                 = String_Kind_if | Token_Kind_Flag_call_rhs,
  Token_Kind_do                 = String_Kind_do,
  Token_Kind_else               = String_Kind_else,
  Token_Kind_return             = String_Kind_return,
  Token_Kind_break              = String_Kind_break,
  Token_Kind_while              = String_Kind_while,
} Token_Kind;

Cstr cstr_from_token_kind(Token_Kind kind) {
  switch (kind) {
  case Token_Kind_source_leave:      return "end of file";
  case Token_Kind_source_enter:      return "start of file";
  case Token_Kind_error:             return "invalid token";
  case Token_Kind_int:               return "integer";
  case Token_Kind_float:             return "float";
  case Token_Kind_plus:              return "+";
  case Token_Kind_plus_prefix:       return "+";
  case Token_Kind_minus:             return "-";
  case Token_Kind_minus_prefix:      return "-";
  case Token_Kind_star:              return "*";
  case Token_Kind_at:                return "@";
  case Token_Kind_at_prefix:         return "@";
  case Token_Kind_dot:               return ".";
  case Token_Kind_equal:             return "=";
  case Token_Kind_colon:             return ":";
  case Token_Kind_brace_open:        return "[";
  case Token_Kind_brace_prefix_open: return "[";
  case Token_Kind_brace_close:       return "]";
  case Token_Kind_paren_open:        return "(";
  case Token_Kind_paren_close:       return ")";
  case Token_Kind_curly_open:        return "{";
  case Token_Kind_curly_close:       return "}";
  case Token_Kind_semicolon:         return ";";
  case Token_Kind_comma:             return ",";
  case Token_Kind_arrow:             return "->";
  case Token_Kind_bang:              return "!";
  case Token_Kind_bang_equal:        return "!=";
  case Token_Kind_equal_equal:       return "==";
  case Token_Kind_less_equal:        return "<=";
  case Token_Kind_less:              return "<";
  case Token_Kind_greater_equal:     return ">=";
  case Token_Kind_greater:           return ">";
  case Token_Kind_backslash:         return "\\";
  case Token_Kind_quote:             return "'";
  case Token_Kind_str:               return "string";
  case Token_Kind_sharp:             return "#";
  case Token_Kind_slash:             return "/";
  case Token_Kind_percent:           return "%";
  case Token_Kind_dot_dot:           return "..";
  case Token_Kind_pipe:              return "|";
  case Token_Kind_pike:              return "^";
  case Token_Kind_ampersand:         return "&";
  case Token_Kind_name:              return "identifier";
  case Token_Kind_if:                return "if";
  case Token_Kind_do:                return "do";
  case Token_Kind_else:              return "else";
  case Token_Kind_return:            return "return";
  case Token_Kind_break:             return "break";
  case Token_Kind_while:             return "while";
  }
  return "token";
}
