void typeid_of_irid_narrow(Typeid_Tasks* typeid_tasks, IRid irid, Typeid new_typeid);
void typeid_narrow_record(Typeid_Tasks* typeid_tasks, IR_FieldAccess field_access, Typeid new_typeid_of_field) {
  Typeid old_typeid_of_record = typeid_of_irid(field_access.record);
  assert(typeid_tag_equal(old_typeid_of_record, TypeTag_record));
  Recordsid recordsid = old_typeid_of_record.recordsid;
  Set_Record records = recordsid_get_set_records(recordsid);
  Set_Record new_records = {0};
  For_Set_Record (fields, records) {
    Set_Field new_fields = set_field_init(set_field_length(fields));
    b32 is_meet_not_empty = false;
    For_Set_Field (field, fields) {
      if (field.name == field_access.name) {
        Typeid meet = typeid_meet(new_typeid_of_field, field.typeid);
        is_meet_not_empty = typeid_is_empty(meet);
        b32 is_meet_empty = !is_meet_not_empty;
        if (is_meet_empty) break;
        Field new_field = { .name = field.name, .typeid = meet, .offset=field.offset };
        set_field_put(new_fields, new_field);
      }
      else {
        set_field_put(new_fields, field);
      }
    }
    if (is_meet_not_empty) {
      set_record_put(new_records, new_fields);
    }
  }
  Typeid new_typeid_of_record = typeid_record(new_records);
  Typeid old_typeid_of_record = typeid_of_irid(field_access);
  typeid_of_irid_narrow(typeid_tasks, field_access.record, old_typeid_of_record);
}

void typeid_of_irid_narrow(Typeid_Tasks* typeid_tasks, IRid irid, Typeid new_typeid) {
  Typeid old_typeid = typeid_of_irid(irid);
  typeid_tasks_push(typeid_tasks, irid, old_typeid);
  typeid_of_irid_set(irid, new_typeid);
  if (irid_tag_equal(irid, IRTag_FieldAccess)) {
    typeid_narrow_record(typeid_tasks, irid_field_access(irid), new_typeid);
  }
}

void typeid_of_irid_binary_narrow(Typeid_Tasks* typeid_tasks, IRid irid, Typeid new_typeid_one, Typeid new_typeid_two) {
  IR_binary binary = irid_binary(irid);
  typeid_of_irid_narrow(&typeid_tasks, binary.one, new_typeid);
  typeid_of_irid_narrow(&typeid_tasks, binary.two, new_typeid);
}

b32 typeid_tag_of_irid_binary_operand_equal(IRid irid, TypeTag type_tag) {
  Pair_Typeid pair = typeid_of_irid_binary(irid);
  return pair.tag == type_tag && pair.tag == type_tag;
}

Typeid_Tasks typeid_narrow_int_eq(IRid irid) {
  Pair_Typeid pair = typeid_of_irid_binary(irid);

  Typeid_Tasks typeid_tasks = {0};
  Typeid new_typeid  = typeid_int_intersection(pair.one.intid, pair.two.intid);

  typeid_of_irid_binary_narrow(&typeid_tasks, irid, new_typeid, new_typeid);

  return typeid_tasks;
}

Typeid_Tasks typeid_narrow_int_ne(IRid irid) {
  Pair_Typeid pair = typeid_of_irid_binary(irid);

  Typeid_Tasks typeid_tasks = {0};
  Typeid new_typeid = typeid_int_no_intersection(pair.one.intid, pair.two.intid);

  typeid_of_irid_binary_narrow(&typeid_tasks, irid, new_typeid, new_typeid);

  return typeid_tasks;
}

Typeid_Tasks typeid_narrow_int_lt(IRid irid) {
  Pair_Typeid pair = typeid_of_irid_binary(irid);

  Typeid_Tasks typeid_tasks = {0};
  Typeid new_typeid_one = typeid_int_less(pair.one.intid, pair.two.intid);
  Typeid new_typeid_two = typeid_int_greater_or_equal(pair.two.intid, pair.one.intid);

  typeid_of_irid_binary_narrow(&typeid_tasks, irid, new_typeid_one, new_typeid_two);

  return typeid_tasks;
}

Typeid typeid_of_irid_narrow_nez(Typeid old_typeid) {
  Typeid new_typeid = old_typeid;
  switch (typeid_tag(old_typeid)) {
  case TypeTag_int: {
    Intervals old_intervals = intid_intervals(old_typeid.intid);
    Intervals new_intervals = {0};
    for (s32 i = 0; i < old_intervals.slice.length; i++) {
      Interval interval = slice_at(old_intervals.slice, i);
      // TODO: this doesn't make sense
      if (interval.one <= 0 && 0 <= interval.two) {
        if (interval.one == 0 && 0 != interval.two) {
          intervals_push(new_intervals, interval.one+1, interval.two);
        }
        else if (interval.one != 0 && 0 == interval.two) {
          intervals_push(new_intervals, interval.one, interval.two-1);
        }
        i++;
        for (; i < old_intervals.slice.length; i++) {
          Interval interval = slice_at(old_intervals.slice, i);
          intervals_push(new_intervals, interval.one, interval.two);
        }
        break;
      }
      else {
        intervals_push(new_intervals, interval.one, interval.two);
      }
    }
    new_typeid = sem_int_type_from_intervals(new_intervals);
  } break;
  default: log_todo("narrow nez %s", istr_from_typeid(old_typeid)); break;
  }
  return new_typeid;
}

Typeid typeid_of_irid_narrow_eqz(Typeid old_typeid) {
  Typeid new_typeid = old_typeid;
  switch (typeid_tag(old_typeid)) {
  case TypeTag_int: {
    b32 have_zero = intid_is_have(old_typeid.intid, 0);
    if (have_zero) {
      new_typeid = sem_type_int(0);
    }
    else {
      new_typeid = TyidNone;
    }
  } break;
  default: log_todo("narrow eqz %s", istr_from_typeid(old_typeid)); break;
  }
  return new_typeid;
}
