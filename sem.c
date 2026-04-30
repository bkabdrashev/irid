void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source);
  I32 max_ast_length   = source_length + 2;
  Ast_Node* ast_buffer = xmalloc(sizeof(Ast_Node) * 2*max_ast_length);
  Ast ast              = ast_from_source(source, ast_buffer);
  Fun* cfg_buffer      = xmalloc(sizeof(Fun)*ast.length);
  Block* block_buffer  = xmalloc(sizeof(Block)*ast.length);
  Ir* ir_buffer        = xmalloc(sizeof(Ir)*ast.length);
  Record* record_buffer= xmalloc(sizeof(Record)*ast.length);
  Funs funs            = irgen_ast(ast, cfg_buffer, block_buffer, ir_buffer, record_buffer);
                         free(ast.base);
  C8* buffer           = xmalloc(MB(64));
  Cstr result          = cstr_from_cfg(funs, buffer);
  free(cfg_buffer);
  free(block_buffer);
  free(ir_buffer);
  free(record_buffer);
  test_at_source(result, expected, file_name, line, source);
  free(buffer);
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void sem_test(void) {
  // test("a = 1; if a do a + a;", "");
}

#undef test

