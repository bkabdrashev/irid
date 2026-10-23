I32 irid_run_path(Cstr path) {
  Cstr builtin        = file_read("basic.i");
  Umi  builtin_length = file_size("basic.i");
  Cstr source         = file_read(path);
  Umi  source_length  = file_size(path);
  if (!builtin || !source) {
    fprintf(stderr, "cannot read '%s'\n", !builtin ? "basic.i" : path);
    return 1;
  }
  I32  length         = (I32)(source_length + builtin_length + 32);

  Arena arena         = arena_init(KB(4) * length);
                        str_init(&arena, 2*length);

  Source_Map sources = {0};
  Diags diags = diags_init(&arena, 256);
  File_Id builtin_file = source_add(&sources, &arena, "basic.i", builtin, builtin_length);
  File_Id source_file  = source_add(&sources, &arena, path, source, source_length);
  if (!builtin_file || !source_file) {
    fprintf(stderr, "cannot register source files\n");
    arena_free(&arena);
    return 1;
  }

  Tokens builtin_tokens = lex_source(&arena, &sources, builtin_file, &diags);
  Ast_Block ast         = parse_tokens(&arena, builtin_tokens, &diags);

  Tokens source_tokens = lex_source(&arena, &sources, source_file, &diags);
  Ast_Block source_ast = parse_tokens(&arena, source_tokens, &diags);

  B8 color = isatty(STDERR_FILENO) && !getenv("NO_COLOR");
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stderr, color);
    arena_free(&arena);
    return 1;
  }

  Ast_Node* node = ast_new_node(&arena, Ast_Kind_block);
  node->block    = source_ast;

  fa_init(&arena, ast.list, 1);
  fa_add(ast.list, node);

  Funs funs            = irgen_ast(&arena, ast, length, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stderr, color);
    arena_free(&arena);
    return 1;
  }
  {
    Cstr result          = cstr_from_funs(&arena, funs);
    printf("\n%s", result);
  }
                         sem_funs(&arena, funs, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stderr, color);
    arena_free(&arena);
    return 1;
  }
  {
    Cstr result          = cstr_from_sem(&arena, funs);
    printf("\n%s", result);
  }
  diag_print_all(&diags, &sources, stderr, color);
                         llvm_funs(&arena, funs);

  arena_free(&arena);
  return 0;
}

// TODO: add import
