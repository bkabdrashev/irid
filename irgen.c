typedef I32 Irid;
typedef I32 Blockid;
typedef I32 Funid;

typedef enum {
  Ir_Kind_nop = 0,

  Ir_Kind_int = 5,

  Ir_Kind_add = 32,
  Ir_Kind_sub = 33,
  Ir_Kind_mul = 34,

  Ir_Kind_neg = 64,

  Ir_Kind_load      = 65,
  Ir_Kind_load_var  = 66,
  Ir_Kind_store     = 67,
  Ir_Kind_store_var = 68,
  Ir_Kind_ptr       = 69,

  Ir_Kind_record          = 80,
  Ir_Kind_position_offset = 81,
  Ir_Kind_position_update = 82,
  Ir_Kind_name_offset     = 83,
  Ir_Kind_name_update     = 84,

  Ir_Kind_fun  = 85,
  Ir_Kind_call = 86,

} Ir_Kind;

typedef struct Ir Ir;
struct Ir {
  Ir_Kind kind;
  union {
    I64 i64;
    struct {
      Irid one;
      Irid two;
    } binary;
    struct {
      Irid one;
    } unary;
  };
};

typedef struct {
  Irid  first;
  I32   length;
} Irids;

typedef struct {
  Blockid blockid;
} Jump;

typedef struct {
  Irid cond;
  Jump eqz;
  Jump nez;
} Branch;

typedef enum {
  Block_Kind_none,
  Block_Kind_jump,
  Block_Kind_branch,
} Block_Kind;

typedef struct Block Block;
struct Block {
  Block_Kind kind;
  I32 pred_count;
  Irids irids;
  union {
    Jump   jump;
    Branch branch;
  };
};

typedef struct {
  Blockid entryid;
  Blockid leaveid;
  Irid    returnid;
} Fun;

typedef struct {
  Fun* base;
  I32  length;
} Funs;

typedef struct {
  Block* base;
  I32    length;
} Blocks;

typedef struct {
  Ir* base;
  I32 length;
} Irs;

typedef struct {
  Funid   funid;
  Blockid blockid;
} Frame;

typedef struct {
  Fun** base;
  I32   length;
} Fun_Stack;

typedef struct {
  Funs    funs;
  Fun_Stack fun_stack;
  Blocks  blocks;
  Irs     irs;
  Irs     ir_stack;
  Irid    irid_nil;
} Irgen;

Irgen irgen = {};

Irid irid_push_int(I64 i64) {
  Ir ir = { Ir_Kind_int, .i64 = i64 };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Block* blockid_get(Blockid blockid) {
  return &irgen.blocks.base[blockid];
}

Blockid block_new() {
  Fun* fun = top(irgen.fun_stack);
  Blockid blockid = irgen.blocks.length;
  Block* block = &new(irgen.blocks);
  fun->leaveid = blockid;
  memset(block, 0, sizeof(Block));
  block->irids.first = irgen.ir_stack.length;
  return blockid;
}

Funid fun_enter(Astid astid) {
  Funid funid = irgen.funs.length;
  Fun*  fun = &new(irgen.funs);
  add(irgen.fun_stack, fun);
  fun->entryid  = block_new();
  fun->returnid = irgen.irid_nil;
  return funid;
}

void fun_leave() {
  Fun* fun = pop(irgen.fun_stack);
  Block* block = &get(irgen.blocks, fun->leaveid);
  Irid first = irgen.irs.length;
  for (Irid i = block->irids.first; i < irgen.ir_stack.length; i++) {
    Ir ir = get(irgen.ir_stack, i);
    add(irgen.irs, ir);
  }
  block->kind = Block_Kind_jump;
  block->irids.first  = first;
  block->irids.length = irgen.irs.length - first;
  Block last = { Block_Kind_none, .pred_count = 0, .irids = {0, 0}, .jump = {0} };
  block->jump.blockid = push(irgen.blocks, last);
}

Fun* fun_get(Funid funid) {
  return &irgen.funs.base[funid];
}

Funs cfg_from_ast(Ast ast, Fun* fun_buffer, Block* block_buffer, Ir* ir_buffer) {
  irgen.funs.base   = fun_buffer;
  irgen.blocks.base = block_buffer;
  irgen.irs.base    = ir_buffer;
  irgen.ir_stack.base  = xmalloc(ast.length * sizeof(Ir));
  irgen.fun_stack.base = xmalloc(ast.length * sizeof(Fun*));
  irgen.irid_nil    = (Irid){ 0 };
  Ir nil = {0, {0}};
  add(irgen.irs, nil);
  for (Astid astid = {0}; astid.index < ast.length; astid.index++) {
    Ast_Node node = ast.nodes[astid.index];
    switch (node.kind) {
    case Ast_Kind_source_enter: {
      fun_enter(astid);
    } break;
    case Ast_Kind_int: {
      irid_push_int(node.i64);
    } break;
    case Ast_Kind_source_leave: {
      fun_leave();
    } break;
    default: assert(0);
    }
  }
  free(irgen.ir_stack.base);
  free(irgen.fun_stack.base);
  return irgen.funs;
}

Cstr cstr_from_cfg(Funs funs, C8* buffer) {
  String_Builder sb = string_builder_begin(buffer);
  for (I32 f = 0; f < funs.length; f++) {
    Fun fun = irgen.funs.base[f];
    string_builder_push_cstr(&sb, "@");
    string_builder_push_i64(&sb,   f);
    string_builder_push_cstr(&sb, " {");

    Block block = irgen.blocks.base[fun.entryid];
    string_builder_push_cstr(&sb, "\n  .");
    string_builder_push_i64(&sb, fun.entryid);
    string_builder_push_cstr(&sb, ":");
    for (Irid irid = block.irids.first; irid < block.irids.first + block.irids.length; irid++) {
      Ir ir = irgen.irs.base[irid];
      string_builder_push_cstr(&sb, "\n    r");
      string_builder_push_i64(&sb, irid);
      string_builder_push_cstr(&sb, " = ");
      switch (ir.kind) {
      case Ir_Kind_int:
        string_builder_push_cstr(&sb, "int ");
        string_builder_push_i64(&sb, ir.i64);
      break;
      default: assert(0);
      }
    }
    switch (block.kind) {
    case Block_Kind_none:
      string_builder_push_cstr(&sb, "\n  end");
    break;
    case Block_Kind_jump:
      string_builder_push_cstr(&sb, "\n  jump .");
      string_builder_push_i64(&sb, block.jump.blockid);
    break;
    case Block_Kind_branch:
      string_builder_push_cstr(&sb, "\n  if r");
      string_builder_push_i64(&sb, block.branch.cond);
      string_builder_push_cstr(&sb, " then .");
      string_builder_push_i64(&sb, block.branch.nez.blockid);
      string_builder_push_cstr(&sb, " else .");
      string_builder_push_i64(&sb, block.branch.eqz.blockid);
    break;
    }

    string_builder_push_cstr(&sb, "\n  ret r");
    string_builder_push_i64(&sb, fun.returnid);
    string_builder_push_cstr(&sb, "\n}");
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

void _test_ir(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source);
  I32 max_ast_length   = source_length + 2;
  Ast_Node* ast_buffer = xmalloc(sizeof(Ast_Node) * 2*max_ast_length);
  Ast ast              = ast_from_source(source, ast_buffer);
  Fun* cfg_buffer      = xmalloc(sizeof(Fun)*ast.length);
  Block* block_buffer  = xmalloc(sizeof(Block)*ast.length);
  Ir* ir_buffer        = xmalloc(sizeof(Ir)*ast.length);
  Funs funs            = cfg_from_ast(ast, cfg_buffer, block_buffer, ir_buffer);
                         free(ast.nodes);
  C8* buffer           = xmalloc(MB(64));
  Cstr result          = cstr_from_cfg(funs, buffer);
  free(irgen.funs.base);
  free(irgen.blocks.base);
  free(irgen.irs.base);
  test_at_source(result, expected, file_name, line, source);
  free(buffer);
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void irgen_test(void) {
  test("1", "");
}

#undef test

