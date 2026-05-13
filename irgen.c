typedef I32 Irid;
typedef I32 Blockid;
typedef I32 Funid;
typedef I32 Symbolid;
typedef I32 Varid;
typedef I32 Recordid;

typedef enum Ir_Flag {
  Ir_Flag_unary  = Ast_Flag_unary,
  Ir_Flag_binary = Ast_Flag_binary,
} Ir_Flag;

typedef enum Ir_Kind {
  Ir_Kind_nop = 0,

  Ir_Kind_int = Ast_Kind_int,

  Ir_Kind_add = Ast_Kind_add,
  Ir_Kind_sub = Ast_Kind_sub,
  Ir_Kind_mul = Ast_Kind_mul,
  Ir_Kind_eq  = Ast_Kind_eq,
  Ir_Kind_ne  = Ast_Kind_ne,
  Ir_Kind_lt  = Ast_Kind_lt,
  Ir_Kind_le  = Ast_Kind_le,
  Ir_Kind_gt  = Ast_Kind_gt,
  Ir_Kind_ge  = Ast_Kind_ge,

  Ir_Kind_join = Ast_Kind_join,

  Ir_Kind_neg = Ast_Kind_neg,

  Ir_Kind_load      = Ast_Kind_load,
  Ir_Kind_var       = Ast_Kind_name,
  Ir_Kind_ptr       = Ast_Kind_ptr,
  Ir_Kind_store     = 128 | Ir_Flag_binary,

  Ir_Kind_record          = 131,
  Ir_Kind_position_offset = 132,
  Ir_Kind_position_update = 133,
  Ir_Kind_name_offset     = 134,
  Ir_Kind_name_update     = 135,

  Ir_Kind_fun  = 136,
  Ir_Kind_call = 137,

} Ir_Kind;

typedef struct Irid_Pair Irid_Pair;
struct Irid_Pair { Irid one; Irid two; };

typedef struct Name_Offset Name_Offset;
struct Name_Offset { Irid of; Istr at; };

typedef struct Position_Offset Position_Offset;
struct Position_Offset { Irid of; I32 at; };

typedef struct Ir Ir;
struct Ir {
  Ir_Kind kind;
  union {
    I64 i64;
    Varid varid;
    Recordid recordid;
    Irid_Pair binary;
    Irid unary;
    Position_Offset position;
    Name_Offset     name;
  };
};

typedef struct Record Record;
struct Record {
  I32      length;
  Irid*    assigned;
  Irid*    declared;
  Istr*    names;
  Hash_Map positions;
};

typedef struct Field Field;
struct Field {
  Istr name;
  I32  position;
  Irid assigned;
  Irid declared;
};

typedef struct Irids Irids;
struct Irids {
  Irid* base;
  I32   length;
};


typedef struct Jump Jump;
struct Jump {
  Blockid blockid;
};

typedef struct Branch Branch;
struct Branch {
  Irid cond;
  Jump eqz;
  Jump nez;
};

typedef enum Block_Kind {
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
  Hash_Map out_var_typeids;
  Hash_Map in_var_typeids;
  union {
    Jump   jump;
    Branch branch;
  };
};

typedef struct Fun Fun;
struct Fun {
  Blockid entryid;
  Blockid leaveid;
  Irid    returnid;
  I32     var_count;
};

typedef struct Funs Funs;
struct Funs {
  Fun* base;
  I32  length;
};

typedef struct Blocks Blocks;
struct Blocks {
  Block* base;
  I32    length;
};

typedef struct Blockids Blockids;
struct Blockids {
  Blockid* base;
  I32      length;
};

typedef struct Records Records;
struct Records {
  Record* base;
  I32     length;
};

typedef struct Recordid_Stack Recordid_Stack;
struct Recordid_Stack {
  Recordid* base;
  I32       length;
};

typedef struct Irs Irs;
struct Irs {
  Ir* base;
  I32 length;
};

typedef struct Fun_Stack Fun_Stack;
struct Fun_Stack {
  Fun** base;
  I32   length;
};

typedef struct Irgen Irgen;
struct Irgen {
  Ast     ast;

  Arena*  perm_arena;
  Arena*  temp_arena;

  Funs    funs;
  Blocks  blocks;
  Records records;
  Irs     irs;
  Irid    irid_nil;
  Varid   varid_nil;
  Varid   varids;

  Fun_Stack      fun_stack;
  Scope_Stack    scope_stack;
  Irs            ir_stack;
  Irids          irid_stack;
  Blocks         block_stack;
  Recordid_Stack recordid_stack;
  Blockids       headerids;
};

Irgen irgen = {};

Irid ir_push_int(I64 i64) {
  Ir ir = { Ir_Kind_int, .i64 = i64 };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid ir_push_var(Varid varid) {
  Ir ir = { Ir_Kind_var, .varid = varid };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid ir_push_unary(Ir_Kind kind, Irid one) {
  Ir ir = { kind, .unary = one };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid ir_push_binary(Ir_Kind kind, Irid one, Irid two) {
  Ir ir = { kind, .binary = { .one = one, .two = two } };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid ir_push_record(Recordid recordid) {
  Ir ir = { Ir_Kind_record, .recordid = recordid };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Irid ir_push_position_offset(Irid record, I32 position) {
  Ir ir = { Ir_Kind_position_offset, .position = { .of = record, .at = position } };
  Irid irid = push(irgen.ir_stack, ir);
  return irid;
}

Ir_Kind irid_kind(Irid irid) {
  Ir_Kind kind = get(irgen.irs, irid).kind;
  return kind;
}

Name_Offset irid_name_offset(Irid irid) {
  return get(irgen.irs, irid).name;
}

B8 irid_kind_equal(Irid irid, Ir_Kind kind) {
  return get(irgen.irs, irid).kind == kind;
}

Irid_Pair irid_binary(Irid irid) {
  return get(irgen.irs, irid).binary;
}

Irid irid_unary(Irid irid) {
  return get(irgen.irs, irid).unary;
}

I64 irid_int(Irid irid) {
  return get(irgen.irs, irid).i64;
}

Istr irid_istr(Irid irid) {
  return get(irgen.irs, irid).varid;
}

Istr irid_recordid(Irid irid) {
  return get(irgen.irs, irid).recordid;
}

I32 recordid_length(Recordid recordid) {
  Record record = get(irgen.records, recordid);
  return record.length;
}

Recordid recordid_new(I32 length) {
  Record record = {};
  record.length = length;
  record.names    = arena_push_zero(irgen.perm_arena, length*sizeof(Istr));
  record.assigned = arena_push_zero(irgen.perm_arena, length*sizeof(Irid));
  record.declared = arena_push_zero(irgen.perm_arena, length*sizeof(Irid));
  record.positions = hash_map_init(irgen.perm_arena, length);
  Recordid recordid = push(irgen.records, record);
  return recordid;
}

void recordid_push_assign_position(Recordid recordid, I32 position, Irid value) {
  Record* record = &get(irgen.records, recordid);
  record->assigned[position] = value;
}
void recordid_push_assign_name(Recordid recordid, Istr name, I32 position) {
  Record* record = &get(irgen.records, recordid);
  hash_map_put(&record->positions, name, position);
  record->names[position] = name;
}
void recordid_push_declare_position(Recordid recordid, I32 position, Irid value) {
  Record* record = &get(irgen.records, recordid);
  record->declared[position] = value;
}
void recordid_push_declare_name(Recordid recordid, Istr name, I32 position) {
  Record* record = &get(irgen.records, recordid);
  hash_map_put(&record->positions, name, position);
  record->names[position] = name;
}
Field field_nil = {istr_nil, 0, 0, 0};
Field recordid_get_by_name(Recordid recordid, Istr name) {
  Record record = get(irgen.records, recordid);
  Field field = {};
  I32 position = hash_map_get(&record.positions, name);
  field.name     = record.names[position];
  field.assigned = record.assigned[position];
  field.declared = record.declared[position];
  field.position = position;
  return field;
}

Field recordid_get_by_position(Recordid recordid, I32 position) {
  Record record = get(irgen.records, recordid);
  Field field = {};
  field.name     = record.names[position];
  field.assigned = record.assigned[position];
  field.declared = record.declared[position];
  field.position = position;
  return field;
}

Record recordid_get(Recordid recordid) {
  Record record = get(irgen.records, recordid);
  return record;
}

Block* blockid_get(Blockid blockid) {
  return &irgen.blocks.base[blockid];
}

Branch blockid_branch(Blockid blockid) {
  return irgen.blocks.base[blockid].branch;
}

Jump blockid_jump(Blockid blockid) {
  return irgen.blocks.base[blockid].jump;
}

Blockid ir_put_branch(Irid cond) {
  Fun* fun = top(irgen.fun_stack);
  Block* block = &get(irgen.block_stack, fun->leaveid);
  block->branch.cond = cond;
  block->kind = Block_Kind_branch;
  return fun->leaveid;
}

Blockid irgen_put_jump() {
  Fun* fun = top(irgen.fun_stack);
  Block* block = &get(irgen.block_stack, fun->leaveid);
  block->kind = Block_Kind_jump;
  return fun->leaveid;
}

void irgen_jump_link_to(Blockid blockid, Blockid jumpto) {
  Block* block = &get(irgen.block_stack, blockid);
  block->jump.blockid = jumpto;
}

void irgen_nez_link_to(Blockid blockid, Blockid jumpto) {
  Block* block = &get(irgen.block_stack, blockid);
  block->branch.nez.blockid = jumpto;
}

void irgen_eqz_link_to(Blockid blockid, Blockid jumpto) {
  Block* block = &get(irgen.block_stack, blockid);
  block->branch.eqz.blockid = jumpto;
}

Block* ir_block_leave() {
  Fun* fun = top(irgen.fun_stack);
  Block* block = &get(irgen.block_stack, fun->leaveid);
  block->leaveid = irgen.ir_stack.length - block->entryid;
  return block;
}

Blockid ir_block_new() {
  ir_block_leave();
  Fun* fun = top(irgen.fun_stack);
  Blockid blockid = irgen.block_stack.length;
  Block* block = &new(irgen.block_stack);
  fun->leaveid = blockid;
  memset(block, 0, sizeof(Block));
  block->entryid = irgen.ir_stack.length;
  return blockid;
}

void scope_enter() {
  assert(0);
}

void scope_leave() {
  del(irgen.scope_stack);
}

Fun* funid_get(Funid funid) {
  return &get(irgen.funs, funid);
}

Funid ir_fun_enter() {
  scope_enter();
  Funid funid = irgen.funs.length;
  Fun*  fun = &new(irgen.funs);
  add(irgen.fun_stack, fun);
  {
    Blockid blockid = irgen.block_stack.length;
    Block* block = &new(irgen.block_stack);
    memset(block, 0, sizeof(Block));
    block->entryid = irgen.ir_stack.length;
    fun->entryid = blockid;
    fun->leaveid = blockid;
  }
  fun->returnid = irgen.ir_stack.length;
  fun->var_count = 0;
  return funid;
}

void ir_fun_leave() {
  Fun* fun = top(irgen.fun_stack);

  {
    Block* block = ir_block_leave();
    block->kind = Block_Kind_jump;
    Block last = { Block_Kind_none, .pred_count = 0, .entryid = 0, .leaveid = 0, .jump = {0} };
    fun->leaveid = irgen.block_stack.length;
    block->jump.blockid = push(irgen.block_stack, last);
  }

  {
    Blockid first = irgen.blocks.length;
    Blockid irid = irgen.irs.length;
    for (Irid irid = fun->returnid; irid < irgen.ir_stack.length; irid++) {
      Ir ir = get(irgen.ir_stack, irid);
      add(irgen.irs, ir);
    }
    for (Blockid i = fun->entryid; i <= fun->leaveid; i++) {
      Block block = get(irgen.block_stack, i);
      block.entryid = irid;
      irid += block.leaveid;
      block.leaveid = irid;
      add(irgen.blocks, block);
    }
    fun->entryid = first;
    fun->leaveid = irgen.blocks.length;
  }
  scope_leave();
}

void string_builder_push_ir(String_Builder* sb, Irid irid, Ir ir) {
  string_builder_push_cstr(sb, "\n    r");
  string_builder_push_i64(sb, irid);
  string_builder_push_cstr(sb, " = ");
  switch (ir.kind) {
  case Ir_Kind_nop:
    string_builder_push_cstr(sb, "nop");
  break;
  case Ir_Kind_int:
    string_builder_push_cstr(sb, "int ");
    string_builder_push_i64(sb, ir.i64);
  break;
  case Ir_Kind_var:
    string_builder_push_cstr(sb, "var ");
    string_builder_push_i64(sb, ir.varid);
  break;
  case Ir_Kind_position_offset:
    string_builder_push_cstr(sb, "position offset ");
    string_builder_push_cstr(sb, "r");
    string_builder_push_i64(sb, ir.position.of);
    string_builder_push_cstr(sb, ".");
    string_builder_push_i64(sb, ir.position.at);
  break;
  case Ir_Kind_name_offset:
    string_builder_push_cstr(sb, "name offset ");
    string_builder_push_cstr(sb, "r");
    string_builder_push_i64(sb, ir.name.of);
    string_builder_push_cstr(sb, ".");
    string_builder_push_istr(sb, ir.name.at);
  break;
  case Ir_Kind_record:
    string_builder_push_cstr(sb, "record");
    for (I32 i = 0; i < recordid_length(ir.recordid); i++) {
      Field field = recordid_get_by_position(ir.recordid, i);
      if (field.name) {
        string_builder_push_cstr(sb, " ");
        string_builder_push_istr(sb, field.name);
        if (field.declared != irgen.irid_nil) {
          string_builder_push_cstr(sb, ":");
          string_builder_push_cstr(sb, "r");
          string_builder_push_i64(sb, field.declared);
        }
        if (field.assigned != irgen.irid_nil) {
          string_builder_push_cstr(sb, "=");
          string_builder_push_cstr(sb, "r");
          string_builder_push_i64(sb, field.assigned);
        }
      }
      else {
        string_builder_push_cstr(sb, " r");
        string_builder_push_i64(sb, field.assigned);
      }
    }
  break;
  case Ir_Kind_add:  string_builder_push_cstr(sb, "add "); break;
  case Ir_Kind_mul:  string_builder_push_cstr(sb, "mul "); break;
  case Ir_Kind_sub:  string_builder_push_cstr(sb, "sub "); break;
  case Ir_Kind_join: string_builder_push_cstr(sb, "join "); break;
  case Ir_Kind_neg: string_builder_push_cstr(sb, "neg "); break;
  case Ir_Kind_load: string_builder_push_cstr(sb, "load "); break;
  case Ir_Kind_ptr:  string_builder_push_cstr(sb, "ptr "); break;
  case Ir_Kind_store: string_builder_push_cstr(sb, "store "); break;
  case Ir_Kind_eq: string_builder_push_cstr(sb, "eq "); break;
  case Ir_Kind_ne: string_builder_push_cstr(sb, "ne "); break;
  case Ir_Kind_lt: string_builder_push_cstr(sb, "lt "); break;
  case Ir_Kind_le: string_builder_push_cstr(sb, "le "); break;
  case Ir_Kind_gt: string_builder_push_cstr(sb, "gt "); break;
  case Ir_Kind_ge: string_builder_push_cstr(sb, "ge "); break;
  default: {
    assert(0);
  } break;
  }
  if (ir.kind & Ir_Flag_binary) {
    string_builder_push_cstr(sb, "r");
    string_builder_push_i64(sb, ir.binary.one);
    string_builder_push_cstr(sb, " r");
    string_builder_push_i64(sb, ir.binary.two);
  }
  else if (ir.kind & Ir_Flag_unary) {
    string_builder_push_cstr(sb, "r");
    string_builder_push_i64(sb, ir.unary);
  }
}

void irgen_print() {
  C8 buf[1024];
  String_Builder sb = string_builder_begin(buf);
  printf("ir_stack:");
  for (I32 i = 0; i < irgen.ir_stack.length; i++) {
    Ir ir = get(irgen.ir_stack, i);
    string_builder_push_ir(&sb, i, ir);
  }
  Cstr cstr = string_builder_end(&sb);
  printf("%s\n", cstr);
  printf("irid_stack:");
  for (I32 i = 0; i < irgen.irid_stack.length; i++) {
    Irid irid = get(irgen.irid_stack, i);
    printf(" r%d", irid);
  }
  printf("\n");
}

Varid ir_get_sym(Istr istr) {
  for (I32 i = 0; i < irgen.scope_stack.length; i++) {
    Hash_Map* scope = &get(irgen.scope_stack, i);
    Varid varid = hash_map_get(scope, istr);
    if (varid) {
      return varid;
    }
  }
  Varid varid = irgen.varids++;
  Hash_Map* scope = &top(irgen.scope_stack);
  hash_map_put(scope, istr, varid);
  return varid;
}

Funs ir_ast(Arena* arena, Ast ast) {
  Arena temp = arena_init(arena->capacity);
  irgen.ast = ast;
  irgen.perm_arena = arena;
  irgen.temp_arena = &temp;
  irgen.funs.base      = arena_push(irgen.perm_arena, sizeof(Fun)*ast.length);
  irgen.funs.length    = 0;
  irgen.blocks.base    = arena_push(irgen.perm_arena, sizeof(Block)*ast.length);
  irgen.blocks.length  = 0;
  irgen.irs.base       = arena_push(irgen.perm_arena, sizeof(Ir)*ast.length);
  irgen.irs.length     = 0;
  irgen.records.base   = arena_push(irgen.perm_arena, sizeof(Record)*ast.length);
  irgen.records.length = 0;
  irgen.fun_stack.base        = arena_push(irgen.temp_arena, ast.length * sizeof(Fun*));
  irgen.fun_stack.length      = 0;
  irgen.block_stack.base      = arena_push(irgen.temp_arena, ast.length * sizeof(Block));
  irgen.block_stack.length    = 0;
  irgen.scope_stack.base      = arena_push(irgen.temp_arena, ast.length * sizeof(Hash_Map));
  irgen.scope_stack.length    = 0;
  irgen.ir_stack.base         = arena_push(irgen.temp_arena, ast.length * sizeof(Ir));
  irgen.ir_stack.length       = 0;
  irgen.irid_stack.base       = arena_push(irgen.temp_arena, ast.length * sizeof(Irid));
  irgen.irid_stack.length     = 0;
  irgen.recordid_stack.base   = arena_push(irgen.temp_arena, ast.length * sizeof(Recordid));
  irgen.recordid_stack.length = 0;
  irgen.headerids.base        = arena_push(irgen.temp_arena, ast.length * sizeof(Blockid));
  irgen.headerids.length      = 0;
  irgen.irid_nil = 0;
  Ir ir_nil = {0, {0}};
  add(irgen.ir_stack, ir_nil);
  add(irgen.irs, ir_nil);
  irgen.varid_nil = 0;
  irgen.varids = 1;

  for (Astid astid = 0; astid < irgen.ast.length; astid++) {
    Ast_Node node = get(irgen.ast, astid);
    switch (node.kind) {
    case Ast_Kind_source_enter: {
      ir_fun_enter(astid);
    } break;
    case Ast_Kind_source_split: {} break;
    case Ast_Kind_source_leave: {
      ir_fun_leave();
    } break;
    case Ast_Kind_block_value_enter: {
      scope_enter();
    } break;
    case Ast_Kind_block_split: {} break;
    case Ast_Kind_block_value_leave: {
      scope_leave();
    } break;
    case Ast_Kind_tuple_enter: {
      Recordid tuple = recordid_new(node.list.length);
      add(irgen.recordid_stack, tuple);
    } break;
    case Ast_Kind_tuple_split: {
      Recordid tuple = top(irgen.recordid_stack);
      Irid val = pop(irgen.irid_stack);
      recordid_push_assign_position(tuple, node.split.position, val);
    } break;
    case Ast_Kind_tuple_leave: {
      Recordid tuple = pop(irgen.recordid_stack);
      Irid irid = ir_push_record(tuple);
      add(irgen.irid_stack, irid);
    } break;
    case Ast_Kind_record_enter: {
      Recordid record = recordid_new(node.list.length);
      add(irgen.recordid_stack, record);
    } break;
    case Ast_Kind_record_split: {
      Recordid record = top(irgen.recordid_stack);
      Irid val = pop(irgen.irid_stack);
      recordid_push_assign_position(record, node.split.position, val);
    } break;
    case Ast_Kind_record_assign: {
      Recordid record = top(irgen.recordid_stack);
      Irid val = pop(irgen.irid_stack);
      recordid_push_assign_position(record, node.field.position, val);
      recordid_push_assign_name(record, node.field.name, node.field.position);
      astid++; // skips split
    } break;
    case Ast_Kind_record_declare: {
      Recordid record = top(irgen.recordid_stack);
      Irid val = pop(irgen.irid_stack);
      recordid_push_declare_position(record, node.field.position, val);
      recordid_push_declare_name(record, node.field.name, node.field.position);
      astid++; // skips split
    } break;
    case Ast_Kind_record_leave: {
      Recordid record = pop(irgen.recordid_stack);
      Irid irid = ir_push_record(record);
      add(irgen.irid_stack, irid);
    } break;
    case Ast_Kind_int: {
      Irid irid = ir_push_int(node.i64);
      add(irgen.irid_stack, irid);
    } break;
    case Ast_Kind_name: {
      Fun* fun = top(irgen.fun_stack);
      fun->var_count++;
      Varid varid = ir_get_sym(node.istr);
      Irid var = ir_push_var(varid);
      Irid irid = ir_push_unary(Ir_Kind_load, var);
      add(irgen.irid_stack, irid);
    } break;
    case Ast_Kind_ptr_leave:
    case Ast_Kind_load_leave:
    case Ast_Kind_neg_leave: {
      Irid one = pop(irgen.irid_stack);
      Irid irid = ir_push_unary((Ir_Kind)node.kind, one);
      add(irgen.irid_stack, irid);
    } break;
    case Ast_Kind_dot_leave: {
      Irid rhs = pop(irgen.irid_stack);
      Irid lhs = pop(irgen.irid_stack);
      Ir* rhs_ir = &get(irgen.ir_stack, rhs);
      if (rhs_ir->kind == Ir_Kind_int) {
        rhs_ir->kind = Ir_Kind_position_offset;
        rhs_ir->position.at = rhs_ir->i64;
        rhs_ir->position.of = lhs;
        rhs_ir->kind = Ir_Kind_position_offset;
        Irid irid = ir_push_unary(Ir_Kind_load, rhs);
        add(irgen.irid_stack, irid);
      }
      else if (rhs_ir->kind == Ir_Kind_load) {
        Ir* ir_ptr = &get(irgen.ir_stack, rhs_ir->unary);
        if (ir_ptr->kind == Ir_Kind_var) {
          ir_ptr->kind = Ir_Kind_name_offset;
          ir_ptr->name.at = ir_ptr->varid;
          ir_ptr->name.of = lhs;
          add(irgen.irid_stack, rhs);
        }
        else {
          assert(0);
        }
      }
      else {
        assert(0);
      }
    } break;
    case Ast_Kind_join_leave:
    case Ast_Kind_mul_leave:
    case Ast_Kind_eq_leave:  case Ast_Kind_ne_leave:
    case Ast_Kind_le_leave:  case Ast_Kind_lt_leave:
    case Ast_Kind_ge_leave:  case Ast_Kind_gt_leave:
    case Ast_Kind_add_leave: {
      Irid two = pop(irgen.irid_stack);
      Irid one = pop(irgen.irid_stack);
      Irid irid = ir_push_binary((Ir_Kind)node.kind, one, two);
      add(irgen.irid_stack, irid);
    } break;
    case Ast_Kind_assign_tuple_enter: {
      Irid rhs = pop(irgen.irid_stack);
      Irid start = irgen.ir_stack.length;
      for (I32 i = 0; i < node.list.length; i++) {
        Irid pos = ir_push_position_offset(rhs, i);
        ir_push_unary(Ir_Kind_load, pos);
      }
      for (I32 i = node.list.length-1; i >= 0; i--) {
        add(irgen.irid_stack, 2*i + start + 1);
      }
    } break;
    case Ast_Kind_assign_tuple_leave:
      astid++; // NOTE: skips = or ,
    break;
    case Ast_Kind_assign_tuple_split:
    case Ast_Kind_assign_leave: {
      Irid lhs = pop(irgen.irid_stack);
      Irid rhs = pop(irgen.irid_stack);
      Ir* ir = &get(irgen.ir_stack, lhs);
      if (ir->kind == Ir_Kind_load) {
        ir->kind = Ir_Kind_store;
        ir->binary.two = rhs;
      }
    } break;
    case Ast_Kind_if_split: {
      Irid cond = pop(irgen.irid_stack);
      Blockid headerid     = ir_put_branch(cond);
      Blockid nez_blockid  = ir_block_new();
      irgen_nez_link_to(headerid, nez_blockid);
      add(irgen.headerids, headerid);
    } break;
    case Ast_Kind_if_leave: {
      Blockid headerid    = pop(irgen.headerids);
      Blockid blockid     = irgen_put_jump();
      Blockid eqz_blockid = ir_block_new();
      irgen_eqz_link_to(headerid, eqz_blockid);
      irgen_jump_link_to(blockid, eqz_blockid);
    } break;
    case Ast_Kind_if_leave_else_enter: {
      Blockid headerid    = pop(irgen.headerids);
      Blockid blockid     = irgen_put_jump();
      Blockid eqz_blockid = ir_block_new();
      irgen_eqz_link_to(headerid, eqz_blockid);
      add(irgen.headerids, blockid);
    } break;
    case Ast_Kind_else_leave: {
      Blockid nez_blockid = pop(irgen.headerids);
      Blockid blockid     = irgen_put_jump();
      Blockid end_blockid = ir_block_new();
      irgen_jump_link_to(nez_blockid, end_blockid);
      irgen_jump_link_to(blockid, end_blockid);
    } break;
    default:
      assert(0);
    break;
    }
  }

  arena_free(irgen.temp_arena);
  return irgen.funs;
}

Cstr cstr_from_funs(C8* buffer, Funs funs) {
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
        string_builder_push_ir(&sb, irid, ir);
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
  Umi source_length    = strlen(source) + 2;
  Arena arena          = arena_init(KB(64) * source_length);
  Ast ast              = ast_from_source(&arena, source);
  Funs funs            = ir_ast(&arena, ast);
  C8* buffer           = arena_push(&arena, KB(1) * source_length);
  Cstr result          = cstr_from_funs(buffer, funs);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void irgen_test(void) {
  test("a = 1; a+a; b+b", "");
}

#undef test
