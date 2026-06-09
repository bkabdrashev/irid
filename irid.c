void irid_run_path(Cstr path) {
  Cstr source          = file_read(path);
  I32 source_length    = strlen(source) + 32;
  Arena arena          = arena_init(KB(4) * source_length);
  str_init(&arena, 2*source_length);
  Tokens tokens        = lex_source(&arena, source);
  Ast_Block ast        = parse_tokens(&arena, tokens);
  Funs funs            = irgen_ast(&arena, ast, source_length);
                         sem_funs(&arena, funs);
  C8* buffer           = arena_push(&arena, 64 * source_length);
  Cstr result          = cstr_from_sem(funs, buffer);
  printf("sem:\n%s", result);
                         llvm_funs(&arena, funs);
  arena_free(&arena);
}
