void irid_run_path(Cstr path) {
  Cstr builtin        = file_read("builtin.i");
  I32  builtin_length = strlen(builtin);
  Cstr source         = file_read(path);
  I32  source_length  = strlen(source);
  I32  length         = source_length + builtin_length + 32;

  Arena arena         = arena_init(KB(4) * length);
                        str_init(&arena, 2*length);

  Tokens builtin_tokens = lex_source(&arena, builtin);
  Ast_Block ast         = parse_tokens(&arena, builtin_tokens);

  Tokens source_tokens = lex_source(&arena, source);
  Ast_Block source_ast = parse_tokens(&arena, source_tokens);

  Ast_Node* node = ast_new_node(&arena, Ast_Kind_block);
  node->block    = source_ast;

  fa_init(&arena, ast.list, 1);
  fa_add(ast.list, node);

  Funs funs            = irgen_ast(&arena, ast, length);
                         sem_funs(&arena, funs);
  {
    C8* buffer           = arena_push(&arena, 64 * length);
    Cstr result          = cstr_from_sem(funs, buffer);
    printf("\n%s", result);
  }
                         llvm_funs(&arena, funs);

  arena_free(&arena);
}
