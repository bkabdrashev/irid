typedef I32 Typeid;
typedef I32 Type_Recordid;
typedef Hash_Set* Recordid_Set;

typedef struct Range Range;
struct Range {
  I64 lo;
  I64 hi;
};

typedef struct Ranges Ranges;
struct Ranges {
  I32 length;
  Range pairs[];
};
typedef Ranges* Intid;

typedef struct Intid_Pair Intid_Pair;
struct Intid_Pair {
  Intid one;
  Intid two;
};

typedef struct Type_Record Type_Record;
struct Type_Record {
  I32      length;
  I32      bit_size;
  I32      bit_align;
  I32*     offsets;
  Typeid*  declared;
  Istr*    names;
  Hash_Map positions;
};

typedef struct Type_Field Type_Field;
struct Type_Field {
  Istr   name;
  I32    position;
  I32    offset;
  Typeid declared;
};

Type_Field type_field_nil = {istr_nil, 0, 0, 0};

typedef struct Type_Records Type_Records;
struct Type_Records {
  Type_Record* base;
  I32          length;
};

typedef enum {
  Mem_Kind_stack,
} Mem_Kind;

typedef struct {
  Mem_Kind kind;
  Range offset_range;
  I32 field_offset;
  I32 field_depth;
  union {
    Varid varid;
  };
} Mem_Cell;

typedef struct {
  // TODO: consider optimization where Mem_Stack, Mem_Global, Mem_Heap, Mem_Unkown, Mem_Null
  //       are used instead Mem_Kind. DOD
  I32 length;
  Mem_Cell cells[];
} Pointer;
typedef Pointer* Ptrid;

typedef enum Type_Kind {
  Type_Kind_none,
  Type_Kind_int,
  Type_Kind_ptr,
  Type_Kind_record,
} Type_Kind;

typedef struct Type Type;
struct Type {
  Type_Kind kind;
  I32 bit_align;
  I32 bit_size;
  union {
    Intid intid;
    Ptrid ptrid;
    Recordid_Set recordid_set;
  };
};

typedef struct Types Types;
struct Types {
  Type* base;
  I32   length;
};

typedef struct Sem_Tasks Sem_Tasks;
struct Sem_Tasks {
  Hash_Map* out_vars;
  Typeid* typeids;
  Varid*  varids;
  I32     length;
};

typedef struct Sem Sem;
struct Sem {
  Arena* temp_arena;
  Arena* perm_arena;
  Funs   funs;
  Dense_Map typeid_of_irids;
  Types types;
  Type_Records records;
  Dense_Map workset;
  Blockids worklist;
};

Typeid typeid_nil = 0;
Sem sem = {};

typedef struct Typeid_Pair Typeid_Pair;
struct Typeid_Pair { Typeid one; Typeid two; };

typedef struct Type_Pair Type_Pair;
struct Type_Pair { Type one; Type two; };

I64 intid_max(Intid intid) {
  return intid->pairs[intid->length-1].hi;
}

B8 intid_have(Intid intid, I64 i64) {
  for (I32 i = 0; i < intid->length; i++) {
    Range pair = intid->pairs[i];
    if (pair.lo >= i64 && i64 <= pair.hi) {
      return true;
    }
  }
  return false;
}

I64 intid_min(Intid intid) {
  return intid->pairs[0].lo;
}

B8 intid_is_single(Intid intid) {
  I64 min = intid_min(intid);
  I64 max = intid_max(intid);
  return min == max;
}

Typeid typeid_of_irid(Irid irid) {
  return dense_map_get(sem.typeid_of_irids, irid);
}

Type type_of_irid(Irid irid) {
  Typeid typeid = typeid_of_irid(irid);
  Type type = get(sem.types, typeid);
  return type;
}

Typeid typeid_of_varid(Blockid blockid, Varid varid) {
  Block* block = blockid_get(blockid);
  Typeid typeid = hash_map_get(&block->out_var_typeids, varid);
  return typeid;
}

void typeid_of_varid_put(Blockid blockid, Varid varid, Typeid typeid) {
  Block* block = blockid_get(blockid);
  hash_map_put(&block->out_var_typeids, varid, typeid);
}

Type type_of_varid(Blockid blockid, Varid varid) {
  Typeid typeid = typeid_of_varid(blockid, varid);
  Type type = get(sem.types, typeid);
  return type;
}

void typeid_of_irid_put(Irid irid, Typeid typeid) {
  dense_map_put(&sem.typeid_of_irids, irid, typeid);
}

I32 typeid_align(Typeid typeid) {
  Type type = get(sem.types, typeid);
  return type.bit_align;
}

I32 typeid_size(Typeid typeid) {
  Type type = get(sem.types, typeid);
  return type.bit_size;
}

Ranges* sem_ranges_init(I32 max_len) {
  Ranges* result = arena_push(sem.temp_arena, sizeof(Ranges) + max_len * sizeof(Range));
  result->length = 0;
  return result;
}

Pointer* sem_pointer_init(I32 max_len) {
  Pointer* result = arena_push(sem.temp_arena, sizeof(Pointer) + max_len * sizeof(Mem_Cell));
  result->length = 0;
  return result;
}

Typeid typeid_ints(Ranges* ranges) {
  Typeid typeid = sem.types.length++;

  I64 min = intid_min(ranges);
  I64 max = intid_max(ranges);
  I32 bit_size = bits_needed(min, max);

  sem.types.base[typeid].kind = Type_Kind_int;
  sem.types.base[typeid].intid = ranges;
  sem.types.base[typeid].bit_align = bit_size;
  sem.types.base[typeid].bit_size = bit_size;
  return typeid;
}

Typeid typeid_int_interval(I64 min, I64 max) {
  struct { I32 length; Range pairs[1]; } pair = { .length = 1, .pairs[0].lo = min, .pairs[0].hi = max };
  Ranges* ranges = (Ranges*)&pair;
  return typeid_ints(ranges);
}

Typeid typeid_int(I64 i64) {
  return typeid_int_interval(i64, i64);
}

Typeid typeid_intid_offset(Intid intid, I64 offset) {
  Ranges* ranges = sem_ranges_init(intid->length);
  ranges->length = intid->length;
  for (I32 i = 0; i < intid->length; i++) {
    Range offset_pair = { .lo = intid->pairs[i].lo + offset, .hi = intid->pairs[i].hi + offset };
    ranges->pairs[i] = offset_pair;
  }

  return typeid_ints(ranges);
}

Typeid typeid_intid_merge(Intid one, Intid two) {
  Ranges* new_ints = sem_ranges_init(one->length + two->length);
  I32 len_one = one->length;
  I32 len_two = two->length;
  I32 o = 0; I32 t = 0;
  Range start;
  if (one->pairs[o].lo < two->pairs[t].lo) start = one->pairs[o++];
  else start = two->pairs[t++];
  while (o < len_one && t < len_two) {
    Range next;
    if (one->pairs[o].lo < two->pairs[t].lo) next = one->pairs[o++];
    else next = two->pairs[t++];

    if (start.hi == I64_MAX || start.hi + 1 >= next.lo) {
      if (start.hi < next.hi) start.hi = next.hi;
    }
    else {
      new_ints->pairs[new_ints->length++] = start;
      start = next;
    }
  }
  if (o < len_one) {
    while (o < len_one && (start.hi == I64_MAX || start.hi + 1 >= one->pairs[o].lo)) {
      if (start.hi < one->pairs[o].hi) {
        start.hi = one->pairs[o].hi;
        o++;
        break;
      }
      o++;
    }
  }
  else {
    while (t < len_two && (start.hi == I64_MAX || start.hi + 1 >= two->pairs[t].lo)) {
      if (start.hi < two->pairs[t].hi) {
        start.hi = two->pairs[t].hi;
        t++;
        break;
      }
      t++;
    }
  }
  new_ints->pairs[new_ints->length++] = start;
  for (; o < len_one; o++) {
    new_ints->pairs[new_ints->length++] = one->pairs[o];
  }
  for (; t < len_two; t++) {
    new_ints->pairs[new_ints->length++] = two->pairs[t];
  }
  return typeid_ints(new_ints);
}

Typeid typeid_ptr(Pointer* ptr) {
  Typeid typeid = sem.types.length++;
  sem.types.base[typeid].kind = Type_Kind_ptr;
  sem.types.base[typeid].ptrid = ptr;
  return typeid;
}

Typeid typeid_ptr_var(Varid varid) {
  Range zero_range = { .lo = 0, .hi = 0 };
  Pointer* pointer = sem_pointer_init(1);
  pointer->length = 1;
  pointer->cells[0].field_depth = 0;
  pointer->cells[0].field_offset = 0;
  pointer->cells[0].offset_range = zero_range;
  pointer->cells[0].kind = Mem_Kind_stack;
  pointer->cells[0].varid = varid;
  return typeid_ptr(pointer);
}

I32 type_recordid_length(Type_Recordid recordid) {
  Type_Record record = get(sem.records, recordid);
  return record.length;
}

Type_Field type_recordid_get_by_position(Type_Recordid recordid, I32 position) {
  Type_Record record = get(sem.records, recordid);
  Type_Field field = {};
  field.name  = record.names[position];
  field.declared = record.declared[position];
  field.offset   = record.offsets[position];
  field.position = position;
  return field;
}

Type_Field type_recordid_get_by_name(Type_Recordid recordid, Istr name) {
  Type_Record record = get(sem.records, recordid);
  I32 position = hash_map_get(&record.positions, name);
  return type_recordid_get_by_position(recordid, position);
}

Type_Field type_recordid_get_by_offset(Type_Recordid recordid, I32 offset) {
  Type_Record record = get(sem.records, recordid);
  for (I32 i = 1; i < record.length; i++) {
    if (offset < record.offsets[i]) {
      return type_recordid_get_by_position(recordid, i-1);
    }
  }
  return type_recordid_get_by_position(recordid, record.length-1);
}

Typeid typeid_recordid_set(Hash_Set* recordid_set) {
  Type type = {};
  Typeid typeid = sem.types.length++;
  type.kind = Type_Kind_record;
  type.recordid_set = recordid_set;
  Type_Recordid type_recordid = recordid_set->list[0];
  Type_Record type_record = get(sem.records, type_recordid);
  type.bit_align = type_record.bit_align;
  type.bit_size  = type_record.bit_size;
  for (I32 j = 1; j < recordid_set->len; j++) {
    Type_Recordid type_recordid = recordid_set->list[j];
    Type_Record type_record = get(sem.records, type_recordid);
    type.bit_align = max(type.bit_align, type_record.bit_align);
    type.bit_size  = max(type.bit_size, type_record.bit_size);
  }
  sem.types.base[typeid] = type;
  return typeid;
}

Typeid typeid_type_recordid(Type_Recordid type_recordid) {
  return typeid_recordid_set(NULL);
}

Typeid typeid_record(Recordid recordid) {
  Record record = recordid_get(recordid);
  Type_Record type_record = {};
  type_record.length = record.length;
  C8* mark = arena_mark(sem.temp_arena);
  type_record.offsets  = arena_push(sem.perm_arena, sizeof(I32)    * record.length);
  type_record.names = record.names;
  type_record.positions = record.positions;

  // I32 offset = 0;
  // for (I32 i = 0; i < record.length; i++) {
  //   Typeid typeid = typeid_of_irid(record.assigned[i]);
  //   I32 align = typeid_align(typeid);
  //   offset = align_up(offset, align);
  //   type_record.bit_align = max(type_record.bit_align, align);
  //   type_record.offsets[i]  = offset;
  //   offset += typeid_size(typeid);
  // }
  // type_record.bit_size = align_up(offset, type_record.bit_align);
  // return typeid_type_recordid();
}

Typeid typeid_join(Typeid one, Typeid two) {
  Type type_one = get(sem.types, one);
  Type type_two = get(sem.types, two);
  Typeid result = typeid_nil;
  if (type_one.kind == Type_Kind_none) {
    result = two;
  }
  else if (type_two.kind == Type_Kind_none) {
    result = one;
  }
  else if (type_one.kind == Type_Kind_int && type_two.kind == Type_Kind_int) {
    result = typeid_intid_merge(type_one.intid, type_two.intid);
  }
  else if (type_one.kind == Type_Kind_record && type_two.kind == Type_Kind_record) {
    assert(0);
  }
  else {
    assert(0);
  }
  return result;
}

B8 typeid_not_equal(Typeid one, Typeid two) {
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

Typeid typeid_of_irid_unary(Irid irid) {
  Irid irid_one = irid_unary(irid);
  Typeid result = typeid_of_irid(irid_one);
  return result;
}

Type type_of_irid_unary(Irid irid) {
  Typeid typeid = typeid_of_irid_unary(irid);
  Type result = get(sem.types, typeid);
  return result;
}

Intid_Pair intid_pair_of_irid_binary(Irid irid) {
  Typeid_Pair typeids = typeid_of_irid_binary(irid);
  Intid_Pair result = {};
  result.one = get(sem.types, typeids.one).intid;
  result.two = get(sem.types, typeids.two).intid;
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

Typeid typeid_intid_intersection(Intid one, Intid two) {
  Ranges* new_ranges = sem_ranges_init(max(one->length, two->length));
  I32 o = 0; I32 t = 0;
  while (o < one->length && t < two->length) {
    I64 lo = max(one->pairs[o].lo, two->pairs[t].lo);
    I64 hi = min(one->pairs[o].hi, two->pairs[t].hi);
    if (lo <= hi) {
      Range pair = { .lo = lo, .hi = hi };
      new_ranges->pairs[new_ranges->length++] = pair;
    }
    if (one->pairs[o].hi < two->pairs[t].hi) {
      o++;
    }
    else {
      t++;
    }
  }
  return typeid_ints(new_ranges);
}

Typeid typeid_intid_no_intersection(Intid one, Intid two) {
  Ranges* new_ranges = sem_ranges_init(max(one->length, two->length));
  I32 o = 0; I32 t = 0;
  Range range1 = one->pairs[o];
  Range range2 = two->pairs[t];
  while (o < one->length && t < two->length) {
    if (range1.hi < range2.lo) {
      // [1,2] [...    | [...
      //       [3, 4]  | [3, 4]
      new_ranges->pairs[new_ranges->length++] = range1;
      range1 = one->pairs[o++];
    }
    else if (range1.lo > range2.hi) {
      //       [3, 4] [.... | [3 4]
      // [1,2] [...         | [...
      new_ranges->pairs[new_ranges->length++] = range2;
      range2 = two->pairs[t++];
    }
    else if (range1.lo == range2.lo && range1.hi == range2.hi) {
      range1 = one->pairs[o++];
      range2 = two->pairs[t++];
    }
    else if (range1.lo == range2.lo && range1.hi > range2.hi) {
      // [1     8] [... | [4  8]
      // [1 3] [...     |    [...
      range1.lo = range2.hi+1;
      range2 = two->pairs[t++];
    }
    else if (range1.lo == range2.lo && range1.hi < range2.hi) {
      // [1 3] [...     |    [...
      // [1     8] [... | [4  8]
      range2.lo = range1.hi+1;
      range1 = one->pairs[o++];
    }
    else if (range1.lo < range2.lo && range1.hi == range2.hi) {
      // [1   3] [...  | [...
      //   [2 3] [...  | [...
      Range range = { .lo = range1.lo, .hi = range2.lo - 1 };
      new_ranges->pairs[new_ranges->length++] = range;
      range1 = one->pairs[o++];
      range2 = two->pairs[t++];
    }
    else if (range1.lo > range2.lo && range1.hi == range2.hi) {
      //   [2 3] [... | [...
      // [1   3] [... | [...
      Range range = { .lo = range2.lo, .hi = range1.lo - 1 };
      new_ranges->pairs[new_ranges->length++] = range;
      range1 = one->pairs[o++];
      range2 = two->pairs[t++];
    }
    else if (range1.lo < range2.lo && range1.hi < range2.hi) {
      // [1...5] [...  |    [...
      //    [3..8]     | [6..8]
      Range range = { .lo = range1.lo, .hi = range2.lo - 1 };
      new_ranges->pairs[new_ranges->length++] = range;
      range2.lo = range1.hi+1;
      range1 = one->pairs[o++];
    }
    else if (range1.lo > range2.lo && range1.hi > range2.hi) {
      //    [3..8]     | [6..8]
      // [1...5] [...  |    [...
      Range range = { .lo = range2.lo, .hi = range1.lo - 1 };
      new_ranges->pairs[new_ranges->length++] = range;
      range1.lo = range2.hi+1;
      range2 = two->pairs[t++];
    }
    else if (range1.lo < range2.lo && range1.hi > range2.hi) {
      // [1         8]  | [4..8]
      //    [2 3] [...  |   [...
      Range range = { .lo = range1.lo, .hi = range2.lo - 1 };
      new_ranges->pairs[new_ranges->length++] = range;
      range1.lo = range2.hi+1;
      range2 = two->pairs[t++];
    }
    else if (range1.lo > range2.lo && range1.hi < range2.hi) {
      //    [2 3] [...  |   [...
      // [1         8]  | [4..8]
      Range range = { .lo = range2.lo, .hi = range1.lo - 1 };
      new_ranges->pairs[new_ranges->length++] = range;
      range2.lo = range1.hi+1;
      range1 = one->pairs[o++];
    }
  }
  while (o < one->length) {
    new_ranges->pairs[new_ranges->length++] = range1;
    range1 = one->pairs[o++];
  }
  while (t < two->length) {
    new_ranges->pairs[new_ranges->length++] = range2;
    range2 = two->pairs[t++];
  }
  return typeid_ints(new_ranges);
}

Typeid typeid_intid_exclude(Intid intid, I64 val) {
  Ranges* new_ints = sem_ranges_init(intid->length);
  for (I32 i = 0; i < intid->length; i++) {
    if (intid->pairs[i].lo <= val && val <= intid->pairs[i].hi) {
      if (intid->pairs[i].lo != val || val != intid->pairs[i].hi) {
        if (intid->pairs[i].lo == val) {
          Range pair = { .lo = intid->pairs[i].lo+1, .hi = intid->pairs[i].hi };
          new_ints->pairs[new_ints->length++] = pair;
        }
        else if (val == intid->pairs[i].hi) {
          Range pair = { .lo = intid->pairs[i].lo, .hi = intid->pairs[i].hi-1 };
          new_ints->pairs[new_ints->length++] = pair;
        }
        else {
          Range pair1 = { .lo = intid->pairs[i].lo, .hi = val-1 };
          new_ints->pairs[new_ints->length++] = pair1;
          Range pair2 = { .lo = val+1, .hi = intid->pairs[i].hi };
          new_ints->pairs[new_ints->length++] = pair2;
        }
      }
      i++;
      for (; i < intid->length; i++) {
        new_ints->pairs[new_ints->length++] = intid->pairs[i];
      }
      break;
    }
    else {
      new_ints->pairs[new_ints->length++] = intid->pairs[i];
    }
  }
  return typeid_ints(new_ints);
}

Type_Field recordid_set_get_by_offset(Recordid_Set recordid_set, I32 offset) {
  Type_Field result = type_recordid_get_by_offset(recordid_set->list[0], offset);
  for (I32 i = 1; i < recordid_set->len; i++) {
    Type_Field field = type_recordid_get_by_offset(recordid_set->list[i], offset);
    result.declared = typeid_join(result.declared, field.declared);
  }
  return result;
}

I32 recordid_set_get_offset_by_name(Recordid_Set recordid_set, Istr name) {
  Type_Recordid type_recordid = recordid_set->list[0];
  I32 offset = type_recordid_get_by_name(type_recordid, name).offset;
  for (I32 i = 1; i < recordid_set->len; i++) {
    Type_Recordid type_recordid = recordid_set->list[i];
    Type_Field field = type_recordid_get_by_name(type_recordid, name);
    if (offset != field.offset) {
      return -1;
    }
  }
  return offset;
}

void sem_typeid_of_irid_narrow(Sem_Tasks* tasks, Irid irid, Typeid new_typeid);
void sem_narrow_record(Sem_Tasks* tasks, Name_Offset name_offset, Typeid new_typeid_of_field) {
  Typeid old_typeid_of_record = typeid_of_irid(name_offset.of);
  assert(typeid_kind_equal(old_typeid_of_record, Type_Kind_record));
}

void sem_tasks_push(Sem_Tasks* tasks, Irid irid, Typeid old_typeid) {
}

void sem_tasks_push_var(Sem_Tasks* tasks, Varid varid, Typeid old) {
  tasks->varids[tasks->length] = varid;
  tasks->typeids[tasks->length] = old;
  tasks->length++;
}

void sem_typeid_of_irid_narrow(Sem_Tasks* tasks, Irid irid, Typeid new_typeid) {
  if (irid_kind_equal(irid, Ir_Kind_load)) {
    assert(0);
    Istr istr = irid_istr(irid);
    Typeid old_typeid = hash_map_get(tasks->out_vars, istr);
    if (hash_map_change_if_exists(tasks->out_vars, istr, new_typeid)) {
      sem_tasks_push_var(tasks, istr, old_typeid);
    }
  }
  else if (irid_kind_equal(irid, Ir_Kind_name_offset)) {
    Name_Offset offset = irid_name_offset(irid);
    sem_narrow_record(tasks, offset, new_typeid);
  }
}

void sem_typeid_of_irid_binary_narrow(Sem_Tasks* tasks, Irid irid, Typeid new_typeid_one, Typeid new_typeid_two) {
  Irid_Pair binary = irid_binary(irid);
  sem_typeid_of_irid_narrow(tasks, binary.one, new_typeid_one);
  sem_typeid_of_irid_narrow(tasks, binary.two, new_typeid_two);
}

void sem_typeid_narrow_int_eq(Sem_Tasks* tasks, Irid irid) {
  Intid_Pair pair = intid_pair_of_irid_binary(irid);
  Typeid new_typeid = typeid_intid_intersection(pair.one, pair.two);
  sem_typeid_of_irid_binary_narrow(tasks, irid, new_typeid, new_typeid);
}

void sem_typeid_narrow_int_ne(Sem_Tasks* tasks, Irid irid) {
  Irid_Pair binary = irid_binary(irid);
  Type_Pair pair = type_of_irid_binary(irid);
  if (intid_is_single(pair.one.intid)) {
    I64 val = intid_min(pair.one.intid);
    Typeid new_typeid_two = typeid_intid_exclude(pair.two.intid, val);
    sem_typeid_of_irid_narrow(tasks, binary.two, new_typeid_two);
  }
  if (intid_is_single(pair.two.intid)) {
    I64 val = intid_min(pair.two.intid);
    Typeid new_typeid_one = typeid_intid_exclude(pair.one.intid, val);
    sem_typeid_of_irid_narrow(tasks, binary.one, new_typeid_one);
  }
}

Typeid typeid_narrow_eqz(Typeid typeid) {
  Type type = get(sem.types, typeid);
  switch (type.kind) {
  case Type_Kind_int: {
    B8 have_zero = intid_have(type.intid, 0);
    if (have_zero) {
      return typeid_int(0);
    }
    else {
      return typeid_nil;
    }
  } break;
  default: break;
  }
  return typeid;
}

Typeid typeid_narrow_nez(Typeid typeid) {
  Type type = get(sem.types, typeid);
  switch (type.kind) {
  case Type_Kind_int: {
    Intid intid = type.intid;
    return typeid_intid_exclude(type.intid, 0);
  } break;
  default: break;
  }
  return typeid;
}

void sem_narrow_nez(Sem_Tasks* tasks, Blockid blockid) {
  Irid condition = blockid_branch(blockid).cond;
  switch (irid_kind(condition)) {
  case Ir_Kind_eq: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      sem_typeid_narrow_int_eq(tasks, condition);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_ne: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      assert(0);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_lt: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      assert(0);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_le: assert(0);
  case Ir_Kind_gt: assert(0);
  case Ir_Kind_ge: assert(0);
  case Ir_Kind_load: {
    assert(0);
    Istr istr = irid_istr(condition);
    Typeid old_typeid = hash_map_get(tasks->out_vars, istr);
    Typeid new_typeid = typeid_narrow_nez(old_typeid);
    if (hash_map_change_if_exists(tasks->out_vars, istr, new_typeid)) {
      sem_tasks_push_var(tasks, istr, old_typeid);
    }
  } break;
  default: {
    Typeid old_typeid = typeid_of_irid(condition);
    Typeid new_typeid = typeid_narrow_eqz(old_typeid);
    typeid_of_irid_put(condition, new_typeid);
    sem_tasks_push(tasks, condition, old_typeid);
  } break;
  }
}

void sem_narrow_eqz(Sem_Tasks* tasks, Blockid blockid) {
  Irid condition = blockid_branch(blockid).cond;
  switch (irid_kind(condition)) {
  case Ir_Kind_eq: {
    if (typeid_kind_of_irid_binary_operands_equal(condition, Type_Kind_int)) {
      sem_typeid_narrow_int_ne(tasks, condition);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_ne: {
    assert(0);
  } break;
  case Ir_Kind_lt: {
    assert(0);
  } break;
  case Ir_Kind_le: assert(0); // log_todo("Ir_Kind_le narrow nez");
  case Ir_Kind_gt: assert(0); // log_todo("Ir_Kind_gt narrow nez");
  case Ir_Kind_ge: assert(0); // log_todo("Ir_Kind_ge narrow nez");
  case Ir_Kind_load: {
    assert(0);
    Istr istr = irid_istr(condition);
    Typeid old = hash_map_get(tasks->out_vars, istr);
    Typeid new = typeid_narrow_eqz(old);
    if (hash_map_change_if_exists(tasks->out_vars, istr, new)) {
      sem_tasks_push_var(tasks, istr, old);
    }
  } break;
  default: {
    Typeid old_typeid = typeid_of_irid(condition);
    Typeid new_typeid = typeid_narrow_eqz(old_typeid);
    typeid_of_irid_put(condition, new_typeid);
    sem_tasks_push(tasks, condition, old_typeid);
  } break;
  }
}

void sem_worklist_push(Blockid blockid) {
  B8 exist = dense_map_get(sem.workset, blockid);
  if (!exist) {
    dense_map_put(&sem.workset, blockid, true);
    add(sem.worklist, blockid);
  }
}

B8 sem_worklist_is_not_empty() {
  return !empty(sem.worklist);
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
  Typeid result = typeid_of_irid(irid);
  Ir_Kind kind = irid_kind(irid);
  switch (kind) {
  case Ir_Kind_int: {
    I64 i64 = irid_int(irid);
    result  = typeid_int(i64);
  } break;
  case Ir_Kind_var: {
    Istr istr = irid_istr(irid);
    result = typeid_ptr_var(istr);
  } break;
  case Ir_Kind_join: {
    Typeid_Pair pair = typeid_of_irid_binary(irid);
    result = typeid_join(pair.one, pair.two);
  } break;
  case Ir_Kind_add: {
    if (typeid_kind_of_irid_binary_operands_equal(irid, Type_Kind_int)) {
      // TODO: overflow/underflow
      Intid_Pair pair = intid_pair_of_irid_binary(irid);
      I64 max_one = intid_max(pair.one);
      I64 max_two = intid_max(pair.two);
      I64 max = max_one + max_two;
      I64 min_one = intid_min(pair.one);
      I64 min_two = intid_min(pair.two);
      I64 min = min_one + min_two;
      result = typeid_int_interval(min, max);
    }
  } break;
  case Ir_Kind_eq: {
    if (typeid_kind_of_irid_binary_operands_equal(irid, Type_Kind_int)) {
      // TODO: special cases
      result = typeid_int_interval(0, 1);
    }
  } break;
  case Ir_Kind_record: {
    Recordid recordid = irid_recordid(irid);
    result = typeid_record(recordid);
  } break;
  case Ir_Kind_name_offset: {
    Name_Offset name_offset = irid_name_offset(irid);
    Ir_Kind ir_kind = irid_kind(name_offset.of);
    Type of_type = type_of_irid(name_offset.of);
    if (ir_kind == Ir_Kind_load) {
      Recordid_Set recordid_set = of_type.recordid_set;
      I32 offset = recordid_set_get_offset_by_name(recordid_set, name_offset.at);
      Ptrid ptrid = type_of_irid_unary(name_offset.of).ptrid;
      assert(ptrid->length == 1); // TODO: non-trival cases
      assert(ptrid->cells[0].kind == Mem_Kind_stack);
      Pointer* ptr = sem_pointer_init(ptrid->length);
      ptr->length = ptrid->length;
      for (I32 i = 0; i < ptrid->length; i++) {
        Mem_Cell cell = ptrid->cells[i];
        ptr->cells[i].offset_range = cell.offset_range;
        ptr->cells[i].field_depth = cell.field_depth + 1;
        ptr->cells[i].field_offset = cell.field_offset + offset;
        ptr->cells[i].kind = cell.kind;
        ptr->cells[i].varid = cell.varid; // TODO: other mem kinds
      }
      result = typeid_ptr(ptr);
    }
  } break;
  case Ir_Kind_load: {
    Irid one = irid_unary(irid);
    Type one_type = type_of_irid(one);
    if (one_type.kind == Type_Kind_ptr) {
      Ptrid ptrid = one_type.ptrid;
      for (I32 i = 0; i < ptrid->length; i++) {
        Mem_Cell cell = ptrid->cells[i];
        switch (cell.kind) {
        case Mem_Kind_stack: {
          Typeid var_typeid = typeid_of_varid(blockid, cell.varid);
          I32 field_offset = cell.field_offset;
          for (I32 d = 0; d < cell.field_depth; d++) {
            Type var_type = get(sem.types, var_typeid);
            assert(var_type.kind == Type_Kind_record);
            Type_Field field = recordid_set_get_by_offset(var_type.recordid_set, field_offset);
            field_offset -= field.offset;
            var_typeid = field.declared;
          }
          result = var_typeid;
        } break;
        default: assert(0);
        }
      }
    }
    else {
    }
  } break;
  case Ir_Kind_store: {
    Irid_Pair binary = irid_binary(irid);
    Type   lhs = type_of_irid(binary.one);
    Typeid rhs = typeid_of_irid(binary.two);
    if (lhs.kind == Type_Kind_ptr) {
      Ptrid ptrid = lhs.ptrid;
      for (I32 i = 0; i < ptrid->length; i++) {
        Mem_Cell cell = ptrid->cells[i];
        switch (cell.kind) {
        case Mem_Kind_stack: {
            // TODO: Have to recreate whole record with slightly updated field, which
            //       seems wasteful. Lazy update maybe better option.
          typeid_of_varid_put(blockid, cell.varid, rhs);
          result = rhs;
        } break;
        default: assert(0);
        }
      }
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_ptr: {
    Irid one = irid_unary(irid);
    Ir_Kind one_kind = irid_kind(one);
    if (one_kind == Ir_Kind_load) {
      Irid addr = irid_unary(one);
      result = typeid_of_irid(addr);
    }
  } break;
  default: assert(0);
  }
  typeid_of_irid_put(irid, result);
}

void sem_unnarrow(Sem_Tasks tasks) {
  for (I32 i = 0; i < tasks.length; i++) {
    Istr   var = tasks.varids[i];
    Typeid old = tasks.typeids[i];
    hash_map_put(tasks.out_vars, var, old);
  }
}

void sem_block(Blockid blockid) {
  Block* block = blockid_get(blockid);
  for (Irid i = block->entryid; i < block->leaveid; i++) {
    sem_ir(blockid, i);
  }

  if (block->kind == Block_Kind_branch) {
    Sem_Tasks tasks = {};
    tasks.out_vars = &blockid_get(blockid)->out_var_typeids;
    tasks.length = 0;
    tasks.varids = arena_push(sem.temp_arena, sizeof(Varid) * tasks.out_vars->len);
    tasks.typeids = arena_push(sem.temp_arena, sizeof(Typeid) * tasks.out_vars->len);
    { // condition is not equal to zero branch
      sem_narrow_nez(&tasks, blockid);
      Jump jump = blockid_branch(blockid).nez;
      sem_jump(blockid, jump.blockid);
      sem_unnarrow(tasks);
      tasks.length = 0;
    }
    { // condition is equal to zero branch
      sem_narrow_eqz(&tasks, blockid);
      Jump jump = blockid_branch(blockid).eqz;
      sem_jump(blockid, jump.blockid);
      sem_unnarrow(tasks);
      tasks.length = 0;
    }
  }
  else if (block->kind == Block_Kind_jump) {
    Jump jump = blockid_jump(blockid);
    sem_jump(blockid, jump.blockid);
  }
}

Typeid sem_funid(Funid funid) {
  Fun* fun = funid_get(funid);
  for (Blockid b = fun->entryid; b < fun->leaveid; b++) {
    Block* block = blockid_get(b);
    block->out_var_typeids = hash_map_init(sem.perm_arena, fun->var_count);
    block->in_var_typeids  = hash_map_init(sem.perm_arena, fun->var_count);
    sem_worklist_push(b);
  }
  // init block with typeids

  while (sem_worklist_is_not_empty()) {
    Blockid blockid = sem_worklist_pop();
    sem_block(blockid);
  }

  Irid funid_return = fun->returnid;
  return typeid_of_irid(funid_return);
}

void sem_funs(Arena* arena, Funs funs) {
  Arena temp = arena_init(arena->capacity);
  sem.perm_arena = arena;
  sem.temp_arena = &temp;
  sem.funs = funs;
  sem.worklist.base = arena_push(arena, sizeof(Blockid)*irgen.blocks.length);
  sem.worklist.length = 0;
  sem.workset         = dense_map_init(arena, sizeof(Blockid)*irgen.blocks.length);
  sem.typeid_of_irids = dense_map_init(arena, sizeof(Typeid)*irgen.irs.length);

  sem.records.base = arena_push(arena, sizeof(Type_Record)*irgen.irs.length);
  sem.records.length = 1;

  sem.types.base = arena_push(arena, sizeof(Type)*irgen.irs.length);
  sem.types.base[0].kind = Type_Kind_none;
  sem.types.length = 1;

  for (I32 f = 0; f < funs.length; f++) {
    sem_funid(f);
  }
  arena_free(&temp);
}

void string_builder_push_type(String_Builder* sb, Typeid typeid) {
  Type type = get(sem.types, typeid);
  switch (type.kind) {
  case Type_Kind_none:
    string_builder_push_cstr(sb, "<none>");
  break;
  case Type_Kind_int: {
    for (I32 i = 0; i < type.intid->length; i++ ) {
      I64 one = type.intid->pairs[i].lo;
      I64 two = type.intid->pairs[i].hi;
      if (one == two) {
        string_builder_push_i64(sb, one);
      }
      else {
        string_builder_push_i64(sb, one);
        string_builder_push_cstr(sb, "..");
        string_builder_push_i64(sb, two);
      }
      if (i+1 < type.intid->length) {
        string_builder_push_cstr(sb, "\\");
      }
    }
  } break;
  case Type_Kind_ptr: {
    for (I32 i = 0; i < type.intid->length; i++) {
      Mem_Cell cell = type.ptrid->cells[i];
      switch (cell.kind) {
      case Mem_Kind_stack: {
        string_builder_push_cstr(sb, "@");
        string_builder_push_istr(sb, cell.varid);
      } break;
      default:
        string_builder_push_cstr(sb, "<ptr deadbeef>");
      break;
      }
      if (cell.field_offset) {
        string_builder_push_cstr(sb, "+");
        string_builder_push_i64(sb, cell.field_offset);
      }
      if (i+1 < type.intid->length) {
        string_builder_push_cstr(sb, "\\");
      }
    }
  } break;
  case Type_Kind_record: {
    Hash_Set* recordid_set = type.recordid_set;
    for (I32 i = 0; i < recordid_set->len; i++) {
      Type_Recordid key = recordid_set->list[i];
      string_builder_push_cstr(sb, "record");
      string_builder_push_cstr(sb, "(");
      for (I32 pos = 0; pos < type_recordid_length(key); pos++) {
        Type_Field field = type_recordid_get_by_position(key, pos);
        if (field.name) {
          string_builder_push_istr(sb, field.name);
          string_builder_push_cstr(sb, "'");
          string_builder_push_i64(sb, field.offset);
          if (field.declared != irgen.irid_nil) {
            string_builder_push_cstr(sb, ":");
            string_builder_push_type(sb, field.declared);
          }
        }
        else {
          string_builder_push_type(sb, field.declared);
        }
        if (pos+1 < type_recordid_length(key)) {
          string_builder_push_cstr(sb, ", ");
        }
      }
      string_builder_push_cstr(sb, ")");
      if (i+1 < recordid_set->len) {
        string_builder_push_cstr(sb, "\\");
      }
    }
  } break;
  }
}

Cstr cstr_from_sem(Funs funs, C8* buffer) {
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

      string_builder_push_cstr(&sb, " in{");
      for (I32 i = 0; i < block.in_var_typeids.len; i++) {
        Istr istr = block.in_var_typeids.list[i];
        Typeid typeid = hash_map_get(&block.in_var_typeids, istr);
        string_builder_push_istr(&sb, istr);
        string_builder_push_cstr(&sb, ": ");
        string_builder_push_type(&sb, typeid);
        if (i+1 < block.in_var_typeids.len) {
          string_builder_push_cstr(&sb, "; ");
        }
      }
      string_builder_push_cstr(&sb, "}");

      string_builder_push_cstr(&sb, " out{");
      for (I32 i = 0; i < block.out_var_typeids.len; i++) {
        Istr istr = block.out_var_typeids.list[i];
        Typeid typeid = hash_map_get(&block.out_var_typeids, istr);
        string_builder_push_istr(&sb, istr);
        string_builder_push_cstr(&sb, ": ");

        string_builder_push_type(&sb, typeid);
        if (i+1 < block.out_var_typeids.len) {
          string_builder_push_cstr(&sb, "; ");
        }
      }
      string_builder_push_cstr(&sb, "}");
      for (Irid irid = block.entryid; irid < block.leaveid; irid++) {
        Ir ir = irgen.irs.base[irid];
        string_builder_push_ir(&sb, irid, ir);
        string_builder_push_cstr(&sb, " : ");
        Typeid typeid = dense_map_get(sem.typeid_of_irids, irid);
        string_builder_push_type(&sb, typeid);
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

void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Umi source_length    = strlen(source) + 2;
  Arena arena          = arena_init(KB(64) * source_length);
  Ast ast              = ast_from_source(&arena, source);
  Funs funs            = ir_ast(&arena, ast);
                         sem_funs(&arena, funs);
  C8* buffer           = arena_push(&arena, 32 * source_length);
  Cstr result          = cstr_from_sem(funs, buffer);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_sem(source, expected, __FILE__, __LINE__)

void sem_test(void) {
/*
TODO:
Consider lazy types
  r = (x=1; y=3)
  if ... do
    r.x = 2
    r.y = 4
  r // (x=1; y=3)\(x=2; y=4) -- not (x=1\2)
///////////////////
  r = (x=1; y=3)
  if ... do r = (x=2; y=4)
  r // (x=1; y=3)\(x=2; y=4) -- not (x=1\2)


  a = 1
  b = 2
  if ... do
    a = 3
    b = 4
  if a == 3 do
    b // 4

main {
  .0:
    r1 = int 1
    r2 = var a
    r3 = store r2 r1
    r4 = int 2
    r5 = var b
    r6 = store r5 r4
    r7 = int 0
    if r7 then .1 else .2
  .1: in{a:1; b:2} out{a:3; b:4}
    r8 = int 3
    r9 = var a
    r10 = store r9 r8
    r11 = int 4
    r12 = var b
    r13 = store r12 r11
    jump .2
  .2: in{(a:1; b:2) \ (a:3;b:4)}
    r14 = var a    // @a
    r15 = load r14 // 1\3
    r16 = int 3
    r17 = eq r15 r16 // a == 3
    if r17 then .3 else .4
  .3: in{a:3; b:4}
    r18 = var b
    r19 = load r18
    jump .4
  .4:
    jump .5
  .5:
  ret r1
}
*/
  // test("a=(x=1; y=3); b=@a; b@.x = 2", "");
}

#undef test
