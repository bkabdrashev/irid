typedef enum {
  Token_Kind_Flag_prefix   = 1 << 8,
  Token_Kind_Flag_call_rhs = 1 << 9,
} Token_Kind_Flag;

typedef enum {
  Token_Kind_source_leave       = 0,
  Token_Kind_source_enter       = 1,
  Token_Kind_name               = 2 | Token_Kind_Flag_call_rhs,
  Token_Kind_int                = 3 | Token_Kind_Flag_call_rhs,
  Token_Kind_plus               = 4,
  Token_Kind_plus_prefix        = 4 | Token_Kind_Flag_prefix,
  Token_Kind_minus              = 6,
  Token_Kind_minus_prefix       = 6 | Token_Kind_Flag_prefix,
  Token_Kind_star               = 8,
  Token_Kind_at                 = 9,
  Token_Kind_at_prefix          = 9 | Token_Kind_Flag_prefix,
  Token_Kind_dot                = 10,
  Token_Kind_equal              = 11,
  Token_Kind_colon              = 12,
  Token_Kind_brace_open         = 13,
  Token_Kind_brace_prefix_open  = 14 | Token_Kind_Flag_prefix,
  Token_Kind_brace_close        = 15,
  Token_Kind_paren_open         = 16 | Token_Kind_Flag_call_rhs,
  Token_Kind_paren_close        = 17,
  Token_Kind_curly_open         = 18 | Token_Kind_Flag_call_rhs,
  Token_Kind_curly_close        = 19,
  Token_Kind_semicolon          = 20,
  Token_Kind_comma              = 21,
  Token_Kind_if                 = 22 | Token_Kind_Flag_call_rhs,
  Token_Kind_do                 = 23,
  Token_Kind_else               = 24,
  Token_Kind_arrow              = 25,
  Token_Kind_return             = 26,
  Token_Kind_while              = 27,
  Token_Kind_break              = 28,
} Token_Kind;

typedef const char* Cstr;
typedef struct { I32 index; } Istr;

typedef struct {
  C8* base;
  Umi length;
} Str;

typedef struct {
  C8* buffer_top;
  C8* buffer_bot;
  Str* strings;
  Token_Kind* token_kinds;
  Umi cap;
} Internal_Strings;

typedef struct {
  C8* base;
  Umi size;
} String_Builder;

Internal_Strings internal_strings = {0};
void istr_init(Umi capacity) {
  capacity = power_of_2_up(capacity);
  internal_strings.buffer_bot  = xmalloc(capacity);
  internal_strings.buffer_top  = internal_strings.buffer_bot;
  internal_strings.strings     = xcalloc(sizeof(Str), capacity);
  internal_strings.token_kinds = xcalloc(sizeof(Token_Kind), capacity);
  internal_strings.cap = capacity;
}

Token_Kind token_kind_from_istr(Istr istr) {
  return internal_strings.token_kinds[istr.index];
}

Cstr cstr_from_istr(Istr istr) {
  return internal_strings.strings[istr.index].base;
}

Umi istr_length(Istr istr) {
  return internal_strings.strings[istr.index].length;
}

Istr istr_from_cstr_token_kind(Cstr str, Token_Kind kind) {
  Umi len  = strlen(str);
  Istr istr = { .index = hash_bytes(str, len) };
  for (;;) {
    istr.index &= internal_strings.cap - 1;
    Str slice = internal_strings.strings[istr.index];
    if (!slice.base) {
      slice.base = internal_strings.buffer_top;
      slice.length = len;
      memcpy(slice.base, str, len + 1);
      internal_strings.buffer_top += len + 1;
      internal_strings.strings[istr.index] = slice;
      internal_strings.token_kinds[istr.index] = kind;
      return istr;
    }
    else if (slice.length == len && strncmp(slice.base, str, len) == 0) {
      return istr;
    }
    istr.index++;
  }
  return (Istr){0};
}

Istr istr_from_range(Cstr begin, Cstr end) {
  Umi len   = end - begin;
  Istr istr = { .index = hash_bytes(begin, len) };
  for (;;) {
    istr.index &= internal_strings.cap - 1;
    Str slice = internal_strings.strings[istr.index];
    if (!slice.base) {
      slice.base = internal_strings.buffer_top;
      slice.length = len;
      memcpy(slice.base, begin, len);
      slice.base[len] = '\0';
      internal_strings.buffer_top += len + 1;
      internal_strings.strings[istr.index] = slice;
      internal_strings.token_kinds[istr.index] = Token_Kind_name;
      return istr;
    }
    else if (slice.length == len && strncmp(slice.base, begin, len) == 0) {
      return istr;
    }
    istr.index++;
  }
}

String_Builder string_builder_begin(C8* buffer) {
  String_Builder sb;
  sb.base = buffer;
  sb.size = 0;
  return sb;
}

Cstr string_builder_end(String_Builder* sb) {
  sb->base[sb->size] = '\0';
  return sb->base;
}

void string_builder_push_cstr(String_Builder* sb, Cstr str) {
  while (*str) {
    sb->base[sb->size++] = *str++;
  }
}

void string_builder_push_s64(String_Builder* sb, I64 val) {
  C8 line_str[20];
  sprintf(line_str, "%li", val);
  string_builder_push_cstr(sb, line_str);
}

void string_builder_push_istr(String_Builder* sb, Istr str) {
  Cstr internal = cstr_from_istr(str);
  string_builder_push_cstr(sb, internal);
}

void string_builder_push_string_builder(String_Builder* sb, String_Builder one) {
  for (Umi i = 0; i < one.size; i++) {
    sb->base[sb->size++] = one.base[i];
  }
}

B8 cstr_eq(Cstr a, Cstr b) {
  return strcmp(a, b) == 0;
}

B8 test_str(Cstr one, Cstr two) {
  Umi o = strlen(one);
  Umi t = strlen(two);
  if (o != t) {
    printf("Test error length %zu vs %zu:\n%s\nvs\n%s\n", o, t, one, two);
    return false;
  }
  for (Umi u = 0; u < o; u++) {
    if (one[u] != two[u]) {          
      printf("Test error '%c' vs '%c' at %zu:\n%s\nvs\n%s\n", one[u], two[u], u, one, two);
      return false;
    }
  }
  return true;
}

B8 test_at_source(Cstr testee, Cstr expected, Cstr file_name, I32 line, Cstr source) {
  if (test_str(testee, expected)) {
    return true;
  }
  else {
    printf("%s(%i): at test source: %s\n", file_name, line, source);
    return false;
  }
}
