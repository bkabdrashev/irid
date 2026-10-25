I32 irid_run_path(Cstr path) {
  profile_run_begin();
  profile_stage_begin("read files", 0);
  Cstr builtin        = file_read("basic.i");
  Umi  builtin_length = file_size("basic.i");
  Cstr source         = file_read(path);
  Umi  source_length  = file_size(path);
  if (!builtin || !source) {
    fprintf(stderr, "cannot read '%s'\n", !builtin ? "basic.i" : path);
    profile_stage_end();
    profile_run_end();
    profile_print(stderr);
    return 1;
  }
  I32  length         = (I32)(source_length + builtin_length + 32);
  I32  result         = 0;

  profile_stage_begin("init arena", 0);
  Arena arena         = arena_init(KB(4) * length);
                        str_init(&arena, 2*length);

  Source_Map sources = {0};
  Diags diags = diags_init(&arena, 256);
  File_Id builtin_file = source_add(&sources, &arena, "basic.i", builtin, builtin_length);
  File_Id source_file  = source_add(&sources, &arena, path, source, source_length);
  if (!builtin_file || !source_file) {
    fprintf(stderr, "cannot register source files\n");
    result = 1;
    goto finish;
  }

  I32 builtin_lines = source_line_count(source_get(&sources, builtin_file));
  I32 source_lines  = source_line_count(source_get(&sources, source_file));
  profile_set_input(path, source_lines, "basic.i", builtin_lines);

  profile_stage_begin("lex basic.i", builtin_lines);
  Tokens builtin_tokens = lex_source(&arena, &sources, builtin_file, &diags);

  profile_stage_begin("parse basic.i", builtin_lines);
  Ast_Block ast         = parse_tokens(&arena, builtin_tokens, &diags);

  profile_stage_begin("lex source", source_lines);
  Tokens source_tokens = lex_source(&arena, &sources, source_file, &diags);

  profile_stage_begin("parse source", source_lines);
  Ast_Block source_ast = parse_tokens(&arena, source_tokens, &diags);

  B8 color = isatty(STDERR_FILENO) && !getenv("NO_COLOR");
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stderr, color);
    result = 1;
    goto finish;
  }

  profile_stage_begin("irgen", 0);
  Ast_Node* node = ast_new_node(&arena, Ast_Kind_block);
  node->block    = source_ast;

  fa_init(&arena, ast.list, 1);
  fa_add(ast.list, node);

  Funs funs            = irgen_ast(&arena, ast, length, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stderr, color);
    result = 1;
    goto finish;
  }

  profile_stage_begin("print ir", 0);
  {
    Cstr text            = cstr_from_funs(&arena, funs);
    printf("\n%s", text);
  }

  profile_stage_begin("sem", 0);
                         sem_funs(&arena, funs, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stderr, color);
    result = 1;
    goto finish;
  }

  profile_stage_begin("print sem", 0);
  {
    Cstr text            = cstr_from_sem(&arena, funs);
    printf("\n%s", text);
  }
  diag_print_all(&diags, &sources, stderr, color);

  profile_stage_begin("llvm codegen", 0);
                         llvm_funs(&arena, funs);

finish:
  profile_run_end();
  arena_free(&arena);
  profile_print(stderr);
  return result;
}

// TODO: add import
