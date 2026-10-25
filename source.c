typedef I32 File_Id;

typedef struct {
  File_Id file;
  U32     start;
  U32     end;
} Loc;

typedef struct {
  Cstr path;
  Cstr data;
  Umi  length;
  U32* line_starts;
  I32  line_count;
} Source_File;

#define SOURCE_MAX 128

typedef struct {
  Source_File base[SOURCE_MAX];
  I32         length;
} Source_Map;

#define LOC_NONE ((Loc){0})

B8 loc_is_valid(Loc loc) {
  return loc.file > 0;
}

Loc loc_join(Loc one, Loc two) {
  if (!loc_is_valid(one)) return two;
  if (!loc_is_valid(two) || one.file != two.file) return one;
  Loc result = one;
  result.start = min(one.start, two.start);
  result.end   = max(one.end, two.end);
  return result;
}

Loc loc_at(File_Id file, Cstr base, Cstr begin, Cstr end) {
  Loc loc = {0};
  loc.file  = file;
  loc.start = (U32)(begin - base);
  loc.end   = (U32)(end - base);
  return loc;
}

I32 source_add(Source_Map* map, Arena* arena, Cstr path, Cstr data, Umi length) {
  if (map->length >= SOURCE_MAX) return 0;
  if (length > (Umi)UINT32_MAX) return 0;

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

  Source_File* file = &map->base[map->length++];
  file->path        = path;
  file->data        = data;
  file->length      = length;
  file->line_starts = line_starts;
  file->line_count  = line_count;
  return map->length;
}

Source_File* source_get(Source_Map* map, File_Id file) {
  if (file <= 0 || file > map->length) return NULL;
  return &map->base[file-1];
}

I32 source_line_count(Source_File* file) {
  if (!file || file->length == 0) return 0;
  I32 count = file->line_count;
  C8 last = file->data[file->length - 1];
  if (last == '\n' || last == '\r') {
    count -= 1;
  }
  return count > 0 ? count : 0;
}

I32 source_line_index(Source_File* file, U32 offset) {
  if (offset > file->length) offset = (U32)file->length;
  I32 low  = 0;
  I32 high = file->line_count - 1;
  while (low < high) {
    I32 mid = (low + high + 1) / 2;
    if (file->line_starts[mid] <= offset) {
      low = mid;
    }
    else {
      high = mid - 1;
    }
  }
  return low;
}

U32 source_line_start(Source_File* file, I32 line) {
  if (line < 0) line = 0;
  if (line >= file->line_count) line = file->line_count - 1;
  return file->line_starts[line];
}

U32 source_line_end(Source_File* file, I32 line) {
  U32 start = source_line_start(file, line);
  U32 end   = (line + 1 < file->line_count)
            ? file->line_starts[line+1]
            : (U32)file->length;
  while (end > start && (file->data[end-1] == '\n' || file->data[end-1] == '\r')) {
    end -= 1;
  }
  return end;
}

I32 source_line_column(Source_File* file, U32 offset) {
  if (offset > file->length) offset = (U32)file->length;
  I32 line = source_line_index(file, offset);
  return (I32)(offset - source_line_start(file, line)) + 1;
}
