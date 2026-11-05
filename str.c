typedef struct Str Str;
struct Str {
  I32 length;
  C8  base[];
};

typedef struct Internal Internal;
struct Internal {
  Arena* arena;
  Hash_Set set;
};

typedef struct String_Builder String_Builder;
struct String_Builder {
  Arena* arena;
  C8*    base;
  Umi    size;
  Umi    cap;
};

Str_View str_view(Cstr start, Cstr end) {
  Str_View result = { start, end - start };
  return result;
}

Internal internal_strs  = {0};
const Str* str_nil = {0};

Cstr cstr_from_str(Str* str) {
  return str->base;
}

Internal internal_init(Arena* arena, Umi capacity) {
  Internal internal = {};
  internal.arena = arena;
  internal.set = hash_set_init(arena, capacity);
  return internal;
}

Str* internal_from_range(Internal internal, Cstr begin, Cstr end) {
  I32 len = end - begin;
  I32 i = hash_bytes(begin, len);
  for (;;) {
    i &= internal.set.cap - 1;
    Str* str = internal.set.keys[i];
    if (!str) {
      Str* new_str = arena_push(internal.arena, sizeof(Str) + (len+1) * sizeof(C8));
      new_str->length = len;
      for (I32 j = 0; j < len; j++) {
        new_str->base[j] = begin[j];
      }
      new_str->base[len] = '\0';
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

Str* str_from_range(Cstr begin, Cstr end) {
  return internal_from_range(internal_strs, begin, end);
}

Str* internal_from_cstr(Internal internal, Cstr cstr) {
  I32 len = strlen(cstr);
  Cstr end = cstr + len;
  return internal_from_range(internal, cstr, end);
}

Str* internal_from_str_view(Internal internal, Str_View str_view) {
  return internal_from_range(internal, str_view.base, str_view.base + str_view.length);
}

Str* str_from_str_view(Str_View str_view) {
  return internal_from_str_view(internal_strs, str_view);
}

Str* str_from_cstr(Cstr cstr) {
  I32 len = strlen(cstr);
  Cstr end = cstr + len;
  return str_from_range(cstr, end);
}

void str_init(Arena* arena, I32 capacity) {
  internal_strs.arena = arena;
  internal_strs.set = hash_set_init(arena, capacity);
}

void string_builder_reserve(String_Builder* sb, Umi extra) {
  assert(extra <= SIZE_MAX - sb->size - 1);
  Umi needed = sb->size + extra + 1;
  if (needed <= sb->cap) return;
  assert(sb->arena);
  Umi new_cap = sb->cap ? sb->cap : 256;
  while (new_cap < needed) {
    if (new_cap > SIZE_MAX / 2) {
      new_cap = needed;
      break;
    }
    new_cap *= 2;
  }
  C8* new_base = arena_push(sb->arena, new_cap);
  if (sb->base) {
    memcpy(new_base, sb->base, sb->size);
  }
  sb->base = new_base;
  sb->cap  = new_cap;
}

String_Builder string_builder_begin(Arena* arena) {
  String_Builder sb = { .arena = arena };
  return sb;
}

Cstr string_builder_end(String_Builder* sb) {
  string_builder_reserve(sb, 0);
  sb->base[sb->size] = '\0';
  return sb->base;
}

void string_builder_push_cstr(String_Builder* sb, Cstr str) {
  Umi length = strlen(str);
  string_builder_reserve(sb, length);
  memcpy(sb->base + sb->size, str, length);
  sb->size += length;
}

void string_builder_push_str_view(String_Builder* sb, Str_View str_view) {
  string_builder_reserve(sb, str_view.length);
  memcpy(sb->base + sb->size, str_view.base, str_view.length);
  sb->size += str_view.length;
}

void string_builder_push_indent(String_Builder* sb, I32 indent) {
  Umi length = indent * 2;
  string_builder_reserve(sb, length);
  for (I32 i = 0; i < indent; i++) {
    sb->base[sb->size++] = ' ';
    sb->base[sb->size++] = ' ';
  }
}

void string_builder_push_i64(String_Builder* sb, I64 val) {
  C8 line_str[21];
  sprintf(line_str, "%li", val);
  string_builder_push_cstr(sb, line_str);
}

void string_builder_push_f64(String_Builder* sb, F64 val) {
  C8 line_str[64];
  snprintf(line_str, sizeof(line_str), "%g", val);
  string_builder_push_cstr(sb, line_str);
}

void string_builder_push_str(String_Builder* sb, Str* str) {
  string_builder_reserve(sb, str->length);
  memcpy(sb->base + sb->size, str->base, str->length);
  sb->size += str->length;
}

void string_builder_push_string_builder(String_Builder* sb, String_Builder one) {
  string_builder_reserve(sb, one.size);
  if (one.size) {
    memcpy(sb->base + sb->size, one.base, one.size);
    sb->size += one.size;
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
