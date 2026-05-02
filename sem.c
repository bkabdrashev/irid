typedef I32 Typeid;

typedef enum Type_Kind Type_Kind;
enum Type_Kind {
  Type_Kind_none,
};

typedef struct Sem_Tasks Sem_Tasks;
struct Sem_Tasks {};

typedef struct Sem Sem;
struct Sem {
  DMap typeid_of_irid;
};

typedef struct Typeid_Pair Typeid_Pair;
struct Typeid_Pair { Typeid one; Typeid two; };

Typeid typeid_of_irid(Irid irid) {
  return get(sem.typeid_of_irid, irid);
}

Typeid_Pair typeid_of_irid_binary(Irid irid) {
  Irid_Pair pair = irid_binary(irid);
  Typeid one = typeid_of_irid(pair.one);
  Typeid two = typeid_of_irid(pair.two);
  Typeid_Pair result = { one, two };
  return result;
}

B8 typeid_kind_of_irid_binary_operands_equal(Irid irid, Type_Kind type_kind) {
  Pair_Typeid pair = typeid_of_irid_binary(irid);
  return pair.tag == type_tag && pair.tag == type_tag;
}

Sem_Tasks sem_narrow_nez(Blockid blockid) {
  Sem_Tasks tasks = {};
  Irid condition = blockid_branch_condition(blockid);
  switch (irid_kind(condition)) {
  case Ir_Kind_eq: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_eq(condition);
    }
    else {
      log_todo("Ir_Kind_eq narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_ne: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_ne(condition);
    }
    else {
      log_todo("Ir_Kind_ne narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_lt: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_lt(condition);
    }
    else {
      log_todo("Ir_Kind_lt narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_le: log_todo("Ir_Kind_le narrow nez");
  case Ir_Kind_gt: log_todo("Ir_Kind_gt narrow nez");
  case Ir_Kind_ge: log_todo("Ir_Kind_ge narrow nez");
  default: {
    Typeid old_typeid = typeid_of_irid(condition);
    Typeid new_typeid = typeid_of_irid_narrow_nez(old_typeid);
    typeid_of_irid_set(irid, new_typeid);
    typeid_tasks_push(typeid_tasks, irid, old_typeid);
  } break;
  }

  return typeid_tasks;
}

Typeid_Tasks sem_narrow_eqz(Branchid branchid) {
  Typeid_Tasks typeid_tasks = {};
  IRid condition = branchid_condition(branchid);
  switch (ir_tag(condition)) {
  case Ir_Kind_eq: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_ne(condition);
    }
    else {
      log_todo("Ir_Kind_eq narrow eqz between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_ne: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_eq(condition);
    }
    else {
      log_todo("Ir_Kind_ne narrow eqz between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_lt: log_todo("Ir_Kind_lt narrow eqz"); break;
  case Ir_Kind_le: log_todo("Ir_Kind_le narrow eqz"); break;
  case Ir_Kind_gt: log_todo("Ir_Kind_gt narrow eqz"); break;
  case Ir_Kind_ge: log_todo("Ir_Kind_ge narrow eqz"); break;
  default: {
    Typeid old_typeid = typeid_of_irid(condition);
    Typeid new_typeid = typeid_of_irid_narrow_eqz(old_typeid);
    typeid_of_irid_set(irid, new_typeid);
    typeid_tasks_push(typeid_tasks, irid, old_typeid);
  } break;
  }

  return typeid_tasks;
}



Typeid typeid_join(Typeid one, Typeid two) {
  assert(0);
  return one;
}

B8 typeid_not_equal(Typeid one, Typeid two) {
  assert(0);
  return one != two;
}

void sem_worklist_push(Blockid blockid) {
}

void sem_jump(Funid funid, Blockid from_blockid, Blockid to_blockid) {
  B8 is_jump_updates_var_typeid = false;
  Block* from_block = blockid_get(from_blockid);
  Block* to_block = blockid_get(to_blockid);
  Hash_Map* outvars = &from_block->out_var_typeids;
  Hash_Map* invars  = &to_block->in_var_typeids;
  for (I32 i = 0; i < outvars->len; i++) {
    Istr name = outvars->list[i];
    Typeid out_var_typeid = hash_map_get(outvars, name);
    Typeid old_var_typeid = hash_map_get(invars,  name);
    Typeid new_var_typeid = typeid_join(out_var_typeid, old_var_typeid);

    if (typeid_not_equal(new_var_typeid, old_var_typeid)) {
      hash_map_put(invars, name, new_var_typeid);
      hash_map_put(&to_block->out_var_typeids, name, new_var_typeid);
      is_jump_updates_var_typeid = true;
    }
  }
  if (is_jump_updates_var_typeid) {
    sem_worklist_push(to_blockid);
  }
}

void sem_ir(Blockid blockid, Irid irid) {
}

void sem_block(Funid funid, Blockid blockid) {
  Block* block = blockid_get(blockid);
  for (Irid i = block->entryid; i <= block->leaveid; i++) {
    sem_ir(blockid, i);
  }

  if (block->kind == Block_Kind_branch) {
    Block* block = blockid_get(blockid);
    { // condition is not equal to zero branch
      Typeid_Tasks undo_narrow = branchid_narrow_nez(branchid);
      Jumpid jumpid = jumpid_of_bbid_nez(blockid);
      sem_jumpid(funid, jumpid);
      typeid_unnarrow(branchid);
    }
    { // condition is equal to zero branch
      Typeid_Tasks undo_narrow = branchid_narrow_eqz(branchid);
      Jumpid jumpid = jumpid_of_bbid_eqz(blockid);
      sem_jumpid(funid, jumpid);
      typeid_unnarrow(undo_narrow);
    }
  }
  else {
    Jumpid jumpid = jumpid_of_bbid(blockid);
    sem_jumpid(funid, jumpid);
  }
}

Tyid sem_funid(Funid funid) {
  log_info("start  typecheck funid: %s", cstr_from_funid(funid));
  log_increment_tab();

  Slice_BBid basic_blocks = funid_get_bbid_slice(funid);
  BBid first_basic_block_id = basic_blocks.offset;
  sem_scc_bbid(first_basic_block_id);
  sem_worklist_push(first_basic_block_id);

  typeid_init_typeid_of_bb_varid(bbgen.var_count, irgen.bb_paramids_count);

  while (sem_worklist_is_not_empty(funid, first_basic_block_id)) {
    BBid bbid = sem_worklist_next_bbid(funid);
    sem_block(funid, bbid);
  }

  sem_unset_working_funid(funid);
  
  log_decrement_tab();
  log_info("finish typecheck funid: %s", cstr_from_funid(funid));

  IRid funid_return = funid_return(funid);
  return sem_typeid_of_irid(funid_return);
}

void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source);
  I32 max_ast_length   = source_length + 2;
  Ast_Node* ast_buffer = xmalloc(sizeof(Ast_Node) * 2*max_ast_length);
  Ast ast              = ast_from_source(source, ast_buffer);
  Fun* funs_buffer      = xmalloc(sizeof(Fun)*ast.length);
  Block* block_buffer  = xmalloc(sizeof(Block)*ast.length);
  Ir* ir_buffer        = xmalloc(sizeof(Ir)*ast.length);
  Record* record_buffer= xmalloc(sizeof(Record)*ast.length);
  Funs funs            = irgen_ast(ast, funs_buffer, block_buffer, ir_buffer, record_buffer);
                         free(ast.base);
                         sem_funid(0);
  C8* buffer           = xmalloc(MB(64));
  Cstr result          = cstr_from_sem(funs, buffer);
  free(funs_buffer);
  free(block_buffer);
  free(ir_buffer);
  free(record_buffer);
  test_at_source(result, expected, file_name, line, source);
  free(buffer);
}

#define test(source, expected) _test_ir(source, expected, __FILE__, __LINE__)

void sem_test(void) {
/*
a = 1
b = 2
if a do { a = 4 }
a+b

main {
  .0: in{}, out{ a: 1, b: 2 }
    r1 = int 1
    r2 = store var a = r1
    r3 = int 2
    r4 = store var b = r3
    r5 = load var a
    if r5 then .1 else .2
  .1: in{}, out{ a: 4 }
    r6 = int 4
    r7 = store var a = r6
    jump .2
  .2: in{ a: 1 or 4; b: 2 }, out{}
    r8 = load var a
    r9 = load var b
    r10 = add r8 r9
    jump .3
  .3:
  ret r0
}
 
------------------
a = 1
b = @a
if 2 do { b@ = 3 }
a+b@

main {
  .0: in{}, out{ a: 1, b: @a }
    r1 = int 1
    r2 = store var a = r1
    r3 = load var a
    r4 = ptr r3 // @a
    r5 = store var b = r4
    r6 = int 2
    if r6 then .1 else .2
  .1: in{ b: @a }, out{ a: 3 }
    r7 = int 3
    r8 = load var b  //  @a
    r9 = store r8 r7
    jump .2
  .2: in{ a: 1 or 3; b: @a }, out{}
    r10 = load var a
    r11 = load var b
    r12 = load r11
    r13 = add r10 r12
    jump .3
  .3:
  ret r0
}
------------------------------
a = 1
b = 2
c = @b
if 3 do { c = @a }
if 4 do { c@ = 5 }
a+b+c@

main {
  .0: in{}, out{ a: 1; b: 2; c: @b }
    r1 = int 1
    r2 = store var a = r1
    r3 = int 2
    r4 = store var b = r3
    r5 = load var b
    r6 = ptr r5
    r7 = store var c = r6
    r8 = int 3
    if r8 then .1 else .2
  .1:
    r9 = load var a
    r10 = ptr r9
    r11 = store var c = r10
    jump .2
  .2: in{}, out{ c: }
    r12 = int 4
    if r12 then .3 else .4
  .3: in{ c }, out{ b: }
    r13 = int 5
    r14 = load var c   
    r15 = store r14 r13
    jump .4
  .4:
    r16 = load var a
    r17 = load var b
    r18 = add r16 r17
    r19 = load var c
    r20 = load r19
    r21 = add r18 r20
    jump .5
  .5:
  ret r0
}
*/

  test("", "");
}

#undef test

