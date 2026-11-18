I32 irid_run_path(Cstr path) {
  Sources sources = sources_init(16, 256);
  profile_run_begin();
  profile_stage_begin("read files");
  Str_View sv_path = str_view_from_cstr(path);
  Source*  source  = sources_add(&sources, sv_path);
  profile_set_input(&sources);

  profile_stage_begin("lex source");
  Tokens    tokens  = lex_source(source, &sources.diags);
  profile_stage_begin("parse source");
  Ast*      ast     = parse_tokens(&sources, tokens);
  // profile_stage_begin("init arena");
  profile_stage_begin("irgen");
  Arena* arena = arena_init(KB(2) * sources.ast_count);
  Fun_Pool funs = irgen_ast_root(sources, arena, ast);

  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    goto finish;
  }
  profile_stage_begin("print ir");
  {
    Str_View str_view = str_view_from_funs(arena, funs);
    file_write("build/irgen.out", str_view);
  }
  profile_stage_begin("sem");
  sem_funs(arena, funs, &sources.diags);
  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    goto finish;
  }
  profile_stage_begin("print sem");
  {
    Str_View str_view = str_view_from_sem(arena, funs);
    file_write("build/sem.out", str_view);
  }

  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
  }

  profile_stage_begin("llvm codegen");
  llvm_funs(arena, funs);

finish:
  profile_run_end();
  arena_free(arena);
  str_free();
  profile_print(stderr);
  sources_free(&sources);
  return 0;
}

I32 irid_sem_path(Cstr path) {
  Sources  sources = sources_init(16, 256);
  Str_View sv_path = str_view_from_cstr(path);
  Source*  source  = sources_add(&sources, sv_path);
  Tokens   tokens  = lex_source(source, &sources.diags);
  Ast*     ast     = parse_tokens(&sources, tokens);
  Arena*   arena   = arena_init(KB(2) * sources.ast_count);
  Fun_Pool funs    = irgen_ast_root(sources, arena, ast);

  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    goto finish;
  }
  {
    Str_View str_view = str_view_from_funs(arena, funs);
    file_write("build/irgen.out", str_view);
  }
  sem_funs(arena, funs, &sources.diags);
  if (diags_has_errors(&sources.diags)) {
    fputs(diag_render_all(arena, &sources.diags, false), stdout);
    // goto finish;
  }
  {
    Str_View str_view = str_view_from_sem(arena, funs);
    file_write("build/sem.out", str_view);
  }

finish:
  arena_free(arena);
  str_free();
  sources_free(&sources);
  return 0;
}
