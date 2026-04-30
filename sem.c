void sem_jump(Funid funid, Blockid from_blockid, Blockid to_blockid) {
  b32 is_jump_updates_typeid_of_invar = false;
  Varids varids = ;
  for (I32 v = varids.enterid; v < varids.leaveid, v++) {
    Typeid from_bb_varid_typeid = typeid_of_bb_varid(from_bbid, varid);
    Typeid old_bb_invar_typeid  = typeid_of_bb_invar(bbid, varid);
    Typeid new_bb_invar_typeid  = typeid_join(from_bb_varid_typeid, old_bb_invar_typeid);

    if (typeid_not_equal(new_bb_invar_typeid, old_bb_invar_typeid)) {
      typeid_of_bb_invar_set(bbid, varid, new_bb_irparam_typeid);
      typeid_of_bb_varid_set(bbid, varid, new_bb_irparam_typeid);
      is_jump_updates_typeid_of_invar = true;
    }
  }
  if (is_jump_updates_typeid_of_bb_param || is_jump_updates_typeid_of_memloc) {
    sem_worklist_push(funid, jump_to_bbid);
  }
}

void sem_irid(Blockid blockid, Irid irid) {
}

void sem_bbid(Funid funid, BBid bbid) {
  Block* block = blockid_get(bbid);
  for (Irid i = entryid; i <= block->leaveid; i++) {
    sem_irid(bbid, i);
  }

  if (block->kind == Block_Kind_branch) {
    Branchid branchid = branchid_of_bbid(bbid);
    { // condition is not equal to zero branch
      Typeid_Tasks undo_narrow = branchid_narrow_nez(branchid);
      Jumpid jumpid = jumpid_of_bbid_nez(bbid);
      sem_jumpid(funid, jumpid);
      typeid_unnarrow(branchid);
    }
    { // condition is equal to zero branch
      Typeid_Tasks undo_narrow = branchid_narrow_eqz(branchid);
      Jumpid jumpid = jumpid_of_bbid_eqz(bbid);
      sem_jumpid(funid, jumpid);
      typeid_unnarrow(undo_narrow);
    }
  }
  else {
    Jumpid jumpid = jumpid_of_bbid(bbid);
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
    sem_bbid(funid, bbid);
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
main {
  .0: in{}, out{ a: 1, b: 2 }
    r1 = int 1
    r2 = store var a = r1
    r3 = int 2
    r4 = store var b = r3
    r5 = load var a
    if r5 then .1 else .2
  .1: in{ a: 1, b: 2 }, out{ a: 4, b: 2 }
    r6 = int 4
    r7 = store var a = r6
    jump .2
  .2: in{ a: 1 or 4, b: 2 }, out{ a: 1 or 4, b:  }
    r8 = load var a
    r9 = load var b
    r10 = add r8 r9
    jump .3
  .3:
  ret r0
}*/

  test("", "");
}

#undef test

