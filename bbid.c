Typeid_Tasks branchid_narrow_nez(Branchid branchid) {
  Typeid_Tasks typeid_tasks = {0};
  IRid condition = branchid_condition(branchid);
  switch (irid_tag(condition)) {
  case IRTag_eq: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_eq(condition);
    }
    else {
      log_todo("IRTag_eq narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case IRTag_ne: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_ne(condition);
    }
    else {
      log_todo("IRTag_ne narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case IRTag_lt: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_lt(condition);
    }
    else {
      log_todo("IRTag_lt narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case IRTag_le: log_todo("IRTag_le narrow nez");
  case IRTag_gt: log_todo("IRTag_gt narrow nez");
  case IRTag_ge: log_todo("IRTag_ge narrow nez");
  default: {
    Typeid old_typeid = typeid_of_irid(condition);
    Typeid new_typeid = typeid_of_irid_narrow_nez(old_typeid);
    typeid_of_irid_set(irid, new_typeid);
    typeid_tasks_push(typeid_tasks, irid, old_typeid);
  } break;
  }

  return typeid_tasks;
}

Typeid_Tasks branchid_narrow_eqz(Branchid branchid) {
  Typeid_Tasks typeid_tasks = {0};
  IRid condition = branchid_condition(branchid);
  switch (ir_tag(condition)) {
  case IRTag_eq: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_ne(condition);
    }
    else {
      log_todo("IRTag_eq narrow eqz between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case IRTag_ne: {
    if (typeid_tag_of_irid_binary_operand_equal(condition, TypeTag_int)) {
      typeid_tasks = typeid_narrow_int_eq(condition);
    }
    else {
      log_todo("IRTag_ne narrow eqz between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case IRTag_lt: log_todo("IRTag_lt narrow eqz"); break;
  case IRTag_le: log_todo("IRTag_le narrow eqz"); break;
  case IRTag_gt: log_todo("IRTag_gt narrow eqz"); break;
  case IRTag_ge: log_todo("IRTag_ge narrow eqz"); break;
  default: {
    Typeid old_typeid = typeid_of_irid(condition);
    Typeid new_typeid = typeid_of_irid_narrow_eqz(old_typeid);
    typeid_of_irid_set(irid, new_typeid);
    typeid_tasks_push(typeid_tasks, irid, old_typeid);
  } break;
  }

  return typeid_tasks;
}
