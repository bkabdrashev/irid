typedef enum {
  Diag_Note    = 0,
  Diag_Warning = 1,
  Diag_Error   = 2,
} Diag_Severity;

typedef enum {
  Diag_Code_none = 0,

  Diag_Code_unknown_character,
  Diag_Code_invalid_digit,
  Diag_Code_unterminated_string,
  Diag_Code_unterminated_comment,
  Diag_Code_missing_digits,
  Diag_Code_number_overflow,

  Diag_Code_expected_token,
  Diag_Code_expected_expression,
  Diag_Code_unexpected_token,
  Diag_Code_unclosed_delimiter,
  Diag_Code_unmatched_delimiter,
  Diag_Code_duplicate_declaration,
  Diag_Code_bad_declaration_target,
  Diag_Code_expected_block,
  Diag_Code_too_many_errors,

  Diag_Code_opened_here,
  Diag_Code_previous_declaration,
} Diag_Code;

typedef union {
  struct { Token_Kind expected; Token_Kind found; B8 end_of_line; } expected;
  struct { Token_Kind open; Token_Kind close; }     delimiter;
  struct { I32 digit; I32 base; }                    invalid_digit;
  struct { U32 byte; }                               unknown_char;
  struct { Str* name; }                              name;
  struct { Cstr text; }                              text;
} Diag_Data;

typedef struct {
  Diag_Severity severity;
  Diag_Code     code;
  Loc           loc;
  Diag_Data     data;
  I32           note_of;
} Diag;

#define DIAG_ERROR_LIMIT 50

typedef struct {
  Diag* base;
  I32   length;
  I32   cap;
  I32   error_count;
  I32   warning_count;
  B8    limit_hit;
  B8    printed;
} Diags;

Diags diags_init(Arena* arena, I32 cap) {
  Diags result = {0};
  result.base  = arena_push_zero(arena, sizeof(Diag) * cap);
  result.cap   = cap;
  return result;
}

Diag* diag_push(Diags* diags, Diag_Severity severity, Diag_Code code, Loc loc, I32 note_of) {
  if (!diags || diags->limit_hit) return NULL;
  if (severity == Diag_Error && diags->error_count >= DIAG_ERROR_LIMIT) {
    diags->limit_hit = true;
    return NULL;
  }
  if (diags->length >= diags->cap) {
    if (severity == Diag_Error) {
      diags->limit_hit = true;
    }
    return NULL;
  }
  Diag* diag = &diags->base[diags->length++];
  *diag = (Diag){
    .severity = severity,
    .code     = code,
    .loc      = loc,
    .note_of  = note_of,
  };
  if (severity == Diag_Error) {
    diags->error_count += 1;
  }
  else if (severity == Diag_Warning) {
    diags->warning_count += 1;
  }
  return diag;
}

Diag* diag_error(Diags* diags, Diag_Code code, Loc loc) {
  return diag_push(diags, Diag_Error, code, loc, -1);
}

Diag* diag_warning(Diags* diags, Diag_Code code, Loc loc) {
  return diag_push(diags, Diag_Warning, code, loc, -1);
}

Diag* diag_note(Diags* diags, I32 parent, Diag_Code code, Loc loc) {
  return diag_push(diags, Diag_Note, code, loc, parent);
}

B8 diags_has_errors(Diags* diags) {
  return diags && (diags->error_count > 0 || diags->limit_hit);
}

Cstr cstr_from_diag_code(Diag_Code code) {
  switch (code) {
  case Diag_Code_unknown_character:      return "L001";
  case Diag_Code_invalid_digit:          return "L002";
  case Diag_Code_unterminated_string:    return "L003";
  case Diag_Code_unterminated_comment:   return "L004";
  case Diag_Code_missing_digits:         return "L005";
  case Diag_Code_number_overflow:        return "L006";
  case Diag_Code_expected_token:         return "P001";
  case Diag_Code_expected_expression:    return "P002";
  case Diag_Code_unexpected_token:       return "P003";
  case Diag_Code_unclosed_delimiter:     return "P004";
  case Diag_Code_unmatched_delimiter:    return "P005";
  case Diag_Code_duplicate_declaration:  return "P006";
  case Diag_Code_bad_declaration_target: return "P007";
  case Diag_Code_expected_block:         return "P008";
  case Diag_Code_too_many_errors:        return "P009";
  case Diag_Code_opened_here:            return "N001";
  case Diag_Code_previous_declaration:   return "N002";
  default:                               return "????";
  }
}

static Cstr diag_color_error(void) { return "\033[1;31m"; }
static Cstr diag_color_warn(void)  { return "\033[1;33m"; }
static Cstr diag_color_note(void)  { return "\033[1;36m"; }
static Cstr diag_color_dim(void)   { return "\033[2m"; }
static Cstr diag_color_reset(void) { return "\033[0m"; }

static void diag_print_lexeme(FILE* out, Source_Map* sources, Loc loc) {
  Source_File* file = source_get(sources, loc.file);
  if (!file || loc.start >= file->length) return;
  U32 start = loc.start;
  U32 end   = loc.end;
  if (end > file->length) end = (U32)file->length;
  if (end <= start) end = start + 1;
  U32 length = end - start;
  if (length > 32) {
    fwrite(file->data + start, 1, 29, out);
    fputs("...", out);
  }
  else {
    fwrite(file->data + start, 1, length, out);
  }
}

static void diag_print_found(FILE* out, Source_Map* sources, Token_Kind kind, Loc loc) {
  if (kind == Token_Kind_source_leave) {
    fputs("end of file", out);
    return;
  }
  if (kind == Token_Kind_source_enter) {
    fputs("start of file", out);
    return;
  }
  if (kind == Token_Kind_error) {
    fputs("invalid token", out);
    return;
  }
  if (kind == Token_Kind_name || kind == Token_Kind_int
      || kind == Token_Kind_float || kind == Token_Kind_str) {
    fputc('\'', out);
    diag_print_lexeme(out, sources, loc);
    fputc('\'', out);
    return;
  }
  fprintf(out, "'%s'", cstr_from_token_kind(kind));
}

static void diag_print_message(FILE* out, Diag* diag, Source_Map* sources) {
  switch (diag->code) {
  case Diag_Code_unknown_character: {
    U8 byte = (U8)diag->data.unknown_char.byte;
    if (isprint(byte)) {
      fprintf(out, "unknown character '%c'", byte);
    }
    else {
      fprintf(out, "unknown character 0x%02x", byte);
    }
  } break;
  case Diag_Code_invalid_digit:
    fprintf(out, "digit '%c' is not valid in a base-%d literal",
            diag->data.invalid_digit.digit, diag->data.invalid_digit.base);
  break;
  case Diag_Code_unterminated_string:
    fputs("unterminated string literal", out);
  break;
  case Diag_Code_unterminated_comment:
    fputs("unterminated block comment", out);
  break;
  case Diag_Code_missing_digits:
    fprintf(out, "expected at least one digit after '%s'", diag->data.text.text);
  break;
  case Diag_Code_number_overflow:
    fputs("numeric literal is too large", out);
  break;
  case Diag_Code_expected_token:
    fprintf(out, "expected '%s' but found ",
            cstr_from_token_kind(diag->data.expected.expected));
    diag_print_found(out, sources, diag->data.expected.found, diag->loc);
  break;
  case Diag_Code_expected_expression:
    if (diag->data.expected.end_of_line) {
      fputs("expected an expression, found end of line", out);
    }
    else {
      fputs("expected an expression, found ", out);
      diag_print_found(out, sources, diag->data.expected.found, diag->loc);
    }
  break;
  case Diag_Code_unexpected_token:
    fputs("unexpected ", out);
    diag_print_found(out, sources, diag->data.expected.found, diag->loc);
  break;
  case Diag_Code_unclosed_delimiter:
    fprintf(out, "expected '%s' to close this '%s'",
            cstr_from_token_kind(diag->data.delimiter.close),
            cstr_from_token_kind(diag->data.delimiter.open));
  break;
  case Diag_Code_unmatched_delimiter:
    fprintf(out, "unmatched '%s'",
            cstr_from_token_kind(diag->data.delimiter.open));
  break;
  case Diag_Code_duplicate_declaration:
    fprintf(out, "duplicate declaration of '%s'", diag->data.name.name->base);
  break;
  case Diag_Code_bad_declaration_target:
    fputs("expected a name before ':'", out);
  break;
  case Diag_Code_expected_block:
    fputs("expected an indented block", out);
  break;
  case Diag_Code_too_many_errors:
    fputs("too many errors emitted; stopping now", out);
  break;
  case Diag_Code_opened_here:
    fprintf(out, "'%s' opened here",
            cstr_from_token_kind(diag->data.delimiter.open));
  break;
  case Diag_Code_previous_declaration:
    fprintf(out, "'%s' was first declared here", diag->data.name.name->base);
  break;
  default:
    fputs("unknown diagnostic", out);
  break;
  }
}

static I32 diag_display_column(Source_File* file, U32 line_start, U32 offset) {
  I32 column = 0;
  if (offset > file->length) offset = (U32)file->length;
  for (U32 at = line_start; at < offset; at++) {
    if (file->data[at] == '\t') {
      column = (column / 4 + 1) * 4;
    }
    else {
      column += 1;
    }
  }
  return column;
}

static void diag_print_window(FILE* out, Source_File* file, U32 start, U32 end,
                              I32 window_start, I32 window_end, I32 line_width) {
  I32 column = 0;
  if (window_start > 0) fputs("...", out);
  for (U32 at = start; at < end; at++) {
    if (column >= window_end) break;
    C8 c = file->data[at];
    if (c == '\t') {
      I32 next = (column / 4 + 1) * 4;
      for (I32 at_column = column; at_column < next; at_column++) {
        if (at_column >= window_start && at_column < window_end) {
          fputc(' ', out);
        }
      }
      column = next;
    }
    else {
      if (column >= window_start) fputc(c, out);
      column += 1;
    }
  }
  if (window_end < line_width) fputs("...", out);
}

static I32 diag_digits(I32 value) {
  I32 result = 1;
  while (value >= 10) {
    value /= 10;
    result += 1;
  }
  return result;
}

static void diag_print_snippet(FILE* out, Source_Map* sources, Loc loc, Cstr label,
                               Cstr caret_color, B8 color) {
  if (!loc_is_valid(loc)) return;
  Source_File* file = source_get(sources, loc.file);
  if (!file) return;

  I32 line        = source_line_index(file, loc.start);
  U32 line_start  = source_line_start(file, line);
  U32 line_end    = source_line_end(file, line);
  I32 caret_start = diag_display_column(file, line_start, loc.start);

  U32 caret_end_offset = loc.end;
  if (caret_end_offset <= loc.start) caret_end_offset = loc.start + 1;
  if (caret_end_offset > line_end) caret_end_offset = line_end;
  I32 caret_end = diag_display_column(file, line_start, caret_end_offset);
  if (caret_end <= caret_start) caret_end = caret_start + 1;

  I32 line_width = diag_display_column(file, line_start, line_end);
  I32 max_width  = 120;
  I32 window_start = 0;
  if (line_width > max_width && caret_start > max_width - 20) {
    window_start = caret_start - 40;
    if (window_start < 0) window_start = 0;
  }
  I32 window_end = window_start + max_width;
  if (window_end > line_width) window_end = line_width;
  I32 prefix_width = window_start > 0 ? 3 : 0;

  I32 shown_caret_start = caret_start;
  if (shown_caret_start < window_start) shown_caret_start = window_start;
  if (shown_caret_start >= window_end
      && window_end < line_width
      && window_end > window_start) {
    shown_caret_start = window_end - 1;
  }
  I32 shown_caret_end = caret_end;
  if (shown_caret_end > window_end) shown_caret_end = window_end;
  if (shown_caret_end <= shown_caret_start) {
    shown_caret_end = shown_caret_start + 1;
  }
  I32 caret_column = prefix_width + (shown_caret_start - window_start);
  I32 caret_width  = shown_caret_end - shown_caret_start;
  if (caret_width < 1) caret_width = 1;

  I32 width = diag_digits(line + 1);
  Cstr dim = color ? diag_color_dim() : "";
  Cstr reset = color ? diag_color_reset() : "";
  Cstr caret = color ? caret_color : "";

  fprintf(out, "%*s%s-->%s %s:%d:%d\n",
          width, "", dim, reset, file->path, line + 1, caret_start + 1);
  fprintf(out, "%*s%s|%s\n", width + 1, "", dim, reset);
  fprintf(out, "%*d %s|%s ", width, line + 1, dim, reset);
  diag_print_window(out, file, line_start, line_end,
                    window_start, window_end, line_width);
  fputc('\n', out);

  fprintf(out, "%*s%s|%s ", width + 1, "", dim, reset);
  for (I32 i = 0; i < caret_column; i++) fputc(' ', out);
  fputs(caret, out);
  for (I32 i = 0; i < caret_width; i++) fputc('^', out);
  fputs(reset, out);
  if (label) {
    fprintf(out, " %s", label);
  }
  fputc('\n', out);
}

static void diag_print_one(FILE* out, Diags* diags, I32 index, Source_Map* sources, B8 color) {
  Diag* diag = &diags->base[index];
  Cstr sev;
  Cstr sev_color;
  switch (diag->severity) {
  case Diag_Error:
    sev = "error";
    sev_color = color ? diag_color_error() : "";
  break;
  case Diag_Warning:
    sev = "warning";
    sev_color = color ? diag_color_warn() : "";
  break;
  default:
    sev = "note";
    sev_color = color ? diag_color_note() : "";
  break;
  }
  Cstr reset = color ? diag_color_reset() : "";

  if (diag->severity == Diag_Note) {
    fprintf(out, "%snote%s: ", sev_color, reset);
  }
  else {
    fprintf(out, "%s%s[%s]%s: ",
            sev_color, sev, cstr_from_diag_code(diag->code), reset);
  }
  diag_print_message(out, diag, sources);
  fputc('\n', out);
  Cstr caret_color = diag_color_error();
  if (diag->severity == Diag_Warning) caret_color = diag_color_warn();
  else if (diag->severity == Diag_Note) caret_color = diag_color_note();
  diag_print_snippet(out, sources, diag->loc, NULL, caret_color, color);
}

void diag_print_all(Diags* diags, Source_Map* sources, FILE* out, B8 color) {
  if (!diags || diags->printed) return;
  diags->printed = true;

  for (I32 i = 0; i < diags->length; i++) {
    Diag* diag = &diags->base[i];
    if (diag->severity == Diag_Note) continue;
    diag_print_one(out, diags, i, sources, color);
    for (I32 j = 0; j < diags->length; j++) {
      if (diags->base[j].severity == Diag_Note && diags->base[j].note_of == i) {
        diag_print_one(out, diags, j, sources, color);
      }
    }
  }

  if (diags->limit_hit) {
    fprintf(out, "%serror[P009]%s: too many errors emitted; stopping now\n",
            color ? diag_color_error() : "", color ? diag_color_reset() : "");
  }
  if (diags->error_count > 0 || diags->warning_count > 0) {
    fprintf(out, "%d error%s, %d warning%s\n",
            diags->error_count, diags->error_count == 1 ? "" : "s",
            diags->warning_count, diags->warning_count == 1 ? "" : "s");
  }
}
