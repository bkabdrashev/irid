typedef enum Ir_Flag {
  Ir_Flag_unary  = Ast_Flag_unary,
  Ir_Flag_binary = Ast_Flag_binary,
} Ir_Flag;

typedef enum Ir_Kind {
  Ir_Kind_nop = 0,

  Ir_Kind_int = Ast_Kind_int,

  Ir_Kind_add = Ast_Kind_add | Ir_Flag_binary,
  Ir_Kind_sub = Ast_Kind_sub | Ir_Flag_binary,
  Ir_Kind_mul = Ast_Kind_mul | Ir_Flag_binary,
  Ir_Kind_eq  = Ast_Kind_eq | Ir_Flag_binary,
  Ir_Kind_ne  = Ast_Kind_ne | Ir_Flag_binary,
  Ir_Kind_lt  = Ast_Kind_lt | Ir_Flag_binary,
  Ir_Kind_le  = Ast_Kind_le | Ir_Flag_binary,
  Ir_Kind_gt  = Ast_Kind_gt | Ir_Flag_binary,
  Ir_Kind_ge  = Ast_Kind_ge | Ir_Flag_binary,
  Ir_Kind_call = Ast_Kind_call | Ir_Flag_binary,

  Ir_Kind_join = Ast_Kind_join | Ir_Flag_binary,

  Ir_Kind_neg = Ast_Kind_neg | Ir_Flag_unary,

  Ir_Kind_load      = Ast_Kind_load | Ir_Flag_unary,
  Ir_Kind_var       = Ast_Kind_name,
  Ir_Kind_declare   = Ast_Kind_declare,
  Ir_Kind_ptr       = Ast_Kind_ptr | Ir_Flag_unary,
  Ir_Kind_store     = 128 | Ir_Flag_binary,

  Ir_Kind_record          = 131,
  Ir_Kind_position_offset = 132,
  Ir_Kind_position_update = 133,
  Ir_Kind_name_offset     = 134,
  Ir_Kind_name_update     = 135,

  Ir_Kind_fun  = 136,
  Ir_Kind_arg  = 137,

  Ir_Kind_range = 138 | Ir_Flag_binary,
  Ir_Kind_bits  = 139 | Ir_Flag_binary,

} Ir_Kind;

typedef struct Ir    Ir;
typedef struct Block Block;
typedef struct Fun   Fun;
typedef struct Type  Type;

typedef struct Ir_Pair Ir_Pair;
struct Ir_Pair { Ir* one; Ir* two; };

typedef struct Name_Offset Name_Offset;
struct Name_Offset { Ir* of; Str* at; };

typedef struct Declare Declare;
struct Declare { Var* var; };

typedef struct Position_Offset Position_Offset;
struct Position_Offset { Ir* of; I32 at; };

typedef struct Record Record;
struct Record {
  I32      length;
  Ir**     assigned;
  Ir**     declared;
  I32*     offsets;
  Str**    names;
  Var**    vars;
  Hash_Map position_from_name;
};

typedef struct Type Type;
typedef struct Var Var;
typedef struct Blocks Blocks;

typedef enum Var_State {
  Var_State_unresolved,
  Var_State_resolving,
  Var_State_resolved,
} Var_State;

struct Var {
  Var_State state;
  Var*  parent;
  Str*  name;
  Type* declared;
  Ir*     declared_ir;
  Blocks* blocks;
};

struct Ir {
  Ir_Kind kind;
  union {
    I64 i64;
    Var*    var;
    Fun*    fun;
    Record* record;
    Ir_Pair binary;
    Ir*     unary;
    Position_Offset position;
    Name_Offset     name;
    Declare         declare;
  };
};

typedef struct Ir_Pool Ir_Pool;
struct Ir_Pool {
  I32 length;
  Ir* base;
};

typedef struct Irs Irs;
struct Irs {
  I32 length;
  Ir* base[];
};

typedef struct Field Field;
struct Field {
  Str* name;
  I32  position;
  I32  offset;
  Ir*  assigned;
  Ir*  declared;
  Type* declared_type;
  Type* assigned_type;
  Var*  var;
};

typedef struct Jump Jump;
struct Jump {
  Block* to_block;
};

typedef struct Branch Branch;
struct Branch {
  Ir*  cond;
  Jump eqz;
  Jump nez;
};

typedef struct Funs Funs;
struct Funs {
  Fun* base;
  I32  length;
};

typedef struct Block_Pool Block_Pool;
struct Block_Pool {
  I32    length;
  Block* base;
};

typedef enum Block_Kind {
  Block_Kind_none,
  Block_Kind_jump,
  Block_Kind_branch,
} Block_Kind;

typedef enum Block_State {
  Block_State_unknown,
  Block_State_unreachable,
  Block_State_reachable,
} Block_State;

struct Blocks {
  I32    length;
  Block* base[];
};

typedef struct Block_List Block_List;
struct Block_List {
  I32    length;
  Block** base;
};

struct Block {
  Block_Kind kind;
  Block_State state;
  B8 is_present_in_worklist;


  B8 is_scc_visited;
  B8 is_on_scc_stack;
  I32 sccid;
  I32 low_sccid;

  B8 is_dfs_visited;
  I32 rpo;
  Block* idom;

  Block_List preds;

  Irs* irs;
  Hash_Map out_var_types;
  union {
    Jump   jump;
    Branch branch;
  };
};

typedef struct Record_Pool Record_Pool;
struct Record_Pool {
  I32     length;
  Record* base;
};

struct Fun {
  Str*    name;
  Blocks* blocks;
  Var*    arg;
  Ir*     return_ir;
  I32     var_count;
};

typedef struct Fun_Stack Fun_Stack;
struct Fun_Stack {
  Fun** base;
  I32   length;
};

typedef struct Irgen Irgen;
struct Irgen {
  Ast_Block ast;

  Arena*  perm_arena;
  Arena*  temp_block_arena;
  Arena*  temp_ir_arena;

  Funs        funs;
  Block_Pool  blocks;
  Record_Pool records;
  Ir_Pool     irs;

  Ir*         irid_nil;

  Fun_Stack   fun_stack;
  Blocks*     block_stack;
  Hash_Map*   builtins;
  Scope_Stack scope_stack;
};

Irgen irgen = {};

Field record_get_by_name(Record* record, Str* name) {
  Field field = {};
  I32 position = hash_map_get_i32(&record->position_from_name, name);
  field.name     = record->names[position];
  field.assigned = record->assigned[position];
  field.var      = record->vars[position];
  field.position = position;
  return field;
}

Field record_get_by_position(Record* record, I32 position) {
  Field field = {};
  field.name     = record->names[position];
  field.assigned = record->assigned[position];
  field.position = position;
  return field;
}

void string_builder_push_irid(String_Builder* sb, Ir* ir) {
  string_builder_push_cstr(sb, "r");
  I32 irid = ir - irgen.irs.base;
  string_builder_push_i64(sb, irid);
}

void string_builder_push_var(String_Builder* sb, Var* var) {
  if (var->parent) {
    string_builder_push_var(sb, var->parent);
    string_builder_push_cstr(sb, ".");
  }
  string_builder_push_str(sb, var->name);
}

void string_builder_push_fun(String_Builder* sb, Fun* fun) {
  string_builder_push_cstr(sb, "@");
  if (fun->name) {
    string_builder_push_str(sb, fun->name);
  }
  else {
    I32 fun_offset = fun - irgen.funs.base;
    string_builder_push_i64(sb, fun_offset);
  }
}

void string_builder_push_blockid(String_Builder* sb, Block* block) {
  string_builder_push_cstr(sb, "b");
  I32 blockid = block - irgen.blocks.base;
  string_builder_push_i64(sb, blockid);
}

void string_builder_push_block(String_Builder* sb, Block* block, I32 indent);
void string_builder_push_ir(String_Builder* sb, Ir* ir) {
  string_builder_push_irid(sb, ir);
  string_builder_push_cstr(sb, " = ");
  switch (ir->kind) {
  case Ir_Kind_nop:
    string_builder_push_cstr(sb, "nop");
  break;
  case Ir_Kind_int:
    string_builder_push_cstr(sb, "int ");
    string_builder_push_i64(sb, ir->i64);
  break;
  case Ir_Kind_declare: {
    Var* var = ir->declare.var;
    string_builder_push_cstr(sb, "declare ");
    string_builder_push_var(sb, var);
    string_builder_push_cstr(sb, ": ");
    string_builder_push_irid(sb, var->declared_ir);
    for (I32 i = 0; i < var->blocks->length; i++) {
      string_builder_push_block(sb, var->blocks->base[i], 3);
    }
  } break;
  case Ir_Kind_var:
    string_builder_push_cstr(sb, "var ");
    string_builder_push_var(sb, ir->var);
  break;
  case Ir_Kind_fun:
    string_builder_push_cstr(sb, "fun ");
    string_builder_push_fun(sb, ir->fun);
  break;
  case Ir_Kind_position_offset:
    string_builder_push_cstr(sb, "position offset ");
    string_builder_push_irid(sb, ir->position.of);
    string_builder_push_cstr(sb, ".");
    string_builder_push_i64(sb, ir->position.at);
  break;
  case Ir_Kind_name_offset:
    string_builder_push_cstr(sb, "name offset ");
    string_builder_push_irid(sb, ir->name.of);
    string_builder_push_cstr(sb, ".");
    string_builder_push_str(sb, ir->name.at);
  break;
  case Ir_Kind_record:
    string_builder_push_cstr(sb, "record");
    for (I32 i = 0; i < ir->record->length; i++) {
      Field field = record_get_by_position(ir->record, i);
      if (field.name) {
        string_builder_push_cstr(sb, " ");
        string_builder_push_str(sb, field.name);
        if (field.declared != irgen.irid_nil) {
          string_builder_push_cstr(sb, ":");
          string_builder_push_irid(sb, field.declared);
        }
        if (field.assigned != irgen.irid_nil) {
          string_builder_push_cstr(sb, "=");
          string_builder_push_irid(sb, field.assigned);
        }
      }
      else {
        string_builder_push_cstr(sb, " ");
        string_builder_push_irid(sb, field.assigned);
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
  case Ir_Kind_call: string_builder_push_cstr(sb, "call "); break;
  case Ir_Kind_range: string_builder_push_cstr(sb, "range "); break;
  case Ir_Kind_bits: string_builder_push_cstr(sb, "bits "); break;
  default: {
    assert(0);
  } break;
  }
  if (ir->kind & Ir_Flag_binary) {
    string_builder_push_irid(sb, ir->binary.one);
    string_builder_push_cstr(sb, " ");
    string_builder_push_irid(sb, ir->binary.two);
  }
  else if (ir->kind & Ir_Flag_unary) {
    string_builder_push_irid(sb, ir->unary);
  }
}

void string_builder_push_block(String_Builder* sb, Block* block, I32 indent) {
  string_builder_push_cstr(sb, "\n");
  string_builder_push_indent(sb, indent);
  string_builder_push_blockid(sb, block);
  string_builder_push_cstr(sb, ":");
  for (I32 irid = 0; irid < block->irs->length; irid++) {
    Ir* ir = block->irs->base[irid];
    string_builder_push_cstr(sb, "\n");
    string_builder_push_indent(sb, indent+1);
    string_builder_push_ir(sb, ir);
  }
  switch (block->kind) {
  case Block_Kind_none:
  break;
  case Block_Kind_jump:
    string_builder_push_cstr(sb, "\n");
    string_builder_push_indent(sb, indent+1);
    string_builder_push_cstr(sb, "jump ");
    string_builder_push_blockid(sb, block->jump.to_block);
  break;
  case Block_Kind_branch:
    string_builder_push_cstr(sb, "\n");
    string_builder_push_indent(sb, indent+1);
    string_builder_push_cstr(sb, "if ");
    string_builder_push_irid(sb, block->branch.cond);
    string_builder_push_cstr(sb, " then ");
    string_builder_push_blockid(sb, block->branch.nez.to_block);
    string_builder_push_cstr(sb, " else ");
    string_builder_push_blockid(sb, block->branch.eqz.to_block);
  break;
  }
}

Cstr cstr_from_funs(C8* buffer, Funs funs) {
  String_Builder sb = string_builder_begin(buffer);
  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &irgen.funs.base[f];
    string_builder_push_fun(&sb,   fun);
    string_builder_push_cstr(&sb, " {");

    for (I32 b = 0; b < fun->blocks->length; b++) {
      Block* block = fun->blocks->base[b];
      string_builder_push_block(&sb, block, 1);
    }

    string_builder_push_cstr(&sb, "\n  ret ");
    string_builder_push_irid(&sb, fun->return_ir);
    string_builder_push_cstr(&sb, "\n}\n");
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

void irgen_print(void) {
  assert(0);
}

void irgen_irs_end(Irs* list) {
  arena_release_mark(irgen.temp_ir_arena, list);
}

void irgen_irs_push(Irs* list, Ir* ir) {
  arena_push(irgen.temp_ir_arena, sizeof(Ir*));
  list->base[list->length++] = ir;
}

Irs* irgen_irs_temp(void) {
  return arena_push_zero(irgen.temp_ir_arena, sizeof(Irs));
}

Irs* irgen_irs_perm(Irs* temp_list) {
  I32 size = sizeof(Irs) + temp_list->length * sizeof(Ir*);
  Irs* perm_list = arena_push(irgen.perm_arena, size);
  memcpy(perm_list, temp_list, size);
  arena_release_mark(irgen.temp_ir_arena, temp_list);
  return perm_list;
}

void irgen_blocks_end(Blocks* list) {
  arena_release_mark(irgen.temp_block_arena, list);
}

void irgen_blocks_push(Blocks* list, Block* block) {
  arena_push(irgen.temp_block_arena, sizeof(Block*));
  list->base[list->length++] = block;
}

Blocks* irgen_blocks_temp(void) {
  return arena_push_zero(irgen.temp_block_arena, sizeof(Blocks));
}

Blocks* irgen_blocks_perm(Blocks* temp_list) {
  I32 size = sizeof(Blocks) + temp_list->length * sizeof(Block*);
  Blocks* perm_list = arena_push(irgen.perm_arena, size);
  memcpy(perm_list, temp_list, size);
  arena_release_mark(irgen.temp_block_arena, temp_list);
  return perm_list;
}

Block* irgen_top_block(void) {
  Fun* fun = top(irgen.fun_stack);
  Block* block = fun->blocks->base[fun->blocks->length-1];
  return block;
}

Ir* irgen_push(Ir ir) {
  Block* block = irgen_top_block();
  Ir* new_ir = &new(irgen.irs);
  *new_ir = ir;
  irgen_irs_push(block->irs, new_ir);
  return new_ir;
}

Ir* irgen_pop(void) {
  Block* block = irgen_top_block();
  Ir* new_ir = &pop(irgen.irs);
  fa_del(block->irs);
  return new_ir;
}

Ir* irgen_push_int(I64 i64) {
  Ir ir = { Ir_Kind_int, .i64 = i64 };
  return irgen_push(ir);
}

Ir* irgen_push_var(Var* var) {
  Ir ir = { Ir_Kind_var, .var = var };
  return irgen_push(ir);
}

Ir* irgen_push_fun(Fun* fun) {
  Ir ir = { Ir_Kind_fun, .fun = fun };
  return irgen_push(ir);
}

Ir* irgen_push_arg(void) {
  Ir ir = { Ir_Kind_arg, {0} };
  return irgen_push(ir);
}

Ir* irgen_push_declare(Var* var) {
  Ir ir = { Ir_Kind_declare, .declare = { .var = var }};
  return irgen_push(ir);
}

Ir* irgen_push_unary(Ir_Kind kind, Ir* one) {
  Ir ir = { kind, .unary = one };
  return irgen_push(ir);
}

Ir* irgen_push_binary(Ir_Kind kind, Ir* one, Ir* two) {
  Ir ir = { kind, .binary = { .one = one, .two = two } };
  return irgen_push(ir);
}

Ir* irgen_push_record(Record* record) {
  Ir ir = { Ir_Kind_record, .record = record };
  return irgen_push(ir);
}

Ir* irgen_push_position_offset(Ir* record, I32 position) {
  Ir ir = { Ir_Kind_position_offset, .position = { .of = record, .at = position } };
  return irgen_push(ir);
}

Ir* irgen_push_name_offset(Ir* record, Str* name) {
  Ir ir = { Ir_Kind_name_offset, .name = { .of = record, .at = name } };
  return irgen_push(ir);
}

Record* record_new(I32 length) {
  Fun* fun = top(irgen.fun_stack);

  Record* new_record = &new(irgen.records);
  new_record->length   = length;
  new_record->names    = arena_push_zero(irgen.perm_arena, length*sizeof(Str*));
  new_record->assigned = arena_push_zero(irgen.perm_arena, length*sizeof(Ir*));
  new_record->declared = arena_push_zero(irgen.perm_arena, length*sizeof(Ir*));
  new_record->offsets  = arena_push_zero(irgen.perm_arena, length*sizeof(Type*));
  new_record->position_from_name = hash_map_init(irgen.perm_arena, length);
  fun->var_count += length;
  return new_record;
}

void record_push_assign_position(Record* record, I32 position, Ir* value) {
  record->assigned[position] = value;
}
void record_push_assign_name(Record* record, Str* name, I32 position) {
  hash_map_put_i32(&record->position_from_name, name, position);
  record->names[position] = name;
}
void record_push_declare_position(Record* record, I32 position, Ir* value) {
  record->declared[position] = value;
}
void record_push_declare_name(Record* record, Str* name, I32 position) {
  hash_map_put_i32(&record->position_from_name, name, position);
  record->names[position] = name;
}

Block* irgen_put_branch(Ir* cond) {
  Block* block = irgen_top_block();
  block->branch.cond = cond;
  block->kind = Block_Kind_branch;
  return block;
}

Block* irgen_put_jump(void) {
  Block* block = irgen_top_block();
  block->kind = Block_Kind_jump;
  return block;
}

Block* irgen_block_leave(void) {
  Block* block = irgen_top_block();
  block->irs = irgen_irs_perm(block->irs);
  return block;
}

Block* irgen_block_new(void) {
  irgen_block_leave();
  Fun* fun = top(irgen.fun_stack);
  Block* block = &new(irgen.blocks);
  irgen_blocks_push(fun->blocks, block);
  memset(block, 0, sizeof(Block));
  block->irs = irgen_irs_temp();
  return block;
}

Ir* irgen_ast_node(Ast_Node* node);

Symbol* irgen_get_sym(Str* str) {
  for (I32 i = 0; i < irgen.scope_stack.length; i++) {
    Hash_Map* scope = irgen.scope_stack.base[i];
    Symbol* sym = hash_map_get(scope, str);
    if (sym) {
      return sym;
    }
  }
  return 0;
}

void irgen_decl_var(Var* var, Ast_Node* node) {
  Fun* fun = top(irgen.fun_stack);
  Fun temp_fun = {};
  temp_fun.name = 0;
  temp_fun.var_count = 0;
  add(irgen.fun_stack, &temp_fun);
  {
    Block* block = &new(irgen.blocks);
    memset(block, 0, sizeof(Block));
    block->irs = irgen_irs_temp();
    temp_fun.blocks = irgen_blocks_temp();
    irgen_blocks_push(temp_fun.blocks, block);
  }

  Ir* ir = irgen_ast_node(node);
  fun->var_count += temp_fun.var_count;
  irgen_block_leave();
  var->blocks = irgen_blocks_perm(temp_fun.blocks);
  var->declared_ir = ir;
  del(irgen.fun_stack);
}

void irgen_link_jump_to_block(Jump* jump, Block* block);
void irgen_scope_enter(Hash_Map* scope) {
  Fun* fun = top(irgen.fun_stack);
  for (I32 i = 0; i < scope->len; i++) {
    Str* key = scope->list[i];
    {
      Symbol* sym = irgen_get_sym(key);
      if (sym) assert(0);
    }
    Var* var = arena_push_zero(irgen.perm_arena, sizeof(Var));
    Symbol* sym = hash_map_get(scope, key);
    sym->var = var;
    var->name = key;

    fun->var_count++;
  }
  add(irgen.scope_stack, scope);

  for (I32 i = 0; i < scope->len; i++) {
    Str* key = scope->list[i];
    Symbol* sym = hash_map_get(scope, key);
    irgen_decl_var(sym->var, sym->ast);
    irgen_push_declare(sym->var);
  }
}

void irgen_scope_leave(void) {
  del(irgen.scope_stack);
}

Fun* irgen_fun_enter(void) {
  Fun* fun = &new(irgen.funs);
  fun->name = 0;
  add(irgen.fun_stack, fun);
  {
    Block* block = &new(irgen.blocks);
    memset(block, 0, sizeof(Block));
    block->irs = irgen_irs_temp();
    fun->blocks = irgen_blocks_temp();
    irgen_blocks_push(fun->blocks, block);
  }
  fun->return_ir = &top(irgen.irs);
  fun->var_count = irgen.builtins->len;
  return fun;
}

void irgen_link_jump_to_block(Jump* jump, Block* block) {
  jump->to_block = block;
  block->preds.length++;
}

Fun* irgen_fun_leave(void) {
  irgen_block_leave();
  Fun* fun = pop(irgen.fun_stack);
  fun->blocks = irgen_blocks_perm(fun->blocks);
  irgen_scope_leave();
  return fun;
}

void irgen_assign(Ast_Node* lhs, Ir* rhs) {
  switch (lhs->kind) {
  case Ast_Kind_tuple: {
    for (I32 i = 0; i < lhs->list->length; i++) {
      Ir* at = irgen_push_position_offset(rhs, i);
      Ast_Node* node = lhs->list->base[i];
      irgen_assign(node, at);
    }
  } break;
  default : {
    Ir* lhs_ir = irgen_ast_node(lhs);
    if (lhs_ir->kind == Ir_Kind_load) {
      lhs_ir->kind = Ir_Kind_store;
      lhs_ir->binary.two = rhs;
    }
  } break;
  }
}

Ir* irgen_ast_node(Ast_Node* node) {
  Ir* result = 0;
  switch (node->kind) {
  case Ast_Kind_int: {
    result = irgen_push_int(node->i64);
  } break;
  case Ast_Kind_name: {
    Symbol* sym = irgen_get_sym(node->str);
    if (sym) {
      Ir* var_ir = irgen_push_var(sym->var);
      result = irgen_push_unary(Ir_Kind_load, var_ir);
    }
    else {
      assert(0);
    }
  } break;
  case Ast_Kind_dot: {
    Ir* lhs = irgen_ast_node(node->binary.lhs);
    if (node->binary.rhs->kind == Ast_Kind_name) {
      Ir* name_offset_ir = irgen_push_name_offset(lhs, node->binary.rhs->str);
      result = irgen_push_unary(Ir_Kind_load, name_offset_ir);
    }
  } break;
  case Ast_Kind_assign: {
    Ir* rhs = irgen_ast_node(node->binary.rhs);
    irgen_assign(node->binary.lhs, rhs);
  } break;
  case Ast_Kind_tuple: {
    Record* record = record_new(node->list->length);
    for (I32 i = 0; i < node->list->length; i++) {
      Ir* ir = irgen_ast_node(node->list->base[i]);
      record_push_assign_position(record, i, ir);
    }
    result = irgen_push_record(record);
  } break;
  case Ast_Kind_record: {
    Record* record = record_new(node->list->length);
    for (I32 i = 0; i < node->list->length; i++) {
      Ast_Node* field_node = node->list->base[i];
      if (field_node->kind == Ast_Kind_assign_field) {
        Ir* ir = irgen_ast_node(field_node->declare.rhs);
        record_push_assign_name(record, field_node->declare.str, i);
        record_push_assign_position(record, i, ir);
      }
      else if (field_node->kind == Ast_Kind_declare_field) {
        Ir* ir = irgen_ast_node(field_node->declare.rhs);
        record_push_assign_name(record, field_node->declare.str, i);
        record_push_declare_position(record, i, ir);
      }
      else {
        Ir* ir = irgen_ast_node(field_node);
        record_push_assign_position(record, i, ir);
      }
    }
    result = irgen_push_record(record);
  } break;
  case Ast_Kind_block: {
    irgen_scope_enter(node->block.scope);
    for (I32 i = 0; i < node->block.list->length; i++) {
      Ast_Node* exp = node->block.list->base[i];
      irgen_ast_node(exp);
    }
    irgen_scope_leave();
  } break;
  case Ast_Kind_iblock: {
    for (I32 i = 0; i < node->list->length; i++) {
      Ast_Node* exp = node->list->base[i];
      irgen_ast_node(exp);
    }
  } break;
  case Ast_Kind_ptr: {
    Ir* unary = irgen_ast_node(node->unary);
    if (unary->kind == Ir_Kind_load) {
      result = unary->unary;
      irgen_pop();
    }
    else {
      result = irgen_push_unary(Ir_Kind_ptr, unary);
    }
  } break;
  case Ast_Kind_load:
  case Ast_Kind_pos: case Ast_Kind_neg: {
    Ir* unary = irgen_ast_node(node->unary);
    result = irgen_push_unary((Ir_Kind)node->kind | Ir_Flag_unary, unary);
  } break;
  case Ast_Kind_call: case Ast_Kind_subscript:
  case Ast_Kind_array:
  case Ast_Kind_eq: case Ast_Kind_ne:
  case Ast_Kind_le: case Ast_Kind_lt:
  case Ast_Kind_ge: case Ast_Kind_gt:
  case Ast_Kind_mul: case Ast_Kind_join:
  case Ast_Kind_add: case Ast_Kind_sub: {
    Ir* lhs = irgen_ast_node(node->binary.lhs);
    Ir* rhs = irgen_ast_node(node->binary.rhs);
    result = irgen_push_binary((Ir_Kind)node->kind | Ir_Flag_binary, lhs, rhs);
  } break;
  case Ast_Kind_if: {
    Ast_Node* cond = node->binary.lhs;
    Ast_Node* then = node->binary.rhs;
    Ir* cond_ir = irgen_ast_node(cond);
    Block* branch_from_block = irgen_put_branch(cond_ir);
    Block* nez_block = irgen_block_new();
    irgen_link_jump_to_block(&branch_from_block->branch.nez, nez_block);
    irgen_ast_node(then);
    Block* jump_from_block = irgen_put_jump();
    Block* eqz_block = irgen_block_new();
    irgen_link_jump_to_block(&jump_from_block->jump, eqz_block);
    irgen_link_jump_to_block(&branch_from_block->branch.eqz, eqz_block);
  } break;
  case Ast_Kind_else: {
    Ast_Node* cond = node->binary.lhs->binary.lhs;
    Ast_Node* then_node = node->binary.lhs->binary.rhs;
    Ast_Node* else_node = node->binary.rhs;
    Ir* cond_ir = irgen_ast_node(cond);
    Block* branch_from_block = irgen_put_branch(cond_ir);
    Block* nez_block = irgen_block_new();
    irgen_link_jump_to_block(&branch_from_block->branch.nez, nez_block);
    irgen_ast_node(then_node);
    Block* jump_from_nez = irgen_put_jump();
    Block* eqz_block = irgen_block_new();
    irgen_link_jump_to_block(&branch_from_block->branch.eqz, eqz_block);
    irgen_ast_node(else_node);
    Block* jump_from_eqz = irgen_put_jump();
    Block* merge_block = irgen_block_new();
    irgen_link_jump_to_block(&jump_from_eqz->jump, merge_block);
    irgen_link_jump_to_block(&jump_from_nez->jump, merge_block);
  } break;
  case Ast_Kind_while: {
    Ast_Node* cond = node->binary.lhs;
    Ast_Node* then = node->binary.rhs;
    Block* while_entry    = irgen_put_jump();
    Block* while_header   = irgen_block_new();
    irgen_link_jump_to_block(&while_entry->jump, while_header);
    Ir* cond_ir = irgen_ast_node(cond);
    Block* branch_from_header = irgen_put_branch(cond_ir);
    Block* body_entry         = irgen_block_new(); // not equal zero
    irgen_link_jump_to_block(&branch_from_header->branch.nez, body_entry);
                                irgen_ast_node(then);
    Block* jump_from_body_to_header = irgen_put_jump();
    irgen_link_jump_to_block(&jump_from_body_to_header->jump, while_header);
    Block* while_exit           = irgen_block_new(); // equal zero
    irgen_link_jump_to_block(&branch_from_header->branch.eqz, while_exit);
  } break;
  case Ast_Kind_fun: {
    irgen_fun_enter();
    Var* arg_var = arena_push_zero(irgen.perm_arena, sizeof(Var));
    irgen_decl_var(arg_var, node->binary.lhs);
    Hash_Map scope;
    {
      Ast_Node* lhs = node->binary.lhs;
      if (lhs->kind == Ast_Kind_record) {
        scope = hash_map_init(irgen.perm_arena, lhs->list->length);
        for (I32 i = 0; i < lhs->list->length; i++) {
          Ast_Node* param = lhs->list->base[i];
          if (param->kind == Ast_Kind_declare_field) {
            Symbol* sym = arena_push(irgen.perm_arena, sizeof(Symbol));
            sym->ast = param->declare.rhs;
            hash_map_put(&scope, param->declare.str, sym);
          }
          else if (node->kind == Ast_Kind_assign_field) {
            assert(0);
          }
          else {
            assert(0);
          }
        }
        irgen_scope_enter(&scope);
      }
      else {
        assert(0);
      }
    }
    Ir* rhs = irgen_ast_node(node->binary.rhs);
    Fun* fun = irgen_fun_leave();
    fun->return_ir = rhs;
    fun->arg = arg_var;
    result = irgen_push_fun(fun);
  } break;
  case Ast_Kind_return: case Ast_Kind_break:
  case Ast_Kind_return_value: case Ast_Kind_break_value:
  case Ast_Kind_block_value:
  case Ast_Kind_if_value: case Ast_Kind_else_value:
  case Ast_Kind_none: { assert(0); }
  case Ast_Kind_declare: { assert(0); }
  case Ast_Kind_assign_field: { assert(0); }
  case Ast_Kind_declare_field: { assert(0); }
  }
  return result;
}

Funs irgen_ast(Arena* arena, Ast_Block ast, I32 total_nodes) {
  Arena temp_block_arena = arena_init(arena->capacity);
  Arena temp_ir_arena    = arena_init(arena->capacity);
  irgen.ast = ast;
  irgen.perm_arena = arena;
  irgen.temp_block_arena = &temp_block_arena;
  irgen.temp_ir_arena    = &temp_ir_arena;
  irgen.funs.base      = arena_push(irgen.perm_arena, total_nodes * sizeof(Fun));
  irgen.funs.length    = 0;
  irgen.blocks.base    = arena_push(irgen.perm_arena, total_nodes * sizeof(Block));
  irgen.blocks.length  = 0;
  irgen.irs.base       = arena_push(irgen.perm_arena, total_nodes * sizeof(Ir));
  irgen.irs.length     = 0;
  irgen.records.base   = arena_push(irgen.perm_arena, total_nodes * sizeof(Record));
  irgen.records.length = 0;
  irgen.fun_stack.base        = arena_push(irgen.temp_block_arena, total_nodes * sizeof(Fun*));
  irgen.fun_stack.length      = 0;
  irgen.block_stack           = arena_push(irgen.temp_block_arena, sizeof(Block) + total_nodes * sizeof(Block*));
  irgen.block_stack->length   = 0;
  irgen.scope_stack.base      = arena_push(irgen.temp_block_arena, total_nodes * sizeof(Hash_Map*));
  irgen.scope_stack.length    = 0;
  irgen.irid_nil = 0;
  Ir ir_nil = {0, {0}};
  add(irgen.irs, ir_nil);

  {
    irgen.builtins = arena_push(irgen.perm_arena, sizeof(Hash_Map));
    *irgen.builtins = hash_map_init(irgen.perm_arena, 1);
    Str* i32_str = str_from_cstr("I32");
    Symbol* i32_sym = arena_push_zero(irgen.perm_arena, sizeof(Symbol));
    Var* i32_var = arena_push_zero(irgen.perm_arena, sizeof(Var));
    i32_sym->var = i32_var;
    i32_var->name = i32_str;

    Fun temp_fun = {};
    temp_fun.var_count = 0;
    add(irgen.fun_stack, &temp_fun);
    {
      Block* block = &new(irgen.blocks);
      memset(block, 0, sizeof(Block));
      block->irs = irgen_irs_temp();
      temp_fun.blocks = irgen_blocks_temp();
      irgen_blocks_push(temp_fun.blocks, block);
    }

    Ir* i32_min = irgen_push_int(I32_MIN);
    Ir* i32_max = irgen_push_int(I32_MAX);
    Ir* i32_range = irgen_push_binary(Ir_Kind_range, i32_min, i32_max);
    Ir* i32_bits = irgen_push_int(32);
    i32_var->declared_ir = irgen_push_binary(Ir_Kind_bits, i32_range, i32_bits);
    hash_map_put(irgen.builtins, i32_str, i32_sym);
    add(irgen.scope_stack, irgen.builtins);

    irgen_block_leave();

    i32_var->blocks = irgen_blocks_perm(temp_fun.blocks);

    del(irgen.fun_stack);
  }

  irgen_fun_enter();
  irgen_scope_enter(ast.scope);
  for (I32 i = 0; i < ast.list->length; i++) {
    irgen_ast_node(ast.list->base[i]);
  }
  irgen_fun_leave();
  arena_free(irgen.temp_block_arena);
  arena_free(irgen.temp_ir_arena);
  return irgen.funs;
}

void _test_ir(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source) + 2;
  Arena arena          = arena_init(KB(2) * source_length);
                         str_init(&arena, 2*source_length);
  Tokens tokens        = lex_source(&arena, source);
  Ast_Block ast        = parse_tokens(&arena, tokens);
  Funs funs            = irgen_ast(&arena, ast, source_length);
  C8* buffer           = arena_push(&arena, KB(1) * source_length);
  Cstr result          = cstr_from_funs(buffer, funs);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void irgen_test(void) {
  // test("a: 1; wh 2 do { if 3 do { a = 1 } }; a+a", "");
  // test("b:a; a: 2", "");
  // test("a:1", "");
  // test("A: (val:1; next:@B); B: (val:2; next:@A)", "");
  // test("A: (val:1; next:@B); B: (val:2; next:@A); a: A; b: B; a.next = @b; a.next@.val", "");
  // test("a:I32; a=0; wh a != 10 do {a = a+ 1}", "");
  test("foo:(a:I32; b:I32) -> a+b; foo(1)", "");
}

#undef test
