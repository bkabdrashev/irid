I32 irid_run_path(Cstr path) {
  Sources sources = sources_init(16, 256);
  profile_run_begin();
  profile_stage_begin("read files", 0);
  Str_View data = file_read(path);
  if (!data.base) {
    fprintf(stderr, "cannot read '%s'\n", path);
    profile_stage_end();
    profile_run_end();
    profile_print(stderr);
    return 1;
  }
  I32  result = 0;

  Source*   source  = sources_add_str_view(&sources, data);
  I32 source_lines  = source_line_count(source);
  profile_set_input(path, source_lines);

  profile_stage_begin("lex source", source_lines);
  Tokens    tokens  = lex_source(source, &sources.diags);
  profile_stage_begin("parse source", source_lines);
  Ast*      ast     = parse_tokens(&sources, tokens);
  // profile_stage_begin("init arena", 0);
  profile_stage_begin("irgen", 0);
  Arena* arena = arena_init(KB(2) * sources.ast_count);
  Fun_Pool funs = irgen_ast_root(sources, arena, ast);

  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    result = 1;
    goto finish;
  }
  profile_stage_begin("print ir", 0);
  {
    Cstr text            = cstr_from_funs(arena, funs);
    printf("\n%s", text);
  }
  profile_stage_begin("sem", 0);
  sem_funs(arena, funs, &sources.diags);
  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    result = 1;
    goto finish;
  }
  profile_stage_begin("print sem", 0);
  {
    Cstr cstr = cstr_from_sem(arena, funs);
    printf("\n%s", cstr);
  }

  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
  }

  profile_stage_begin("llvm codegen", 0);
  llvm_funs(arena, funs);

finish:
  profile_run_end();
  arena_free(arena);
  str_free();
  arena_free(source->arena);
  sources_free(&sources);
  profile_print(stderr);
  return result;
}

I32 irid_sem_path(Cstr path) {
  Sources sources = sources_init(16, 256);
  Str_View data = file_read(path);
  if (!data.base) {
    fprintf(stderr, "cannot read '%s'\n", path);
    return 1;
  }
  Source*   source  = sources_add_str_view(&sources, data);
  Tokens    tokens  = lex_source(source, &sources.diags);
  Ast*      ast     = parse_tokens(&sources, tokens);
  Arena* arena = arena_init(KB(2) * sources.ast_count);
  Fun_Pool funs = irgen_ast_root(sources, arena, ast);

  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    goto finish;
  }
  {
    Cstr text = cstr_from_funs(arena, funs);
    printf("\n%s", text);
  }
  sem_funs(arena, funs, &sources.diags);
  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    goto finish;
  }
  {
    Cstr cstr = cstr_from_sem(arena, funs);
    printf("\n%s", cstr);
  }

finish:
  arena_free(arena);
  str_free();
  arena_free(source->arena);
  sources_free(&sources);
  profile_print(stderr);
  return 0;
}
