typedef struct Source Source;
struct Source {
  Arena* arena;
  Cstr path;
  Cstr data;
  Umi  length;
  U32* line_starts;
  I32  line_count;
};

typedef struct Loc Loc;
struct Loc {
  Source* source;
  U32 start;
  U32 end;
};

#define SOURCE_MAX 128

typedef struct Source_Pool Source_Pool;
struct Source_Pool {
  I32     length;
  Source* base;
};

typedef struct Sources Sources;
struct Sources {
  Source_Pool pool;
  Internal    paths;
  Hash_Map    map;   // path -> source
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

Source* sources_add(Sources* sources, Cstr path, Cstr data, Umi length) {
  if (sources->map.len >= SOURCE_MAX) assert(0);
  if (length > (Umi)UINT32_MAX)      assert(0);

  Str* internal_path = internal_from_cstr(sources->paths, path);
  Source* source = hash_map_get(sources->map, internal_path);
  if (!source) {
    Arena* arena = arena_init(KB(1) * length);
    I32 line_count = 1;
    for (Umi i = 0; i < length; i++) {
      if (data[i] == '\n') {
        line_count += 1;
      }
      else if (data[i] == '\r') {
        line_count += 1;
        if (i + 1 < length && data[i+1] == '\n') i += 1;
      }
    }

    U32* line_starts = arena_push(arena, sizeof(U32) * line_count);
    line_starts[0] = 0;
    I32 line = 1;
    for (Umi i = 0; i < length; i++) {
      if (data[i] == '\n') {
        line_starts[line++] = (U32)(i + 1);
      }
      else if (data[i] == '\r') {
        if (i + 1 < length && data[i+1] == '\n') i += 1;
        line_starts[line++] = (U32)(i + 1);
      }
    }
    assert(line == line_count);

    Source* source = &new(sources->pool);
            source->arena       = arena;
            source->path        = path;
            source->data        = data;
            source->length      = length;
            source->line_starts = line_starts;
            source->line_count  = line_count;
  }
  return source;
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

U32 source_line_start(Source* file, I32 line) {
  return file->line_starts[line];
}

U32 source_line_end(Source* file, I32 line) {
  U32 start = source_line_start(file, line);
  U32 end   = (line + 1 < file->line_count)
            ? file->line_starts[line+1]
            : (U32)file->length;
  while (end > start && (file->data[end-1] == '\n' || file->data[end-1] == '\r')) {
    end -= 1;
  }
  return end;
}

I32 source_line_column(Source* file, U32 offset) {
  I32 line = source_line_index(file, offset);
  return (I32)(offset - source_line_start(file, line)) + 1;
}
