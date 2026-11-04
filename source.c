typedef struct Ast_List Ast_List;
typedef struct Ast_Block Ast_Block;
struct Ast_Block {
  Hash_Map* scope;
  Ast_List* list;
};

typedef struct Source Source;
struct Source {
  Arena* arena;
  Str* path;
  Cstr data;
  Umi  length;
  U32* line_starts;
  I32  line_count;
  Ast_Block ast;
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

Str_View loc_str_view(Loc loc) {
  Str_View result = { loc.source->data + loc.start, loc.end - loc.start };
  return result;
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
