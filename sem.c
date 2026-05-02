typedef I32 Typeid;

typedef enum Type_Kind {
  Type_Kind_none,
  Type_Kind_int,
  Type_Kind_record,
} Type_Kind;

typedef struct Type Type;
struct Type {
  Type_Kind kind;
  union {
    I32 intid;
  };
};

typedef struct Types Types;
struct Types {
  Type* base;
  I32   length;
};

typedef struct Sem_Tasks Sem_Tasks;
struct Sem_Tasks {
  Typeid* typeids;
  Istr*   vars;
  I32     length;
};

typedef struct Sem Sem;
struct Sem {
  Dense_Map typeid_of_irid;
  Types types;
  Dense_Map workset;
  Blockids worklist;
};

Sem sem = {};

typedef struct Typeid_Pair Typeid_Pair;
struct Typeid_Pair { Typeid one; Typeid two; };

typedef struct Type_Pair Type_Pair;
struct Type_Pair { Type one; Type two; };

Typeid typeid_of_irid(Irid irid) {
  return get(sem.typeid_of_irid, irid);
}

void typeid_of_irid_put(Irid irid, Typeid typeid) {
  put(sem.typeid_of_irid, irid, typeid);
}

Typeid typeid_join(Typeid one, Typeid two) {
  assert(0);
  return one;
}

B8 typeid_not_equal(Typeid one, Typeid two) {
  assert(0);
  return one != two;
}

B8 typeid_equal(Typeid one, Typeid two) {
  assert(0);
  return one == two;
}

B8 typeid_kind_equal(Typeid one, Typeid two) {
  Type type_one = get(sem.types, one);
  Type type_two = get(sem.types, two);
  return type_one.kind == type_two.kind;
}

Typeid_Pair typeid_of_irid_binary(Irid irid) {
  Irid_Pair pair = irid_binary(irid);
  Typeid one = typeid_of_irid(pair.one);
  Typeid two = typeid_of_irid(pair.two);
  Typeid_Pair result = { one, two };
  return result;
}

Type_Pair type_of_irid_binary(Irid irid) {
  Typeid_Pair typeids = typeid_of_irid_binary(irid);
  Type_Pair result = {};
  result.one = get(sem.types, typeids.one);
  result.two = get(sem.types, typeids.two);
  return result;
}

Type_Kind typeid_kind(Typeid typeid) {
  Type type = get(sem.types, typeid);
  return type.kind;
}

B8 typeid_kind_of_irid_binary_operands_equal(Irid irid, Type_Kind type_kind) {
  Typeid_Pair pair = typeid_of_irid_binary(irid);
  return typeid_kind(pair.one) == type_kind && typeid_kind(pair.two) == type_kind;
}

Typeid typeid_int_intersection(Typeid one, Typeid two) {
  // Type type_one = get(sem.types, one);
  // Type type_two = get(sem.types, two);
  assert(0);
  return one;
}

void typeid_of_irid_narrow(Sem_Tasks* tasks, Irid irid, Typeid new_typeid);
void typeid_narrow_record(Sem_Tasks* tasks, Name_Offset name_offset, Typeid new_typeid_of_field) {
  Typeid old_typeid_of_record = typeid_of_irid(name_offset.of);
  assert(typeid_kind_equal(old_typeid_of_record, Type_Kind_record));
  // Recordsid recordsid = old_typeid_of_record.recordsid;
  // Set_Record records = recordsid_get_set_records(recordsid);
  // Set_Record new_records = {};
  // For_Set_Record (fields, records) {
  //   Set_Field new_fields = set_field_init(set_field_length(fields));
  //   b32 is_meet_not_empty = false;
  //   For_Set_Field (field, fields) {
  //     if (field.name == field_access.name) {
  //       Typeid meet = typeid_meet(new_typeid_of_field, field.typeid);
  //       is_meet_not_empty = typeid_is_empty(meet);
  //       b32 is_meet_empty = !is_meet_not_empty;
  //       if (is_meet_empty) break;
  //       Field new_field = { .name = field.name, .typeid = meet, .offset=field.offset };
  //       set_field_put(new_fields, new_field);
  //     }
  //     else {
  //       set_field_put(new_fields, field);
  //     }
  //   }
  //   if (is_meet_not_empty) {
  //     set_record_put(new_records, new_fields);
  //   }
  // }
  // Typeid new_typeid_of_record = typeid_record(new_records);
  // Typeid old_typeid_of_record = typeid_of_irid(name_offset);
  // typeid_of_irid_narrow(typeid_tasks, name_offset.record, old_typeid_of_record);
}

void typeid_tasks_push(Sem_Tasks* tasks, Irid irid, Typeid old_typeid) {
}

void typeid_of_irid_narrow(Sem_Tasks* tasks, Irid irid, Typeid new_typeid) {
  Typeid old_typeid = typeid_of_irid(irid);
  typeid_tasks_push(tasks, irid, old_typeid);
  typeid_of_irid_put(irid, new_typeid);
  if (irid_kind_equal(irid, Ir_Kind_name_offset)) {
    Name_Offset offset = irid_name_offset(irid);
    typeid_narrow_record(tasks, offset, new_typeid);
  }
}



void typeid_of_irid_binary_narrow(Sem_Tasks* tasks, Irid irid, Typeid new_typeid_one, Typeid new_typeid_two) {
  Irid_Pair binary = irid_binary(irid);
  typeid_of_irid_narrow(tasks, binary.one, new_typeid_one);
  typeid_of_irid_narrow(tasks, binary.two, new_typeid_two);
}

Sem_Tasks typeid_narrow_int_eq(Irid irid) {
  Type_Pair pair = type_of_irid_binary(irid);

  Sem_Tasks tasks = {};
  Typeid new_typeid  = typeid_int_intersection(pair.one.intid, pair.two.intid);

  typeid_of_irid_binary_narrow(&tasks, irid, new_typeid, new_typeid);

  return tasks;
}

Sem_Tasks sem_narrow_nez(Blockid blockid) {
  Sem_Tasks tasks = {};
  Irid condition = blockid_branch(blockid).cond;
  switch (irid_kind(condition)) {
  case Ir_Kind_eq: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      tasks = typeid_narrow_int_eq(condition);
    }
    else {
      assert(0);
      // log_todo("Ir_Kind_eq narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_ne: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      assert(0);
      // tasks = typeid_narrow_int_ne(condition);
    }
    else {
      assert(0);
      // log_todo("Ir_Kind_ne narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_lt: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      assert(0);
      // tasks = typeid_narrow_int_lt(condition);
    }
    else {
      assert(0);
      // log_todo("Ir_Kind_lt narrow nez between %s, %s", istr_from_typeid(typeid_one), istr_form_typeid(typeid_two));
    }
  } break;
  case Ir_Kind_le: assert(0); // log_todo("Ir_Kind_le narrow nez");
  case Ir_Kind_gt: assert(0); // log_todo("Ir_Kind_gt narrow nez");
  case Ir_Kind_ge: assert(0); // log_todo("Ir_Kind_ge narrow nez");
  default: {
    assert(0);
    // Typeid old_typeid = typeid_of_irid(condition);
    // Typeid new_typeid = typeid_of_irid_narrow_nez(old_typeid);
    // typeid_of_irid_set(irid, new_typeid);
    // typeid_tasks_push(typeid_tasks, irid, old_typeid);
  } break;
  }

  return tasks;
}

void sem_worklist_push(Blockid blockid) {
  B8 exist = dense_map_get(&sem.workset, blockid);
  if (!exist) {
    dense_map_put(&sem.workset, blockid, true);
    add(sem.worklist, blockid);
  }
}

B8 sem_worklist_is_not_empty() {
  return empty(sem.worklist);
}

Blockid sem_worklist_pop() {
  Blockid blockid = pop(sem.worklist);
  dense_map_put(&sem.workset, blockid, false);
  return blockid;
}

void sem_jump(Blockid from_blockid, Blockid to_blockid) {
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
  assert(0);
}

void sem_unnarrow(Sem_Tasks tasks) {
  assert(0);
}

void sem_block(Blockid blockid) {
  Block* block = blockid_get(blockid);
  for (Irid i = block->entryid; i <= block->leaveid; i++) {
    sem_ir(blockid, i);
  }

  if (block->kind == Block_Kind_branch) {
    { // condition is not equal to zero branch
      Sem_Tasks undo_narrow = sem_narrow_nez(blockid);
      Jump jump = blockid_branch(blockid).nez;
      sem_jump(blockid, jump.blockid);
      sem_unnarrow(undo_narrow);
    }
    { // condition is equal to zero branch
      // Typeid_Tasks undo_narrow = branchid_narrow_eqz(branchid);
      // Jumpid jumpid = jumpid_of_bbid_eqz(blockid);
      // sem_jumpid(funid, jumpid);
      // typeid_unnarrow(undo_narrow);
    }
  }
  else if (block->kind == Block_Kind_jump) {
    Jump jump = blockid_jump(blockid);
    sem_jump(blockid, jump.blockid);
  }
}

Typeid sem_funid(Funid funid) {
  Fun* fun = funid_get(funid);
  sem_worklist_push(fun->entryid);
  assert(0);
  // init block with typeids

  while (sem_worklist_is_not_empty()) {
    Blockid blockid = sem_worklist_pop();
    sem_block(blockid);
  }

  Irid funid_return = fun->returnid;
  return typeid_of_irid(funid_return);
}

Cstr cstr_from_sem(Funs funs, C8* buffer) {
  return "todo";
}

void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source);
                         sem_funid(0);
  C8* buffer           = arena_push(&arena, 32 * source_length);
  Cstr result          = cstr_from_sem(funs, buffer);
  test_at_source(result, expected, file_name, line, source);
  arena_deinit(&arena);
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

  test("1 + 2", "");
}

#undef test

