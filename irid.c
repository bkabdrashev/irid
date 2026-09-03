void irid_run_path(Cstr path) {
  Cstr builtin       = file_read("builtin.i");
  I32 builtin_length = strlen(builtin) + 32;
  Cstr source        = file_read(path);
  I32 source_length  = strlen(source) + 32;
  I32 length = source_length + builtin_length;

  Arena arena        = arena_init(KB(4) * length);
                                 str_init(&arena, 2*length);

  Tokens source_tokens  = lex_source(&arena, source);
  Ast_Block source_ast  = parse_tokens(&arena, source_tokens);

  Ast_Node* node = arena_push(&arena, sizeof(Ast_Node));
  node->kind = Ast_Kind_block;
  node->block = source_ast;

  Tokens builtin_tokens = lex_source(&arena, builtin);
  Ast_Block ast = parse_tokens(&arena, builtin_tokens);
  // {
  //   C8* buffer    = arena_push(&arena, 4*length);
  //   Cstr result   = cstr_from_ast(buffer, ast);
  //   printf("%s\n", result);
  // }

  // {
  //   printf("%s\n", source);
  //   Cstr result = cstr_from_slice_token(&arena, source_tokens);
  //   printf("%s\n", result);
  // }
  I32 size = sizeof(Ast_List) + 1 * sizeof(Ast_Node*);
  ast.list = arena_push(&arena, size);
  ast.list->base[0] = node;
  ast.list->length = 1;

  {
    C8* buffer    = arena_push(&arena, 4*length);
    Cstr result   = cstr_from_ast(buffer, ast);
    printf("%s\n", result);
  }

  Funs funs            = irgen_ast(&arena, ast, length);
  {
    C8* buffer           = arena_push(&arena, 64 * length);
    Cstr result          = cstr_from_funs(funs, buffer);
    printf("\n%s", result);
  }

                         sem_funs(&arena, funs);
  {
    C8* buffer           = arena_push(&arena, 64 * length);
    Cstr result          = cstr_from_sem(funs, buffer);
    printf("\n%s", result);
  }
                         llvm_funs(&arena, funs);
  arena_free(&arena);
}
