typedef enum String_Kind {
  String_Kind_none   = 0,
  String_Kind_name   = 128 | (1 << 9),
  String_Kind_if     = 129 | (1 << 9),
  String_Kind_do     = 139,
  String_Kind_else   = 140,
  String_Kind_return = 141,
  String_Kind_break  = 142,
  String_Kind_while  = 143,
} String_Kind;

typedef const char* Cstr;

typedef struct Str Str;
struct Str {
  String_Kind kind;
  I32         length;
  C8          base[];
};

typedef struct Internal Internal;
struct Internal {
  Arena* arena;
  Hash_Set set;
};

typedef struct String_Builder String_Builder;
struct String_Builder {
  C8* base;
  Umi size;
};

Internal internal = {0};
const Str* str_nil = {0};
String_Kind token_kind_from_str(Str* str) {
  return str->kind;
}

Str* str_from_cstr_with_kind(Cstr cstr, String_Kind kind) {
  I32 len = strlen(cstr);
  I32 i = hash_bytes(cstr, len);
  for (;;) {
    i &= internal.set.cap - 1;
    Str* str = internal.set.keys[i];
    if (!str) {
      Str* new_str = arena_push(internal.arena, sizeof(Str) + len * sizeof(C8));
      new_str->length = len;
      new_str->kind = kind;
      for (I32 j = 0; j < len; j++) {
        new_str->base[j] = cstr[j];
      }
      internal.set.keys[i] = new_str;
      return new_str;
    }
    else {
      if (str->length == len) {
        I32 j = 0;
        for (; j < str->length; j++) {
          if (str->base[j] != cstr[j]) {
            break;
          }
        }
        if (j == str->length) return str;
      }
    }
    i++;
  }
}

Str* str_from_range(Cstr begin, Cstr end) {
  I32 len = end - begin;
  I32 i = hash_bytes(begin, len);
  for (;;) {
    i &= internal.set.cap - 1;
    Str* str = internal.set.keys[i];
    if (!str) {
      Str* new_str = arena_push(internal.arena, sizeof(Str) + len * sizeof(C8));
      new_str->length = len;
      new_str->kind   = String_Kind_name;
      for (I32 j = 0; j < len; j++) {
        new_str->base[j] = begin[j];
      }
      internal.set.keys[i] = new_str;
      return new_str;
    }
    else {
      if (str->length == len) {
        I32 j = 0;
        for (; j < str->length; j++) {
          if (str->base[j] != begin[j]) {
            break;
          }
        }
        if (j == str->length) return str;
      }
    }
    i++;
  }
}

Str* str_from_cstr(Cstr cstr) {
  I32 len = strlen(cstr);
  Cstr end = cstr + len;
  return str_from_range(cstr, end);
}

void str_init(Arena* arena, I32 capacity) {
  internal.arena = arena;
  internal.set = hash_set_init(arena, capacity);
  str_from_cstr_with_kind("if", String_Kind_if);
  str_from_cstr_with_kind("do", String_Kind_do);
  str_from_cstr_with_kind("el", String_Kind_else);
  str_from_cstr_with_kind("re", String_Kind_return);
  str_from_cstr_with_kind("wh", String_Kind_while);
  str_from_cstr_with_kind("br", String_Kind_break);
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

void string_builder_push_indent(String_Builder* sb, I32 indent) {
  for (I32 i = 0; i < indent; i++) {
    sb->base[sb->size++] = ' ';
    sb->base[sb->size++] = ' ';
  }
}

void string_builder_push_i64(String_Builder* sb, I64 val) {
  C8 line_str[20];
  sprintf(line_str, "%li", val);
  string_builder_push_cstr(sb, line_str);
}

void string_builder_push_str(String_Builder* sb, Str* str) {
  for (I32 i = 0; i < str->length; i++) {
    sb->base[sb->size++] = str->base[i];
  }
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

void test_at_source(Cstr testee, Cstr expected, Cstr file_name, I32 line, Cstr source) {
  if (!test_str(testee, expected)) {
    printf("%s(%i): at test source: %s\n", file_name, line, source);
  }
}
