Irid irid_new_record(Funid funid, Slice_Ir_Field fields) {
  Ir ir = { IrKind_record , .record = { .fields = fields }};
  Irid id = irid_new_ir(funid);
  return id;
}

Jumpid irid_cond_then(Funid funid, Astid_Binary cond_then, Blockid* blockid) {
}

void irid_assign(Funid funid, Astid astid, Irid irid) {
  switch (astid_kind(astid)) {
  case AstKind_name : {
    irid_new_store_var(funid, varid, regid);
  } break;
  case AstKind_record : {
    Astid_Record astid_record = astid_record(astid);
    for (S32 i = 0; i < slice_ast_field_length(astid_record.fields); i++) {
      Ast_Field field = slice_ast_field_at(astid_record.fields, i);
      Irid at = irid_new_binary(funid, IrKind_position_access, irid, i);
      if (field.name) {
        irid_assign(funid, field.name, at);
        Irid type = irid_gen(funid, field.astid);
        irid_new_binary(funid, IrKind_type_assert, at, tp);
      }
      else {
        irid_assign(funid, field.astid, at);
      }
    }
  } break;
  default : {
    Irid lhs_irid = irid_gen(funid, astid);
    if (irid_kind(funid, lhs_irid) == IrKind_load) {
      irid_set_kind(funid, lhs_irid, IrKind_store)
      irid_set_binary_two(funid, lhs_irid, irid);
    }
  } break;
  }
}

Irid irid_gen(Funid funid, Astid astid) {
  Irid result = IridNone;
  switch (astid_kind(astid)) {
  // (1; a = 2+3; b : 4)
  // stack:
  // r1 = 1
  // r2 = 2
  // r3 = 3
  // r4 = r2 + r3
  // r5 = 4
  // r6 = record r1 r4 r5
  //      {   0: r1,   1: r4,   2: r5 }
  //      { nil: r1, "a": r4, "b": r5 }
  //      {   value,  assign, declare }
  // 
  case Ast_Kind_record_enter : {
    S32 record_length = astid_record_length(astid);
    irid_record_enter(record_length);
  //      {   ?: ?,   ?: ?,   ?: ? }
  //      {   ?: ?,   ?: ?,   ?: ? }
  //      {   ?,  ?, ? }
  } break;
  case Ast_Kind_record_assign  : {
  // r( 1 (2 3 +) = a
    Istr name = astid_name(irgen_consume_astid());
    irid_record_push_name(name, irgen_pop_irid());
  //      {   0: r1;   1: r4;   ?: ? }
  //      { "a":  1;     ?:?;    ?:? }
  //      { nil: r1; "a": r4;   ?: ? }
  //      {   value;  assign;      ? }
  } break;
  case Ast_Kind_record_declare : {
    Istr name = astid_name(astid);
    irid_record_push_name(name, irgen_top_irid());
  //      {   0: r1;   1: r4;   ?: ? }
  //      { "a":  1;     ?:?;    ?:? }
  //      { nil: r1; "a": r4;   ?: ? }
  //      {   value;  assign;      ? }
  } break;
  case Ast_Kind_record_leave : {
    result = irid_record_leave();
  } break;
  case AstKind_return : {
    Irid irid = irid_gen_unary(funid, astid);
    irid_put_return(funid, irid);
    Jumpid jumpid = blockid_set_jump(blockid);
    jumpid_add_arg(jumpid, irid);
    irid_put_pending_jumpid(funid, jumpid);
    blockid_new(funid);
  } break;
  case AstKind_block_value_start : {
    irid_scope_enter(funid);
    irid_push_return_list(funid);
    irid_push_jumps(funid);
  } break;
  case AstKind_block_value_end : {
    result = irid_pop_return_list(funid);
    irid_scope_leave(funid);
  } break;
  case AstKind_block_start : {
    irid_scope_enter(funid);
  } break;
  case AstKind_block_end : {
    irid_scope_leave(funid);
  } break;
  case AstKind_assign : {
    /*
    1, 2, +, =, r(a, r(b, c))
    r1 = 1
    r2 = 2
    r3 = r1 + r2
    r4 = r3.0
    store var a r4
    r5 = r3.1
    r6 = r5.0
    store var b r6
    r7 = r5.1
    store var c r7

    irid_assign () {
      if name {
        store var with top irid
      }
      if record start {
        record_length = astid_record_length(astid);
        for (i in 0..length) {
          Irid at = irid_new_binary(funid, IrKind_position_access, irid, i);
          assign_value = at;
        }
      }
      if record end {
        irgen state pop assign record
      }
      else {
      assign_value = top ir
      push irgen state assign default
      }
    }
    ...
    1, 2, +, s=, a @ e=

    r1 = 1
    r2 = 2
    r3 = r1 + r2
    r4 = load a

    if irgen state assign default {
      if top ir is load {
        change top ir to store
        set store's second param assign_value
      }
    }

    if irgen state assign record {
      assign_value = gen next position access
      irid_assign()
    }
    */
  } break;
  case AstKind_call :
  case AstKind_array :
  case AstKind_add :
  case AstKind_sub :
  case AstKind_not_equal :
  case AstKind_equal :
  case AstKind_less : {
    Irid_Binary binary = irid_get_binary(funid);
    result = irid_new_binary(funid, astid_kind(astid), binary);
  } break;
  case AstKind_load :
  case AstKind_neg :
  case AstKind_pos : {
    Irid unary = irid_gen_unary(funid, astid);
    result = irid_new_unary(funid, astid_kind(astid), unary);
  } break;
  case AstKind_addr_of : {
    Astid of = astid_unary(astid);
    result = irid_addr_of(funid, of);
  } break;
  case AstKind_inr : {
    S64 s64_val = astid_int(astid);
    result = irid_new_int(funid, s64_val);
  } break;
  case AstKind_name : {
    Symid symid = sym_get(ast->name);
    if (symid_is_valid(symid)) {
      if (symid_is_fun(symid)) {
        Funid sym_funid = symid_funid(symid);
        result = irid_new_funref(funid, sym_funid, ast->name);
      }
      else if (symid_is_decl(symid)) {
        result = symid_irid(symid);
      }
      else {
        // NOTE: experiment with variables instead of scalars, which means this is not SSA
        //       this means that instead of bb params for irids, we have bb params for varids
        result = irid_new_load_varid(funid, symid_varid(symid));
      }
    }
    else {
      irgen_error("not found");
    }
  } break;
  case Ast_Kind_if_enter: {
  // cond if_enter statement if_leave
    Irid cond_irid       = irid_top(funid);
    Branchid branchid    = blockid_set_branch(funid, cond_irid);
    Blockid nez_blockid  = blockid_new(funid);
    branchid_nez_link_to_blockid(funid, branchid);
  } break;
  case Ast_Kind_if_leave: {
    Jumpid jumpid        = blockid_set_jump(funid);
    Blockid eqz_blockid  = blockid_new(funid);
    branchid_eqz_link_to_blockid(funid, branchid);
    jumpid_link_to_blockid(funid, jumpid);
  } break;
  case Ast_Kind_if_leave_else_enter: {
  // cond if_enter statement if_leave_else_enter statement else_leave
    Jumpid jumpid_nez   = blockid_set_jump(funid);
    Blockid eqz_blockid = blockid_new(funid);
    branchid_eqz_link_to_blockid(funid, branchid);
  } break;
  case AstKind_else_leave: {
    Jumpid jumpid_eqz   = blockid_set_jump(funid);
    Blockid end_blockid = blockid_new(funid);
    jumpid_link_to_blockid(funid, jumpid_nez);
    jumpid_link_to_blockid(funid, jumpid_eqz);
  } break;
  case AstKind_else_value_leave: {
  // cond if_enter expression if_leave_else expression else_value_leave
    Irid top1_irid      = irid_top1(funid);
    Irid top2_irid      = irid_top2(funid);
    Jumpid jumpid_eqz   = blockid_set_jump(funid);
    Blockid end_blockid = blockid_new(funid);
    jumpid_link_to_blockid(funid, jumpid_nez);
    jumpid_link_to_blockid(funid, jumpid_eqz);
    result = irid_join(top1_irid, top2_irid);
  } break;
  case AstKind_while: {
    Ast_Binary cond_then            = astid_binary(astid);
    Blockid while_entry             = blockid;
    Jumpid jumpid_from_entry        = blockid_set_jump(blockid);
    Blockid while_header            = blockid_new(funid);
    jumpid_link_to_blockid(jumpid);
    Irid cond_irid                  = irid_gen(funid, cond_then.one);
    Branchid branchid               = irid_set_branchid(funid, cond_irid);

    Blockid body_entry              = blockid_new(funid); // not equal zero
    branchid_nez_link_to_blockid(branch_from_header);
                                      blockid_seal(funid, body_entry);
                                      irid_gen(funid, cond_then.two, blockid);
    Jumpid jump_from_body_to_header = blockid_set_jump(body_entry);
    jumpid_link_to_blockid(jump_from_body_to_header);
                                      blockid_seal(while_header);

    Blockid while_exit              = blockid_new(funid); // equal zero
    branchid_eqz_link_to_blockid(branch_from_header);
                                      blockid_seal(while_exit);
  } break;
  case AstKind_field_access : {
    Ast_Binary field_access = astid_binary(astid);
    Irid lhs_irid = irid_gen(funid, field_access.one, blockid);
    if (astid_kind(field_access.two) == AstKind_name) {
      istr name = astid_name(field_access.two)
      if (irid_kind(funid, lhs_irid) == AstKind_load) {
        irid_set_kind(funid, lhs_irid, IrKind_name_offset)
        irid_set_name_offset_name(funid, lhs_irid, name);
      }
      else (irid_kind(funid, lhs_irid) == AstKind_load_var) {
        lhs_irid = irid_new_name_offset(funid, lhs_irid, name);
      }
    }
    else if (astid_kind(field_access.two) == AstKind_int) {
      S64 position = astid_int(field_access.two)
      if (irid_kind(funid, lhs_irid) == AstKind_load) {
        irid_set_kind(funid, lhs_irid, IrKind_position_offset)
        irid_set_position_offset_position(funid, lhs_irid, position);
      }
      else (irid_kind(funid, lhs_irid) == AstKind_load_var) {
        lhs_irid = irid_new_position_offset(funid, lhs_irid, position);
      }
    }
    else {
      irgen_error("field access can only be with a name or a integer");
    }
    result = irid_new_load(funid, lhs_irid);
  } break;
  case AstKind_const_field_access : {
    Irid irid_lhs = irid_gen(funid, field_access.one);
    if (astid_kind(field_access.two) == AstKind_name) {
      istr name = astid_name(field_access.two)
      result = irid_new_const_name_access(funid, irid_lhs, name);
    }
    else {
      irgen_error("constant access can only be with a name or a integer");
    }
  } break;
  }
  return result;
}

Funid funid_from_astid(Astid astid) {
  {
    Funid funid = funid_get(astid);
    if (funid_is_valid(funid)) return funid;
  }
  Funid funid = funid_new(astid);
  Astid param = astid_fun_param(astid);
  Irid arg_irid = irid_new_arg(funid);
  irid_assign(funid, param, funid, arg_irid);
  Irid return_irid = irid_gen(funid, astid, funid);
  fundid_set_return_irid(funid, return_irid);
}

Funid irgen_from_source(Cstr source, Cstr path) {
  Astid ast   = astid_from_source(source, path);
  Funid funid = funid_from_astid(astid);
  return funid;
}

