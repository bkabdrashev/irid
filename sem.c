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
  Type* declared;
  Hash_Set stack;
};

typedef struct Pointer_Pair Pointer_Pair;
struct Pointer_Pair { Pointer* one; Pointer* two; };

typedef struct Function Function;
struct Function { Type* arg; Type* ret; };

typedef enum Type_Kind {
  Type_Kind_none,
  Type_Kind_int,
  Type_Kind_ptr,
  Type_Kind_record,
  Type_Kind_fun,
} Type_Kind;

struct Type {
  Type_Kind kind;
  B8  size_defined;
  I16 bits_align;
  I16 bits_size;
  union {
    Ranges*  ranges;
    Pointer* pointer;
    Record*  record;
    Function* fun;
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

  I32 sccid;
  Blocks* scc_stack;

  I32 current_fun_var_count;
};

Sem sem = {};

typedef struct Type_Pair Type_Pair;
struct Type_Pair { Type* one; Type* two; };

Field type_record_get_by_name(Block* block, Record* record, Str* name);
Field type_record_get_by_position(Block* block, Record* record, I32 pos);
Type* type_of_var(Block* block, Var* var);
Type* type_of_ir(Ir* ir);
Type* type_join(Type* one, Type* two);
Type* type_meet(Type* one, Type* two);
Type* type_pointer_declared(Pointer* pointer);
void  sem_ensure_declared(Var* var);

void string_builder_push_type(String_Builder* sb, Block* block, Type* type) {
  if (!type) return;
  if (type->size_defined) {
    string_builder_push_cstr(sb, "bits(");
    string_builder_push_i64(sb, type->bits_size);
    string_builder_push_cstr(sb, ", ");
  }
  switch (type->kind) {
  case Type_Kind_none:
    string_builder_push_cstr(sb, "none");
  break;
  case Type_Kind_fun: {
    string_builder_push_type(sb, block, type->fun->arg);
    string_builder_push_cstr(sb, " -> ");
    string_builder_push_type(sb, block, type->fun->ret);
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
  if (type->size_defined) {
    string_builder_push_cstr(sb, ") ");
  }
}

Cstr cstr_from_sem(Funs funs, C8* buffer) {
  String_Builder sb = string_builder_begin(buffer);
  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &irgen.funs.base[f];
    string_builder_push_fun(&sb, fun);
    string_builder_push_cstr(&sb, " : ");
    string_builder_push_type(&sb, fun->blocks->base[0], fun->type);
    string_builder_push_cstr(&sb, " {");

    for (I32 b = 0; b < fun->blocks->length; b++) {
      Block* block = fun->blocks->base[b];
      string_builder_push_cstr(&sb, "\n  ");
      string_builder_push_blockid(&sb, block);
      string_builder_push_cstr(&sb, ":");

      string_builder_push_i64(&sb, block->sccid);

      string_builder_push_cstr(&sb, "$");
      string_builder_push_i64(&sb, block->state);

      string_builder_push_cstr(&sb, "|");
      for (I32 i = 0; i < block->preds.length; i++) {
        Block* pred = block->preds.base[i];
        string_builder_push_blockid(&sb, pred);
        string_builder_push_cstr(&sb, ",");
      }
      string_builder_push_cstr(&sb, "|");

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
        string_builder_push_cstr(&sb, "\n");
        string_builder_push_indent(&sb, 2);
        string_builder_push_ir(&sb, ir);
        string_builder_push_cstr(&sb, " : ");
        Type* type = hash_map_get(&sem.type_of_irs, ir);
        string_builder_push_type(&sb, block, type);
      }
      switch (block->kind) {
      case Block_Kind_none:
      break;
      case Block_Kind_jump:
        string_builder_push_cstr(&sb, "\n");
        string_builder_push_indent(&sb, 2);
        string_builder_push_cstr(&sb, "jump ");
        string_builder_push_blockid(&sb, block->jump.to_block);
      break;
      case Block_Kind_branch:
        string_builder_push_cstr(&sb, "\n");
        string_builder_push_indent(&sb, 2);
        string_builder_push_cstr(&sb, "if ");
        string_builder_push_irid(&sb, block->branch.cond);
        string_builder_push_cstr(&sb, " then ");
        string_builder_push_blockid(&sb, block->branch.nez.to_block);
        string_builder_push_cstr(&sb, " else ");
        string_builder_push_blockid(&sb, block->branch.eqz.to_block);
      break;
      }
    }
    string_builder_push_cstr(&sb, "\n}\n");
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
    if (pair.lo <= i64 && i64 <= pair.hi) {
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
  //    one is subtype of two
  // (1..3) is subtype of 1 -- false
  // (1..2) is subtype of (1..3) -- true

  if (one->kind != two->kind) return false;
  switch (one->kind) {
  case Type_Kind_none: {
    return true;
  } break;
  case Type_Kind_int: {
    return ranges_is_subrange(one->ranges, two->ranges);
  } break;
  case Type_Kind_record: {
    if (one->record->length != two->record->length) return false;
    for (I32 i = 0; i < one->record->length; i++) {
      Field field_one = type_record_get_by_position(block, one->record, i);
      Field field_two;
      if (field_one.name) {
        field_two = type_record_get_by_name(block, two->record, field_one.name);
      }
      else {
        field_two = type_record_get_by_position(block, two->record, i);
      }
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
  case Type_Kind_fun: {
    return false;
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
    result = !ranges_have(type->ranges, 0);
  } break;
  default: assert(0);
  }
  return result;
}

B8 type_is_const(Type* type) {
  switch (type->kind) {
  case Type_Kind_none: {
    return true;
  } break;
  case Type_Kind_int: {
    return ranges_is_single(type->ranges);
  } break;
  case Type_Kind_record: {
    for (I32 i = 0; i < type->record->length; i++) {
      Type* field_declared = type_of_ir(type->record->declared[i]);
      if (field_declared) {
        if (!type_is_const(field_declared)) {
          return false;
        }
      }
      else {
        return false;
      }
    }
    return false;
  } break;
  case Type_Kind_ptr: {
    return type->pointer->stack.len == 1;
  } break;
  case Type_Kind_fun: {
    return false;
  } break;
  }
  return false;
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

Type* type_ranges(Ranges* ranges) {
  Type* new_type = &new(sem.types);

  if (ranges->length == 0) {
    new_type->kind = Type_Kind_none;
  }
  else {
    new_type->kind   = Type_Kind_int;
    new_type->ranges = ranges;

    I64 min = ranges_min(ranges);
    I64 max = ranges_max(ranges);
    I32 bit_size = bits_needed(min, max);
    new_type->bits_align = bit_size;
    new_type->bits_size  = bit_size;
  }

  return new_type;
}

Type* type_range(I64 min, I64 max) {
  Ranges* ranges = sem_ranges_init(1);
  ranges->length = 1;
  ranges->pairs[0].lo = min;
  ranges->pairs[0].hi = max;
  return type_ranges(ranges);
}

Type* type_int(I64 i64) {
  return type_range(i64, i64);
}

Type* type_ranges_offset(Ranges* ranges, I64 offset) {
  Ranges* new_ranges = sem_ranges_init(ranges->length);
  new_ranges->length = ranges->length;
  for (I32 i = 0; i < ranges->length; i++) {
    Range offset_pair = { .lo = ranges->pairs[i].lo + offset, .hi = ranges->pairs[i].hi + offset };
    new_ranges->pairs[i] = offset_pair;
  }

  return type_ranges(ranges);
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
  return type_ranges(new_ints);
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

Type* type_record(Record* record) {
  Type* new_type = &new(sem.types);
  if (record->length == 0) {
    new_type->kind = Type_Kind_none;
  }
  else {
    new_type->kind = Type_Kind_record;
    new_type->record = record;
  }
  return new_type;
}

Type* type_ranges_intersection(Ranges* one, Ranges* two);
Type* type_ptr_stack(Hash_Set stack);

Type* type_meet(Type* one, Type* two) {
  Type* result = sem.type_none;
  if (one->kind != two->kind) return result;

  switch (one->kind) {
  case Type_Kind_none: break;
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
  case Type_Kind_fun: {
    result = sem.type_none;
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
  Type* new_type = &new(sem.types);
  new_type->kind = Type_Kind_ptr;
  new_type->pointer = ptr;
  return new_type;
}

Type* type_ptr_stack(Hash_Set stack) {
  Pointer* pointer = arena_push_zero(sem.perm_arena, sizeof(Pointer));
  pointer->stack = stack;
  assert(stack.len >= 1);
  type_pointer_declared(pointer);
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

Type* type_define_size(I32 bits_size, Type* bits_of) {
  Type* new_type = &new(sem.types);
  *new_type = *bits_of;
  new_type->size_defined = true;
  new_type->bits_size = bits_size;
  new_type->bits_align = bits_size;
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
  return type_ranges(new_ranges);
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
  return type_ranges(new_ranges);
}

Type* type_ranges_exclude(Ranges* ranges, I64 val) {
  Ranges* new_ints = sem_ranges_init(ranges->length+1);
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
  return type_ranges(new_ints);
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
      new_type->size_defined = old_type->size_defined;
      new_type->bits_size = old_type->bits_size;
      new_type->bits_align = old_type->bits_align;
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

I64 sem_sat_dec(I64 x) { return x == I64_MIN ? I64_MIN : x - 1; }
I64 sem_sat_inc(I64 x) { return x == I64_MAX ? I64_MAX : x + 1; }

Type* sem_ranges_clamp(Ranges* ranges, I64 lo, I64 hi) {
  Ranges* line = sem_ranges_init(1);
  line->length = 1;
  line->pairs[0].lo = lo;
  line->pairs[0].hi = hi;
  return type_ranges_intersection(ranges, line);
}

Ir_Kind sem_cmp_negate(Ir_Kind op) {
  switch (op) {
  case Ir_Kind_lt: return Ir_Kind_ge;
  case Ir_Kind_le: return Ir_Kind_gt;
  case Ir_Kind_gt: return Ir_Kind_le;
  case Ir_Kind_ge: return Ir_Kind_lt;
  default:         return op;
  }
}

// narrows both operands of `ir` assuming `one op two` holds
void sem_type_narrow_int_cmp(Sem_Tasks* tasks, Ir* ir, Ir_Kind op) {
  Type_Pair pair = type_of_ir_binary(ir);
  Ranges* ra = pair.one->ranges;
  Ranges* rb = pair.two->ranges;
  I64 amin = ranges_min(ra);
  I64 amax = ranges_max(ra);
  I64 bmin = ranges_min(rb);
  I64 bmax = ranges_max(rb);
  Type* na = 0;
  Type* nb = 0;
  switch (op) {
  case Ir_Kind_lt:
    na = sem_ranges_clamp(ra, I64_MIN, sem_sat_dec(bmax));
    nb = sem_ranges_clamp(rb, sem_sat_inc(amin), I64_MAX);
  break;
  case Ir_Kind_le:
    na = sem_ranges_clamp(ra, I64_MIN, bmax);
    nb = sem_ranges_clamp(rb, amin, I64_MAX);
  break;
  case Ir_Kind_gt:
    na = sem_ranges_clamp(ra, sem_sat_inc(bmin), I64_MAX);
    nb = sem_ranges_clamp(rb, I64_MIN, sem_sat_dec(amax));
  break;
  case Ir_Kind_ge:
    na = sem_ranges_clamp(ra, bmin, I64_MAX);
    nb = sem_ranges_clamp(rb, I64_MIN, amax);
  break;
  default:
    assert(0);
  break;
  }
  sem_type_of_ir_binary_narrow(tasks, ir, na, nb);
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
    Type_Pair types = type_of_ir_binary(cond_ir);
    if (types.one->kind == Type_Kind_int && types.two->kind == Type_Kind_int) {
      sem_type_narrow_int_ne(tasks, cond_ir);
    }
    else if (types.one->kind == Type_Kind_ptr && types.two->kind == Type_Kind_ptr) {
      sem_type_narrow_ptr_ne(tasks, cond_ir);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_lt: case Ir_Kind_le:
  case Ir_Kind_gt: case Ir_Kind_ge: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
      sem_type_narrow_int_cmp(tasks, cond_ir, cond_ir->kind);
    }
  } break;
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
  case Ir_Kind_lt: case Ir_Kind_le:
  case Ir_Kind_gt: case Ir_Kind_ge: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_int)) {
      sem_type_narrow_int_cmp(tasks, cond_ir, sem_cmp_negate(cond_ir->kind));
    }
  } break;
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
    if (pred->state == Block_State_reachable || block->state < Block_State_reachable) {
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
  if (var->kind == Var_Kind_constant) {
    printf("cannot assign a constant '%s'\n", var->name->base);
    assert(0);
  }
  if (!var->declared) {
    hash_map_put(&block->out_var_types, var, type);
  }
  else {
    if (type_is_subtype(block, type, var->declared)) {
      if (type->kind == Type_Kind_record) {
        for (I32 i = 0; i < type->record->length; i++) {
          Field field = type_record_get_by_position(block, type->record, i);
          Field var_field = type_record_get_by_name(block, var->declared->record, field.name);
          type_of_var_put(block, var_field.var, field.assigned_type);
        }
      }
      else {
        type->size_defined = var->declared->size_defined;
        type->bits_size = var->declared->bits_size;
        type->bits_align = var->declared->bits_align;
        hash_map_put(&block->out_var_types, var, type);
      }
    }
    else {
      printf("not subtype\n");
      assert(0);
    }
  }
}

Type* type_of_fun(Fun* fun) {
  return fun->type;
}

void sem_block(Block* block);
void sem_init_block_preds(Block* block);

void sem_record_declare_fields(Var* var, Type* type) {
  if (type->kind != Type_Kind_record) return;
  type->record->vars = arena_push(sem.perm_arena, type->record->length*sizeof(Var*));
  for (I32 i = 0; i < type->record->length; i++) {
    Var* field_var = arena_push_zero(sem.perm_arena, sizeof(Var));
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
    I32 cap = sem.current_fun_var_count;
    for (I32 i = 0; i < var->blocks->length; i++) {
      Block* block = var->blocks->base[i];
      sem_init_block_preds(block);
      block->out_var_types = hash_map_init(sem.perm_arena, cap);
      sem_block(block);
    }
  }

  Type* type = type_of_ir(var->declared_ir);
  var->declared = type;
  if (type) {
    if (type_is_const(type)) {
      var->kind = Var_Kind_constant;
    }
    else {
      var->kind = Var_Kind_declared;
    }
    sem_record_declare_fields(var, type);
  }

  var->state = Var_State_resolved;
}

Type* sem_fun(Fun* fun);
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
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_int && types.two->kind == Type_Kind_int) {
      // TODO: overflow/underflow
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);
      I64 max_one = ranges_max(pair.one);
      I64 max_two = ranges_max(pair.two);
      I64 max = max_one + max_two;
      I64 min_one = ranges_min(pair.one);
      I64 min_two = ranges_min(pair.two);
      I64 min = min_one + min_two;
      result = type_range(min, max);
      if (types.one->size_defined && types.two->size_defined) {
        I16 bits_max_size = max(types.one->bits_size, types.two->bits_size);
        if (result->bits_size > bits_max_size) {
          min = bits_min(bits_max_size);
          max = bits_max(bits_max_size);
          result->bits_size = bits_max_size;
          result->bits_align = bits_max_size;
          result->ranges->pairs[0].lo = min;
          result->ranges->pairs[0].hi = max;
        }
      }
      else if (types.one->size_defined) {
        I16 bits_max_size = types.one->bits_size;
        if (result->bits_size > bits_max_size) {
          min = bits_min(bits_max_size);
          max = bits_max(bits_max_size);
          result->bits_size = bits_max_size;
          result->bits_align = bits_max_size;
          result->ranges->pairs[0].lo = min;
          result->ranges->pairs[0].hi = max;
        }
      }
      else if (types.two->size_defined) {
        I16 bits_max_size = types.two->bits_size;
        if (result->bits_size > bits_max_size) {
          min = bits_min(bits_max_size);
          max = bits_max(bits_max_size);
          result->bits_size = bits_max_size;
          result->bits_align = bits_max_size;
          result->ranges->pairs[0].lo = min;
          result->ranges->pairs[0].hi = max;
        }
      }
    }
  } break;
  case Ir_Kind_sub: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_int && types.two->kind == Type_Kind_int) {
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);
      I64 max_one = ranges_max(pair.one);
      I64 max_two = ranges_max(pair.two);
      I64 max = max_one - max_two;
      I64 min_one = ranges_min(pair.one);
      I64 min_two = ranges_min(pair.two);
      I64 min = min_one - min_two;
      result = type_range(min, max);
      if (types.one->size_defined && types.two->size_defined) {
        I16 bits_max_size = max(types.one->bits_size, types.two->bits_size);
        result->bits_size  = bits_max_size;
        result->bits_align = bits_max_size;
        result->ranges->pairs[0].lo = bits_min(bits_max_size);
        result->ranges->pairs[0].hi = bits_max(bits_max_size);
      }
      else if (types.one->size_defined) {
        I16 bits_max_size = types.one->bits_size;
        result->bits_size  = bits_max_size;
        result->bits_align = bits_max_size;
        result->ranges->pairs[0].lo = bits_min(bits_max_size);
        result->ranges->pairs[0].hi = bits_max(bits_max_size);
      }
      else if (types.two->size_defined) {
        I16 bits_max_size = types.two->bits_size;
        result->bits_size  = bits_max_size;
        result->bits_align = bits_max_size;
        result->ranges->pairs[0].lo = bits_min(bits_max_size);
        result->ranges->pairs[0].hi = bits_max(bits_max_size);
      }
    }
  } break;
  case Ir_Kind_eq: case Ir_Kind_ne:
  case Ir_Kind_lt: case Ir_Kind_le:
  case Ir_Kind_gt: case Ir_Kind_ge: {
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_int)) {
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);
      if (ranges_is_single(pair.one) && ranges_is_single(pair.two)) {
        I64 x = ranges_min(pair.one);
        I64 y = ranges_min(pair.two);
        I64 val = 0;
        switch (ir->kind) {
        case Ir_Kind_eq: val = x == y; break;
        case Ir_Kind_ne: val = x != y; break;
        case Ir_Kind_lt: val = x <  y; break;
        case Ir_Kind_le: val = x <= y; break;
        case Ir_Kind_gt: val = x >  y; break;
        case Ir_Kind_ge: val = x >= y; break;
        default: break;
        }
        result = type_int(val);
      }
      else {
        result = type_range(0, 1);
      }
    }
  } break;
  case Ir_Kind_record: {
    result = type_record(ir->record);
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
  case Ir_Kind_fun: {
    I32 save_count = sem.current_fun_var_count;
    sem.current_fun_var_count = ir->fun->var_count;
    result = sem_fun(ir->fun);
    sem.current_fun_var_count = save_count;
  } break;
  case Ir_Kind_call: {
    Type_Pair types = type_of_ir_binary(ir);
    Type* call = types.one;
    Type* arg  = types.two;
    if (call->kind == Type_Kind_fun) {
      Function* fun = call->fun;
      // if (fun->arg->kind == Type_Kind_record && fun->arg->record->length == 1 && arg->kind != Type_Kind_record) {
      //   Field field_two = type_record_get_by_position(block, fun->arg->record, 0);
      //   if (type_is_subtype(block, arg, field_two.declared_type)) {
      //     result = call->fun->ret;
      //   }
      //   else {
      //     printf("call not subtype\n");
      //     assert(0);
      //   }
      // }
      // else
      if (type_is_subtype(block, arg, fun->arg)) {
        result = fun->ret;
      }
      else {
        printf("call not subtype\n");
        assert(0);
      }
    }
    else {
      assert(0);
    }
  } break;

  case Ir_Kind_range: {
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_int)) {
      Ranges_Pair pair = ranges_pair_of_ir_binary(ir);

      if (ranges_is_single(pair.one) && ranges_is_single(pair.two)) {
        I64 val_one = ranges_min(pair.one);
        I64 val_two = ranges_min(pair.two);
        if (val_one < val_two) {
          result = type_range(val_one, val_two);
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
          result = type_define_size(val, one_type);
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
  if (block->state == Block_State_reachable) {
    if (block->kind == Block_Kind_branch) {
      Type* cond_type = type_of_ir(block->branch.cond);
      if (type_is_true(cond_type)) {
        block->branch.nez.to_block->state = Block_State_reachable;
        if (block->branch.eqz.to_block->state == Block_State_unknown) {
          block->branch.eqz.to_block->state = Block_State_unreachable;
        }
      }
      else if (type_is_false(cond_type)) {
        block->branch.eqz.to_block->state = Block_State_reachable;
        if (block->branch.nez.to_block->state == Block_State_unknown) {
          block->branch.nez.to_block->state = Block_State_unreachable;
        }
      }
      else {
        block->branch.nez.to_block->state = Block_State_reachable;
        block->branch.eqz.to_block->state = Block_State_reachable;
      }
    }
    else if (block->kind == Block_Kind_jump) {
      block->jump.to_block->state = Block_State_reachable;
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

typedef struct Loop Loop;
struct Loop {
  Block*   header;
  Block*   preheader;
  Block*   latch;
  Block*   exit;
  Block*   body_entry;
  Hash_Set body;
  Loop*    parent;
  I32      depth;
};

typedef struct Loops Loops;
struct Loops {
  Loop* base;
  I32   length;
};

typedef struct Loop_Var Loop_Var;
struct Loop_Var {
  Var* var;
  Ir*  store;
  I32  store_count;
  I64  step;
  B8   clean;
  B8   affine;
};

typedef struct Loop_Count Loop_Count;
struct Loop_Count {
  B8  known;
  I64 n;
};

void sem_dfs_postorder(Block* block, Blocks* postorder) {
  if (block->is_dfs_visited) return;
  block->is_dfs_visited = true;
  if (block->kind == Block_Kind_jump) {
    sem_dfs_postorder(block->jump.to_block, postorder);
  }
  else if (block->kind == Block_Kind_branch) {
    sem_dfs_postorder(block->branch.nez.to_block, postorder);
    sem_dfs_postorder(block->branch.eqz.to_block, postorder);
  }
  fa_add(postorder, block);
}

Blocks* sem_cfg_rpo(Fun* fun) {
  I32 n = fun->blocks->length;
  Blocks* postorder = arena_push(sem.temp_arena, sizeof(Blocks) + n*sizeof(Block*));
  postorder->length = 0;
  sem_dfs_postorder(fun->blocks->base[0], postorder);

  Blocks* rpo = arena_push(sem.temp_arena, sizeof(Blocks) + postorder->length*sizeof(Block*));
  rpo->length = 0;
  for (I32 i = postorder->length-1; i >= 0; i--) {
    Block* block = postorder->base[i];
    block->rpo = rpo->length;
    fa_add(rpo, block);
  }
  return rpo;
}

Block* sem_dom_intersect(Block* one, Block* two) {
  while (one != two) {
    while (one->rpo > two->rpo) one = one->idom;
    while (two->rpo > one->rpo) two = two->idom;
  }
  return one;
}

void sem_cfg_doms(Fun* fun, Blocks* rpo) {
  for (I32 i = 0; i < fun->blocks->length; i++) {
    fun->blocks->base[i]->idom = 0;
  }
  Block* entry = rpo->base[0];
  entry->idom = entry;
  B8 changed = true;
  while (changed) {
    changed = false;
    for (I32 i = 1; i < rpo->length; i++) {
      Block* block = rpo->base[i];
      Block* new_idom = 0;
      for (I32 p = 0; p < block->preds.length; p++) {
        Block* pred = block->preds.base[p];
        if (pred->idom) {
          if (new_idom) {
            new_idom = sem_dom_intersect(new_idom, pred);
          }
          else {
            new_idom = pred;
          }
        }
      }
      if (new_idom && block->idom != new_idom) {
        block->idom = new_idom;
        changed = true;
      }
    }
  }
}

B8 sem_dominates(Block* one, Block* two) {
  for (;;) {
    if (two == one) return true;
    if (!two->idom || two->idom == two) return false;
    two = two->idom;
  }
}

Hash_Set sem_natural_loop(Fun* fun, Block* header, Block* latch) {
  I32 n = fun->blocks->length;
  Hash_Set body = hash_set_init(sem.temp_arena, n);
  hash_set_put(&body, header);
  Block** stack = arena_push(sem.temp_arena, n*sizeof(Block*));
  I32 sp = 0;
  if (hash_set_put(&body, latch)) {
    stack[sp++] = latch;
  }
  while (sp > 0) {
    Block* block = stack[--sp];
    for (I32 i = 0; i < block->preds.length; i++) {
      Block* pred = block->preds.base[i];
      if (hash_set_put(&body, pred)) {
        stack[sp++] = pred;
      }
    }
  }
  return body;
}

Loops sem_loops_discover(Fun* fun, Blocks* rpo) {
  I32 n = fun->blocks->length;
  Loops loops = {};
  loops.base = arena_push(sem.temp_arena, 2*n*sizeof(Loop));
  loops.length = 0;

  for (I32 i = 0; i < rpo->length; i++) {
    Block* tail = rpo->base[i];
    Block* succs[2];
    I32    ns = 0;
    if (tail->kind == Block_Kind_jump) {
      succs[ns++] = tail->jump.to_block;
    }
    else if (tail->kind == Block_Kind_branch) {
      succs[ns++] = tail->branch.nez.to_block;
      succs[ns++] = tail->branch.eqz.to_block;
    }
    for (I32 s = 0; s < ns; s++) {
      Block* head = succs[s];
      if (sem_dominates(head, tail)) {
        Loop* loop = &loops.base[loops.length++];
        memset(loop, 0, sizeof(Loop));
        loop->header = head;
        loop->latch  = tail;
        loop->body   = sem_natural_loop(fun, head, tail);
      }
    }
  }

  for (I32 l = 0; l < loops.length; l++) {
    Loop* loop = &loops.base[l];
    Block* head = loop->header;
    for (I32 p = 0; p < head->preds.length; p++) {
      Block* pred = head->preds.base[p];
      if (!hash_set_exists(&loop->body, pred)) {
        loop->preheader = pred;
      }
    }
    if (head->kind == Block_Kind_branch) {
      Block* nez = head->branch.nez.to_block;
      Block* eqz = head->branch.eqz.to_block;
      if (hash_set_exists(&loop->body, nez)) {
        loop->body_entry = nez;
        loop->exit       = eqz;
      }
      else {
        loop->body_entry = eqz;
        loop->exit       = nez;
      }
    }
  }

  for (I32 a = 0; a < loops.length; a++) {
    Loop* la = &loops.base[a];
    for (I32 b = 0; b < loops.length; b++) {
      if (a == b) continue;
      Loop* lb = &loops.base[b];
      if (lb->header == la->header) continue;
      if (hash_set_exists(&lb->body, la->header)) {
        if (!la->parent || lb->body.len < la->parent->body.len) {
          la->parent = lb;
        }
      }
    }
  }
  for (I32 a = 0; a < loops.length; a++) {
    Loop* la = &loops.base[a];
    I32 depth = 0;
    for (Loop* p = la->parent; p; p = p->parent) depth++;
    la->depth = depth;
  }

  return loops;
}

Hash_Map sem_loops_innermost(Fun* fun, Loops loops) {
  Hash_Map map = hash_map_init(sem.temp_arena, fun->blocks->length);
  for (I32 l = 0; l < loops.length; l++) {
    Loop* loop = &loops.base[l];
    for (I32 i = 0; i < loop->body.len; i++) {
      Block* block = loop->body.list[i];
      Loop* cur = hash_map_get(&map, block);
      if (!cur || loop->body.len < cur->body.len) {
        hash_map_put(&map, block, loop);
      }
    }
  }
  return map;
}

B8 sem_ir_is_load_of_var(Ir* ir, Var* var) {
  return ir->kind == Ir_Kind_load
      && ir->unary->kind == Ir_Kind_var
      && ir->unary->var == var;
}

B8 sem_loop_invariant_int(Loop* loop, Hash_Map* loopvars, Ir* ir, I64* out) {
  if (ir->kind == Ir_Kind_int) {
    *out = ir->i64;
    return true;
  }
  if (ir->kind == Ir_Kind_load && ir->unary->kind == Ir_Kind_var) {
    Var* var = ir->unary->var;
    if (hash_map_get(loopvars, var)) return false;
    sem_ensure_declared(var);
    Type* type = type_of_var(loop->preheader, var);
    if (type && type->kind == Type_Kind_int && ranges_is_single(type->ranges)) {
      *out = ranges_min(type->ranges);
      return true;
    }
  }
  return false;
}

B8 sem_loop_affine(Loop* loop, Hash_Map* loopvars, Var* var, Ir* store, I64* step_out) {
  Ir* rhs = store->binary.two;
  if (rhs->kind != Ir_Kind_add && rhs->kind != Ir_Kind_sub) return false;
  Ir* other     = 0;
  B8  self_left = false;
  if (sem_ir_is_load_of_var(rhs->binary.one, var)) {
    other     = rhs->binary.two;
    self_left = true;
  }
  else if (sem_ir_is_load_of_var(rhs->binary.two, var)) {
    other     = rhs->binary.one;
    self_left = false;
  }
  else {
    return false;
  }
  if (rhs->kind == Ir_Kind_sub && !self_left) return false;
  I64 val;
  if (!sem_loop_invariant_int(loop, loopvars, other, &val)) return false;
  *step_out = (rhs->kind == Ir_Kind_sub) ? -val : val;
  return true;
}

Loop_Count sem_loop_count(Loop* loop, Hash_Map* loopvars) {
  Loop_Count result = {};
  Ir* cond  = loop->header->branch.cond;
  if (cond->kind != Ir_Kind_eq && cond->kind != Ir_Kind_ne && cond->kind != Ir_Kind_lt
   && cond->kind != Ir_Kind_le && cond->kind != Ir_Kind_gt && cond->kind != Ir_Kind_ge) {
    return result;
  }
  Ir* cond_lhs = cond->binary.one;
  Ir* cond_rhs = cond->binary.two;

  Var* v      = 0;
  Ir*  bound_ir = 0;
  B8   v_left   = false;
  if (cond_lhs->kind == Ir_Kind_load && cond_lhs->unary->kind == Ir_Kind_var) {
    Loop_Var* lv = hash_map_get(loopvars, cond_lhs->unary->var);
    if (lv && lv->affine) { v = cond_lhs->unary->var; bound_ir = cond_rhs; v_left = true; }
  }
  if (!v && cond_rhs->kind == Ir_Kind_load && cond_rhs->unary->kind == Ir_Kind_var) {
    Loop_Var* lv = hash_map_get(loopvars, cond_rhs->unary->var);
    if (lv && lv->affine) { v = cond_rhs->unary->var; bound_ir = cond_lhs; v_left = false; }
  }
  if (!v) return result;

  I64 bound;
  if (!sem_loop_invariant_int(loop, loopvars, bound_ir, &bound)) return result;

  Loop_Var* lv = hash_map_get(loopvars, v);
  I64 step = lv->step;

  sem_ensure_declared(v);
  Type* v0_type = type_of_var(loop->preheader, v);
  if (!v0_type || v0_type->kind != Type_Kind_int || !ranges_is_single(v0_type->ranges)) {
    return result;
  }
  I64 v0 = ranges_min(v0_type->ranges);

  Ir_Kind op = cond->kind;
  if (!v_left) {
    switch (cond->kind) {
    case Ir_Kind_lt: op = Ir_Kind_gt; break;
    case Ir_Kind_le: op = Ir_Kind_ge; break;
    case Ir_Kind_gt: op = Ir_Kind_lt; break;
    case Ir_Kind_ge: op = Ir_Kind_le; break;
    default:         op = cond->kind;          break;
    }
  }

  I64 d = bound - v0;
  switch (op) {
  case Ir_Kind_ne: {
    if (step == 0) {
      if (d == 0) { result.known = true; result.n = 0; }
      return result;
    }
    if (d % step == 0 && d / step >= 0) {
      result.known = true;
      result.n     = d / step;
    }
    return result;
  }
  case Ir_Kind_eq: {
    if (v0 != bound) { result.known = true; result.n = 0; return result; }
    if (step == 0) return result;
    result.known = true; result.n = 1;
    return result;
  }
  case Ir_Kind_lt: {
    if (v0 >= bound) { result.known = true; result.n = 0; return result; }
    if (step <= 0) return result;
    result.known = true; result.n = (d + step - 1) / step;
    return result;
  }
  case Ir_Kind_le: {
    if (v0 > bound) { result.known = true; result.n = 0; return result; }
    if (step <= 0) return result;
    result.known = true; result.n = d / step + 1;
    return result;
  }
  case Ir_Kind_gt: {
    if (v0 <= bound) { result.known = true; result.n = 0; return result; }
    if (step >= 0) return result;
    I64 dd = v0 - bound;
    I64 ss = -step;
    result.known = true; result.n = (dd + ss - 1) / ss;
    return result;
  }
  case Ir_Kind_ge: {
    if (v0 < bound) { result.known = true; result.n = 0; return result; }
    if (step >= 0) return result;
    I64 dd = v0 - bound;
    I64 ss = -step;
    result.known = true; result.n = dd / ss + 1;
    return result;
  }
  default: return result;
  }
}

void sem_loop_seed(Loop* loop, Hash_Map* innermost) {
  if (!loop->preheader) return;
  if (loop->header->kind != Block_Kind_branch) return;

  I32 cap = sem.current_fun_var_count + 1;
  Hash_Map  loopvars = hash_map_init(sem.temp_arena, cap);
  Loop_Var* pool     = arena_push(sem.temp_arena, cap*sizeof(Loop_Var));
  I32       pool_len = 0;

  for (I32 i = 0; i < loop->body.len; i++) {
    Block* block = loop->body.list[i];
    for (I32 j = 0; j < block->irs->length; j++) {
      Ir* ir = block->irs->base[j];
      if (ir->kind != Ir_Kind_store) continue;
      if (ir->binary.one->kind != Ir_Kind_var) continue;
      Var* var = ir->binary.one->var;
      Loop_Var* lv = hash_map_get(&loopvars, var);
      if (!lv) {
        lv = &pool[pool_len++];
        memset(lv, 0, sizeof(Loop_Var));
        lv->var   = var;
        lv->clean = true;
        hash_map_put(&loopvars, var, lv);
      }
      lv->store_count++;
      lv->store = ir;
      B8 exclusive = (Loop*)hash_map_get(innermost, block) == loop;
      B8 once      = sem_dominates(block, loop->latch);
      if (!exclusive || !once) lv->clean = false;
    }
  }

  for (I32 i = 0; i < pool_len; i++) {
    Loop_Var* lv = &pool[i];
    if (lv->store_count == 1 && lv->clean) {
      I64 step;
      if (sem_loop_affine(loop, &loopvars, lv->var, lv->store, &step)) {
        lv->affine = true;
        lv->step   = step;
      }
    }
  }

  Loop_Count count = sem_loop_count(loop, &loopvars);

  for (I32 i = 0; i < pool_len; i++) {
    Loop_Var* lv  = &pool[i];
    Var*      var = lv->var;
    sem_ensure_declared(var);
    Type* seeded = 0;
    if (lv->affine && count.known) {
      Type* v0_type = type_of_var(loop->preheader, var);
      if (v0_type && v0_type->kind == Type_Kind_int && ranges_is_single(v0_type->ranges)) {
        I64 v0    = ranges_min(v0_type->ranges);
        I64 final = v0 + count.n * lv->step;
        seeded = type_range(min(v0, final), max(v0, final));
        if (var->declared) {
          seeded->size_defined = var->declared->size_defined;
          seeded->bits_size    = var->declared->bits_size;
          seeded->bits_align   = var->declared->bits_align;
        }
      }
    }
    if (!seeded) {
      seeded = var->declared ? var->declared : sem.type_none;
    }
    hash_map_put(&loop->header->out_var_types, var, seeded);
  }
}

Type* sem_fun(Fun* fun) {
  Type* fun_type = type_of_fun(fun);
  if (fun_type) return fun_type;

  sem_ensure_declared(fun->arg_var);

  Block* entry_block = fun->blocks->base[0];
  entry_block->state = Block_State_reachable;
  for (I32 b = 0; b < fun->blocks->length; b++) {
    Block* block = fun->blocks->base[b];
    sem_init_block_preds(block);
    sem_scc_block(block);
    block->out_var_types = hash_map_init(sem.perm_arena, fun->var_count);
    block->assigned      = hash_map_init(sem.perm_arena, fun->var_count);
  }

  Blocks*  rpo       = sem_cfg_rpo(fun);
                       sem_cfg_doms(fun, rpo);
  Loops    loops     = sem_loops_discover(fun, rpo);
  Hash_Map innermost = sem_loops_innermost(fun, loops);

  for (I32 i = 0; i < rpo->length; i++) {
    Block* block = rpo->base[i];
    for (I32 l = 0; l < loops.length; l++) {
      if (loops.base[l].header == block) {
        sem_loop_seed(&loops.base[l], &innermost);
      }
    }
    sem_block(block);
  }

  Type* new_type = &new(sem.types);
  new_type->kind = Type_Kind_fun;
  new_type->fun = arena_push(sem.perm_arena, sizeof(Function));
  new_type->fun->arg = fun->arg_var->declared;
  new_type->fun->ret = type_of_var(fun->ret_block, fun->ret_ir->var);
  fun->ret_ir->var->declared = new_type->fun->ret;
  Type* ret_type = type_of_ir(fun->ret_ir);
  assert(ret_type->kind == Type_Kind_ptr);
  ret_type->pointer->declared = new_type->fun->ret;
  fun->type = new_type;
  return new_type;
}

void sem_funs(Arena* arena, Funs funs) {
  Arena temp = arena_init(arena->capacity);
  sem.perm_arena = arena;
  sem.temp_arena = &temp;
  sem.funs = funs;
  sem.sccid = 0;
  sem.scc_stack = arena_push(arena, sizeof(Blocks) + sizeof(Block*)*irgen.blocks.length);
  sem.scc_stack->length = 0;
  sem.type_of_irs = hash_map_init(arena, irgen.irs.length);
  sem.types.base = arena_push(arena, irgen.irs.length * sizeof(Type));
  sem.types.length = 0;

  sem.type_none = &new(sem.types);
  sem.type_none->kind = Type_Kind_none;

  sem.current_fun_var_count = irgen.builtins->len;
  // for (I32 i = 0; i < irgen.builtins->len; i++) {
  //   Str* str = irgen.builtins->list[i];
  //   Symbol* sym = hash_map_get(irgen.builtins, str);
  //   sem_ensure_declared(sym->var_ir->var);
  // }

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    sem.current_fun_var_count = fun->var_count;
    sem_fun(fun);
  }
  arena_free(&temp);
}

void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  I32 source_length    = strlen(source) + 32;
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
  // test("a:I32; b: I32; a = b; a", "");
  // test("a:I32; a=0; wh a < 8 do {a = a + 1}; a", "");
  // test("a:I32; a=0; wh a != 8 do {a = a + 2}; a", "");
  // test("a:I32; a=0; wh a != 8 do {a = a + 3}; a", "");
  // test("a:I32; a=5; wh a > 0 do {a = a - 1}; a", "");
  // test("a:I32; b:I32; a=0; wh a < 8 do {b=0; wh b != 3 do { b = b + 1 }; a = a + 1}; a", "");
  // test("a:I32; b:I32; a=0; b=0; wh a < 8 do {wh b != 3 do { b = b + 1 }; a = a + 3}; a+b", "");
  // test("a:I32; if a != 10 do {a = 1}; a", "");
  // test("I32 = 0; I32", "");
  // test("Vec2 : (x:I32; y:I32); Vec2 = (x=1+2; y=2+3); Vec2.x + Vec2.y", "");
  // test("a:I32 = 2; a=3; a+a", "");
  // test("foo:(a:I32) -> a+1; foo(2)", "");
  // test("foo:(a:I32) -> { if 1\\2 re 2 el re 3 }; foo(2)", "");
  // test("foo:() I32 -> I32 bar(); bar:()->foo()", "");
  // test("if 1\\2 do 3 el 4;", "");
  // test("a:(x:1\\2; y:3\\4); a = (y=3; x=1); a.x", "");
  // test("a:(x:I32; y:I32); a = (y=1; x=2); a.x", "");
  // test("a:I32; b:@I32; b = @a; b@ = 1", "");
  // test("a:I32; foo:() -> a;", "");
  // test("putchar: #c putchar (char:I32) -> I32", "");
  test("a : 1; a = 1", "");
}

#undef test
