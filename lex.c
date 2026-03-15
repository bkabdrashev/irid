typedef struct {
  cstr source;
  cstr stream;
  b8   wasnewline;
  b8   wasspace;
} Lexer;

Lexer lexer = {0};



void lex_source(cc8* source, cc8* file_path) {
  lexer = { .source=source, .stream = source, .wasnewline = true, .wasspace = true };
}


