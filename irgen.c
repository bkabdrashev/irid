typedef enum {
  Ir_Kind_none = 0,
} Ir_Kind;

typedef struct Ir Ir;
struct Ir {
  Ir_Kind kind;
  Ir* one;
  Ir* two;
};

typedef struct Basic_Block Basic_Block;
typedef struct {
  Basic_Block* bb;
} Jump;

typedef struct {
  Jump eqz;
  Jump nez;
} Branch;

struct Basic_Block {
  Ir* irs;
  I32 length;
  union {
    Jump* jump;
    Branch* branch;
  };
};

typedef struct {
  Basic_Block* bbs;
  I32 length;
} Fun;

typedef struct {
  Fun* funs;
  I32  length;
} Cfg;

Cfg cfg_from_ast(Ast ast, Fun* cfg_buffer) {
  Cfg cfg = { .funs = cfg_buffer };
  return cfg;
}

Cstr cstr_from_cfg(Cfg cfg, C8* buffer) {
  return "todo";
}

void _test_ir(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source);
  I32 max_ast_length   = source_length + 2;
  Ast_Node* ast_buffer = xmalloc(sizeof(Ast_Node) * 2*max_ast_length);
  Ast ast              = ast_from_source(source, ast_buffer);
  Fun* cfg_buffer      = xmalloc(sizeof(Fun));
  Cfg cfg              = cfg_from_ast(ast, cfg_buffer);
                         free(ast.nodes);
  C8* buffer           = xmalloc(6);
  Cstr result          = cstr_from_cfg(cfg, buffer);
  free(cfg.funs);
  test_at_source(result, expected, file_name, line, source);
  free(buffer);
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void irgen_test(void) {
  test("1", "{}");
}

#undef test

