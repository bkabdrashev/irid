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

typedef struct Strs Strs;
struct Strs {
  I32  length;
  Str* base[];
};

void irid_run_paths(Strs paths) {
  Cstr builtin        = file_read("builtin.i");
  I32  builtin_length = strlen(builtin);
  I32  length = 0;
  for (I32 i = 0; i < paths.length; i++) {
    Str* path = paths.base[i];
    I32 size = file_size(path->base);
    length += size;
  }
  Arena arena         = arena_init(KB(4) * length);
                        str_init(&arena, 2*length);

  Tokens builtin_tokens = lex_source(&arena, builtin);
  Ast_Block ast         = parse_tokens(&arena, builtin_tokens);


  for (I32 i = 0; i < paths.length-1; i++) {
    Ast_Node* declare_node = ast_new_node(&arena, Ast_Kind_declare);
    Ast_Decls* fa_temp(&arena, temp_map);
    Str* path            = paths.base[i];
    Cstr source          = file_read(path->base);
    Tokens source_tokens = lex_source(&arena, source);
    Ast_Block source_ast = parse_tokens(&arena, source_tokens);
    I32 dot;
    for (dot = 0; dot < path->length; dot++) {
      if (path->base[dot] == '.') {
        break;
      }
      if (dot + 1 >= path->length) {
        assert(0);
      }
    }
    Str* name = str_from_range(path->base, path->base + dot);
    Ast_Decl decl = { name, source_ast.scope };
    fa_extend(parser->decls_arena, decls, decl);
    parse_decls_push(parser, temp_map, path, source_ast);
  }

  Ast_Node* node = ast_new_node(&arena, Ast_Kind_block);
  node->block    = source_ast;

  fa_init(&arena, ast.list, 1);
  fa_add(ast.list, node);

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
