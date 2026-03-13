void sem_jumpid(Funid funid, BBid from_bbid, Jumpid jumpid) {
  BBid to_bbid = jumpid_to_bbid(jumpid);
  Slice_IRid jump_irargs = jumpid_irarg_slice(jumpid);
  b32 is_jump_updates_bb_param_typeid = sem_bbid_is_not_visited(to_bbid);
  For_Slice_IRid (jump_irarg, jump_irargs) {
    IRid bb_irparam = bbid_paramid(jump_irarg);
    Typeid jump_irarg_typeid     = typeid_of_irid(jump_irarg);
    Typeid old_bb_irparam_typeid = typeid_of_irid(bb_irparam);
    Typeid new_bb_irparam_typeid = typeid_join(jump_irarg_typeid, old_bb_irparam_typeid);
    if (typeid_not_equal(new_bb_irparam_typeid, old_bb_irparam_tyid)) {
      log_info("%s is updated. old: %s, new: %s\n", cstr_from_irid(bb_irparam), cstr_from_typeid(old_bb_irparam_typeid), cstr_from_typeid(new_bb_irparam_typeid));
      typeid_of_irid_set(bb_irparam, new_bb_irparam_typeid);
      is_jump_updates_typeid_of_bb_param = true;
    }
  }
  // TODO other memory locations
  b32 is_jump_updates_typeid_of_invar = false;
  Slice_Varid varids = bbid_varid_slice(from_bbid);
  For_Slice_Varid (varid, varids) {
    Typeid from_bb_varid_typeid = typeid_of_bb_varid(from_bbid, varid);
    Typeid old_bb_invar_typeid  = typeid_of_bb_invar(bbid, varid);
    Typeid new_bb_invar_typeid  = typeid_join(from_bb_varid_typeid, old_bb_invar_typeid);

    if (typeid_not_equal(new_bb_invar_typeid, old_bb_invar_typeid)) {
      log_info("%s is updated. old: %s, new: %s\n", cstr_from_varid(varid), cstr_from_typeid(old_bb_invar_typeid), cstr_from_typeid(new_bb_invar_typeid));
      typeid_of_bb_invar_set(bbid, varid, new_bb_irparam_typeid);
      typeid_of_bb_varid_set(bbid, varid, new_bb_irparam_typeid);
      is_jump_updates_typeid_of_invar = true;
    }
  }
  if (is_jump_updates_typeid_of_bb_param || is_jump_updates_typeid_of_memloc) {
    sem_worklist_push(funid, jump_to_bbid);
  }
}

void sem_irid(BBid bbid, IRid irid) {
  log_info("start  typecheck irid: %s", cstr_from_irid(irid));
  log_increment_tab();

  Typeid result = typeid_of_irid(irid);

  switch (irid_tag(irid)) {
    case IRTag_nop: {
    } break;
    case IRTag_str: {
      Str str = irid_str(irid);
      result = typeid_str(str);
    } break;
    case IRTag_int: {
      Int int = irid_int(irid);
      result  = typeid_int(int);
    } break;
    case IRTag_join: {
      Pair_Typeid pair_typeid = typeid_of_irid_binary(irid);
      result = typeid_join(pair_typeid.one, pair_typeid.two);
    } break;
    case IRTag_meet: {
      Pair_Typeid pair_typeid = typeid_of_irid_binary(irid);
      result = typeid_meet(pair_typeid.one, pair_typeid.two);
    } break;
    case IRTag_add: {
      if (typeid_tag_of_irid_binary_operand_equal(irid, TypeTag_int)) {
        Pair_Typeid pair_typeid = typeid_of_irid_binary(irid);
        if (intid_is_only_zero(pair_typeid.one.intid)) {
          result = pair_typeid.two;
        }
        else if (intid_is_only_zero(pair_typeid.one.intid)) {
          result = pair_typeid.one;
        }
        else if (sccid_is_binary_inside_loop(irid)) {
          result = typeid_int_from_range(S64_MIN, S64_MAX);
        }
        else {
          result = typeid_int_sub(pair_typeid.one.intid, pair_typeid.two.intid);
        }
      }
      else {
        log_todo("sem irid IRTag_add for %s", cstr_from_ir(irid));
      }
    } break;
    case IRTag_sub: {
      if (typeid_tag_of_irid_binary_operand_equal(irid, TypeTag_int)) {
        Pair_Intid pair_typeid = typeid_of_irid_binary(irid);
        if (intid_is_only_zero(pair_typeid.one.intid)) {
          result = typeid_int_negate(pair_typeid.two.intid);
        }
        else if (intid_is_only_zero(pair_typeid.intid)) {
          result = pair_typeid.one;
        }
        else if (sccid_is_binary_inside_loop(irid)) {
          result = typeid_int_from_range(S64_MIN, S64_MAX);
        }
        else {
          result = typeid_int_sub(pair_typeid.one.intid, pair_typeid.two.intid);
        }
      }
      else {
        log_todo("sem irid IRTag_sub for %s", cstr_from_ir(irid));
      }
    } break;
    case IRTag_eq: {
      if (typeid_tag_of_irid_binary_operand_equal(irid, TypeTag_int)) {
        Pair_Intid pair = intid_of_typeid_of_irid_binary(irid);
        if (intid_pair_always_equal(pair)) {
          result = typeid_int(1);
        }
        else if (intid_pair_always_not_equal(pair)) {
          result = typeid_int(0);
        }
        else {
          result = typeid_int_from_range(0, 1);
        }
      }
      else {
        log_todo("sem irid IRTag_eq for %s", cstr_from_ir(irid));
      }
    } break;
    case IRTag_ne: {
      if (typeid_tag_of_irid_binary_operand_equal(irid, TypeTag_int)) {
        Pair_Intid pair = intid_of_typeid_of_irid_binary(irid);
        if (intid_pair_always_not_equal(pair)) {
          result = typeid_int(1);
        }
        else if (intid_pair_always_equal(pair)) {
          result = typeid_int(0);
        }
        else {
          result = typeid_int_from_range(0, 1);
        }
      }
      else {
        log_todo("sem irid IRTag_ne for %s", cstr_from_ir(irid));
      }
    } break;
    case IRTag_neg: {
      if (typeid_tag_of_irid_unary_operand_equal(irid, TypeTag_int)) {
        Intid intid = typeid_of_irid(irid).intid;
        result = typeid_int_negate(intid);
      }
      else {
        log_todo("sem irid IRTag_neg for %s", cstr_from_ir(irid));
      }
    } break;
    case IRTag_store: {
      Pair_Typeid pair_typeid = typeid_of_irid_binary(irid);
      if (typeid_tag_equal(pair_typeid.one, TypeTag_ptr)) {
        Ptrid ptrid = pair_typeid.one.ptrid;
        Set_Memloc set_memloc = ptrid_get_set_of_memlocs(ptrid);
        For_Set_Memloc (memloc, set_memloc) {
          Typeid typeid = typeid_of_memloc(memloc);
          typeid_of_memloc_set(memloc, pair_typeid.two);
        }
      }
      else {
        sem_error("Should be a pointer");
      }
    } break;
    case IRTag_store_var: {
      IR_Store_Var store_var = irid_store_var(irid);
      Typeid typeid = typeid_of_irid(store_var.irid);
      typeid_of_bb_varid_set(bbid, reg->store_var.varir, tyid);
    } break;
    case IRTag_load: {
      Typeid typeid = typeid_of_irid_unary(irid);
      if (typeid_tag_equal(typeid, TypeTag_ptr)) {
        Typeid ptr_to_typeid = typeid_of_ptrid_to(bbid, typeid.ptrid);
        result = ptr_to_typeid;
      }
      else {
        sem_error("Should be a pointer");
      }
    } break;
    case IRTag_load_var: {
      IR_Load_Var load_var = irid_load_var(irid);
      result = typeid_of_bb_varid(bbid, load_var.varid);
    } break;
    case IRTag_addr_of: {
      IR_Addr_Var addr_var = irid_addr_var(irid);
      Typeid typeid_scalar = typeid_of_irid(addr_var.irid);
      Typeid old_typeid = typeid_of_bb_varid(bbid, addr_var.varid);
      Typeid new_typeid = typeid_join(old_typeid, new_typeid);
      typeid_of_bb_varid_set(bbid, addr_var.varid, new_typeid);
      result = typeid_ptr_from_varid(addr_var.varid);
    } break;
    case IRTag_call: {
      Pair_Typeid pair_typeid = typeid_of_irid_binary(irid);
      if (typeid_tag_equal(pair_typeid.one, TypeTag_fun)) {
        log_todo("sem irid IRTag_call function for %s", cstr_from_ir(irid));
      }
      else if (typeid_pair_tag_equal(pair_typeid, TypeTag_record)) {
        Set_Record initial_records  = recordsid_get_set_records(pair_typeid.one.recordsid);
        Set_Record changing_records = recordsid_get_set_records(pair_typeid.two.recordsid);
        if (set_record_length(initial_records) < set_record_length(initial_records)) {
          sem_error("Number of record options is less than is needed");
        }
        else if (set_record_length(initial_records) == 1) {
          Set_Field initial_fields  = set_record_get_first(initial_records);
          Set_Field changing_fields = set_record_get_first(changing_records);
          Set_Field new_fields      = set_field_init(set_field_length(fields));
          s32 position = 0;
          For_Set_Field (change, changing_fields) {
            if (str_is_valid(change.name)) {
              Field field = set_field_get_by_name(initial_fields, change.name);
              if (set_field_is_valid(field)) {
                if (typeid_is_subtype(change.typeid, field.typeid)) {
                  change.position = field.position;
                  position = field.position + 1;
                  set_field_put(new_fields, change);
                }
                else {
                  sem_error("not subtype");
                }
              }
              else {
                sem_error("no such field");
              }
            }
            else {
              Field field = set_field_get_by_position(initial_fields, position);
              if (set_field_is_valid(field)) {
                if (typeid_is_subtype(change.typeid, field.typeid)) {
                  change.position = field.position;
                  position = field.position + 1;
                  set_field_put(new_fields, change);
                }
                else {
                  sem_error("not subtype");
                }
              }
              else {
                sem_error("no such field");
              }
            }
          }
          result = typeid_record(new_fields);
        }
        else {
          log_todo("sem irid IRTag_call many records copy for %s", cstr_from_irid(irid));
        }
      }
    } break;
    case IRTag_record: {
      IR_Record ir_record  = irid_record(irid);
      Set_Field new_fields = set_field_init(ir_record.fields.length);
      s32 position = 0; s32 offset = 0;
      For_Slice_IR_Field (field, ir_record.fields) {
        Typeid typeid = typeid_of_irid(field.irid);
        offset = typeid_align_up(typeid, offset);
        Field field = {.name = field.name, .position=position, .typeid=typeid, .offset=offset};
        set_field_put(new_fields, field);
        position++;
        offset += typeid_size(typeid);
      }
      log_todo("sem irid IRTag_record #constants");
      result = typeid_record(fields);
    } break;
    case IRTag_const_field_access: {
      IR_Field_Access field_access = irid_field_access(irid);
      Typeid typeid = typeid_of_regid(field_access.record);
      if (typeid_tag_equal(typeid, TypeTag_record)) {
        Set_Recordid recordids = recordsid_get_set_records(pair_typeid.one.recordsid);
        For_Set_Recordid (recordid, recordids) { 
          Set_Field fields = recordid_get_set_field(recordid);
          Field field = set_field_get_by_name(fields, field_access.name);
          if (set_field_is_valid(field)) {
            result = typeid_join(result, field.typeid);
          }
          else {
            sem_error("no such constant");
          }
        }
      }
      else {
        log_todo("sem irid IRTag_const_field_access");
      }
    } break;
    case IRTag_name_offset: {
      IR_Name_Access access = irid_name_access(irid);
      Typeid typeid = typeid_of_irid(access.record);
      assert(typeid.tag == TypeTag_ptr);
      Typeid typeid_ptr = typeid_of_ptrid_to(bbid, typeid.ptrid);
      if (typeid_tag_equal(typeid_ptr, TypeTag_record)) {
        Set_Recordid recordids = recordsid_get_set_records(pair_typeid.one.recordsid);
        s32 field_offset = set_recordid_is_all_offsets_same(recordids);
        if (field_offset >= 0) {
          Field field = set_recordid_get_by_name(fields, access.name);
          if (!set_field_is_valid(field)) {
            sem_error("no such field");
          }
        }
        result = ptrid_field_offset(typeid.ptrid, field_offset);
      }
      else {
        log_todo("sem irid IRTag_field_offset");
      }
    } break;
    case IRTag_position_offset: {
      IR_Position_Access access = irid_position_access(irid);
      Typeid typeid = typeid_of_irid(access.record);
      assert(typeid.tag == TypeTag_ptr);
      Typeid typeid_ptr = typeid_of_ptrid_to(bbid, typeid.ptrid);
      if (typeid_tag_equal(typeid_ptr, TypeTag_record)) {
        Set_Recordid recordids = recordsid_get_set_records(pair_typeid.one.recordsid);
        s32 field_offset = set_recordid_is_all_offsets_same(recordids);
        if (field_offset >= 0) {
          Field field = set_recordid_get_by_position(fields, access.position);
          if (!set_field_is_valid(field)) {
            sem_error("no such field");
          }
        }
        result = ptrid_field_offset(typeid.ptrid, field_offset);
      }
      else {
        log_todo("sem irid IRTag_field_offset");
      }
    } break;
  }
    
  log_decrement_tab();
  log_info("finish typecheck irid: %s", cstr_from_irid(irid));

  return result;
}

void sem_bbid(Funid funid, BBid bbid) {
  log_info("start  typecheck bbid: %s", cstr_from_bbid(bbid));
  log_increment_tab();

  Slice_IRid irids = bbid_get_irid_slice(bbid);

  For_Slice_IRid (irids, irid) {
    sem_irid(bbid, irid);
  }

  if (bbid_is_branch(bbid)) {
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

  log_decrement_tab();
  log_info("finish typecheck bbid: %s", cstr_from_bbid(bbid));
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

cstr cstr_from_sem() {
  String_Builder sb = str_builder_begin();
  Slice_Funid funids = irgen_get_funids();
  For_Slice_Funid (funid, funids) {
    Fun fun = funid_get(funid);
    sb_push_cstr(sb, "@");
    if (fun.name) {
      sb_push_str(sb, fun.name);
    }
    else {
      sb_push_u64(sb, funid);
    }
    sb_push_cstr(sb, " {\n");
    sb_push_sem_bbs(sb, f.bbs);
    sb_push_cstr(sb, "}\n");
  }
  cstr str = cstr_from_sb(sb);
  str_builder_end(sb);
  return str;
}

b32 _test_sem(cstr expected, cstr file_name, s32 line, cstr source) {
  Funid funid = irgen_from_source(source, cstr_from_source_info(file_name, line));

  typeid_init_typeid_of_irid(irgen.irid_count, irgen.bb_paramids_count);
  sem_funid(funid);
  b32 result = test_at_source(cstr_from_sem(), expected, file_name, line, source);
  return result;
}

#define test(source, expected) _test_sem(expected, __FILE__, __LINE__, source)
#undef test
