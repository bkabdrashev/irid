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

typedef struct Ranges_Pair Ranges_Pair;
struct Ranges_Pair { Ranges* one; Ranges* two; };

typedef struct Pointer Pointer;
struct Pointer {
  Hash_Set stack;
  Type* declared;
};

typedef struct Pointer_Pair Pointer_Pair;
struct Pointer_Pair { Pointer* one; Pointer* two; };

typedef enum Type_Kind {
  Type_Kind_none,
  Type_Kind_int,
  Type_Kind_ptr,
  Type_Kind_record,
  Type_Kind_bits,
} Type_Kind;

struct Type {
  Type_Kind kind;
  I16 bits_align;
  I16 bits_size;
  union {
    Ranges*  ranges;
    Pointer* pointer;
    Record*  record;
    Type*    bits_of;
  };
};

typedef struct Type_Pool Type_Pool;
struct Type_Pool {
  Type* base;
  I32   length;
};

typedef struct Types Types;
struct Types {
  Type** base;
  I32    length;
};

typedef struct Sem_Tasks Sem_Tasks;
struct Sem_Tasks {
  Hash_Map* out_vars;
  Type**  types;
  Var**   vars;
  I32     length;
};

typedef struct Sem Sem;
struct Sem {
  Arena* temp_arena;
  Arena* perm_arena;
  Funs   funs;
  Hash_Map  type_of_irs;
  Type_Pool types;

  Type* type_none;
  Blocks* worklist;

  I32 sccid;
  Blocks* scc_stack;

  Fun* current_fun;
};

Sem sem = {};

typedef struct Type_Pair Type_Pair;
struct Type_Pair { Type* one; Type* two; };

Field type_record_get_by_position(Block* block, Record* record, I32 pos);
Type* type_of_var(Block* block, Var* var);
Type* type_of_ir(Ir* ir);
Type* type_join(Type* one, Type* two);
Type* type_meet(Type* one, Type* two);
Type* type_pointer_declared(Pointer* pointer);
void  sem_ensure_declared(Var* var);

void string_builder_push_type(String_Builder* sb, Block* block, Type* type) {
  if (!type) return;
  switch (type->kind) {
  case Type_Kind_none:
    string_builder_push_cstr(sb, "<none>");
  break;
  case Type_Kind_bits: {
    string_builder_push_cstr(sb, "bits(");
    string_builder_push_i64(sb, type->bits_size);
    string_builder_push_cstr(sb, ", ");
    string_builder_push_type(sb, block, type->bits_of);
    string_builder_push_cstr(sb, ") ");
  } break;
  case Type_Kind_int: {
    if (type->ranges->length > 1) {
      string_builder_push_cstr(sb, "(");
    }
    for (I32 i = 0; i < type->ranges->length; i++ ) {
      I64 one = type->ranges->pairs[i].lo;
      I64 two = type->ranges->pairs[i].hi;
      if (one == two) {
        string_builder_push_i64(sb, one);
      }
      else {
        string_builder_push_i64(sb, one);
        string_builder_push_cstr(sb, "..");
        string_builder_push_i64(sb, two);
      }
      if (i+1 < type->ranges->length) {
        string_builder_push_cstr(sb, "\\");
      }
    }
    if (type->ranges->length > 1) {
      string_builder_push_cstr(sb, ")");
    }
  } break;
  case Type_Kind_ptr: {
    // Type* declared = type_pointer_declared(type->pointer);
    string_builder_push_cstr(sb, "@");
    // string_builder_push_type(sb, block, declared);

    if (type->pointer->stack.len) {
      string_builder_push_cstr(sb, "|");
      Hash_Set stack = type->pointer->stack;
      for (I32 i = 0; i < stack.len; i++) {
        Var* var = stack.list[i];
        string_builder_push_var(sb, var);
        if (i+1 < stack.len) {
          string_builder_push_cstr(sb, "\\");
        }
      }
      string_builder_push_cstr(sb, "|");
    }
  } break;
  case Type_Kind_record: {
    Record* record = type->record;
    string_builder_push_cstr(sb, "record");
    string_builder_push_cstr(sb, "(");
    for (I32 pos = 0; pos < record->length; pos++) {
      Field field = {};
      field.name     = record->names[pos];
      if (record->vars) {
        Var* var = record->vars[pos];
        field.declared_type = var->declared;
        field.assigned_type = hash_map_get(&block->out_var_types, var);
      }
      else {
        field.declared_type = type_of_ir(record->declared[pos]);
        field.assigned_type = type_of_ir(record->assigned[pos]);
      }

      if (field.name) {
        string_builder_push_str(sb, field.name);
        // string_builder_push_cstr(sb, "'");
        // string_builder_push_i64(sb, field.offset);
        if (field.declared_type) {
          string_builder_push_cstr(sb, ":");
          string_builder_push_type(sb, block, field.declared_type);
        }
        if (field.assigned_type) {
          string_builder_push_cstr(sb, "=");
          string_builder_push_type(sb, block, field.assigned_type);
        }
      }
      else {
        string_builder_push_type(sb, block, field.declared_type);
      }
      if (pos+1 < record->length) {
        string_builder_push_cstr(sb, ", ");
      }
    }
    string_builder_push_cstr(sb, ")");
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

    for (I32 b = 0; b < fun.blocks->length; b++) {
      Block* block = fun.blocks->base[b];
      string_builder_push_cstr(&sb, "\n  ");
      string_builder_push_blockid(&sb, block);
      string_builder_push_cstr(&sb, ":");

      string_builder_push_i64(&sb, block->sccid);

      string_builder_push_cstr(&sb, "$");
      string_builder_push_i64(&sb, block->is_reachable);

      string_builder_push_cstr(&sb, "|");
      for (I32 i = 0; i < block->preds.length; i++) {
        Block* pred = block->preds.base[i];
        string_builder_push_blockid(&sb, pred);
        string_builder_push_cstr(&sb, ",");
      }
      string_builder_push_cstr(&sb, "|");

      string_builder_push_cstr(&sb, " in{");
      for (I32 i = 0; i < block->in_var_types.len; i++) {
        Var* var = block->in_var_types.list[i];
        Type* type = hash_map_get(&block->in_var_types, var);
        string_builder_push_var(&sb, var);
        string_builder_push_cstr(&sb, ": ");
        string_builder_push_type(&sb, block, type);
        if (i+1 < block->in_var_types.len) {
          string_builder_push_cstr(&sb, "; ");
        }
      }
      string_builder_push_cstr(&sb, "}");

      string_builder_push_cstr(&sb, " out{");
      for (I32 i = 0; i < block->out_var_types.len; i++) {
        Var* var = block->out_var_types.list[i];
        Type* type = hash_map_get(&block->out_var_types, var);
        string_builder_push_var(&sb, var);
        string_builder_push_cstr(&sb, "=");

        string_builder_push_type(&sb, block, type);
        if (i+1 < block->out_var_types.len) {
          string_builder_push_cstr(&sb, "; ");
        }
      }
      string_builder_push_cstr(&sb, "}");
      for (I32 i = 0; i < block->irs->length; i++) {
        Ir* ir = block->irs->base[i];
        string_builder_push_ir(&sb, ir);
        string_builder_push_cstr(&sb, " : ");
        Type* type = hash_map_get(&sem.type_of_irs, ir);
        string_builder_push_type(&sb, block, type);
      }
      switch (block->kind) {
      case Block_Kind_none:
      break;
      case Block_Kind_jump:
        string_builder_push_cstr(&sb, "\n    jump ");
        string_builder_push_blockid(&sb, block->jump.to_block);
      break;
      case Block_Kind_branch:
        string_builder_push_cstr(&sb, "\n    if ");
        string_builder_push_irid(&sb, block->branch.cond);
        string_builder_push_cstr(&sb, " then ");
        string_builder_push_blockid(&sb, block->branch.nez.to_block);
        string_builder_push_cstr(&sb, " else ");
        string_builder_push_blockid(&sb, block->branch.eqz.to_block);
      break;
      }
    }

    string_builder_push_cstr(&sb, "\n  ret ");
    string_builder_push_irid(&sb, fun.return_ir);
    string_builder_push_cstr(&sb, "\n}");
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

I64 ranges_max(Ranges* ranges) {
  return ranges->pairs[ranges->length-1].hi;
}

B8 ranges_have(Ranges* ranges, I64 i64) {
  for (I32 i = 0; i < ranges->length; i++) {
    Range pair = ranges->pairs[i];
    if (pair.lo >= i64 && i64 <= pair.hi) {
      return true;
    }
  }
  return false;
}

I64 ranges_min(Ranges* ranges) {
  return ranges->pairs[0].lo;
}

B8 ranges_is_single(Ranges* ranges) {
  I64 min = ranges_min(ranges);
  I64 max = ranges_max(ranges);
  return min == max;
}

B8 ranges_is_subrange(Ranges* one, Ranges* two) {
  // x in two => x in one
  // necessary: |one| < |two|
  I32 o = 0; I32 t = 0;
  while (o < one->length && t < two->length) {
    if (one->pairs[o].lo < two->pairs[t].lo) {
      // 0..
      //   2..
      return false;
    }
    else if (one->pairs[o].lo > two->pairs[t].hi) {
      //      2..
      // 0..1 ...
      t++;
    }
    else if (one->pairs[o].hi <= two->pairs[t].hi) {
      //   2..4
      // 0.....5
      o++;
    }
    else {
      return false;
    }
  }
  return o == one->length;
}

B8 pointer_is_single(Pointer* ptr) {
  return ptr->stack.len == 1;
}

typedef struct Subtype_Visited Subtype_Visited;
struct Subtype_Visited {
  I32 length;
  Pointer_Pair* base;
};

B8 subtype_visited_contains(Subtype_Visited* v, Pointer* one, Pointer* two) {
  for (I32 i = 0; i < v->length; i++) {
    if (v->base[i].one == one && v->base[i].two == two) return true;
  }
  return false;
}

void subtype_visited_push(Subtype_Visited* v, Pointer* one, Pointer* two) {
  arena_push(sem.temp_arena, sizeof(Pointer_Pair));
  v->base[v->length].one = one;
  v->base[v->length].two = two;
  v->length++;
}

B8 type_is_subtype_rec(Block* block, Type* one, Type* two, Subtype_Visited* visited) {
  // is one subtype of two
  // (1..3) is subtype of 1 -- true
  // (1..2) is subtype of (1..3) -- false

  if (two->kind == Type_Kind_bits) return type_is_subtype_rec(block, one, two->bits_of, visited);
  if (one->kind != two->kind) return false;
  switch (one->kind) {
  case Type_Kind_none: {
    return true;
  } break;
  case Type_Kind_bits: {
    assert(0);
  } break;
  case Type_Kind_int: {
    return ranges_is_subrange(one->ranges, two->ranges);
  } break;
  case Type_Kind_record: {
    if (one->record->length != two->record->length) return false;

    for (I32 i = 0; i < one->record->length; i++) {
      Field field_one = type_record_get_by_position(block, one->record, i);
      Field field_two = type_record_get_by_position(block, two->record, i);
      if (!type_is_subtype_rec(block, field_one.assigned_type, field_two.declared_type, visited)) return false;
    }
    return true;
  } break;
  case Type_Kind_ptr: {
    if (subtype_visited_contains(visited, one->pointer, two->pointer)) return true;
    subtype_visited_push(visited, one->pointer, two->pointer);
    Type* one_declared = type_pointer_declared(one->pointer);
    Type* two_declared = type_pointer_declared(two->pointer);
    if (!type_is_subtype_rec(block, one_declared, two_declared, visited)) return false;
    return true;
  } break;
  }
  return false;
}

B8 type_is_false(Type* type) {
  B8 result = false;
  switch (type->kind) {
  case Type_Kind_int: {
    if (ranges_is_single(type->ranges)) {
      I64 val = ranges_min(type->ranges);
      result = val == 0;
    }
    else {
      result = false;
    }
  } break;
  default: assert(0);
  }
  return result;
}

B8 type_is_true(Type* type) {
  B8 result = false;
  switch (type->kind) {
  case Type_Kind_int: {
    if (ranges_is_single(type->ranges)) {
      I64 val = ranges_min(type->ranges);
      result = val == 1;
    }
    else {
      result = false;
    }
  } break;
  default: assert(0);
  }
  return result;
}

B8 type_is_subtype(Block* block, Type* one, Type* two) {
  Subtype_Visited visited = {0};
  visited.base = arena_push(sem.temp_arena, sizeof(Pointer_Pair));
  B8 result = type_is_subtype_rec(block, one, two, &visited);
  arena_release_mark(sem.temp_arena, visited.base);
  return result;
}

Type* type_of_ir(Ir* ir) {
  Type* type = hash_map_get(&sem.type_of_irs, ir);
  return type;
}

void type_of_ir_put(Ir* ir, Type* type) {
  hash_map_put(&sem.type_of_irs, ir, type);
}

Ranges* sem_ranges_init(I32 max_len) {
  Ranges* result = arena_push(sem.perm_arena, sizeof(Ranges) + max_len * sizeof(Range));
  result->length = 0;
  return result;
}

Type* type_ints(Ranges* ranges) {
  Type* new_type = arena_push(sem.perm_arena, sizeof(Type));

  new_type->kind   = Type_Kind_int;
  new_type->ranges = ranges;

  I64 min = ranges_min(ranges);
  I64 max = ranges_max(ranges);
  I32 bit_size = bits_needed(min, max);
  new_type->bits_align = bit_size;
  new_type->bits_size  = bit_size;

  return new_type;
}

Type* type_int_range(I64 min, I64 max) {
  Ranges* ranges = sem_ranges_init(1);
  ranges->length = 1;
  ranges->pairs[0].lo = min;
  ranges->pairs[0].hi = max;
  return type_ints(ranges);
}

Type* type_int(I64 i64) {
  return type_int_range(i64, i64);
}

Type* type_ranges_offset(Ranges* ranges, I64 offset) {
  Ranges* new_ranges = sem_ranges_init(ranges->length);
  new_ranges->length = ranges->length;
  for (I32 i = 0; i < ranges->length; i++) {
    Range offset_pair = { .lo = ranges->pairs[i].lo + offset, .hi = ranges->pairs[i].hi + offset };
    new_ranges->pairs[i] = offset_pair;
  }

  return type_ints(ranges);
}

Type* type_ranges_merge(Ranges* one, Ranges* two) {
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
  return type_ints(new_ints);
}

Field type_record_get_by_position(Block* block, Record* record, I32 position) {
  Field field = {};
  field.name     = record->names[position];
  field.assigned = record->assigned[position];
  field.declared = record->declared[position];
  if (record->vars) {
    field.var      = record->vars[position];
    field.declared_type = field.var->declared;
    field.assigned_type = type_of_var(block, field.var);
  }
  else {
    field.var = 0;
    field.declared_type = type_of_ir(field.declared);
    field.assigned_type = type_of_ir(field.assigned);
  }
  field.offset   = record->offsets[position];
  field.position = position;
  return field;
}

Field type_record_get_by_name(Block* block, Record* record, Str* name) {
  I32 position = hash_map_get_i32(&record->position_from_name, name);
  return type_record_get_by_position(block, record, position);
}

Type* type_record(Block* block, Record* record) {
  Type* new_type = arena_push(sem.perm_arena, sizeof(Type));
  new_type->kind = Type_Kind_record;
  new_type->record = record;
  return new_type;
}

Type* type_ranges_intersection(Ranges* one, Ranges* two);
Type* type_ptr_stack(Hash_Set stack);

Type* type_meet(Type* one, Type* two) {
  Type* result = sem.type_none;
  if (one->kind != two->kind) return result;

  switch (one->kind) {
  case Type_Kind_none: break;
  case Type_Kind_bits: {
    assert(0);
  } break;
  case Type_Kind_int: {
    result = type_ranges_intersection(one->ranges, two->ranges);
  } break;
  case Type_Kind_ptr: {
    Hash_Set meet = hash_set_meet(sem.perm_arena, &one->pointer->stack, &two->pointer->stack);
    result = type_ptr_stack(meet);
  } break;
  case Type_Kind_record: {
    assert(0);
  } break;
  }
  return result;
}

Type* type_join(Type* one, Type* two) {
  Type* result = 0;
  if (one->kind == Type_Kind_none) {
    result = two;
  }
  else if (two->kind == Type_Kind_none) {
    result = one;
  }
  else if (one->kind == Type_Kind_int && two->kind == Type_Kind_int) {
    result = type_ranges_merge(one->ranges, two->ranges);
  }
  else if (one->kind == Type_Kind_record && two->kind == Type_Kind_record) {
    result = one;
  }
  else if (one->kind == Type_Kind_ptr && two->kind == Type_Kind_ptr) {
    Hash_Set new_stack = hash_set_join(sem.perm_arena, &one->pointer->stack, &two->pointer->stack);
    result = type_ptr_stack(new_stack);
  }
  else {
    assert(0);
  }
  return result;
}

Type* type_ptr(Pointer* ptr) {
  Type* new_type = arena_push(sem.perm_arena, sizeof(Type));
  new_type->kind = Type_Kind_ptr;
  new_type->pointer = ptr;
  return new_type;
}

Type* type_ptr_stack(Hash_Set stack) {
  Pointer* pointer = arena_push_zero(sem.perm_arena, sizeof(Pointer));
  pointer->stack = stack;
  assert(stack.len >= 1);
  return type_ptr(pointer);
}

Type* type_ptr_to(Type* type) {
  Pointer* pointer = arena_push_zero(sem.perm_arena, sizeof(Pointer));
  pointer->declared = type;
  return type_ptr(pointer);
}

Type* type_ptr_var(Var* var) {
  Hash_Set stack = hash_set_init(sem.perm_arena, 1);
  hash_set_put(&stack, var);
  return type_ptr_stack(stack);
}

Type* type_pointer_declared(Pointer* pointer) {
  if (pointer->declared) return pointer->declared;
  Hash_Set stack = pointer->stack;
  if (stack.len == 0) return 0;
  Var* first_var = stack.list[0];
  sem_ensure_declared(first_var);
  Type* result = first_var->declared;
  for (I32 i = 1; i < stack.len; i++) {
    Var* var = stack.list[i];
    sem_ensure_declared(var);
    result = type_meet(result, var->declared);
  }
  pointer->declared = result;
  return result;
}

Type* type_bits(I32 bits_size, Type* bits_of) {
  Type* new_type = arena_push(sem.perm_arena, sizeof(Type));
  new_type->kind = Type_Kind_bits;
  new_type->bits_size  = bits_size;
  new_type->bits_align = bits_size;
  new_type->bits_of  = bits_of;
  return new_type;
}

B8 type_not_equal(Type* one, Type* two) {
  return one != two;
}

B8 type_equal(Type* one, Type* two) {
  assert(0);
  return one == two;
}

Type_Pair type_of_ir_binary(Ir* ir) {
  Type* one = type_of_ir(ir->binary.one);
  Type* two = type_of_ir(ir->binary.two);
  Type_Pair result = { one, two };
  return result;
}

Type* type_of_ir_unary(Ir* ir) {
  Type* result = type_of_ir(ir->unary);
  return result;
}

Ranges_Pair ranges_pair_of_ir_binary(Ir* ir) {
  Type_Pair types = type_of_ir_binary(ir);
  Ranges_Pair result = {};
  result.one = types.one->ranges;
  result.two = types.two->ranges;
  return result;
}

Pointer_Pair pointer_pair_of_ir_binary(Ir* ir) {
  Type_Pair types = type_of_ir_binary(ir);
  Pointer_Pair result = {};
  result.one = types.one->pointer;
  result.two = types.two->pointer;
  return result;
}

B8 type_kind_of_ir_binary_operands_equal(Ir* ir, Type_Kind type_kind) {
  Type_Pair pair = type_of_ir_binary(ir);
  return pair.one->kind == type_kind && pair.two->kind == type_kind;
}

Type* type_ranges_intersection(Ranges* one, Ranges* two) {
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
  return type_ints(new_ranges);
}

Type* type_ranges_no_intersection(Ranges* one, Ranges* two) {
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
  return type_ints(new_ranges);
}

Type* type_ranges_exclude(Ranges* ranges, I64 val) {
  Ranges* new_ints = sem_ranges_init(ranges->length);
  for (I32 i = 0; i < ranges->length; i++) {
    if (ranges->pairs[i].lo <= val && val <= ranges->pairs[i].hi) {
      if (ranges->pairs[i].lo != val || val != ranges->pairs[i].hi) {
        if (ranges->pairs[i].lo == val) {
          Range pair = { .lo = ranges->pairs[i].lo+1, .hi = ranges->pairs[i].hi };
          new_ints->pairs[new_ints->length++] = pair;
        }
        else if (val == ranges->pairs[i].hi) {
          Range pair = { .lo = ranges->pairs[i].lo, .hi = ranges->pairs[i].hi-1 };
          new_ints->pairs[new_ints->length++] = pair;
        }
        else {
          Range pair1 = { .lo = ranges->pairs[i].lo, .hi = val-1 };
          new_ints->pairs[new_ints->length++] = pair1;
          Range pair2 = { .lo = val+1, .hi = ranges->pairs[i].hi };
          new_ints->pairs[new_ints->length++] = pair2;
        }
      }
      i++;
      for (; i < ranges->length; i++) {
        new_ints->pairs[new_ints->length++] = ranges->pairs[i];
      }
      break;
    }
    else {
      new_ints->pairs[new_ints->length++] = ranges->pairs[i];
    }
  }
  return type_ints(new_ints);
}

void sem_tasks_push_var(Sem_Tasks* tasks, Var* var, Type* old) {
  tasks->vars[tasks->length] = var;
  tasks->types[tasks->length] = old;
  tasks->length++;
}

void sem_type_of_ir_narrow(Sem_Tasks* tasks, Ir* ir, Type* new_type) {
  if (ir->kind == Ir_Kind_load) {
    Type* ptr_type = type_of_ir(ir->unary);
    Pointer* ptr = ptr_type->pointer;
    for (I32 i = 0; i < ptr->stack.len; i++) {
      Var* var = ptr->stack.list[i];
      Type* old_type = hash_map_get(tasks->out_vars, var);
      if (hash_map_change_if_exists(tasks->out_vars, var, new_type)) {
        sem_tasks_push_var(tasks, var, old_type);
      }
    }
  }
}

void sem_type_of_ir_binary_narrow(Sem_Tasks* tasks, Ir* ir, Type* new_type_one, Type* new_type_two) {
  sem_type_of_ir_narrow(tasks, ir->binary.one, new_type_one);
  sem_type_of_ir_narrow(tasks, ir->binary.two, new_type_two);
}

void sem_type_narrow_int_eq(Sem_Tasks* tasks, Ir* ir) {
  Ranges_Pair pair = ranges_pair_of_ir_binary(ir);
  Type* new_type = type_ranges_intersection(pair.one, pair.two);
  sem_type_of_ir_binary_narrow(tasks, ir, new_type, new_type);
}

void sem_type_narrow_ptr_eq(Sem_Tasks* tasks, Ir* ir) {
  Pointer_Pair pair = pointer_pair_of_ir_binary(ir);
  Hash_Set meet = hash_set_meet(sem.perm_arena, &pair.one->stack, &pair.two->stack);
  Type* new_type = type_ptr_stack(meet);
  sem_type_of_ir_binary_narrow(tasks, ir, new_type, new_type);
}

void sem_type_narrow_int_ne(Sem_Tasks* tasks, Ir* ir) {
  Type_Pair pair = type_of_ir_binary(ir);
  if (ranges_is_single(pair.one->ranges)) {
    I64 val = ranges_min(pair.one->ranges);
    Type* new_type_two = type_ranges_exclude(pair.two->ranges, val);
    sem_type_of_ir_narrow(tasks, ir->binary.two, new_type_two);
  }
  if (ranges_is_single(pair.two->ranges)) {
    I64 val = ranges_min(pair.two->ranges);
    Type* new_type_one = type_ranges_exclude(pair.one->ranges, val);
    sem_type_of_ir_narrow(tasks, ir->binary.one, new_type_one);
  }
}

void sem_type_narrow_ptr_ne(Sem_Tasks* tasks, Ir* ir) {
  Pointer_Pair pair = pointer_pair_of_ir_binary(ir);
  if (pointer_is_single(pair.one)) {
    Hash_Set new_set_two = hash_set_exclude(sem.perm_arena, &pair.two->stack, pair.one->stack.list[0]);
    Type* new_type_two = type_ptr_stack(new_set_two);
    sem_type_of_ir_narrow(tasks, ir->binary.two, new_type_two);
  }
  if (pointer_is_single(pair.two)) {
    Hash_Set new_set_one = hash_set_exclude(sem.perm_arena, &pair.one->stack, pair.two->stack.list[0]);
    Type* new_type_one = type_ptr_stack(new_set_one);
    sem_type_of_ir_narrow(tasks, ir->binary.one, new_type_one);
  }
}

Type* type_narrow_eqz(Type* type) {
  switch (type->kind) {
  case Type_Kind_int: {
    B8 have_zero = ranges_have(type->ranges, 0);
    if (have_zero) {
      return type_int(0);
    }
    else {
      return 0;
    }
  } break;
  default: break;
  }
  return type;
}

Type* type_narrow_nez(Type* type) {
  switch (type->kind) {
  case Type_Kind_int: {
    return type_ranges_exclude(type->ranges, 0);
  } break;
  default: break;
  }
  return type;
}

void sem_narrow_nez(Sem_Tasks* tasks, Block* block) {
  Ir* cond_ir = block->branch.cond;
  switch (cond_ir->kind) {
  case Ir_Kind_eq: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
      sem_type_narrow_int_eq(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_eq(tasks, cond_ir);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_ne: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
      sem_type_narrow_int_ne(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_ne(tasks, cond_ir);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_lt: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
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
    Type* ptr_type = type_of_ir(cond_ir->unary);
    if (ptr_type->kind == Type_Kind_ptr) {
      Pointer* ptr = ptr_type->pointer;
      for (I32 i = 0; i < ptr->stack.len; i++) {
        Var* var = ptr->stack.list[i];
        Type* old_type = hash_map_get(tasks->out_vars, var);
        Type* new_type = type_narrow_nez(old_type);
        if (hash_map_change_if_exists(tasks->out_vars, var, new_type)) {
          sem_tasks_push_var(tasks, var, old_type);
        }
      }
    }
  } break;
  default: {} break;
  }
}

void sem_narrow_eqz(Sem_Tasks* tasks, Block* block) {
  Ir* cond_ir = block->branch.cond;
  switch (cond_ir->kind) {
  case Ir_Kind_eq: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
      sem_type_narrow_int_ne(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_ne(tasks, cond_ir);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_ne: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
      sem_type_narrow_int_eq(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_eq(tasks, cond_ir);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_lt: {
    assert(0);
  } break;
  case Ir_Kind_le: assert(0); // log_todo("Ir_Kind_le narrow nez");
  case Ir_Kind_gt: assert(0); // log_todo("Ir_Kind_gt narrow nez");
  case Ir_Kind_ge: assert(0); // log_todo("Ir_Kind_ge narrow nez");
  case Ir_Kind_load: {
    Type* ptr_type = type_of_ir(cond_ir->unary);
    if (ptr_type->kind == Type_Kind_ptr) {
      Pointer* ptr = ptr_type->pointer;
      for (I32 i = 0; i < ptr->stack.len; i++) {
        Var* var = ptr->stack.list[i];
        Type* old_type = hash_map_get(tasks->out_vars, var);
        Type* new_type = type_narrow_eqz(old_type);
        if (hash_map_change_if_exists(tasks->out_vars, var, new_type)) {
          sem_tasks_push_var(tasks, var, old_type);
        }
      }
    }
  } break;
  default: {} break;
  }
}

void sem_unnarrow(Sem_Tasks tasks) {
  for (I32 i = 0; i < tasks.length; i++) {
    Var*  var = tasks.vars[i];
    Type* old = tasks.types[i];
    hash_map_put(tasks.out_vars, var, old);
  }
}

Type* type_of_var_rec(Block* block, Var* var) {
  Type* type = hash_map_get(&block->out_var_types, var);
  if (type) return type;

  hash_map_put(&block->out_var_types, var, sem.type_none);
  type = sem.type_none;

  for (I32 i = 0; i < block->preds.length; i++) {
    Block* pred = block->preds.base[i];
    if (pred->is_reachable || !block->is_reachable) {
      if (pred->kind == Block_Kind_branch) {
        Sem_Tasks tasks = {};
        tasks.out_vars = &pred->out_var_types;
        tasks.length = 0;
        tasks.vars = arena_push(sem.temp_arena, sizeof(Var*) * tasks.out_vars->len);
        tasks.types = arena_push(sem.temp_arena, sizeof(Type*) * tasks.out_vars->len);
        // TODO: this does narrowing again and again for each variable load
        //       either do it once for all variables, or, narrow only a requested variable
        { // condition is not equal to zero branch
          Jump jump = pred->branch.nez;
          if (jump.to_block == block) {
            sem_narrow_nez(&tasks, pred);
            Type* pred_type = type_of_var_rec(pred, var);
            type = type_join(type, pred_type);
            sem_unnarrow(tasks);
            tasks.length = 0;
          }
        }
        { // condition is equal to zero branch
          Jump jump = pred->branch.eqz;
          if (jump.to_block == block) {
            sem_narrow_eqz(&tasks, pred);
            Type* pred_type = type_of_var_rec(pred, var);
            type = type_join(type, pred_type);
            sem_unnarrow(tasks);
            tasks.length = 0;
          }
        }

        arena_release_mark(sem.temp_arena, tasks.vars);
      }
      else if (pred->kind == Block_Kind_jump) {
        Type* pred_type = type_of_var_rec(pred, var);
        type = type_join(type, pred_type);
      }
    }
  }

  hash_map_put(&block->out_var_types, var, type);
  return type;
}

Type* type_of_var(Block* block, Var* var) {
  Type* type = type_of_var_rec(block, var);
  if (type->kind == Type_Kind_none) {
    type = var->declared ? var->declared : sem.type_none;
    hash_map_put(&block->out_var_types, var, type);
  }
  return type;
}

void type_of_var_put(Block* block, Var* var, Type* type) {
  Type* var_type = var->declared;
  if (type->kind == Type_Kind_bits) {
    type = type->bits_of;
  }
  if (type_is_subtype(block, type, var_type)) {
    if (var_type->kind == Type_Kind_record) {
      for (I32 i = 0; i < var_type->record->length; i++) {
        Type* field_type = type_of_ir(type->record->assigned[i]);
        type->record->declared = var_type->record->declared;
        type->record->vars = var_type->record->vars;
        type_of_var_put(block, var_type->record->vars[i], field_type);
      }
    }
    else if (var_type->kind == Type_Kind_bits) {
      type = type_bits(var_type->bits_size, type);
    }
    hash_map_put(&block->out_var_types, var, type);
  }
  else {
    assert(0);
  }
}

void sem_worklist_sort(void) {
  for (I32 i = 0; i < sem.worklist->length; i++) {
    I32 max_i = i;
    for (I32 j = max_i+1; j < sem.worklist->length; j++) {
      if (sem.worklist->base[max_i]->sccid < sem.worklist->base[j]->sccid) {
        max_i = j;
      }
    }
    Block* temp = sem.worklist->base[i];
    sem.worklist->base[i] = sem.worklist->base[max_i];
    sem.worklist->base[max_i] = temp;
  }
}

void sem_worklist_push(Block* block) {
  if (!block->is_present_in_worklist) {
    block->is_present_in_worklist = true;
    fa_add(sem.worklist, block);
  }
}

B8 sem_worklist_is_not_empty() {
  return sem.worklist->length != 0;
}

Block* sem_worklist_pop(void) {
  Block* block = fa_pop(sem.worklist);
  block->is_present_in_worklist = false;
  return block;
}

void sem_jump(Block* from_block, Block* to_block) {
  B8 is_jump_updates_var_type = false;
  Hash_Map* outvars = &from_block->out_var_types;
  Hash_Map* invars  = &to_block->in_var_types;
  for (I32 i = 0; i < outvars->len; i++) {
    Var* var = outvars->list[i];
    Type* out_var_type = hash_map_get(outvars, var);
    Type* old_var_type = hash_map_get(invars,  var);
    Type* new_var_type = type_join(out_var_type, old_var_type);

    if (type_not_equal(new_var_type, old_var_type)) {
      hash_map_put(invars, var, new_var_type);
      hash_map_put(&to_block->out_var_types, var, new_var_type);
      is_jump_updates_var_type = true;
    }
  }
  if (is_jump_updates_var_type) {
    sem_worklist_push(to_block);
  }
}

void sem_block(Block* block);
void sem_init_block_preds(Block* block);

void sem_record_declare_fields(Var* var, Type* type) {
  if (type->kind != Type_Kind_record) return;
  type->record->vars = arena_push(irgen.perm_arena, type->record->length*sizeof(Var*));
  for (I32 i = 0; i < type->record->length; i++) {
    Var* field_var = arena_push_zero(irgen.perm_arena, sizeof(Var));
    type->record->vars[i] = field_var;
    field_var->name = type->record->names[i];
    field_var->parent = var;
    Type* field_type = type_of_ir(type->record->declared[i]);
    field_var->declared = field_type;
    field_var->state = Var_State_resolved;
    sem_record_declare_fields(field_var, field_type);

    type->bits_align = max(type->bits_align, field_type->bits_align);
    type->bits_size = align_up(type->bits_size, field_type->bits_align);
    type->record->offsets[i] = type->bits_size;
    type->bits_size += field_type->bits_size;
  }
  type->bits_size = align_up(type->bits_size, type->bits_align);
}

void sem_ensure_declared(Var* var) {
  if (var->state == Var_State_resolved) return;
  if (var->state == Var_State_resolving) {
    assert(0);
  }
  var->state = Var_State_resolving;

  if (var->blocks) {
    I32 cap = sem.current_fun->var_count;
    for (I32 i = 0; i < var->blocks->length; i++) {
      Block* block = var->blocks->base[i];
      sem_init_block_preds(block);
      if (!block->out_var_types.keys) {
        block->out_var_types = hash_map_init(sem.perm_arena, cap);
        block->in_var_types  = hash_map_init(sem.perm_arena, cap);
      }
      sem_block(block);
    }
  }

  Type* type = type_of_ir(var->declared_ir);
  var->declared = type;
  sem_record_declare_fields(var, type);

  var->state = Var_State_resolved;
}

void sem_ir(Block* block, Ir* ir) {
  Type* result = type_of_ir(ir);
  switch (ir->kind) {
  case Ir_Kind_int: {
    I64 i64 = ir->i64;
    result  = type_int(i64);
  } break;
  case Ir_Kind_declare: {
    sem_ensure_declared(ir->declare.var);
  } break;
  case Ir_Kind_var: {
    result = type_ptr_var(ir->var);
  } break;
  case Ir_Kind_join: {
    Type_Pair pair = type_of_ir_binary(ir);
    result = type_join(pair.one, pair.two);
  } break;
  case Ir_Kind_add: {
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_int)) {
      // TODO: overflow/underflow
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);
      I64 max_one = ranges_max(pair.one);
      I64 max_two = ranges_max(pair.two);
      I64 max = max_one + max_two;
      I64 min_one = ranges_min(pair.one);
      I64 min_two = ranges_min(pair.two);
      I64 min = min_one + min_two;
      result = type_int_range(min, max);
    }
  } break;
  case Ir_Kind_eq: {
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_int)) {
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);
      if (ranges_is_single(pair.one) && ranges_is_single(pair.two)) {
        I64 val_one = ranges_min(pair.one);
        I64 val_two = ranges_min(pair.two);
        I64 val = val_one == val_two;
        result = type_int(val);
      }
      else {
        result = type_int_range(0, 1);
      }
    }
  } break;
  case Ir_Kind_record: {
    result = type_record(block, ir->record);
  } break;
  case Ir_Kind_name_offset: {
    Ir_Kind ir_kind = ir->name.of->kind;
    Type* of_type = type_of_ir(ir->name.of);
    if (ir_kind == Ir_Kind_load) {
      if (of_type->kind == Type_Kind_record) {
        Record* record = of_type->record;
        I32 position = hash_map_get_i32(&record->position_from_name, ir->name.at);
        Var* var = record->vars[position];
        result = type_ptr_var(var);
      }
    }
  } break;
  case Ir_Kind_load: {
    Type* ptr_type = type_of_ir(ir->unary);
    if (ptr_type->kind == Type_Kind_ptr) {
      Pointer* pointer = ptr_type->pointer;
      Hash_Set stack = pointer->stack;
      assert(stack.len >= 1);
      for (I32 i = 0; i < stack.len; i++) {
        sem_ensure_declared(stack.list[i]);
      }
      result = type_of_var(block, stack.list[0]);
      for (I32 i = 1; i < stack.len; i++) {
        Type* type = type_of_var(block, stack.list[i]);
        result = type_join(result, type);
      }
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_store: {
    Type* lhs = type_of_ir(ir->binary.one);
    Type* rhs = type_of_ir(ir->binary.two);
    if (lhs->kind == Type_Kind_ptr) {
      Pointer* pointer = lhs->pointer;
      Hash_Set stack = pointer->stack;
      assert(stack.len >= 1);
      if (stack.len == 1) {
        Var* var = stack.list[0];
        type_of_var_put(block, var, rhs);
      }
      else {
        for (I32 i = 0; i < stack.len; i++) {
          Var* var = stack.list[i];
          Type* old_type = type_of_var(block, var);
          Type* new_type = type_join(old_type, rhs);
          type_of_var_put(block, var, new_type);
        }
      }
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_ptr: {
    Type* type = type_of_ir(ir->unary);
    result = type_ptr_to(type);
  } break;
  case Ir_Kind_range: {
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_int)) {
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);

      if (ranges_is_single(pair.one) && ranges_is_single(pair.two)) {
        I64 val_one = ranges_min(pair.one);
        I64 val_two = ranges_min(pair.two);
        if (val_one < val_two) {
          result = type_int_range(val_one, val_two);
        }
        else {
          assert(0);
        }
      }
      else {
        assert(0);
      }
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_bits: {
    Type* one_type = type_of_ir(ir->binary.one);
    Type* two_type = type_of_ir(ir->binary.two);
    if (two_type->kind == Type_Kind_int) {
      if (ranges_is_single(two_type->ranges)) {
        I64 val = ranges_min(two_type->ranges);
        if (val < I16_MAX) {
          result = type_bits(val, one_type);
        }
        else {
          assert(0);
        }
      }
      else {
        assert(0);
      }
    }
    else {
      assert(0);
    }
  } break;
  default: assert(0);
  }
  type_of_ir_put(ir, result);
}

void sem_block(Block* block) {
  for (I32 i = 0; i < block->irs->length; i++) {
    sem_ir(block, block->irs->base[i]);
  }
  if (block->is_reachable) {
    if (block->kind == Block_Kind_branch) {
      Type* cond_type = type_of_ir(block->branch.cond);
      if (type_is_true(cond_type)) {
        block->branch.nez.to_block->is_reachable = true;
      }
      else if (type_is_false(cond_type)) {
        block->branch.eqz.to_block->is_reachable = true;
      }
      else {
        block->branch.nez.to_block->is_reachable = true;
        block->branch.eqz.to_block->is_reachable = true;
      }
    }
    else if (block->kind == Block_Kind_jump) {
        block->jump.to_block->is_reachable = true;
    }
  }
}

void sem_scc_block(Block* block);
I32 sem_scc_block_jump(Block* parent, Block* kid) {
  sem_scc_block(kid);
  if (kid->is_on_scc_stack) {
    I32 new_low = min(parent->low_sccid, kid->low_sccid);
    parent->low_sccid = new_low;
  }
  return parent->low_sccid;
}

void sem_scc_block(Block* block) {
  if (block->is_scc_visited) return;
  block->is_scc_visited = true;
  I32 sccid = sem.sccid++;
  block->sccid = sccid;
  block->low_sccid = sccid;
  block->is_on_scc_stack = true;
  fa_add(sem.scc_stack, block);

  if (block->kind == Block_Kind_jump) {
    sccid = sem_scc_block_jump(block, block->jump.to_block);
  }
  else if (block->kind == Block_Kind_branch) {
    sccid = sem_scc_block_jump(block, block->branch.nez.to_block);
    sccid = sem_scc_block_jump(block, block->branch.eqz.to_block);
  }

  if (block->sccid == sccid) {
    Block* pop_block;
    do {
      pop_block = fa_pop(sem.scc_stack);
      pop_block->is_on_scc_stack = false;
      pop_block->sccid = sccid;
    } while (block != pop_block);
  }
}

void sem_init_block_preds(Block* block) {
  if (block->preds.base) return;

  block->preds.base = arena_push(sem.perm_arena, block->preds.length * sizeof(Block*));
  block->preds.length = 0;

  if (block->kind == Block_Kind_jump) {
    Block* to_block = block->jump.to_block;
    sem_init_block_preds(to_block);
    add(to_block->preds, block);
  }
  else if (block->kind == Block_Kind_branch) {
    {
      Block* to_block = block->branch.nez.to_block;
      sem_init_block_preds(to_block);
      add(to_block->preds, block);
    }
    {
      Block* to_block = block->branch.eqz.to_block;
      sem_init_block_preds(to_block);
      add(to_block->preds, block);
    }
  }
}

Type* sem_fun(Fun* fun) {
  fun->blocks->base[0]->is_reachable = true;
  for (I32 b = 0; b < fun->blocks->length; b++) {
    Block* block = fun->blocks->base[b];
    sem_init_block_preds(block);
    sem_scc_block(block);
    block->out_var_types = hash_map_init(sem.perm_arena, fun->var_count);
    block->in_var_types  = hash_map_init(sem.perm_arena, fun->var_count);
    sem_worklist_push(block);
  }
  sem_worklist_sort();
  // init block with types

  while (sem_worklist_is_not_empty()) {
    Block* block = sem_worklist_pop();
    sem_block(block);
  }

  return type_of_ir(fun->return_ir);
}

void sem_funs(Arena* arena, Funs funs) {
  Arena temp = arena_init(arena->capacity);
  sem.perm_arena = arena;
  sem.temp_arena = &temp;
  sem.funs = funs;
  sem.worklist = arena_push(arena, sizeof(Blocks) + sizeof(Block*)*irgen.blocks.length);
  sem.worklist->length = 0;
  sem.scc_stack = arena_push(arena, sizeof(Blocks) + sizeof(Block*)*irgen.blocks.length);
  sem.scc_stack->length = 0;
  sem.type_of_irs = hash_map_init(arena, irgen.irs.length);

  sem.type_none = arena_push(arena, sizeof(Type));
  sem.type_none->kind = Type_Kind_none;

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    sem.current_fun = fun;
    sem_fun(fun);
  }
  arena_free(&temp);
}

void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  I32 source_length    = strlen(source) + 2;
  Arena arena          = arena_init(KB(4) * source_length);
  str_init(&arena, 2*source_length);
  Tokens tokens        = lex_source(&arena, source);
  Ast_Block ast        = parse_tokens(&arena, tokens);
  Funs funs            = irgen_ast(&arena, ast, source_length);
                         sem_funs(&arena, funs);
  C8* buffer           = arena_push(&arena, 32 * source_length);
  Cstr result          = cstr_from_sem(funs, buffer);
  test_at_source(result, expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_sem(source, expected, __FILE__, __LINE__)

void sem_test(void) {
  // TODO:
  // - [ ] loop-updated variables
  // test("a: (x:1; y:2); b:@1; a = (x=1; y=2); b = @a.x; b@ + a.x", "");
  // test("a: (x:0\\1\\3; y:2\\4\\5); b: @(0\\1\\3); a=(x=1; y=2); b=@a.x; if b@ do { a = (x=0; y=5); b@=3; a.y = 4; }", "");
  // test("a: (x:0\\1); a.x=1; if 2 do { a.x = 0 }; if a.x == 0 do a.x", "");
  // test("a: 0\\1; a=1; if 2 do { a = 0 }; a", "");
  // test("a: 0\\1; a=1; if 2 do { a = 0 }; if a do {a+a}; a+a", "");
  // test("a: (x:0\\1; y:2\\3); a = (x=0; y=2); if 5 do { a = (x=1; y=3) }; if a.x == 1 do {a.x}", "");
  // test("a: 0\\1; b: 1\\2; c: @(0\\1\\2); a = 0; b = 2; c = @a; if 3 do { c = @b }; c @= 1; a+b", "");
  // test("a: 0\\1; b: 1\\2; c: @(0\\1\\2); a = 0; b = 2; c = @a; if 3 do { c = @b }; if c == @a do {c @= 1}; a+b", "");
  // test("a: 1\\2; wh 2 do { a = 1 }; a+a", "");
  // test("a: 1\\2\\3; a = 2; wh 2 do { if 3 do { a = 1 } }; a+a", "");
  // test("a: 1\\2; b:@a; b@", "");
  // test("a: 1", "");
  // test("b: a; a: 1", "");
  // test("a: b; b: a", ""); // cyclic dependancy
  // test("b+b; b: a; a: 1", "");
  // test("a: @b; b: @a; b@", ""); // cycle during string builder
  // test("a : 1; b : a; c : b+a; c; c", "");
  // test("A: (val:1; next:@B); B: (val:2; next:@A); a: A; b: B; a.next = @b; a.next@.val", "");
  // test("A: (val:1; next:@A); a: A; a.next = @a; a.next@.val", "");
  // test("a: 1\\2\\3; a = 1; if 0 do { a=2; if 1 do { a+a } }; a+a", "");
  /*
  a := I32 0
  wh a < 10 do {
    a = a + 1
  }

  */
  test("a:I32; b: I32; a = b; a", "");
}

#undef test
