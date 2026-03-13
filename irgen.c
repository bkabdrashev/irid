IRid irid_new_record(Funid funid, Slice_IR_Field fields) {
  IR ir = { IRTag_record , .record = { .fields = fields }};
  IRid id = irid_new_ir(funid);
  return id;
}

Jumpid irid_cond_then(Funid funid, Astid_Binary cond_then, BBid* bbid) {
}

void irid_assign(Funid funid, Astid astid, IRid irid) {
  switch (astid_tag(astid)) {
  case AstTag_name : {
    irid_new_store_var(funid, varid, regid);
  } break;
  case AstTag_record : {
    Astid_Record astid_record = astid_record(astid);
    for (s32 i = 0; i < slice_ast_field_length(astid_record.fields); i++) {
      Ast_Field field = slice_ast_field_at(astid_record.fields, i);
      IRid at = irid_new_binary(funid, IRTag_position_access, irid, i);
      if (field.name) {
        irid_assign(funid, field.name, at);
        IRid type = irid_gen(funid, field.astid);
        irid_new_binary(funid, IRTag_type_assert, at, tp);
      }
      else {
        irid_assign(funid, field.astid, at);
      }
    }
  } break;
  default : {
    IRid lhs_irid = irid_gen(funid, astid);
    if (irid_tag(funid, lhs_irid) == IRTag_load) {
      irid_set_tag(funid, lhs_irid, IRTag_store)
      irid_set_binary_two(funid, lhs_irid, irid);
    }
  } break;
  }
}

IRid irid_gen(Funid funid, Astid astid) {
  IRid result = IRidNone;
  switch(astid_tag(astid)) {
  case AstTag_record : {
    Ast_Record   astid_record = astid_record(astid);
    Slice_IR_Field irid_fields  = slice_ir_field_init(astid_record.fields.length);
    For_Slice_Ast_Field (ast_field, astid_record.fields) {
      IRid irid = irid_gen(funid, ast_field);
      IR_Field field = { .name = ast_field.name, .irid = irid };
      slice_ir_field_push(&irid_fields, field);
    }
    result = irid_new_record(funid, irid_fields);
  } break;
  case AstTag_fun : {
    Funid funid = funid_from_astid(astid);
    result = irid_new_funref(funid);
  } break;
  case AstTag_call : {
    IRid_Binary pair = irid_gen_binary(funid, astid);
    result = irid_new_binary(funid, IRTag_call, pair);
  } break;
  case AstTag_array : {
    IRid_Binary pair = irid_gen_binary(funid, astid);
    result = irid_new_binary(funid, IRTag_array, pair);
  } break;
  case AstTag_return : {
    IRid irid = irid_gen_unary(funid, astid);
    irid_put_return(funid, irid);
    Jumpid jumpid = bbid_set_jump(bbid);
    jumpid_add_arg(jumpid, irid);
    irid_put_pending_jumpid(funid, jumpid);
    bbid_new(funid);
  } break;
  case AstTag_block_value : {
    irid_scope_enter(funid);
    irid_push_return_list(funid);
    irid_push_jumps(funid);
    Ast_Block block = irid_block(astid);
    For_Slice_Astid (astid, block.list) {
      irid_gen(funid, astid);
    }
    result = irid_pop_return_list(funid);
    irid_scope_leave(funid);
  } break;
  case AstTag_block : {
    irid_scope_enter(funid);
    Ast_Block block = irid_block(funid, astid);
    For_Slice_Astid (astid, block.list) {
      irid_gen(funid, astid);
    }
    irid_scope_leave(funid);
  } break;
  case AstTag_assign : {
    Ast_Binary assign = astid_binary(astid);
    IRid rhs_irid     = irid_gen(funid, assign.two);
    irid_assign(funid, assign.one, rhs_irid);
  } break;
  case AstTag_add :
  case AstTag_sub :
  case AstTag_not_equal :
  case AstTag_equal :
  case AstTag_less : {
    IRid_Binary binary = irid_gen_binary(funid, astid);
    result = irid_new_binary(funid, astid_tag(astid), binary);
  } break;
  case AstTag_load :
  case AstTag_neg :
  case AstTag_pos : {
    IRid unary = irid_gen_unary(funid, astid);
    result = irid_new_unary(funid, astid_tag(astid), unary);
  } break;
  case AstTag_addr_of : {
    Astid of = astid_unary(astid);
    result = irid_addr_of(funid, of);
  } break;
  case AstTag_inr : {
    s64 s64_val = astid_int(astid);
    result = irid_new_int(funid, s64_val);
  } break;
  case AstTag_name : {
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
  case AstTag_if: {
    Ast_Binary cond_then = astid_binary(astid);
    IRid cond_irid     = irid_gen(funid, cond_then.one);
    Branchid branchid  = bbid_set_branch(funid, cond_irid);
    BBid nez_bbid      = bbid_new(funid);
    branchid_nez_link_to_bbid(funid, branchid);
                         bbid_seal(funid, nez_bbid);
                         irid_gen(funid, cond_then.two, funid);
    Jumpid jumpid      = bbid_set_jump(funid);
    BBid eqz_bbid      = bbid_new(funid);
    branchid_eqz_link_to_bbid(funid, branchid);
    jumpid_link_to_bbid(funid, jumpid);
    bbid_seal(funid, bbid);
  } break;
  case AstTag_else: {
    Ast_Binary else_cond_then = astid_binary(astid);
    Ast_Binary cond_then      = astid_binary(binary.one);

    IRid cond_irid     = irid_gen(funid, cond_then.one);
    Branchid branchid  = bbid_set_branch(funid, cond_irid);
    BBid nez_bbid      = bbid_new(funid);
    branchid_nez_link_to_bbid(funid, branchid);
                         bbid_seal(funid, nez_bbid);
                         irid_gen(funid, cond_then.two, funid);
    Jumpid jumpid_nez  = bbid_set_jump(funid);
    BBid eqz_bbid      = bbid_new(funid);
    branchid_eqz_link_to_bbid(funid, branchid);
                         bbid_seal(funid, eqz_bbid);
                         irid_gen(funid, else_cond_then.one, funid);

    Jumpid jumpid_eqz  = bbid_set_jump(funid);
    BBid end_bbid      = bbid_new(funid);
    jumpid_link_to_bbid(funid, jumpid_nez);
    jumpid_link_to_bbid(funid, jumpid_eqz);
    bbid_seal(end_bb);
  } break;
  case AstTag_while: {
    Ast_Binary cond_then   = astid_binary(astid);
    BBid while_entry         = bbid;
    Jumpid jumpid_from_entry = bbid_set_jump(bbid);
    BBid while_header        = bbid_new(funid);
    jumpid_link_to_bbid(jumpid);
    IRid cond_irid           = irid_gen(funid, cond_then.one);
    Branchid branchid        = irid_set_branchid(funid, cond_irid);

    BBid body_entry          = bbid_new(funid); // not equal zero
    branchid_nez_link_to_bbid(branch_from_header);
                               bbid_seal(funid, body_entry);
                               irid_gen(funid, cond_then.two, bbid);
    Jumpid jump_from_body_to_header = bbid_set_jump(body_entry);
    jumpid_link_to_bbid(jump_from_body_to_header);
                               bbid_seal(while_header);

    BBid while_exit          = bbid_new(funid); // equal zero
    branchid_eqz_link_to_bbid(branch_from_header);
                               bbid_seal(while_exit);
  } break;
  case AstTag_field_access : {
    Ast_Binary field_access = astid_binary(astid);
    IRid lhs_irid = irid_gen(funid, field_access.one, bbid);
    if (astid_tag(field_access.two) == AstTag_name) {
      istr name = astid_name(field_access.two)
      if (irid_tag(funid, lhs_irid) == AstTag_load) {
        irid_set_tag(funid, lhs_irid, IRTag_name_offset)
        irid_set_name_offset_name(funid, lhs_irid, name);
      }
      else (irid_tag(funid, lhs_irid) == AstTag_load_var) {
        lhs_irid = irid_new_name_offset(funid, lhs_irid, name);
      }
    }
    else if (astid_tag(field_access.two) == AstTag_int) {
      s64 position = astid_int(field_access.two)
      if (irid_tag(funid, lhs_irid) == AstTag_load) {
        irid_set_tag(funid, lhs_irid, IRTag_position_offset)
        irid_set_position_offset_position(funid, lhs_irid, position);
      }
      else (irid_tag(funid, lhs_irid) == AstTag_load_var) {
        lhs_irid = irid_new_position_offset(funid, lhs_irid, position);
      }
    }
    else {
      irgen_error("field access can only be with a name or a integer");
    }
    result = irid_new_load(funid, lhs_irid);
  } break;
  case AstTag_const_field_access : {
    IRid irid_lhs = irid_gen(funid, field_access.one);
    if (astid_tag(field_access.two) == AstTag_name) {
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
  IRid arg_irid = irid_new_arg(funid);
  irid_assign(funid, param, funid, arg_irid);
  IRid return_irid = irid_gen(funid, astid, funid);
  fundid_set_return_irid(funid, return_irid);
}

Funid irgen_from_source(cstr source, cstr path) {
  Astid ast   = astid_from_source(source, path);
  Funid funid = funid_from_astid(astid);
  return funid;
}

