typedef struct Ast_Node Ast_Node;
typedef struct Decls Decls;
typedef struct Source Source;
typedef struct Ir   Ir;
typedef struct Type Type;
struct Source {
  Arena* arena;
  Str* path;
  Cstr data;
  Umi  length;
  U32* line_starts;
  I32  line_count;
  Ast_Node* ast;
  Ir*       ir_var;
  Hash_Map* decls; // str->sym
  Type*     type;
};

typedef struct Loc Loc;
struct Loc {
  Source* source;
  U32 start;
  U32 end;
};

typedef struct Source_Pool Source_Pool;
struct Source_Pool {
  I32     length;
  Source* base;
};

Loc loc_join(Loc one, Loc two) {
  Loc result = one;
  result.start = min(one.start, two.start);
  result.end   = max(one.end, two.end);
  return result;
}

Loc loc_at(Source* source, Cstr base, Cstr begin, Cstr end) {
  Loc loc = {0};
  loc.source = source;
  loc.start  = (U32)(begin - base);
  loc.end    = (U32)(end - base);
  return loc;
}

Str_View str_view_from_loc(Loc loc) {
  Str_View result = { loc.source->data + loc.start, loc.end - loc.start };
  return result;
}

Str* str_from_loc(Loc loc) {
  Str_View str_view = str_view_from_loc(loc);
  return str_from_str_view(str_view);
}

I64 i64_from_loc(Loc loc) {
  Str_View str_view = str_view_from_loc(loc);
  Cstr data = str_view.base;
  Cstr end  = str_view.base + str_view.length;
  U64 result = 0;
  I8 base = 10;

  if (*data == '0') {
    data++;
    if (data < end) {
      if (*data == 'x' || *data == 'X') {
        base = 16;
        data++;
      }
      else if (*data == 'b' || *data == 'B') {
        base = 2;
        data++;
      }
      else {
        base = 8;
      }
    }
  }
  for (; data < end; data++) {
    if (*data != '_') {
      I8 digit = 16;
      switch (*data) {
      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        digit = *data - '0';
      break;
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        digit = *data - 'a' + 10;
      break;
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        digit = *data - 'A' + 10;
      break;
      }
      result = result * base + digit;
    }
  }
  return (I64)result;
}

F64 f64_from_loc(Loc loc) {
  Str* str = str_from_loc(loc);
  return atof(str->base);
}

void string_builder_push_loc_str(String_Builder* sb, Loc loc) {
  Str_View str_view = str_view_from_loc(loc);
  string_builder_push_str_view(sb, str_view);
}

I32 source_line_count(Source* source) {
  I32 count = source->line_count;
  C8 last = source->data[source->length - 1];
  if (last == '\n' || last == '\r') {
    count -= 1;
  }
  return count > 0 ? count : 0;
}

I32 source_line_index(Source* source, U32 offset) {
  I32 low  = 0;
  I32 high = source->line_count - 1;
  while (low < high) {
    I32 mid = (low + high + 1) / 2;
    if (source->line_starts[mid] <= offset) {
      low = mid;
    }
    else {
      high = mid - 1;
    }
  }
  return low;
}

U32 source_line_start(Source* source, I32 line) {
  return source->line_starts[line];
}

U32 source_line_end(Source* source, I32 line) {
  U32 start = source_line_start(source, line);
  U32 end   = source->line_starts[line+1];
  while (end > start && (source->data[end-1] == '\n' || source->data[end-1] == '\r')) {
    end -= 1;
  }
  return end;
}

I32 source_line_column(Source* source, U32 offset) {
  I32 line = source_line_index(source, offset);
  return (I32)(offset - source_line_start(source, line)) + 1;
}
