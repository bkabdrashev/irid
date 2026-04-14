typedef const char* Cstr;
typedef struct { S32 index; } Istr;

typedef struct {
  C8* base;
  Umi length;
} Str;

typedef struct {
  C8* buffer_top;
  C8* buffer_bot;
  Str* strings;
  Umi len;
  Umi cap;
} Internal_Strings;

Internal_Strings internal_strings = {0};
void istr_init(Umi capacity) {
  capacity = power_of_2_up(capacity);
  internal_strings.buffer_bot = xmalloc(capacity);
  internal_strings.buffer_top = internal_strings.buffer_bot;
  internal_strings.strings = xcalloc(sizeof(Str), capacity);
  internal_strings.len     = 0;
  internal_strings.cap     = capacity;
}

Cstr cstr_from_istr(Istr istr) {
  return internal_strings.strings[istr.index].base;
}

Umi istr_length(Istr istr) {
  return internal_strings.strings[istr.index].length;
}

Istr istr_from_cstr(Cstr str) {
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
      return istr;
    }
    else if (slice.length == len && strncmp(slice.base, begin, len) == 0) {
      return istr;
    }
    istr.index++;
  }
}

typedef struct {
  C8* base;
  Umi size;
} String_Builder;

String_Builder string_builder_begin(Arena* arena, Umi capacity) {
  String_Builder sb;
  sb.base = arena_alloc(arena, capacity);
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

void string_builder_push_s64(String_Builder* sb, S64 val) {
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

B8 test_at_source(Cstr testee, Cstr expected, Cstr file_name, S32 line, Cstr source) {
  if (test_str(testee, expected)) {
    return true;
  }
  else {
    printf("%s(%i): at test source: %s\n", file_name, line, source);
    return false;
  }
}

