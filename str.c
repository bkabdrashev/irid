typedef const char* cstr;
typedef struct { s32 index; } Istr;

typedef struct {
  c8* base;
  umi length;
} Str;

typedef struct {
  umi buffer_len;
  c8* buffer;
  Str* strings;
  Istr* istrs;
  umi len;
  umi cap;
} Internal_Strings;

Internal_Strings internal_strings = {0};
void istr_init() {
  internal_strings.buffer  = malloc(MB(4));
  internal_strings.strings = malloc(MB(1));
  internal_strings.istrs   = malloc(MB(1));
  internal_strings.len        = 1;
  internal_strings.cap        = MB(1);
  internal_strings.buffer_len = 1;
  internal_strings.buffer[0]  = '\0';
  internal_strings.lengths[0] = 0;
  internal_strings.strings[0] = (Str){ .base = internal_strings.buffer, .length = 0 };
}

cstr cstr_from_istr(Istr istr) {
  // TODO: figure it out!
  return internal_strings.strings[istr.index].base;
}

Istr istr_from_cstr(cstr str) {
  umi len  = strlen(str);
  assert(IS_POW2(internal_strings.cap));
  assert(internal_strings.len < internal_strings.cap);
  u64 i = hash_bytes(str, len);
  for (;;) {
    i &= internal_strings->cap - 1;
    Str slice = internal_strings.strings[i];
    if (!slice.base) {
      slice.base = internal_strings.buffer + internal_strings.buffer_len;
      slice.length = len;
      memcpy(slice.base, str, len + 1);
      internal_strings.buffer_len += len + 1;
      return istr;
    }
    else {
      cstr iternal_cstr  = internal_strings.strings[istr.index];
      umi iternal_length = internal_strings.lengths[istr.index];
      if (iternal_length == len && strncmp(iternal_cstr, str, len) == 0) {
        return internal_strings->istrs[i];
      }
    }
    i++;
  }
  return (Istr){0};
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
  cstr internal = istr_get(str);
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

