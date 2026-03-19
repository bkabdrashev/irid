typedef const char* cstr;
typedef struct { s32 index; } Istr;

typedef struct {
  c8* base;
  umi length;
} Str;

typedef struct {
  c8* buffer_top;
  c8* buffer_bot;
  Str* strings;
  umi len;
  umi cap;
} Internal_Strings;

Internal_Strings internal_strings = {0};
void istr_init() {
  internal_strings.buffer_bot = malloc(MB(4));
  internal_strings.buffer_top = internal_strings.buffer_bot;
  internal_strings.strings = malloc(MB(1));
  internal_strings.len     = 0;
  internal_strings.cap     = MB(1);
}

cstr cstr_from_istr(Istr istr) {
  return internal_strings.strings[istr.index].base;
}

umi istr_length(Istr istr) {
  return internal_strings.strings[istr.index].length;
}

Istr istr_from_cstr(cstr str) {
  umi len  = strlen(str);
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

Istr istr_from_range(cstr begin, cstr end) {
  umi len   = end - begin;
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
  c8* base;
  umi length;
} String_Builder;

String_Builder string_builder_begin(Arena* arena) {
  String_Builder sb;
  sb.base = arena_alloc(arena, MB(1));
  sb.length = 0;
  return sb;
}

cstr string_builder_end(String_Builder* sb) {
  return sb->base;
}

void string_builder_push_cstr(String_Builder* sb, cstr str) {
  strcat(sb->base, str);
}

void string_builder_push_istr(String_Builder* sb, Istr str) {
  cstr internal = cstr_from_istr(str);
  string_builder_push_cstr(sb, internal);
}

b8 cstr_eq(cstr a, cstr b) {
  return strcmp(a, b) == 0;
}

b8 test_str(cstr one, cstr two) {
  umi o = strlen(one);
  umi t = strlen(two);
  if (o != t) {
    printf("Test error length %zu vs %zu:\n%s\nvs\n%s\n", o, t, one, two);
    return false;
  }
  for (umi u = 0; u < o; u++) {
    if (one[u] != two[u]) {          
      printf("Test error '%c' vs '%c' at %zu:\n%s\nvs\n%s\n", one[u], two[u], u, one, two);
      return false;
    }
  }
  return true;
}

b8 test_at_source(cstr testee, cstr expected, cstr file_name, s32 line, cstr source) {
  if (test_str(testee, expected)) {
    return true;
  }
  else {
    printf("%s(%i): at test source: %s\n", file_name, line, source);
    return false;
  }
}

