typedef I32 Irid;
typedef I32 Blockid;
typedef I32 Funid;

typedef enum {
  Ir_Flag_unary  = 1 << 8,
  Ir_Flag_binary = 1 << 9,
} Ir_Flag;

typedef enum {
  Ir_Kind_nop = 0,

  Ir_Kind_int = Ast_Kind_int,

  Ir_Kind_add = Ast_Kind_add,
  Ir_Kind_sub = Ast_Kind_sub,
  Ir_Kind_mul = Ast_Kind_mul,

  Ir_Kind_neg = Ast_Kind_neg,

  Ir_Kind_load      = Ast_Kind_load,
  Ir_Kind_ptr       = Ast_Kind_ptr,
  Ir_Kind_store     = 128,
  Ir_Kind_load_var  = 129,
  Ir_Kind_store_var = 130,

  Ir_Kind_record          = 131,
  Ir_Kind_position_offset = 132,
  Ir_Kind_position_update = 133,
  Ir_Kind_name_offset     = 134,
  Ir_Kind_name_update     = 135,

  Ir_Kind_fun  = 136,
  Ir_Kind_call = 137,

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
  Irid* base;
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
  Irid entryid;
  Irid leaveid;
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
  Blocks  blocks;
  Irs     irs;
  Irid    irid_nil;
  Fun_Stack fun_stack;
  Irs       ir_stack;
  Irids     irid_stack;
  Blocks    block_stack;
} Irgen;

Irgen irgen = {};

Irid irid_push_int(I64 i64) {
  Ir ir = { Ir_Kind_int, .i64 = i64 };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid irid_push_unary(Ir_Kind kind, Irid one) {
  Ir ir = { kind, .unary = { .one = one } };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid irid_push_binary(Ir_Kind kind, Irid one, Irid two) {
  Ir ir = { kind, .binary = { .one = one, .two = two } };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Block* blockid_get(Blockid blockid) {
  return &irgen.blocks.base[blockid];
}

Blockid block_new() {
  Fun* fun = top(irgen.fun_stack);
  Blockid blockid = irgen.block_stack.length;
  Block* block = &new(irgen.block_stack);
  fun->leaveid = blockid;
  memset(block, 0, sizeof(Block));
  block->entryid = irgen.ir_stack.length;
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

  {
    Blockid first = irgen.blocks.length;
    for (Blockid i = fun->entryid; i <= fun->leaveid; i++) {
      Block block = get(irgen.block_stack, i);
      add(irgen.blocks, block);
    }
    fun->entryid = first;
    fun->leaveid = irgen.blocks.length;
  }

  {
    Block* block = &get(irgen.blocks, fun->leaveid-1);
    Irid first = irgen.irs.length;
    for (Irid i = block->entryid; i < irgen.ir_stack.length; i++) {
      Ir ir = get(irgen.ir_stack, i);
      add(irgen.irs, ir);
    }
    block->kind = Block_Kind_jump;
    block->entryid = first;
    block->leaveid = irgen.irs.length;
    Block last = { Block_Kind_none, .pred_count = 0, .entryid = 0, .leaveid = 0, .jump = {0} };
    block->jump.blockid = push(irgen.blocks, last);
    fun->leaveid = irgen.blocks.length;
  }
}

Fun* fun_get(Funid funid) {
  return &irgen.funs.base[funid];
}

Funs cfg_from_ast(Ast ast, Fun* fun_buffer, Block* block_buffer, Ir* ir_buffer) {
  irgen.funs.base   = fun_buffer;
  irgen.blocks.base = block_buffer;
  irgen.irs.base    = ir_buffer;
  irgen.fun_stack.base   = xmalloc(ast.length * sizeof(Fun*));
  irgen.block_stack.base = xmalloc(ast.length * sizeof(Block));
  irgen.ir_stack.base    = xmalloc(ast.length * sizeof(Ir));
  irgen.irid_stack.base  = xmalloc(ast.length * sizeof(Irid));
  irgen.fun_stack.length   = 0;
  irgen.block_stack.length = 0;
  irgen.ir_stack.length    = 0;
  irgen.irid_stack.length  = 0;
  irgen.irid_nil = (Irid){ 0 };
  Ir nil = {0, {0}};
  add(irgen.ir_stack, nil);
  add(irgen.irs, nil);
  for (Astid astid = {0}; astid.index < ast.length; astid.index++) {
    Ast_Node node = ast.nodes[astid.index];
    Irid irid = irgen.irid_nil;
    switch (node.kind) {
    case Ast_Kind_source_enter: {
      fun_enter(astid);
    } break;
    case Ast_Kind_int: {
      irid = irid_push_int(node.i64);
    } break;
    case Ast_Kind_neg: {
      Irid one = pop(irgen.irid_stack);
      irid = irid_push_unary((Ir_Kind)node.kind, one);
    } break;
    case Ast_Kind_mul: 
    case Ast_Kind_add: {
      Irid two = pop(irgen.irid_stack);
      Irid one = pop(irgen.irid_stack);
      irid = irid_push_binary((Ir_Kind)node.kind, one, two);
    } break;
    case Ast_Kind_source_leave: {
      fun_leave();
    } break;
    default: assert(0);
    }
    add(irgen.irid_stack, irid);
  }
  free(irgen.irid_stack.base);
  free(irgen.ir_stack.base);
  free(irgen.block_stack.base);
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

    for (Blockid b = fun.entryid; b < fun.leaveid; b++) {
      Block block = get(irgen.blocks, b);
      string_builder_push_cstr(&sb, "\n  .");
      string_builder_push_i64(&sb, b);
      string_builder_push_cstr(&sb, ":");
      for (Irid irid = block.entryid; irid < block.leaveid; irid++) {
        Ir ir = irgen.irs.base[irid];
        string_builder_push_cstr(&sb, "\n    r");
        string_builder_push_i64(&sb, irid);
        string_builder_push_cstr(&sb, " = ");
        switch (ir.kind) {
        case Ir_Kind_int:
          string_builder_push_cstr(&sb, "int ");
          string_builder_push_i64(&sb, ir.i64);
        break;
        case Ir_Kind_add: string_builder_push_cstr(&sb, "add "); break;
        case Ir_Kind_mul: string_builder_push_cstr(&sb, "mul "); break;
        case Ir_Kind_sub: string_builder_push_cstr(&sb, "sub "); break;
        case Ir_Kind_neg: string_builder_push_cstr(&sb, "neg "); break;
        default: {
          assert(0);
        } break;
        }
        if (ir.kind & Ir_Flag_binary) {
          string_builder_push_cstr(&sb, "r");
          string_builder_push_i64(&sb, ir.binary.one);
          string_builder_push_cstr(&sb, " r");
          string_builder_push_i64(&sb, ir.binary.two);
        }
        else if (ir.kind & Ir_Flag_unary) {
          string_builder_push_cstr(&sb, "r");
          string_builder_push_i64(&sb, ir.unary.one);
        }
      }
      switch (block.kind) {
      case Block_Kind_none:
      break;
      case Block_Kind_jump:
        string_builder_push_cstr(&sb, "\n    jump .");
        string_builder_push_i64(&sb, block.jump.blockid);
      break;
      case Block_Kind_branch:
        string_builder_push_cstr(&sb, "\n    if r");
        string_builder_push_i64(&sb, block.branch.cond);
        string_builder_push_cstr(&sb, " then .");
        string_builder_push_i64(&sb, block.branch.nez.blockid);
        string_builder_push_cstr(&sb, " else .");
        string_builder_push_i64(&sb, block.branch.eqz.blockid);
      break;
      }
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
  test("1 + -2*3", "");
}

#undef test

