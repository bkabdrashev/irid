typedef struct Range Range;
struct Range {
  I64 lo;
  I64 hi;
};

typedef enum Scalar_Kind {
  Scalar_Kind_int,
  Scalar_Kind_float,
} Scalar_Kind;

typedef struct Scalar Scalar;
struct Scalar {
  Scalar_Kind kind;
  I16         bits_exp;
  I16         bits_man;
  I32         length;
  Range       pairs[];
};

typedef struct Scalar_Pair Scalar_Pair;
struct Scalar_Pair { Scalar* one; Scalar* two; };

typedef struct Pointer Pointer;
struct Pointer {
  Type* declared;
  Hash_Set stack_vars;
};

typedef struct Pointer_Pair Pointer_Pair;
struct Pointer_Pair { Pointer* one; Pointer* two; };

typedef struct Function Function;
struct Function {
  Fun* fun;
  Str* foreign_name;
  I32  foreign_id; // TODO: it is better not to create foreign_id; instead rely on foreign_name since it should be unique per declared
  Type* arg;
  Type* ret;
};

typedef struct Functions Functions;
struct Functions {
  I32       length;
  Function* base[];
};

typedef struct Field Field;
struct Field {
  Str*    name;
  I32     pos;
  Ir*     ir;
  Type*   type;
  I32     offset;
};

typedef struct Record Record;
struct Record {
  I32      length;
  Str**    names;
  Type**   types;
  // NOTE: computed below
  I32*     offsets;
  Hash_Map position_from_name;
  Hash_Map position_from_offset;
};

typedef struct Join Join;
struct Join {
  Hash_Set  records;
};

typedef struct Types Types;
struct Types {
  I32    length;
  Type*  base[];
};

typedef struct Array Array;
struct Array {
  I32    length;
  Type*  of_type;
  Type*  types[];
};

typedef struct Span Span;
struct Span {
  Type*  length;
  Type*  of_type;
};

typedef enum Type_Kind {
  Type_Kind_none,
  Type_Kind_type,
  Type_Kind_scalar,
  Type_Kind_ptr,
  Type_Kind_array,
  Type_Kind_span,
  Type_Kind_record,
  Type_Kind_fun,
  Type_Kind_join_records,
} Type_Kind;

typedef enum Type_Flag {
  Type_Flag_size_defined    = 1 << 0,
  Type_Flag_comptime_const  = 1 << 1,
} Type_Flag;

struct Type {
  Type_Kind kind;
  Type_Flag flag;
  I16 bits_align;
  I16 bits_size;
  union {
    U64       value;
    Type*     type;
    Pointer*  pointer;
    Array*    array;
    Span*     span;
    Scalar*   scalar;
    Record*   record;
    Function* function;
    Str*      str;
    Join*     join;
  };
};

typedef struct Type_Pool Type_Pool;
struct Type_Pool {
  Type* base;
  I32   length;
};

typedef struct Sem_Narrowed Sem_Narrowed;
struct Sem_Narrowed {
  Var*          var;
  Type*         type;
  Sem_Narrowed* next;
};
typedef struct Sem_Tasks Sem_Tasks;
struct Sem_Tasks {
  Block*        block;
  Type*         type;
  Var*          var;
  Sem_Narrowed* narrowed;
};

typedef struct Ignored_Blocks Ignored_Blocks;
struct Ignored_Blocks {
  B8*     bits;       // indexed by block->id, capacity = block_count
  Block** trail;      // blocks added since the root
  I32     trail_len;
  I32     capacity;
};

typedef struct Sem Sem;
struct Sem {
  Arena* temp_arena;
  Arena* perm_arena;
  Funs   funs;
  Functions* foreign_funs;
  Hash_Map  type_of_irs;
  Type_Pool types;

  Hash_Set  type_set;
  Hash_Set  type_type_set;
  Hash_Set  record_set;
  Hash_Set  pointer_set;
  Hash_Set  scalar_set;
  Hash_Set  array_set;
  Hash_Set  span_set;
  Hash_Set  function_set;
  Hash_Set  join_set;

  Type* type_none;
  Type* type_empty_record;
  Type* type_zero;
  Type* type_len;
  Type* fun_len;
  Type* fun_bits;
  Type* fun_float;
  Type* fun_type_type;
  Type* rec_foreign;
  Type* fun_foreign_c;
  Type* bytes_range;

  Type** elements;

  Str*  str_ptr;
  Str*  str_c;
  Fun*  str_from_i8_array;
  struct {
    Type* len;
  } builtin;

  I32 sccid;
  Blocks* scc_stack;
  Fun* current_fun;

  Diags* diags;
  jmp_buf fatal;
  B8 fatal_ready;
  Ir* current_ir;
  Loc current_loc;
  Block* current_block;
};

Sem sem = {};

typedef struct Type_Pair Type_Pair;
struct Type_Pair { Type* one; Type* two; };

Field type_record_get_by_name(Record* record, Str* name);
Field type_record_get_by_position(Record* record, I32 pos);
Type* type_of_var(Block* block, Var* var);
Type* type_of_ir(Ir* ir);
Type* type_join(Type* one, Type* two);
Type* type_meet(Type* one, Type* two);
Type* type_define_size(I32 bits_size, Type* bits_of);
Type* type_pointer_declared(Pointer* pointer);
Type* sem_var_declare(Var* var);
B8 scalar_is_single(Scalar* scalar);
I64 scalar_min(Scalar* scalar);
Type* type_int(I64 i64);
Scalar* sem_scalar_merge(Scalar* one, Scalar* two);
Scalar* sem_scalar_meet(Scalar* one, Scalar* two);
Scalar* sem_scalar_float_widen(Scalar* scalar);
Scalar* sem_scalar_int_to_float(Scalar* scalar, I16 bits_size, I16 bits_man);
B8 type_is_float(Type* type);
Type* type_float_cast(Type* from, Type* to);

void string_builder_push_scalar(String_Builder* sb, Scalar* scalar) {
  if (scalar->length > 1) {
    string_builder_push_cstr(sb, "(");
  }
  for (I32 i = 0; i < scalar->length; i++ ) {
    I64 one = scalar->pairs[i].lo;
    I64 two = scalar->pairs[i].hi;
    if (one == two) {
      string_builder_push_i64(sb, one);
    }
    else {
      string_builder_push_i64(sb, one);
      string_builder_push_cstr(sb, "..");
      string_builder_push_i64(sb, two);
    }
    if (i+1 < scalar->length) {
      string_builder_push_cstr(sb, "\\");
    }
  }
  if (scalar->length > 1) {
    string_builder_push_cstr(sb, ")");
  }
}

void string_builder_push_type(String_Builder* sb, Block* block, Type* type) {
  if (!type) return;
  string_builder_push_i64(sb, type->bits_size);
  if (has_flag(type->flag, Type_Flag_size_defined)) {
    string_builder_push_cstr(sb, "'");
  }
  else {
    string_builder_push_cstr(sb, " ");
  }
  if (type->kind == Type_Kind_scalar && type->scalar->kind == Scalar_Kind_float) {
    string_builder_push_cstr(sb, "float ");
  }
  else {
    string_builder_push_cstr(sb, "bits ");
  }
  switch (type->kind) {
  case Type_Kind_none:
    string_builder_push_cstr(sb, "none");
  break;
  case Type_Kind_type:
    string_builder_push_cstr(sb, "type ");
    string_builder_push_type(sb, block, type->type);
  break;
  case Type_Kind_fun: {
    string_builder_push_type(sb, block, type->function->arg);
    string_builder_push_cstr(sb, " -> ");
    string_builder_push_type(sb, block, type->function->ret);
  } break;
  case Type_Kind_scalar: {
    if (type->scalar->kind == Scalar_Kind_float) {
      string_builder_push_cstr(sb, "f");
      string_builder_push_i64(sb, type->scalar->bits_exp);
      string_builder_push_cstr(sb, ";");
      string_builder_push_i64(sb, type->scalar->bits_man);
      string_builder_push_cstr(sb, " ");
    }
    string_builder_push_scalar(sb, type->scalar);
  } break;
  case Type_Kind_ptr: {
    // Type* declared = type_pointer_declared(type->pointer);
    string_builder_push_cstr(sb, "@");
    // string_builder_push_type(sb, block, declared);

    if (type->pointer->stack_vars.len) {
      string_builder_push_cstr(sb, "|");
      Hash_Set stack = type->pointer->stack_vars;
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
      field.name = record->names[pos];
      field.type = record->types[pos];

      if (field.name) {
        string_builder_push_str(sb, field.name);
        string_builder_push_cstr(sb, "'");
        string_builder_push_i64(sb, field.offset);
        string_builder_push_cstr(sb, ":");
        string_builder_push_type(sb, block, field.type);
      }
      else {
        string_builder_push_type(sb, block, field.type);
      }
      if (pos+1 < record->length) {
        string_builder_push_cstr(sb, ", ");
      }
    }
    string_builder_push_cstr(sb, ")");
  } break;
  case Type_Kind_span: {
    string_builder_push_cstr(sb, "[");
    string_builder_push_type(sb, block, type->span->length);
    string_builder_push_cstr(sb, "]");
    string_builder_push_cstr(sb, "(");
    string_builder_push_type(sb, block, type->span->of_type);
    string_builder_push_cstr(sb, ")");
  } break;
  case Type_Kind_array: {
    string_builder_push_cstr(sb, "[");
    string_builder_push_i64(sb, type->array->length);
    string_builder_push_cstr(sb, "]");
    string_builder_push_cstr(sb, "(");
    string_builder_push_type(sb, block, type->array->of_type);
    string_builder_push_cstr(sb, ")");
    // I32 length = type->array->length;
    // string_builder_push_cstr(sb, "(");
    // for (I32 i = 0; i < length; i++) {
    //   Type* item = type->array->types[i];
    //   string_builder_push_type(sb, block, item);
    //   if (i+1 < length) {
    //     string_builder_push_cstr(sb, ", ");
    //   }
    // }
    // string_builder_push_cstr(sb, ")");
  } break;
  case Type_Kind_join_records: {
    Join* join = type->join;
    string_builder_push_cstr(sb, "join ");
    string_builder_push_cstr(sb, "(");
    for (I32 i = 0; i < join->records.len; i++) {
      Type* element = join->records.list[i];
      string_builder_push_type(sb, block, element);
      if (i+1 < join->records.len) {
        string_builder_push_cstr(sb, " \\ ");
      }
    }
    string_builder_push_cstr(sb, ")");
  } break;
  }
}

Cstr cstr_from_sem(Arena* arena, Funs funs) {
  String_Builder sb = string_builder_begin(arena);
  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &irgen.funs.base[f];
    string_builder_push_fun(&sb, fun);
    if (fun->kind == Fun_Kind_macro) {
      string_builder_push_cstr(&sb, " #macro ");
      continue;
    }
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

      for (I32 i = 0; i < block->sem_irs->length; i++) {
        Ir* ir = block->sem_irs->base[i];
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
    string_builder_push_cstr(&sb, "\n");
    string_builder_push_indent(&sb, 2);
    string_builder_push_cstr(&sb, "ret");
    string_builder_push_cstr(&sb, "\n}\n");

    for (I32 i = 0; i < fun->vars->length; i++) {
      Var* var = fun->vars->base[i];
      string_builder_push_var(&sb, var);
      string_builder_push_cstr(&sb, " = { ");
      for (I32 b = 0; b < fun->blocks->length; b++) {
        Block* block = fun->blocks->base[b];
        Type* type = var->block_types[block->id];
        if (type) {
          string_builder_push_blockid(&sb, block);
          string_builder_push_cstr(&sb, ": ");
          string_builder_push_type(&sb, block, type);
          if (b + 1 < fun->blocks->length) {
            string_builder_push_cstr(&sb, ", ");
          }
        }
      }
      string_builder_push_cstr(&sb, "}\n");
    }
  }
  Cstr result = string_builder_end(&sb);
  return result;
}

Cstr sem_type_name(Type* type) {
  if (!type) return "unknown";
  Arena* temp = sem.temp_arena;
  String_Builder sb = string_builder_begin(temp);
  string_builder_push_type(&sb, sem.current_block, type);
  Cstr built = string_builder_end(&sb);
  C8* result = arena_push(sem.perm_arena, sb.size + 1);
  memcpy(result, built, sb.size + 1);
  return result;
}

Loc sem_loc(Ir* ir) {
  if (ir && loc_is_valid(ir->loc)) return ir->loc;
  if (loc_is_valid(sem.current_loc)) return sem.current_loc;
  return LOC_NONE;
}

Loc sem_loc_for_var(Var* var) {
  if (var && var->declared_ir && loc_is_valid(var->declared_ir->loc)) {
    return var->declared_ir->loc;
  }
  return sem_loc(0);
}

void sem_fail(void) {
  if (sem.fatal_ready) {
    longjmp(sem.fatal, 1);
  }
  assert(0);
}

Diag* sem_diag_impl(Diag_Code code, Loc loc, Cstr compiler_file, I32 compiler_line) {
  return diag_error_impl(sem.diags, code, loc, compiler_file, compiler_line);
}

void sem_error_name_impl(Diag_Code code, Loc loc, Str* name,
                         Cstr compiler_file, I32 compiler_line) {
  Diag* diag = sem_diag_impl(code, loc, compiler_file, compiler_line);
  if (diag) diag->data.name.name = name;
  sem_fail();
}

void sem_error_text_impl(Diag_Code code, Loc loc, Cstr text,
                         Cstr compiler_file, I32 compiler_line) {
  Diag* diag = sem_diag_impl(code, loc, compiler_file, compiler_line);
  if (diag) diag->data.text.text = text;
  sem_fail();
}

void sem_error_types_impl(Diag_Code code, Loc loc, Cstr expected, Cstr found,
                          Cstr compiler_file, I32 compiler_line) {
  Diag* diag = sem_diag_impl(code, loc, compiler_file, compiler_line);
  if (diag) {
    diag->data.types.expected = expected;
    diag->data.types.found    = found;
  }
  sem_fail();
}

void sem_error_type_impl(Diag_Code code, Loc loc, Type* type,
                         Cstr compiler_file, I32 compiler_line) {
  sem_error_types_impl(code, loc, 0, sem_type_name(type),
                       compiler_file, compiler_line);
}

void sem_error_operand_impl(Loc loc, Cstr op, Type* type,
                            Cstr compiler_file, I32 compiler_line) {
  Diag* diag = sem_diag_impl(Diag_Code_invalid_operand, loc,
                             compiler_file, compiler_line);
  if (diag) {
    diag->data.operand.op   = op;
    diag->data.operand.type = sem_type_name(type);
  }
  sem_fail();
}

void sem_error_type_mismatch_impl(Loc loc, Type* expected, Type* found,
                                  Cstr compiler_file, I32 compiler_line) {
  sem_error_types_impl(Diag_Code_type_mismatch, loc,
                       sem_type_name(expected), sem_type_name(found),
                       compiler_file, compiler_line);
}

void sem_error_call_mismatch_impl(Loc loc, Type* expected, Type* found,
                                  Cstr compiler_file, I32 compiler_line) {
  sem_error_types_impl(Diag_Code_call_argument_mismatch, loc,
                       sem_type_name(expected), sem_type_name(found),
                       compiler_file, compiler_line);
}

#define sem_diag(code, loc) \
  sem_diag_impl((code), (loc), __FILE__, __LINE__)
#define sem_error_name(code, loc, name) \
  sem_error_name_impl((code), (loc), (name), __FILE__, __LINE__)
#define sem_error_text(code, loc, text) \
  sem_error_text_impl((code), (loc), (text), __FILE__, __LINE__)
#define sem_error_types(code, loc, expected, found) \
  sem_error_types_impl((code), (loc), (expected), (found), __FILE__, __LINE__)
#define sem_error_type(code, loc, type) \
  sem_error_type_impl((code), (loc), (type), __FILE__, __LINE__)
#define sem_error_operand(loc, op, type) \
  sem_error_operand_impl((loc), (op), (type), __FILE__, __LINE__)
#define sem_error_type_mismatch(loc, expected, found) \
  sem_error_type_mismatch_impl((loc), (expected), (found), __FILE__, __LINE__)
#define sem_error_call_mismatch(loc, expected, found) \
  sem_error_call_mismatch_impl((loc), (expected), (found), __FILE__, __LINE__)
#define sem_error_binary_operand(ir, op) \
  sem_error_binary_operand_impl((ir), (op), __FILE__, __LINE__)
#define sem_error_unary_operand(ir, op) \
  sem_error_unary_operand_impl((ir), (op), __FILE__, __LINE__)

I64 scalar_max(Scalar* scalar) {
  return scalar->pairs[scalar->length-1].hi;
}

I64 scalar_min(Scalar* scalar) {
  return scalar->pairs[0].lo;
}

B8 scalar_is_single(Scalar* scalar) {
  I64 min = scalar_min(scalar);
  I64 max = scalar_max(scalar);
  return min == max;
}

Range scalar_limits(Scalar* scalar) {
  Range range = { .lo = scalar_min(scalar), .hi = scalar_max(scalar) };
  return range;
}

B8 scalar_have(Scalar* scalar, I64 i64) {
  for (I32 i = 0; i < scalar->length; i++) {
    Range pair = scalar->pairs[i];
    if (pair.lo <= i64 && i64 <= pair.hi) {
      return true;
    }
  }
  return false;
}

Scalar* sem_scalar_init(I32 max_len) {
  Scalar* result = arena_push(sem.perm_arena, sizeof(Scalar) + max_len * sizeof(Range));
  result->length = 0;
  return result;
}

Scalar* scalar_reflow(Range range, I16 bits) {
  Scalar* result = sem_scalar_init(2);
  I64 lo_converted = bits_convert(range.lo, bits);
  I64 hi_converted = bits_convert(range.hi, bits);
  if (lo_converted == range.lo && hi_converted == range.hi) {
    result->length = 1;
    result->pairs[0] = range;
  }
  else if ((lo_converted < range.lo && hi_converted == range.hi && range.hi < lo_converted-1)
       ||  (hi_converted > range.hi && lo_converted == range.lo && range.lo > hi_converted+1)) {
    // -128..118 \ 120..127
    result->length = 2;
    result->pairs[0].lo = bits_min(bits);
    result->pairs[0].hi = hi_converted;
    result->pairs[1].lo = lo_converted;
    result->pairs[1].hi = bits_max(bits);
  }
  else {
    // -128..127 or -128..119 \ 120..127
    result->length = 1;
    result->pairs[0].lo = bits_min(bits);
    result->pairs[0].hi = bits_max(bits);
  }
  return result;
}

B8 scalar_is_subrange(Scalar* one, Scalar* two) {
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

B8 pointer_is_single_var(Pointer* ptr) {
  return ptr->stack_vars.len == 1;
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
  arena_extend(sem.temp_arena, v->base, (v->length + 1) * sizeof(Pointer_Pair));
  v->base[v->length].one = one;
  v->base[v->length].two = two;
  v->length++;
}

B8 type_is_same_rec(Type* one, Type* two, Subtype_Visited* visited) {
  B8 result = false;
  if (one->kind != two->kind) {
    result = false;
  }
  else {
    switch (one->kind) {
    case Type_Kind_none: {
      result = true;
    } break;
    case Type_Kind_type: {
      result = true;
    } break;
    case Type_Kind_scalar: {
      B8 size_equal = one->bits_size == two->bits_size && one->bits_align == two->bits_align;
      B8 scalar_equal = one->scalar->bits_exp == two->scalar->bits_exp
                     && one->scalar->bits_man == two->scalar->bits_man
                     && one->scalar->kind     == two->scalar->kind;
      result = size_equal && scalar_equal;
    } break;
    case Type_Kind_record: {
      if (one->record->length != two->record->length) {
        result = false;
      }
      else {
        result = true;
        for (I32 i = 0; i < one->record->length; i++) {
          Field field_one = type_record_get_by_position(one->record, i);
          Field field_two;
          if (field_one.name) {
            field_two = type_record_get_by_name(two->record, field_one.name);
            if (field_two.pos < 0) {
              result = false;
              break;
            }
          }
          else {
            field_two = type_record_get_by_position(two->record, i);
          }
          if (!type_is_same_rec(field_one.type, field_two.type, visited)) {
            result = false;
            break;
          }
        }
      }
    } break;
    case Type_Kind_ptr: {
      if (subtype_visited_contains(visited, one->pointer, two->pointer)) {
        result = true;
      }
      else {
        subtype_visited_push(visited, one->pointer, two->pointer);
        // TODO: need to check whether pointer is global/stack/
        result = true;
        if (two->pointer->stack_vars.len > 0) {
          for (I32 i = 0; i < one->pointer->stack_vars.len; i++) {
            Var* var = one->pointer->stack_vars.list[i];
            if (!hash_set_exists(&two->pointer->stack_vars, var)) {
              result = false;
            }
          }
        }
        Type* one_declared = type_pointer_declared(one->pointer);
        Type* two_declared = type_pointer_declared(two->pointer);
        if (one_declared && two_declared) {
          if (!type_is_same_rec(one_declared, two_declared, visited)) {
            result = false;
          }
        }
      }
    } break;
    case Type_Kind_fun: {
      result = false;
    } break;
    case Type_Kind_array: {
      if (one->array->length == two->array->length) {
        result = type_is_same_rec(one->array->of_type, two->array->of_type, visited);
      }
      else {
        result = false;
      }
    } break;
    case Type_Kind_span: {
      if (type_is_same_rec(one->span->length, two->span->length, visited)) {
        result = type_is_same_rec(one->span->of_type, two->span->of_type, visited);
      }
      else {
        result = false;
      }
    } break;
    case Type_Kind_join_records: {
      if (one->join->records.len == two->join->records.len) {
        result = true;
        for (I32 i = 0; i < one->join->records.len; i++) {
          Type* element = one->join->records.list[i];
          B8 is_exists = hash_set_exists(&two->join->records, element);
          if (!is_exists) {
            result = false;
            break;
          }
        }
      }
      else {
        result = false;
      }
    } break;
    }
  }
  return result;
}

B8 type_is_same(Type* one, Type* two) {
  Subtype_Visited visited = {0};
  visited.base = arena_push(sem.temp_arena, sizeof(Pointer_Pair));
  B8 result = type_is_same_rec(one, two, &visited);
  arena_release_mark(sem.temp_arena, visited.base);
  return result;
}

B8 type_is_subtype_rec(Type* one, Type* two, Subtype_Visited* visited) {
  //    one is subtype of two
  // (1..3) is subtype of 1 -- false
  // (1..2) is subtype of (1..3) -- true
  B8 result = false;
  if (one->kind == two->kind) {
    switch (one->kind) {
    case Type_Kind_none: {
      result = true;
    } break;
    case Type_Kind_type: {
      result = true;
    } break;
    case Type_Kind_scalar: {
      if (one->scalar->kind == two->scalar->kind) {
        result = scalar_is_subrange(one->scalar, two->scalar);
      }
      else if (one->scalar->kind == Scalar_Kind_int && !has_flag(one->flag, Type_Flag_size_defined)) {
        result = true;
      }
    } break;
    case Type_Kind_ptr: {
      if (subtype_visited_contains(visited, one->pointer, two->pointer)) {
        result = true;
      }
      else {
        result = true;
        subtype_visited_push(visited, one->pointer, two->pointer);
        // TODO: need to check whether pointer is global/stack/
        if (two->pointer->stack_vars.len > 0) {
          for (I32 i = 0; i < one->pointer->stack_vars.len; i++) {
            Var* var = one->pointer->stack_vars.list[i];
            if (!hash_set_exists(&two->pointer->stack_vars, var)) {
              result = false;
            }
          }
        }
        Type* one_declared = type_pointer_declared(one->pointer);
        Type* two_declared = type_pointer_declared(two->pointer);
        if (one_declared && two_declared) {
          if (!type_is_subtype_rec(one_declared, two_declared, visited)) {
            result = false;
          }
        }
      }
    } break;
    case Type_Kind_record: {
      result = true;
      if (one->record->length > two->record->length) {
        result = false;
      }
      for (I32 i = 0; i < one->record->length; i++) {
        Field field_one = type_record_get_by_position(one->record, i);
        Field field_two;
        if (field_one.name) {
          field_two = type_record_get_by_name(two->record, field_one.name);
          if (field_two.pos < 0) {
            result = false;
            break;
          }
        }
        else {
          if (i >= two->record->length) {
            result = false;
            break;
          }
          field_two = type_record_get_by_position(two->record, i);
        }
        if (!type_is_subtype_rec(field_one.type, field_two.type, visited)) {
          result = false;
        }
      }
    } break;
    case Type_Kind_array: {
      // TODO: should [2]I8 be subtype of [4]I8?
      if (one->array->length == two->array->length) {
        // NOTE: [2]I8 is not subtype of [2]I16
        result = type_is_same_rec(one->array->of_type, two->array->of_type, visited);
      }
      else {
        result = false;
      }
    } break;
    case Type_Kind_span: {
      if (type_is_subtype_rec(one->span->length, two->span->length, visited)) {
        // NOTE: []I8 is not subtype of []I16
        result = type_is_same_rec(one->span->of_type, two->span->of_type, visited);
      }
      else {
        result = false;
      }
    } break;
    case Type_Kind_fun: {
      result = false;
    } break;
    case Type_Kind_join_records: {
      if (one->join->records.len <= two->join->records.len) {
        result = true;
        for (I32 i = 0; i < one->join->records.len; i++) {
          Type* element = one->join->records.list[i];
          B8 is_exists = hash_set_exists(&two->join->records, element);
          if (!is_exists) {
            result = false;
            break;
          }
        }
      }
      else {
        result = false;
      }
    } break;
    }
  }
  else if (one->kind == Type_Kind_array && two->kind == Type_Kind_span) {
    Type* type_length_one = type_int(one->array->length);
    if (type_is_subtype_rec(type_length_one, two->span->length, visited)) {
      result = type_is_same_rec(one->array->of_type, two->span->of_type, visited);
    }
    else {
      result = false;
    }
  }
  else {
    result = false;
  }

  return result;
}

B8 type_is_false(Type* type) {
  B8 result = false;
  switch (type->kind) {
  case Type_Kind_scalar: {
    if (type->scalar->kind == Scalar_Kind_float) {
      result = -1 <= scalar_min(type->scalar) && scalar_max(type->scalar) <= 0;
    }
    else if (scalar_is_single(type->scalar)) {
      I64 val = scalar_min(type->scalar);
      result = val == 0;
    }
    else {
      result = false;
    }
  } break;
  default: result = false;
  }
  return result;
}

B8 type_is_true(Type* type) {
  B8 result = false;
  switch (type->kind) {
  case Type_Kind_scalar: {
    if (type->scalar->kind == Scalar_Kind_float) {
      result = !scalar_have(type->scalar, -1) && !scalar_have(type->scalar, 0);
    }
    else {
      result = !scalar_have(type->scalar, 0);
    }
  } break;
  default: result = false;
  }
  return result;
}

Type* sem_element_of_join(Ir* ir) {
  I32 irid = ir - irgen.irs.base;
  return sem.elements[irid];
}

void sem_element_of_join_put(Ir* ir, Type* element) {
  I32 irid = ir - irgen.irs.base;
  sem.elements[irid] = element;
}

B8 type_is_const(Type* type) {
  B8 result = false;
  switch (type->kind) {
  case Type_Kind_none: {
    result = true;
  } break;
  case Type_Kind_type: {
    result = true;
  } break;
  case Type_Kind_scalar: {
    result = scalar_is_single(type->scalar);
  } break;
  case Type_Kind_record: {
    result = true;
    for (I32 i = 0; i < type->record->length; i++) {
      Type* field_declared = type->record->types[i];
      if (!type_is_const(field_declared)) {
        result = false;
        break;
      }
    }
  } break;
  case Type_Kind_array: {
    result = type_is_const(type->array->of_type);
  } break;
  case Type_Kind_span: {
    // TODO: what is const?
    result = type_is_const(type->array->of_type);
  } break;
  case Type_Kind_ptr: {
    result = pointer_is_single_var(type->pointer);
  } break;
  case Type_Kind_fun: {
    result = true; // TODO: consider when is false
  } break;
  case Type_Kind_join_records: {
    result = false; // TODO: consider when is false
  } break;
  }
  return result;
}

B8 type_is_subtype(Block* block, Type* one, Type* two) {
  Subtype_Visited visited = {0};
  visited.base = arena_push(sem.temp_arena, sizeof(Pointer_Pair));
  B8 result = type_is_subtype_rec(one, two, &visited);
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

Type* type_in_set(Type* type) {
  Type* result = sem.type_none;
  I32 i = hash_mix(type->value, type->bits_size);
  i = hash_mix(i, type->kind);
  i = hash_mix(i, type->bits_align);
  i = hash_mix(i, type->flag);
  for (;;) {
    i &= sem.type_set.cap - 1;
    Type* key = sem.type_set.keys[i];
    if (!key) {
      sem.type_set.keys[i] = type;
      result = type;
      break;
    }
    else {
      if (key->kind == type->kind
      &&  key->value == type->value
      &&  key->bits_size == type->bits_size
      &&  key->bits_align == type->bits_align
      &&  key->flag == type->flag) {
        del(sem.types);
        result = key;
        break;
      }
    }
    i++;
  }
  return result;
}

B8 sem_scalar_is_equal(Scalar* one, Scalar* two) {
  B8 result = one->kind == two->kind
           && one->bits_exp == two->bits_exp
           && one->bits_man == two->bits_man;

  if (result) {
    if (one->length == two->length) {
      result = true;
      for (I32 i = 0; i < one->length; i++) {
        if (one->pairs[i].lo != two->pairs[i].lo) {
          result = false;
          break;
        }
        else if (one->pairs[i].hi != two->pairs[i].hi) {
          result = false;
          break;
        }
      }
    }
    else {
      result = false;
    }
  }
  return result;
}

Type* type_type(Type* type) {
  Type* result = sem.type_none;
  I32 i = 0;
  for (;;) {
    i &= sem.type_type_set.cap - 1;
    Type* key = sem.type_type_set.keys[i];
    if (!key) {
      sem.type_type_set.keys[i] = type;
      break;
    }
    else {
      if (key == type) {
        type = key;
        break;
      }
    }
    i++;
  }
  Type* new_type = &new(sem.types);
  new_type->kind = Type_Kind_type;
  new_type->bits_align = 0;
  new_type->bits_size  = 0;
  new_type->flag = Type_Flag_comptime_const;
  new_type->type = type;
  result = type_in_set(new_type);
  return result;
}

Record* type_record_init(I32 length) {
  Record* result = arena_push(sem.perm_arena, sizeof(Record));
  result->length  = length;
  result->names   = arena_push_zero(sem.perm_arena, length * sizeof(Str*));
  result->types   = arena_push(sem.perm_arena, length * sizeof(Type*));
  result->offsets = arena_push(sem.perm_arena, length * sizeof(I32));
  result->position_from_name   = hash_map_init(sem.perm_arena, length);
  result->position_from_offset = hash_map_init(sem.perm_arena, length);
  return result;
}

U64 sem_record_hash(Record* record) {
  return 0;
}

B8 type_record_is_equal(Record* one, Record* two) {
  B8 result = false;
  if (one->length == two->length) {
    result = true;
    for (I32 i = 0; i < one->length; i++) {
      if (one->names[i] != two->names[i]) {
        result = false;
        break;
      }
      else if (one->types[i] != two->types[i]) {
        result = false;
        break;
      }
    }
  }
  return result;
}

Type* type_record(Record* record) {
  Type* result = sem.type_none;
  if (record->length > 0) {
    for (I32 i = sem_record_hash(record); ; i++) {
      i &= sem.record_set.cap - 1;
      Record* key_record = sem.record_set.keys[i];
      if (!key_record) {
        sem.record_set.keys[i] = record;
        break;
      }
      else {
        if (type_record_is_equal(key_record, record)) {
          arena_release_mark(sem.perm_arena, record);
          record = key_record;
          break;
        }
      }
    }

    I16 bits_align = 0;
    I16 bits_size = 0;
    for (I32 j = 0; j < record->length; j++) {
      Type* field_type = record->types[j];
      if (field_type->bits_size != 0) {
        bits_align = max(bits_align, field_type->bits_align);
        bits_size = align_up(bits_size, field_type->bits_align);
        record->offsets[j] = bits_size;
        bits_size += field_type->bits_size;
      }
      else {
        record->offsets[j] = -1;
      }
      hash_map_i32_put_i32(&record->position_from_offset, record->offsets[j], j);
      bits_size = align_up(bits_size, bits_align);
    }

    Type* type = &new(sem.types);
    type->kind = Type_Kind_record;
    type->flag = 0;
    type->bits_size  = bits_size;
    type->bits_align = bits_align;
    type->record = record;
    result = type_in_set(type);
  }
  else {
    result = sem.type_none;
  }
  return result;
}

Type* type_join_records(Join* join) {
  Type* result = sem.type_none;
  if (join->records.len > 0) {
    I32 i = 0;
    for (;;) {
      i &= sem.join_set.cap - 1;
      Join* key = sem.join_set.keys[i];
      if (!key) {
        sem.join_set.keys[i] = join;
        break;
      }
      else {
        if (hash_set_is_equal(key->records, join->records)) {
          arena_release_mark(sem.perm_arena, join);
          join = key;
          break;
        }
      }
      i++;
    }

    I16 bits_align = 0;
    I16 bits_size = 0;

    for (I32 e = 0; e < join->records.len; e++) {
      Type* element = join->records.list[e];
      bits_size  = max(bits_size,  element->bits_size);
      bits_align = max(bits_align, element->bits_align);
    }

    Type* new_type = &new(sem.types);
    new_type->kind = Type_Kind_join_records;
    new_type->bits_align = bits_align;
    new_type->bits_size  = bits_size;
    new_type->flag = 0;
    new_type->join = join;
    result = type_in_set(new_type);
  }
  return result;
}

Scalar* sem_scalar_intern(Scalar* scalar) {
  I32 i = 0;
  for (;;) {
    i &= sem.scalar_set.cap - 1;
    Scalar* key = sem.scalar_set.keys[i];
    if (!key) {
      sem.scalar_set.keys[i] = scalar;
      break;
    }
    else {
      if (sem_scalar_is_equal(key, scalar)) {
        arena_release_mark(sem.perm_arena, scalar);
        scalar = key;
        break;
      }
    }
    i++;
  }
  return scalar;
}

Type* type_int_scalar(Scalar* scalar) {
  Type* result = sem.type_none;
  if (scalar->length == 0) {
    result = sem.type_none;
  }
  else {
    scalar->kind     = Scalar_Kind_int;
    scalar->bits_exp = 0;
    scalar->bits_man = 0;
    scalar = sem_scalar_intern(scalar);
    I64 min = scalar_min(scalar);
    I64 max = scalar_max(scalar);
    I16 bits_size = bits_needed(min, max);
    Type* new_type = &new(sem.types);
    new_type->kind = Type_Kind_scalar;
    new_type->bits_align = align_up(bits_size, 8);
    new_type->bits_size  = bits_size;
    new_type->flag = 0;
    if (min == max) {
      new_type->flag = Type_Flag_comptime_const;
    }
    new_type->scalar = scalar;
    result = type_in_set(new_type);
  }

  return result;
}

Type* type_range(I64 min, I64 max) {
  Scalar* scalar = sem_scalar_init(1);
  scalar->length = 1;
  scalar->pairs[0].lo = min;
  scalar->pairs[0].hi = max;
  return type_int_scalar(scalar);
}

Type* type_float_scalar(I16 bits_exp, I16 bits_man, Scalar* scalar) {
  if (scalar->length == 0) {
    return sem.type_none;
  }
  scalar->kind = Scalar_Kind_float;
  scalar->bits_exp = bits_exp;
  scalar->bits_man = bits_man;
  I16 bits_size = bits_exp + bits_man + 1;
  scalar = sem_scalar_intern(scalar);
  Type* new_type = &new(sem.types);
  new_type->kind = Type_Kind_scalar;
  new_type->bits_align = bits_size;
  new_type->bits_size  = bits_size;
  new_type->flag = Type_Flag_size_defined;
  new_type->scalar = scalar;
  return type_in_set(new_type);
}

Type* type_float_full(I16 bits_exp, I16 bits_man) {
  I16 bits_size = bits_exp + bits_man + 1;
  Scalar* scalar = sem_scalar_init(1);
  scalar->length = 1;
  if (bits_size == 32) {
    scalar->pairs[0].lo = I32_MIN;
    scalar->pairs[0].hi = I32_MAX;
  }
  else {
    scalar->pairs[0].lo = I64_MIN;
    scalar->pairs[0].hi = I64_MAX;
  }
  return type_float_scalar(bits_exp, bits_man, scalar);
}

B8 type_is_float(Type* type) {
  return type->scalar->kind == Scalar_Kind_float;
}

B8 type_is_int(Type* type) {
  return type->scalar->kind == Scalar_Kind_int;
}

Scalar* sem_scalar_float_widen(Scalar* scalar) {
  Scalar* result = sem_scalar_init(scalar->length);
  result->length = scalar->length;
  for (I32 i = 0; i < scalar->length; i++) {
    F32 lo = float_f32_of_key(scalar->pairs[i].lo);
    F32 hi = float_f32_of_key(scalar->pairs[i].hi);
    I64 lo_key = isnan(lo) ? (signbit(lo) ? I64_MIN : I64_MAX) : float_key_of_f64((F64)lo);
    I64 hi_key = isnan(hi) ? (signbit(hi) ? I64_MIN : I64_MAX) : float_key_of_f64((F64)hi);
    assert(lo_key <= hi_key);
    result->pairs[i].lo = lo_key;
    result->pairs[i].hi = hi_key;
  }
  return result;
}

Scalar* sem_scalar_int_to_float(Scalar* scalar, I16 bits_size, I16 bits_man) {
  I64 limit = ((I64)1) << (bits_man + 1);
  Scalar* result = sem_scalar_init(scalar->length);
  result->length = scalar->length;
  for (I32 i = 0; i < scalar->length; i++) {
    I64 lo = scalar->pairs[i].lo;
    I64 hi = scalar->pairs[i].hi;
    assert(-limit <= lo && lo <= limit);
    assert(-limit <= hi && hi <= limit);
    if (bits_size == 32) {
      result->pairs[i].lo = float_key_of_f32((F32)lo);
      result->pairs[i].hi = float_key_of_f32((F32)hi);
    }
    else {
      result->pairs[i].lo = float_key_of_f64((F64)lo);
      result->pairs[i].hi = float_key_of_f64((F64)hi);
    }
    assert(result->pairs[i].lo <= result->pairs[i].hi);
  }
  return result;
}

B8 type_float_is_same(Scalar* one, Scalar* two) {
  B8 result = one->bits_exp == two->bits_exp
           && one->bits_man == two->bits_man;
  return result;
}

Type* type_float_meet(Type* one, Type* two) {
  Type* result = sem.type_none;
  if (type_float_is_same(one->scalar, two->scalar)) {
    return type_float_scalar(one->scalar->bits_exp, one->scalar->bits_man, sem_scalar_meet(one->scalar, two->scalar));
  }
  else if (one->scalar->bits_exp == F32_EXP && two->scalar->bits_exp == F64_EXP) {
    result = type_float_scalar(F64_EXP, F64_MAN, sem_scalar_meet(sem_scalar_float_widen(one->scalar), two->scalar));
  }
  else if (one->scalar->bits_exp == F64_EXP && two->scalar->bits_exp == F32_EXP) {
    result = type_float_scalar(F64_EXP, F64_MAN, sem_scalar_meet(one->scalar, sem_scalar_float_widen(two->scalar)));
  }
  return result;
}

Type* type_float_join(Type* one, Type* two) {
  Scalar* scalar;
  if (type_float_is_same(one->scalar, two->scalar)) {
    scalar = sem_scalar_merge(one->scalar, two->scalar);
    return type_float_scalar(one->scalar->bits_exp, one->scalar->bits_man, scalar);
  }
  if (one->scalar->bits_exp == F32_EXP) {
    scalar = sem_scalar_merge(sem_scalar_float_widen(one->scalar), two->scalar);
  }
  else if (two->scalar->bits_exp == F32_EXP) {
    scalar = sem_scalar_merge(one->scalar, sem_scalar_float_widen(two->scalar));
  }
  else {
    sem_error_type_mismatch(sem_loc(0), two, one);
  }
  return type_float_scalar(F64_EXP, F64_MAN, scalar);
}

Type* type_float_cast(Type* from, Type* to) {
  if (type_float_is_same(from->scalar, to->scalar)) {
    return type_float_scalar(to->scalar->bits_exp, to->scalar->bits_man, from->scalar);
  }
  if (from->scalar->bits_exp == F32_EXP && to->scalar->bits_exp == F64_EXP) {
    return type_float_scalar(F64_EXP, F64_MAN, sem_scalar_float_widen(from->scalar));
  }
  if (from->scalar->bits_exp == F64_EXP && to->scalar->bits_exp == F32_EXP) {
    Scalar* full = sem_scalar_init(1);
    full->length = 1;
    full->pairs[0].lo = I32_MIN;
    full->pairs[0].hi = I32_MAX;
    return type_float_scalar(F32_EXP, F32_MAN, full);
  }
  sem_error_type_mismatch(sem_loc(0), to, from);
  return 0;
}

Type* type_function(Function* function) {
  Type* result = sem.type_none;
  I32 i = 0;
  for (;;) {
    i &= sem.scalar_set.cap - 1;
    Function* key = sem.function_set.keys[i];
    if (!key) {
      sem.function_set.keys[i] = function;
      break;
    }
    else {
      if (key->fun == function->fun
      && key->arg == function->arg
      && key->ret == function->ret
      && key->foreign_name == function->foreign_name
      && key->foreign_id == function->foreign_id) {
        arena_release_mark(sem.perm_arena, function);
        function = key;
        break;
      }
    }
    i++;
  }

  Type* new_type = &new(sem.types);
  new_type->kind = Type_Kind_fun;
  new_type->function = function;
  if (new_type->function->fun) {
    new_type->flag = Type_Flag_comptime_const;
  }

  result = type_in_set(new_type);
  return result;
}

Type* type_function_foreign(Function* function, Str* foreign_name) {
  Function* function_foreign = arena_push(sem.perm_arena, sizeof(Function));
  *function_foreign = *function;
  function_foreign->foreign_name = foreign_name;
  function_foreign->foreign_id = fa_push(sem.foreign_funs, function_foreign);
  return type_function(function_foreign);
}

Ir* sem_push_ir(Block* block, Ir sem_ir) {
  Ir* new_ir = &new(irgen.irs);
  *new_ir = sem_ir;
  fa_add(block->sem_irs, new_ir);
  return new_ir;
}

Ir* sem_push_int_extend(Block* block, Ir* value, I16 bits) {
  Ir sem_ir = { Ir_Kind_int_cast, .int_cast = { .value=value, .bits = bits } };
  Ir* new_ir = sem_push_ir(block, sem_ir);
  Type* value_type = type_of_ir(value);
  Type* type = &new(sem.types);
  *type = *value_type;
  type->bits_size = bits;
  type->bits_align = align_up(bits, 8);
  type = type_in_set(type);
  type_of_ir_put(new_ir, type);
  return new_ir;
}

Ir* sem_push_float_cast(Block* block, Ir* value, Type* type) {
  Ir sem_ir = { Ir_Kind_float_cast, .int_cast = { .value=value, .bits=type->bits_size } };
  Ir* new_ir = sem_push_ir(block, sem_ir);
  type_of_ir_put(new_ir, type);
  return new_ir;
}

Ir* sem_push_float_from_int_cast(Block* block, Ir* value, Type* type) {
  Ir sem_ir = { Ir_Kind_float_from_int_cast, .cast_value = value };
  Ir* new_ir = sem_push_ir(block, sem_ir);
  type_of_ir_put(new_ir, type);
  return new_ir;
}

Ir* sem_push_int_from_float_cast(Block* block, Ir* value, Type* type) {
  Ir sem_ir = { Ir_Kind_int_from_float_cast, .cast_value = value };
  Ir* new_ir = sem_push_ir(block, sem_ir);
  type_of_ir_put(new_ir, type);
  return new_ir;
}

Ir* sem_push_record_cast(Block* block, Ir* value, Type* type) {
  Ir sem_ir = { Ir_Kind_record_cast, .cast_value = value };
  Ir* new_ir = sem_push_ir(block, sem_ir);
  type_of_ir_put(new_ir, type);
  return new_ir;
}

Ir* sem_push_array_cast(Block* block, Ir* value, Type* type) {
  Ir sem_ir = { Ir_Kind_array_cast, .cast_value = value };
  Ir* new_ir = sem_push_ir(block, sem_ir);
  type_of_ir_put(new_ir, type);
  return new_ir;
}

Type_Pair type_of_ir_binary(Ir* ir) {
  Type* one = type_of_ir(ir->binary.one);
  Type* two = type_of_ir(ir->binary.two);
  Type_Pair result = { one, two };
  return result;
}

void sem_error_binary_operand_impl(Ir* ir, Cstr op,
                                   Cstr compiler_file, I32 compiler_line) {
  Type_Pair types = type_of_ir_binary(ir);
  Type* bad = types.one;
  if (bad && bad->kind == Type_Kind_scalar) bad = types.two;
  sem_error_operand_impl(ir->loc, op, bad, compiler_file, compiler_line);
}

void sem_error_unary_operand_impl(Ir* ir, Cstr op,
                                  Cstr compiler_file, I32 compiler_line) {
  sem_error_operand_impl(ir->loc, op, type_of_ir(ir->unary),
                         compiler_file, compiler_line);
}

Type* type_of_ir_unary(Ir* ir) {
  Type* result = type_of_ir(ir->unary);
  return result;
}

Type* type_define_size(I32 bits_size, Type* bits_of) {
  Type* type = &new(sem.types);
       *type = *bits_of;
        type->flag |= Type_Flag_size_defined;
        type->bits_size    = bits_size;
        // type->bits_align   = align_up(bits_size, 8);
  return type_in_set(type);
}

Type* sem_scalar_reflow_binary(Block* block, Ir* ir, Range range, Type_Pair types) {
  Type* result = sem.type_none;
  I16 bits_size = bits_needed(range.lo, range.hi);
  I16 res_bits_size = max(types.one->bits_size, types.two->bits_size);
  if (has_flag(types.one->flag, Type_Flag_size_defined) || has_flag(types.two->flag, Type_Flag_size_defined)) {
    if (!has_flag(types.two->flag, Type_Flag_size_defined)) {
      if (types.one->bits_size >= types.two->bits_size) {
        ir->binary.two = sem_push_int_extend(block, ir->binary.two, types.one->bits_size);
      }
      else {
        assert(0);
      }
    }
    else if (!has_flag(types.one->flag, Type_Flag_size_defined)) {
      if (types.two->bits_size >= types.one->bits_size) {
        ir->binary.one = sem_push_int_extend(block, ir->binary.one, types.two->bits_size);
      }
      else {
        assert(0);
      }
    }
    else {
      if (types.two->bits_size > types.one->bits_size) {
        ir->binary.one = sem_push_int_extend(block, ir->binary.one, types.two->bits_size);
      }
      if (types.one->bits_size > types.two->bits_size) {
        ir->binary.two = sem_push_int_extend(block, ir->binary.two, types.one->bits_size);
      }
    }
    Scalar* scalar = scalar_reflow(range, res_bits_size);
    Type* unsized = type_int_scalar(scalar);
    result = type_define_size(res_bits_size, unsized);
  }
  else {
    if (types.two->bits_size > types.one->bits_size) {
      ir->binary.one = sem_push_int_extend(block, ir->binary.one, types.two->bits_size);
    }
    if (types.one->bits_size > types.two->bits_size) {
      ir->binary.two = sem_push_int_extend(block, ir->binary.two, types.one->bits_size);
    }
    types = type_of_ir_binary(ir);
    if (bits_size > types.one->bits_size) {
      ir->binary.one = sem_push_int_extend(block, ir->binary.one, bits_size);
    }
    if (bits_size > types.two->bits_size) {
      ir->binary.two = sem_push_int_extend(block, ir->binary.two, bits_size);
    }
    result = type_range(range.lo, range.hi);
  }
  return result;
}

F64 sem_float_of_key(I64 key, I16 bits_exp) {
  return bits_exp == 8 ? (F64)float_f32_of_key(key) : float_f64_of_key(key);
}

I64 sem_float_key_of_lo(F64 value, I16 bits_exp) {
  if (bits_exp == 8) {
    F32 v = (F32)value;
    if (isfinite(v)) v = nextafterf(v, -INFINITY);
    return float_key_of_f32(v);
  }
  F64 v = value;
  if (isfinite(v)) v = nextafter(v, -INFINITY);
  return float_key_of_f64(v);
}

I64 sem_float_key_of_hi(F64 value, I16 bits_exp) {
  if (bits_exp == 8) {
    F32 v = (F32)value;
    if (isfinite(v)) v = nextafterf(v, INFINITY);
    return float_key_of_f32(v);
  }
  F64 v = value;
  if (isfinite(v)) v = nextafter(v, INFINITY);
  return float_key_of_f64(v);
}

Type* type_int(I64 i64) {
  return type_range(i64, i64);
}

Type* type_float(F64 f64) {
  Scalar* scalar = sem_scalar_init(1);
  scalar->length = 1;
  I64 key = float_key_of_f32((F32)f64);
  scalar->pairs[0].lo = key;
  scalar->pairs[0].hi = key;
  return type_float_scalar(F32_EXP, F32_MAN, scalar);
}

Type* type_scalar_offset(Scalar* scalar, I64 offset) {
  Scalar* new_scalar = sem_scalar_init(scalar->length);
  new_scalar->length = scalar->length;
  for (I32 i = 0; i < scalar->length; i++) {
    Range offset_pair = { .lo = scalar->pairs[i].lo + offset, .hi = scalar->pairs[i].hi + offset };
    new_scalar->pairs[i] = offset_pair;
  }

  return type_int_scalar(scalar);
}

Scalar* sem_scalar_merge(Scalar* one, Scalar* two) {
  Scalar* new_ints = sem_scalar_init(one->length + two->length);
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
  return new_ints;
}

Type* type_scalar_merge(Scalar* one, Scalar* two) {
  return type_int_scalar(sem_scalar_merge(one, two));
}

Field type_record_get_by_position(Record* record, I32 position) {
  Field field = {};
  field.name   = record->names[position];
  field.type   = record->types[position];
  field.offset = record->offsets[position];
  field.pos    = position;
  return field;
}

Field type_record_get_by_name(Record* record, Str* name) {
  I32 position = hash_map_get_i32(&record->position_from_name, name);
  if (position < 0) {
    Field missing = {0};
    missing.name = name;
    missing.pos  = -1;
    return missing;
  }
  return type_record_get_by_position(record, position);
}

I32 type_record_bits_nopad(Record* record, I32 alignment) {
  Type* type_last   = record->types[record->length - 1];
  I32   offset_last = record->offsets[record->length - 1];
  I32   result      = align_up(offset_last + type_last->bits_size, alignment);
  return result;
}

void sem_var_kind(Var* var) {
  if (var->kind != Var_Kind_none) {
    if (has_flag(var->declared->flag, Type_Flag_comptime_const)) {
      var->kind = Var_Kind_constant;
    }
    else {
      var->kind = Var_Kind_declared;
    }
  }
}

Var* sem_var_init(I32 index, Str* name, Var* parent, Type* type);
void sem_record_declare_fields(Var* var, Type* type);
Var* sem_get_var_by_name(Block* block, Var* var, Str* name) {
  sem_var_declare(var);
  if (var->declared->kind == Type_Kind_record) {
    Record* record = var->declared->record;
    I32 position = hash_map_get_i32(&record->position_from_name, name);
    if (position < 0) {
      sem_error_name(Diag_Code_unknown_field, sem_loc(0), name);
    }
    return var->vars[position];
  }
  else if (var->declared->kind == Type_Kind_join_records) {
    // INVARIANT: offsets are equal for each record in the join

    // FIX: this leads to a collion in case of 4 records join.
    // v:A\B\C\D -- A.x and B.x have different offset from C.x and D.x.
    // So if it narrows to only A\B and then later narrows to C\D, v.x has wrong type
    Var* var_common = hash_map_get(&var->join_vars, name);
    if (var_common) {
      return var_common;
    }
    Type* var_type = type_of_var(block, var);
    Join* join = var_type->join;
    B8    is_first    = true;
    Type* type_joined = sem.type_none;
    I16   bits_size   = 0;
    I32   offset_join = -1;
    B8    is_size_defined = false;
    for (I32 i = 0; i < join->records.len; i++) {
      Type*   element = join->records.list[i];
      Record* record  = element->record;
      if (record->length != 0) {
        I32 position = hash_map_get_i32(&record->position_from_name, name);
        if (position < 0) {
          sem_error_name(Diag_Code_join_field_missing, sem_loc(0), name);
        }
        Type* type = record->types[position];
        if (is_first) {
          offset_join = record->offsets[position];
          bits_size   = type->bits_size;
          is_first    = false;
        }
        else if (offset_join != record->offsets[position]) {
          sem_error_name(Diag_Code_join_field_mismatch, sem_loc(0), name);
        }
        if (type_joined == sem.type_none) {
        }
        else if (bits_size != type->bits_size) {
          sem_error_name(Diag_Code_join_field_mismatch, sem_loc(0), name);
        }
        is_size_defined |= has_flag(type->flag, Type_Flag_size_defined);
        type_joined = type_join(type_joined, type);

      }
    }

    if (is_first) {
      sem_error_name(Diag_Code_empty_narrowed_record, sem_loc(0), name);
    }

    if (is_size_defined) {
      type_joined = type_define_size(bits_size, type_joined);
    }

    var_common = sem_var_init(offset_join, name, var, type_joined);
    hash_map_put(&var->join_vars, name, var_common);
    return var_common;
  }
  else if (var->declared->kind == Type_Kind_array) {
    if (name == irgen.str_len) {
      return var->vars[-1];
    }
    else {
      sem_error_name(Diag_Code_unknown_field, sem_loc(0), name);
    }
  }
  else if (var->declared->kind == Type_Kind_span) {
    if (name == irgen.str_len) {
      return var->vars[0];
    }
    else {
      sem_error_name(Diag_Code_unknown_field, sem_loc(0), name);
    }
  }
  else {
    sem_error_type(Diag_Code_invalid_field_base, sem_loc(0), var->declared);
  }
  return 0;
}

I32 sem_get_offset_by_name(Block* block, Ir* ir, Var* var, Str* name) {
  sem_var_declare(var);
  I32 result = -1;
  if (var->declared->kind == Type_Kind_record) {
    Record* record = var->declared->record;
    I32 position = hash_map_get_i32(&record->position_from_name, name);
    if (position < 0) {
      sem_error_name(Diag_Code_unknown_field, sem_loc(0), name);
    }
    result = record->offsets[position];
  }
  else if (var->declared->kind == Type_Kind_join_records) {
    Join* join = type_of_var(block, var)->join;
    Type* first_element = 0;
    for (I32 i = 0; i < join->records.len; i++) {
      Type* element = join->records.list[i];
      Record* record = element->record;
      if (record->length > 0) {
        I32 position = hash_map_get_i32(&record->position_from_name, name);
        if (position < 0) {
          sem_error_name(Diag_Code_join_field_missing, sem_loc(0), name);
        }
        if (result < 0) {
          result = record->offsets[position];
          first_element = element;
        }
        else if (result != record->offsets[position]) {
          sem_error_name(Diag_Code_join_field_mismatch, sem_loc(0), name);
        }
      }
    }
    if (!first_element) {
      sem_error_name(Diag_Code_empty_narrowed_record, sem_loc(0), name);
    }
    sem_element_of_join_put(ir, first_element);
  }
  else if (var->declared->kind == Type_Kind_array) {
    if (name == irgen.str_len) {
      result = -8;
    }
    else {
      sem_error_name(Diag_Code_unknown_field, sem_loc(0), name);
    }
  }
  else if (var->declared->kind == Type_Kind_span) {
    if (name == irgen.str_len) {
      result = 0;
    }
    else {
      sem_error_name(Diag_Code_unknown_field, sem_loc(0), name);
    }
  }
  else {
    sem_error_type(Diag_Code_invalid_field_base, sem_loc(0), var->declared);
  }

  return result;
}

Array* type_array_init(I32 length) {
  Array* result = arena_push(sem.perm_arena, sizeof(Array) + length * sizeof(Type*));
  result->length = length;
  return result;
}

Type* type_array(Array* array) {
  Type* result;
  if (array->length != 0) {
    I32 i = 0;
    for (;;) {
      i &= sem.array_set.cap - 1;
      Array* key_array = sem.array_set.keys[i];
      if (!key_array) {
        sem.array_set.keys[i] = array;
        break;
      }
      else {
        if (key_array->length == array->length && key_array->of_type == array->of_type) {
          B8 is_equal = true;
          I32 length = key_array->length;
          for (I32 j = 0; j < length; j++) {
            Type* type = key_array->types[j];
            if (type != array->types[j]) {
              is_equal = false;
            }
          }
          if (is_equal) {
            arena_release_mark(sem.perm_arena, array);
            array = key_array;
            break;
          }
        }
      }
      i++;
    }

    Type* type = &new(sem.types);
    type->kind = Type_Kind_array;
    type->flag = 0;
    I32 length = array->length;
    type->bits_size  = array->of_type->bits_size  * length;
    type->bits_align = array->of_type->bits_align * length;
    type->array = array;
    result = type_in_set(type);
  }
  else {
    result = sem.type_none;
  }
  return result;
}

Type* type_span(Span* span) {
  Type* result;
  if (span->length != 0) {
    I32 i = 0;
    for (;;) {
      i &= sem.span_set.cap - 1;
      Span* key_span = sem.span_set.keys[i];
      if (!key_span) {
        sem.span_set.keys[i] = span;
        break;
      }
      else {
        if (key_span->length == span->length && key_span->of_type == span->of_type) {
          arena_release_mark(sem.perm_arena, span);
          span = key_span;
          break;
        }
      }
      i++;
    }

    Type* type = &new(sem.types);
    type->kind = Type_Kind_span;
    type->flag = 0;
    type->bits_size  = 64 + 64;
    type->bits_align = 64;
    type->span = span;
    result = type_in_set(type);
  }
  else {
    result = sem.type_none;
  }
  return result;
}

Type* type_pointer(Pointer* pointer);

Type* type_meet(Type* one, Type* two) {
  Type* result = sem.type_none;

  if (one->kind == Type_Kind_none) {
    result = two;
  }
  else if (two->kind == Type_Kind_none) {
    result = one;
  }
  else {
    switch (one->kind) {
    case Type_Kind_none: break;
    case Type_Kind_type: {
      result = type_type(type_meet(one->type, two->type));
    } break;
    case Type_Kind_scalar: {
      if (one->scalar->kind != two->scalar->kind) {
        result = sem.type_none;
      }
      else if (one->scalar->kind == Scalar_Kind_float) {
        result = type_float_meet(one, two);
      }
      else {
        result = type_int_scalar(sem_scalar_meet(one->scalar, two->scalar));
      }
    } break;
    case Type_Kind_ptr: {
      Type* one_declared = type_pointer_declared(one->pointer);
      Type* two_declared = type_pointer_declared(two->pointer);
      Type* meet         = type_meet(one_declared, two_declared);
      Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
      pointer->stack_vars = hash_set_meet(sem.perm_arena, &one->pointer->stack_vars, &two->pointer->stack_vars);
      pointer->declared   = meet;
      result = type_pointer(pointer);
    } break;
    case Type_Kind_record: {
      result = sem.type_none;
    } break;
    case Type_Kind_fun: {
      result = sem.type_none;
    } break;
    case Type_Kind_array: {
      result = sem.type_none;
    } break;
    case Type_Kind_span: {
      result = sem.type_none;
    } break;
    case Type_Kind_join_records: {
      result = sem.type_none;
    } break;
    }
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
  else if (one->kind == Type_Kind_scalar && two->kind == Type_Kind_scalar) {
    if (one->scalar->kind != two->scalar->kind) {
      sem_error_type_mismatch(sem_loc(0), two, one);
    }
    else if (one->scalar->kind == Scalar_Kind_float) {
      result = type_float_join(one, two);
    }
    else {
      Scalar* scalar = sem_scalar_merge(one->scalar, two->scalar);
      if (has_flag(one->flag, Type_Flag_size_defined) || has_flag(two->flag, Type_Flag_size_defined)) {
        I16 bits_size = max(one->bits_size, two->bits_size);
        Type* merged = type_int_scalar(scalar);
        if (merged->bits_size <= bits_size) {
          result = type_define_size(bits_size, merged);
        }
        else {
          result = merged;
        }
      }
      else {
        result = type_int_scalar(scalar);
      }
    }
  }
  else if (one->kind == Type_Kind_ptr && two->kind == Type_Kind_ptr) {
    Type* one_declared = type_pointer_declared(one->pointer);
    Type* two_declared = type_pointer_declared(two->pointer);
    if (type_is_same(one_declared, two_declared)) {
      Type* join = type_join(one_declared, two_declared);
      Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
               pointer->declared   = join;
               pointer->stack_vars = hash_set_join(sem.perm_arena, &one->pointer->stack_vars, &two->pointer->stack_vars);
      result = type_pointer(pointer);
    }
    else {
      sem_error_type_mismatch(sem_loc(0), two, one);
    }
  }
  else if (one->kind == Type_Kind_type && two->kind == Type_Kind_type) {
    Type* join = type_join(one->type, two->type);
    result = type_type(join);
  }
  else if (one->kind == Type_Kind_record && two->kind == Type_Kind_record) {
    Join* join = arena_push(sem.perm_arena, sizeof(Join));
    join->records = hash_set_init(sem.perm_arena, 2);
    hash_set_put(&join->records, one);
    hash_set_put(&join->records, two);
    result = type_join_records(join);
  }
  else if (one->kind == Type_Kind_join_records && two->kind == Type_Kind_join_records) {
    Join* join = arena_push(sem.perm_arena, sizeof(Join));
    join->records = hash_set_join(sem.perm_arena, &one->join->records, &two->join->records);
    result = type_join_records(join);
  }
  else if (one->kind == Type_Kind_join_records && two->kind == Type_Kind_record) {
    Join* join = arena_push(sem.perm_arena, sizeof(Join));
    join->records = hash_set_include(sem.perm_arena, &one->join->records, two);
    result = type_join_records(join);
  }
  else if (one->kind == Type_Kind_record && two->kind == Type_Kind_join_records) {
    Join* join = arena_push(sem.perm_arena, sizeof(Join));
    join->records = hash_set_include(sem.perm_arena, &two->join->records, one);
    result = type_join_records(join);
  }
  else if (one->kind == Type_Kind_array && two->kind == Type_Kind_array) {
    sem_error_text(Diag_Code_unsupported_semantics, sem_loc(0), "joining arrays");
  }
  else {
    sem_error_type_mismatch(sem_loc(0), two, one);
  }
  return result;
}

Type* type_pointer(Pointer* pointer) {
  Type* result = sem.type_none;

  I32 i = 0;
  for (;;) {
    i &= sem.pointer_set.cap - 1;
    Pointer* key = sem.pointer_set.keys[i];
    if (!key) {
      sem.pointer_set.keys[i] = pointer;
      break;
    }
    else {
      if (key->declared == pointer->declared) {
        if (hash_set_is_equal(key->stack_vars, pointer->stack_vars)) {
          arena_release_mark(sem.perm_arena, pointer);
          pointer = key;
          break;
        }
      }
    }
    i++;
  }
  {
    Type* type = &new(sem.types);
          type->kind         = Type_Kind_ptr;
          type->bits_size    = 64;
          type->bits_align   = 64;
          type->flag         = Type_Flag_size_defined;
          type->pointer      = pointer;
    result = type_in_set(type);
  }
  return result;
}

Type* type_pointer_to(Type* type) {
  Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
           pointer->stack_vars = hash_set_init(sem.perm_arena, 1);
           pointer->declared   = type;
  return type_pointer(pointer);
}

Type* type_pointer_var(Var* var) {
  if (var->kind == Var_Kind_pointee) {
    sem_var_declare(var);
  }
  Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
           pointer->stack_vars = hash_set_init(sem.perm_arena, 1);
           pointer->declared   = 0;
  if (var->kind == Var_Kind_pointee) {
    pointer->declared = var->declared;
  }
  else {
    hash_set_put(&pointer->stack_vars, var);
  }
  return type_pointer(pointer);
}

Type* type_pointer_declared(Pointer* pointer) {
  Type* result = sem.type_none;
  if (pointer->declared) {
    result = pointer->declared;
  }
  else {
    Hash_Set stack = pointer->stack_vars;
    if (stack.len == 0) {
      result = 0;
    }
    else {
      Var* first_var = stack.list[0];
      sem_var_declare(first_var);
      Type* declared = first_var->declared;
      for (I32 i = 1; i < stack.len; i++) {
        Var* var = stack.list[i];
        sem_var_declare(var);
        declared = type_meet(declared, var->declared);
      }
      pointer->declared = declared;
      result = declared;
    }
  }
  return result;
}

Scalar_Pair scalar_pair_of_ir_binary(Ir* ir) {
  Type_Pair types = type_of_ir_binary(ir);
  Scalar_Pair result = {};
  result.one = types.one->scalar;
  result.two = types.two->scalar;
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

Scalar* sem_scalar_meet(Scalar* one, Scalar* two) {
  Scalar* new_scalar = sem_scalar_init(max(one->length, two->length));
  I32 o = 0; I32 t = 0;
  while (o < one->length && t < two->length) {
    I64 lo = max(one->pairs[o].lo, two->pairs[t].lo);
    I64 hi = min(one->pairs[o].hi, two->pairs[t].hi);
    if (lo <= hi) {
      Range pair = { .lo = lo, .hi = hi };
      new_scalar->pairs[new_scalar->length++] = pair;
    }
    if (one->pairs[o].hi < two->pairs[t].hi) {
      o++;
    }
    else {
      t++;
    }
  }
  return new_scalar;
}

Type* type_scalar_no_meet(Scalar* one, Scalar* two) {
  Scalar* new_scalar = sem_scalar_init(max(one->length, two->length));
  I32 o = 0; I32 t = 0;
  Range range1 = one->pairs[o];
  Range range2 = two->pairs[t];
  while (o < one->length && t < two->length) {
    if (range1.hi < range2.lo) {
      // [1,2] [...    | [...
      //       [3, 4]  | [3, 4]
      new_scalar->pairs[new_scalar->length++] = range1;
      range1 = one->pairs[o++];
    }
    else if (range1.lo > range2.hi) {
      //       [3, 4] [.... | [3 4]
      // [1,2] [...         | [...
      new_scalar->pairs[new_scalar->length++] = range2;
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
      new_scalar->pairs[new_scalar->length++] = range;
      range1 = one->pairs[o++];
      range2 = two->pairs[t++];
    }
    else if (range1.lo > range2.lo && range1.hi == range2.hi) {
      //   [2 3] [... | [...
      // [1   3] [... | [...
      Range range = { .lo = range2.lo, .hi = range1.lo - 1 };
      new_scalar->pairs[new_scalar->length++] = range;
      range1 = one->pairs[o++];
      range2 = two->pairs[t++];
    }
    else if (range1.lo < range2.lo && range1.hi < range2.hi) {
      // [1...5] [...  |    [...
      //    [3..8]     | [6..8]
      Range range = { .lo = range1.lo, .hi = range2.lo - 1 };
      new_scalar->pairs[new_scalar->length++] = range;
      range2.lo = range1.hi+1;
      range1 = one->pairs[o++];
    }
    else if (range1.lo > range2.lo && range1.hi > range2.hi) {
      //    [3..8]     | [6..8]
      // [1...5] [...  |    [...
      Range range = { .lo = range2.lo, .hi = range1.lo - 1 };
      new_scalar->pairs[new_scalar->length++] = range;
      range1.lo = range2.hi+1;
      range2 = two->pairs[t++];
    }
    else if (range1.lo < range2.lo && range1.hi > range2.hi) {
      // [1         8]  | [4..8]
      //    [2 3] [...  |   [...
      Range range = { .lo = range1.lo, .hi = range2.lo - 1 };
      new_scalar->pairs[new_scalar->length++] = range;
      range1.lo = range2.hi+1;
      range2 = two->pairs[t++];
    }
    else if (range1.lo > range2.lo && range1.hi < range2.hi) {
      //    [2 3] [...  |   [...
      // [1         8]  | [4..8]
      Range range = { .lo = range2.lo, .hi = range1.lo - 1 };
      new_scalar->pairs[new_scalar->length++] = range;
      range2.lo = range1.hi+1;
      range1 = one->pairs[o++];
    }
  }
  while (o < one->length) {
    new_scalar->pairs[new_scalar->length++] = range1;
    range1 = one->pairs[o++];
  }
  while (t < two->length) {
    new_scalar->pairs[new_scalar->length++] = range2;
    range2 = two->pairs[t++];
  }
  return type_int_scalar(new_scalar);
}

Scalar* sem_scalar_exclude(Scalar* scalar, I64 val) {
  Scalar* new_ints = sem_scalar_init(scalar->length+1);
  for (I32 i = 0; i < scalar->length; i++) {
    if (scalar->pairs[i].lo <= val && val <= scalar->pairs[i].hi) {
      if (scalar->pairs[i].lo != val || val != scalar->pairs[i].hi) {
        if (scalar->pairs[i].lo == val) {
          Range pair = { .lo = scalar->pairs[i].lo+1, .hi = scalar->pairs[i].hi };
          new_ints->pairs[new_ints->length++] = pair;
        }
        else if (val == scalar->pairs[i].hi) {
          Range pair = { .lo = scalar->pairs[i].lo, .hi = scalar->pairs[i].hi-1 };
          new_ints->pairs[new_ints->length++] = pair;
        }
        else {
          Range pair1 = { .lo = scalar->pairs[i].lo, .hi = val-1 };
          new_ints->pairs[new_ints->length++] = pair1;
          Range pair2 = { .lo = val+1, .hi = scalar->pairs[i].hi };
          new_ints->pairs[new_ints->length++] = pair2;
        }
      }
      i++;
      for (; i < scalar->length; i++) {
        new_ints->pairs[new_ints->length++] = scalar->pairs[i];
      }
      break;
    }
    else {
      new_ints->pairs[new_ints->length++] = scalar->pairs[i];
    }
  }
  return new_ints;
}

Type* type_float_of_scalar(Type* type, Scalar* scalar) {
  return type_float_scalar(type->scalar->bits_exp, type->scalar->bits_man, scalar);
}

Type* type_scalar_exclude(Scalar* scalar, I64 val) {
  return type_int_scalar(sem_scalar_exclude(scalar, val));
}

Type* sem_join_field_type(Join* join, Str* name) {
  Type* result = sem.type_none;
  for (I32 i = 0; i < join->records.len; i++) {
    Type*   element = join->records.list[i];
    Record* record  = element->record;
    if (record->length == 0) {
      continue;
    }
    I32 position = hash_map_get_i32(&record->position_from_name, name);
    if (position < 0) {
      return sem.type_none;
    }
    result = type_join(result, record->types[position]);
  }
  return result;
}

Ignored_Blocks sem_ignored_blocks_init(Arena* arena, I32 block_count) {
  Ignored_Blocks ignored_blocks = {};
  ignored_blocks.capacity = block_count;
  ignored_blocks.bits = arena_push_zero(arena, sizeof(B8) * block_count);
  ignored_blocks.trail = arena_push(arena, sizeof(Block*) * block_count);
  return ignored_blocks;
}

B8 sem_ignored_blocks_has(Ignored_Blocks* ignored_blocks, Block* block) {
  return ignored_blocks->bits[block->id];
}

void sem_ignored_blocks_add(Ignored_Blocks* ignored_blocks, Block* block) {
  if (ignored_blocks->bits[block->id]) return; // already in an outer context
  ignored_blocks->bits[block->id] = true;
  ignored_blocks->trail[ignored_blocks->trail_len++] = block;
}

I32 sem_ignored_blocks_checkpoint(Ignored_Blocks* ignored_blocks) {
  return ignored_blocks->trail_len;
}

void sem_ignored_blocks_rollback(Ignored_Blocks* ignored_blocks, I32 checkpoint) {
  while (ignored_blocks->trail_len > checkpoint) {
    Block* block = ignored_blocks->trail[--ignored_blocks->trail_len];
    ignored_blocks->bits[block->id] = false;
  }
}

void sem_condition_ignore_false_blocks(Ignored_Blocks* ignored_blocks, Block* block, Var* var) {
  Type* type = type_of_var(block, var);
  if (type_is_false(type)) {
    sem_ignored_blocks_add(ignored_blocks, block);
  }
  else if (!block->is_loop_head) {
    for (I32 i = 0; i < block->preds.length; i++) {
      Block* pred = block->preds.base[i];
      if (pred->state == Block_State_reachable || block->state < Block_State_reachable) {
        sem_condition_ignore_false_blocks(ignored_blocks, pred, var);
      }
    }
  }
}

void sem_condition_ignore_true_blocks(Ignored_Blocks* ignored_blocks, Block* block, Var* var) {
  Type* type = type_of_var(block, var);
  if (type_is_true(type)) {
    sem_ignored_blocks_add(ignored_blocks, block);
  }
  else if (!block->is_loop_head) {
    for (I32 i = 0; i < block->preds.length; i++) {
      Block* pred = block->preds.base[i];
      if (pred->state == Block_State_reachable || block->state < Block_State_reachable) {
        sem_condition_ignore_true_blocks(ignored_blocks, pred, var);
      }
    }
  }
}

Type* sem_type_narrow_meet(Type* narrowed, Type* field) {
  Type* result;
  if (narrowed->kind == Type_Kind_join_records) {
    if (field->kind == Type_Kind_join_records) {
      Join* join = arena_push(sem.perm_arena, sizeof(Join));
            join->records = hash_set_init(sem.perm_arena, field->join->records.len);
      for (I32 i = 0; i < field->join->records.len; i++) {
        Type* element = field->join->records.list[i];
        if (hash_set_exists(&narrowed->join->records, element)) {
          hash_set_put(&join->records, element);
        }
      }
      result = type_join_records(join);
    }
    else if (field->kind == Type_Kind_record) {
      result = hash_set_exists(&narrowed->join->records, field) ? field : sem.type_none;
    }
    else {
      result = type_meet(narrowed, field);
    }
  }
  else if (field->kind == Type_Kind_join_records) {
    result = sem_type_narrow_meet(field, narrowed);
  }
  else if (narrowed->kind == Type_Kind_record && field->kind == Type_Kind_record) {
    result = type_is_same(narrowed, field) ? field : sem.type_none;
  }
  else {
    result = type_meet(narrowed, field);
  }
  return result;
}

void sem_narrow_var(Sem_Tasks* tasks, Var* var, Type* type) {
  Sem_Narrowed* narrowed = arena_push(sem.temp_arena, sizeof(Sem_Narrowed));
                narrowed->var  = var;
                narrowed->type = var->block_types[tasks->block->id];
                narrowed->next = tasks->narrowed;
  tasks->narrowed = narrowed;
  var->block_types[tasks->block->id] = type;
}

void sem_type_of_ir_narrow(Sem_Tasks* tasks, Ir* ir, Type* new_type) {
  if (ir->kind == Ir_Kind_load) {
    Type* ptr_type = type_of_ir(ir->unary);
    Pointer* ptr = ptr_type->pointer;
    if (ptr->stack_vars.len == 1) {
      Var* var = ptr->stack_vars.list[0];
      Type* old_type = var->block_types[tasks->block->id];
      if (old_type) {
        Type* new_type_but_size_as_old = &new(sem.types);
             *new_type_but_size_as_old = *new_type;
              new_type_but_size_as_old->flag       = old_type->flag;
              new_type_but_size_as_old->bits_size  = old_type->bits_size;
              new_type_but_size_as_old->bits_align = old_type->bits_align;
        new_type = type_in_set(new_type_but_size_as_old);
        sem_narrow_var(tasks, var, new_type);
        // TODO: analyse this slop and simplify

        // NOTE: only narrow the ancestors of the variable whose type is being
        // computed, otherwise an unrelated computation would leave them narrowed.
        B8 var_is_ancestor_of_tasks_var = false;
        for (Var* v = var; v; v = v->parent) {
          if (v == tasks->var) {
            var_is_ancestor_of_tasks_var = true;
            break;
          }
        }

        // NOTE: a field may be nested in several joined records (e.g. a.x in
        // (a:(x:1)) \ (a:(x:2))), so the narrowing is propagated up the whole
        // parent chain, not just to the direct parent.
        Var* child = var;
        for (; var_is_ancestor_of_tasks_var && child != tasks->var && child->parent; child = child->parent) {
          Var*  parent = child->parent;
          Type* type_parent_old = parent->block_types[tasks->block->id];
          if (!type_parent_old || type_parent_old->kind == Type_Kind_none) {
            type_parent_old = parent->declared;
          }
          if (!type_parent_old || type_parent_old->kind != Type_Kind_join_records) {
            break;
          }

          Join* join_old = type_parent_old->join;
          Join* join_new = arena_push(sem.perm_arena, sizeof(Join));
                join_new->records = hash_set_init(sem.perm_arena, join_old->records.len);
          for (I32 e = 0; e < join_old->records.len; e++) {
            Type* element = join_old->records.list[e];
            Record* record = element->record;
            I32     pos    = -1;
            if (record->length != 0) {
              pos = child->name
                ? hash_map_get_i32(&record->position_from_name, child->name)
                : hash_map_i32_get_i32(&record->position_from_offset, child->index);
            }
            Type* meet = pos < 0 ? sem.type_none : sem_type_narrow_meet(new_type, record->types[pos]);
            if (meet != sem.type_none) {
              hash_set_put(&join_new->records, element);
            }
            else {
              hash_set_put(&join_new->records, sem.type_empty_record);
            }
          }
          Type* type_parent_new = type_join_records(join_new);
          sem_narrow_var(tasks, parent, type_parent_new);

          // Keep already cached common fields in sync with the narrowed parent.
          for (I32 c = 0; c < parent->join_vars.len; c++) {
            Str* common_name = parent->join_vars.list[c];
            Var* common = hash_map_get(&parent->join_vars, common_name);
            Type* type_common = sem_join_field_type(type_parent_new->join, common_name);
            if (type_common->kind == Type_Kind_record && common->declared->kind == Type_Kind_join_records) {
              // NOTE: a join narrowed down to a single record is still a join
              // for the purposes of the common field variable.
              Join* join_common = arena_push(sem.perm_arena, sizeof(Join));
                    join_common->records = hash_set_init(sem.perm_arena, 1);
              hash_set_put(&join_common->records, type_common);
              type_common = type_join_records(join_common);
            }
            if (type_common != sem.type_none) {
              Type* old_common = common->block_types[tasks->block->id];
              if (old_common) {
                type_common = sem_type_narrow_meet(type_common, old_common);
              }
              if (type_common != sem.type_none && type_common->kind == common->declared->kind) {
                sem_narrow_var(tasks, common, type_common);
              }
            }
          }

          new_type = type_parent_new;
        }
      }
    }
  }
}

void sem_type_of_ir_binary_narrow(Sem_Tasks* tasks, Ir* ir, Type* new_type_one, Type* new_type_two) {
  sem_type_of_ir_narrow(tasks, ir->binary.one, new_type_one);
  sem_type_of_ir_narrow(tasks, ir->binary.two, new_type_two);
}

void sem_type_narrow_int_eq(Sem_Tasks* tasks, Ir* ir) {
  Type_Pair pair = type_of_ir_binary(ir);
  Type* new_type;
  if (type_is_float(pair.one) || type_is_float(pair.two)) {
    new_type = type_meet(pair.one, pair.two);
  }
  else {
    Scalar_Pair scalar = scalar_pair_of_ir_binary(ir);
    new_type = type_int_scalar(sem_scalar_meet(scalar.one, scalar.two));
  }
  sem_type_of_ir_binary_narrow(tasks, ir, new_type, new_type);
}

void sem_type_narrow_ptr_eq(Sem_Tasks* tasks, Ir* ir) {
  Pointer_Pair pair = pointer_pair_of_ir_binary(ir);
  Type* meet = type_meet(pair.one->declared, pair.two->declared);
  Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
           pointer->declared   = meet;
           pointer->stack_vars = hash_set_meet(sem.perm_arena, &pair.one->stack_vars, &pair.two->stack_vars);
  Type* new_type = type_pointer(pointer);
  sem_type_of_ir_binary_narrow(tasks, ir, new_type, new_type);
}

void sem_type_narrow_int_ne(Sem_Tasks* tasks, Ir* ir) {
  Type_Pair pair = type_of_ir_binary(ir);
  if (scalar_is_single(pair.one->scalar)) {
    I64 val = scalar_min(pair.one->scalar);
    Type* new_type_two = type_is_float(pair.two)
      ? type_float_of_scalar(pair.two, sem_scalar_exclude(pair.two->scalar, val))
      : type_scalar_exclude(pair.two->scalar, val);
    sem_type_of_ir_narrow(tasks, ir->binary.two, new_type_two);
  }
  if (scalar_is_single(pair.two->scalar)) {
    I64 val = scalar_min(pair.two->scalar);
    Type* new_type_one = type_is_float(pair.one)
      ? type_float_of_scalar(pair.one, sem_scalar_exclude(pair.one->scalar, val))
      : type_scalar_exclude(pair.one->scalar, val);
    sem_type_of_ir_narrow(tasks, ir->binary.one, new_type_one);
  }
}

void sem_type_narrow_ptr_ne(Sem_Tasks* tasks, Ir* ir) {
  Pointer_Pair pair = pointer_pair_of_ir_binary(ir);
  // TODO: pointer relations
  if (pointer_is_single_var(pair.one)) {
    Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
             pointer->stack_vars = hash_set_exclude(sem.perm_arena, &pair.two->stack_vars, pair.one->stack_vars.list[0]);
             pointer->declared   = pair.two->declared;
    Type* new_type_two = type_pointer(pointer);
    sem_type_of_ir_narrow(tasks, ir->binary.two, new_type_two);
  }
  if (pointer_is_single_var(pair.two)) {
    Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
             pointer->stack_vars = hash_set_exclude(sem.perm_arena, &pair.one->stack_vars, pair.two->stack_vars.list[0]);
             pointer->declared   = pair.one->declared;
    Type* new_type_one = type_pointer(pointer);
    sem_type_of_ir_narrow(tasks, ir->binary.one, new_type_one);
  }
}

I64 sem_sat_dec(I64 x) { return x == I64_MIN ? I64_MIN : x - 1; }
I64 sem_sat_inc(I64 x) { return x == I64_MAX ? I64_MAX : x + 1; }

Type* sem_type_clamp(Type* type, I64 lo, I64 hi) {
  Scalar* line = sem_scalar_init(1);
  line->length = 1;
  line->pairs[0].lo = lo;
  line->pairs[0].hi = hi;
  Scalar* meet = sem_scalar_meet(type->scalar, line);
  if (type_is_float(type)) {
    return type_float_of_scalar(type, meet);
  }
  return type_int_scalar(meet);
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

void sem_type_narrow_int_cmp(Sem_Tasks* tasks, Ir* ir, Ir_Kind op) {
  Type_Pair pair = type_of_ir_binary(ir);
  Scalar* ra = pair.one->scalar;
  Scalar* rb = pair.two->scalar;
  I64 amin = scalar_min(ra);
  I64 amax = scalar_max(ra);
  I64 bmin = scalar_min(rb);
  I64 bmax = scalar_max(rb);
  Type* na = 0;
  Type* nb = 0;
  switch (op) {
  case Ir_Kind_lt:
    na = sem_type_clamp(pair.one, I64_MIN, sem_sat_dec(bmax));
    nb = sem_type_clamp(pair.two, sem_sat_inc(amin), I64_MAX);
  break;
  case Ir_Kind_le:
    na = sem_type_clamp(pair.one, I64_MIN, bmax);
    nb = sem_type_clamp(pair.two, amin, I64_MAX);
  break;
  case Ir_Kind_gt:
    na = sem_type_clamp(pair.one, sem_sat_inc(bmin), I64_MAX);
    nb = sem_type_clamp(pair.two, I64_MIN, sem_sat_dec(amax));
  break;
  case Ir_Kind_ge:
    na = sem_type_clamp(pair.one, bmin, I64_MAX);
    nb = sem_type_clamp(pair.two, I64_MIN, amax);
  break;
  default:
    sem_error_text(Diag_Code_unsupported_semantics, ir->loc, "this comparison");
  break;
  }
  sem_type_of_ir_binary_narrow(tasks, ir, na, nb);
}

Type* type_narrow_eqz(Type* type) {
  switch (type->kind) {
  case Type_Kind_scalar: {
    if (type_is_float(type)) {
      B8 have_zero = scalar_have(type->scalar, 0) || scalar_have(type->scalar, -1);
      if (have_zero) {
        Scalar* zeros = sem_scalar_init(1);
        zeros->length = 1;
        zeros->pairs[0].lo = -1;
        zeros->pairs[0].hi = 0;
        Scalar* meet = sem_scalar_meet(type->scalar, zeros);
        return type_float_of_scalar(type, meet);
      }
      return 0;
    }
    B8 have_zero = scalar_have(type->scalar, 0);
    if (have_zero) {
      return sem.type_zero;
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
  case Type_Kind_scalar: {
    if (type_is_float(type)) {
      Scalar* nonzero = sem_scalar_exclude(type->scalar, 0);
      nonzero = sem_scalar_exclude(nonzero, -1);
      return type_float_of_scalar(type, nonzero);
    }
    return type_scalar_exclude(type->scalar, 0);
  } break;
  default: break;
  }
  return type;
}

void sem_narrow_nez(Sem_Tasks* tasks, Ignored_Blocks* ignored_blocks, Block* block) {
  // cond_ir != 0
  Ir* cond_ir = block->branch.cond;
  switch (cond_ir->kind) {
  case Ir_Kind_eq: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_scalar)) {
      sem_type_narrow_int_eq(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_eq(tasks, cond_ir);
    }
    else {
      sem_error_binary_operand(cond_ir, "comparison");
    }
  } break;
  case Ir_Kind_ne: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_scalar)) {
      sem_type_narrow_int_ne(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_ne(tasks, cond_ir);
    }
    else {
      sem_error_binary_operand(cond_ir, "comparison");
    }
  } break;
  case Ir_Kind_lt: case Ir_Kind_le:
  case Ir_Kind_gt: case Ir_Kind_ge: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_scalar)) {
      sem_type_narrow_int_cmp(tasks, cond_ir, cond_ir->kind);
    }
  } break;
  case Ir_Kind_load: {
    Type* ptr_type = type_of_ir(cond_ir->unary);
    if (ptr_type->kind == Type_Kind_ptr) {
      Pointer* ptr = ptr_type->pointer;
      if (hash_set_exists(&ptr->stack_vars, tasks->var)) {
        Type* new_type = type_narrow_nez(tasks->type);
        sem_narrow_var(tasks, tasks->var, new_type);
      }
      else if (ptr->stack_vars.len == 1) {
        Var* cond_var = ptr->stack_vars.list[0];
        sem_condition_ignore_false_blocks(ignored_blocks, tasks->block, cond_var);
      }
    }
  } break;
  default: {} break;
  }
}

void sem_narrow_eqz(Sem_Tasks* tasks, Ignored_Blocks* ignored_blocks, Block* block) {
  Ir* cond_ir = block->branch.cond;
  switch (cond_ir->kind) {
  case Ir_Kind_eq: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_scalar)) {
      sem_type_narrow_int_ne(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_ne(tasks, cond_ir);
    }
    else {
      sem_error_binary_operand(cond_ir, "comparison");
    }
  } break;
  case Ir_Kind_ne: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_scalar)) {
      sem_type_narrow_int_eq(tasks, cond_ir);
    }
    else if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_ptr)) {
      sem_type_narrow_ptr_eq(tasks, cond_ir);
    }
    else {
      sem_error_binary_operand(cond_ir, "comparison");
    }
  } break;
  case Ir_Kind_lt: case Ir_Kind_le:
  case Ir_Kind_gt: case Ir_Kind_ge: {
    if (type_kind_of_ir_binary_operands_equal(cond_ir, Type_Kind_scalar)) {
      sem_type_narrow_int_cmp(tasks, cond_ir, sem_cmp_negate(cond_ir->kind));
    }
  } break;
  case Ir_Kind_load: {
    Type* ptr_type = type_of_ir(cond_ir->unary);
    if (ptr_type->kind == Type_Kind_ptr) {
      Pointer* ptr = ptr_type->pointer;
      if (hash_set_exists(&ptr->stack_vars, tasks->var)) {
        Type* new_type = type_narrow_eqz(tasks->type);
        sem_narrow_var(tasks, tasks->var, new_type);
      }
      else if (ptr->stack_vars.len == 1) {
        Var* cond_var = ptr->stack_vars.list[0];
        sem_condition_ignore_true_blocks(ignored_blocks, tasks->block, cond_var);
      }
    }
  } break;
  default: {} break;
  }
}

void sem_unnarrow(Sem_Tasks* tasks) {
  while (tasks->narrowed) {
    Sem_Narrowed* narrowed = tasks->narrowed;
    narrowed->var->block_types[tasks->block->id] = narrowed->type;
    tasks->narrowed = narrowed->next;
  }
  Var*  var = tasks->var;
  Type* old = tasks->type;
  var->block_types[tasks->block->id] = old;
}

Type* type_of_var_rec(Ignored_Blocks* ignored_blocks, Block* block, Var* var) {
  if (var->kind == Var_Kind_constant) {
    return var->declared;
  }
  Type* type = var->block_types[block->id];
  if (type) return type;

  type = sem.type_none;

  if (block->is_loop_head) {
    type = var->declared;
    var->block_types[block->id] = type;
    return type;
  }

  var->block_types[block->id] = type;

  for (I32 i = 0; i < block->preds.length; i++) {
    Block* pred = block->preds.base[i];
    if (pred->state == Block_State_reachable || block->state < Block_State_reachable) {
      if (pred->kind == Block_Kind_branch) {
        I32 ignored_blocks_checkpoint = sem_ignored_blocks_checkpoint(ignored_blocks);
        Sem_Tasks tasks = {};
        tasks.block = pred;
        tasks.var = var;
        tasks.type = var->block_types[pred->id];
        void* temp_arena_mark = arena_mark(sem.temp_arena);
        { // condition is not equal to zero branch
          Jump jump = pred->branch.nez;
          if (jump.to_block == block) {
            sem_narrow_nez(&tasks, ignored_blocks, pred);
            if (!sem_ignored_blocks_has(ignored_blocks, pred)) {
              Type* pred_type = type_of_var_rec(ignored_blocks, pred, var);
              type = type_join(type, pred_type);
            }
            sem_unnarrow(&tasks);
            sem_ignored_blocks_rollback(ignored_blocks, ignored_blocks_checkpoint);
          }
        }
        { // condition is equal to zero branch
          Jump jump = pred->branch.eqz;
          if (jump.to_block == block) {
            sem_narrow_eqz(&tasks, ignored_blocks, pred);
            if (!sem_ignored_blocks_has(ignored_blocks, pred)) {
              Type* pred_type = type_of_var_rec(ignored_blocks, pred, var);
              type = type_join(type, pred_type);
            }
            sem_unnarrow(&tasks);
            sem_ignored_blocks_rollback(ignored_blocks, ignored_blocks_checkpoint);
          }
        }
        arena_release_mark(sem.temp_arena, temp_arena_mark);
      }
      else if (pred->kind == Block_Kind_jump) {
        if (!sem_ignored_blocks_has(ignored_blocks, pred)) {
          Type* pred_type = type_of_var_rec(ignored_blocks, pred, var);
          type = type_join(type, pred_type);
        }
      }
    }
  }

  var->block_types[block->id] = type;
  return type;
}

Type* type_of_var(Block* block, Var* var) {
  void* temp_arena_mark = arena_mark(sem.temp_arena);
  Ignored_Blocks ignored_blocks = sem_ignored_blocks_init(sem.temp_arena, sem.current_fun->blocks->length);
  Type* type = type_of_var_rec(&ignored_blocks, block, var);
  if (type->kind == Type_Kind_none) {
    type = var->declared ? var->declared : sem.type_none;
    var->block_types[block->id] = type;
  }
  arena_release_mark(sem.temp_arena, temp_arena_mark);
  return type;
}

Type* type_auto_cast(Block* block, Ir** ir, Type* from, Type* to) {
  if (type_is_same(from, to)) {
    return from;
  }
  Type* result = 0;
  if (from->kind == to->kind) {
    switch (from->kind) {
    case Type_Kind_scalar: {
      if (from->scalar->kind == Scalar_Kind_float && to->scalar->kind == Scalar_Kind_float) {
        result = type_float_cast(from, to);
        if (ir) {
          *ir = sem_push_float_cast(block, *ir, result);
        }
      }
      else if (from->scalar->kind == Scalar_Kind_int && to->scalar->kind == Scalar_Kind_float) {
        result = to;
        if (ir) {
          *ir = sem_push_float_from_int_cast(block, *ir, result);
        }
      }
      else if (from->scalar->kind == Scalar_Kind_int && to->scalar->kind == Scalar_Kind_int) {
        if (ir) {
          *ir = sem_push_int_extend(block, *ir, to->bits_size);
          result = type_of_ir(*ir);
        }
        else {
          result = &new(sem.types);
          *result = *from;
          result->flag       = to->flag;
          result->bits_size  = to->bits_size;
          result->bits_align = align_up(to->bits_size, 8);
        }
      }
    } break;
    case Type_Kind_record: {
      I32 length = from->record->length;
      Record* new_record  = type_record_init(length);
      for (I32 i = 0; i < length; i++) {
        Field field = type_record_get_by_position(from->record, i);
        Field field_var;
        if (field.name) {
          field_var = type_record_get_by_name(to->record, field.name);
          if (field_var.pos < 0) {
            sem_error_name(Diag_Code_unknown_field, sem_loc(0), field.name);
          }
          new_record->names[i] = field_var.name;
          hash_map_put_i32(&new_record->position_from_name, field_var.name, i);
        }
        else {
          if (i >= to->record->length) {
            sem_error_type_mismatch(sem_loc(0), to, from);
          }
          field_var = type_record_get_by_position(to->record, i);
        }
        if (type_is_same(field.type, field_var.type)) {
          new_record->types[i] = field.type;
        }
        else {
          new_record->types[i] = type_auto_cast(block, 0, field.type, field_var.type);
        }
      }
      result = type_record(new_record);

      if (ir) {
        *ir = sem_push_record_cast(block, *ir, result);
      }
    } break;
    case Type_Kind_array: {
      Array* new_array;
      I32 length = to->array->length;
      new_array = type_array_init(length);
      for (I32 i = 0; i < length; i++) {
        Type* from_type = from->array->types[i];
        Type* to_type = to->array->types[i];
        if (!type_is_same(from_type, to_type)) {
          new_array->types[i] = from_type;
        }
        else {
          new_array->types[i] = type_auto_cast(block, 0, from_type, to_type);
        }
      }
      new_array->of_type = type_auto_cast(block, 0, from->array->of_type, to->array->of_type);
      result = type_array(new_array);
      if (ir) {
        *ir = sem_push_array_cast(block, *ir, result);
      }
    } break;
    default: {
      sem_error_type_mismatch(sem_loc(0), to, from);
    }
    }
  }
  else if (to->kind == Type_Kind_ptr) {
    if (from->kind == Type_Kind_array) {
      Type* declared = from->array->of_type;
      result = type_pointer_to(declared);
    }
    else {
      sem_error_type_mismatch(sem_loc(0), to, from);
    }
  }
  else if (to->kind == Type_Kind_span) {
    if (from->kind == Type_Kind_array) {
      Span* span = arena_push(sem.perm_arena, sizeof(Span));
      span->length = type_define_size(64, type_int(from->array->length));
      span->of_type = from->array->of_type;
      result = type_span(span);

      if (ir) {
        Ir* two = *ir;
        if (two->kind == Ir_Kind_load) {
          Ir* ptr_loaded = two->unary;
          Ir  ir_sem = { Ir_Kind_span_from_array_cast, .cast_value = ptr_loaded };
          Ir* ir_new = sem_push_ir(block, ir_sem);
          type_of_ir_put(ir_new, result);
          *ir = ir_new;
        }
        else {
          Var* var = &new(irgen.vars);
               var->kind   = Var_Kind_none;
               var->state  = Var_State_unresolved;
               var->global = irgen.scope_stack.length == 1;
               var->name   = irgen.str_anon;
               var->declared_ir = two;

          sem_var_declare(var);
          Ir* ir_var = 0;
          {
            Ir  ir_sem = { Ir_Kind_var, .var = var };
            ir_var = sem_push_ir(block, ir_sem);
            Type* type_var = type_pointer_var(ir_var->var);
            type_of_ir_put(ir_var, type_var);
          }

          {
            Ir  ir_sem = { Ir_Kind_store, .binary = { .one = ir_var, .two = two } };
            Ir* ir_new = sem_push_ir(block, ir_sem);
            type_of_ir_put(ir_new, sem.type_none);
          }

          {
            Ir  ir_sem = { Ir_Kind_span_from_array_cast, .cast_value = ir_var };
            Ir* ir_new = sem_push_ir(block, ir_sem);
            type_of_ir_put(ir_new, result);
            *ir = ir_new;
          }

        }
      }
    }
    else {
      sem_error_type_mismatch(sem_loc(0), to, from);
    }
  }
  else {
    sem_error_type_mismatch(sem_loc(0), to, from);
  }
  return result;
}

Type* type_cast(Block* block, Ir** ir, Type* from, Type* to) {
  Type* result = type_auto_cast(block, ir, from, to);
  if (!result) {
    if (from->kind == to->kind) {
      switch (from->kind) {
      case Type_Kind_scalar: {
        if (from->scalar->kind == Scalar_Kind_float && to->scalar->kind == Scalar_Kind_int) {
          result = to;
          *ir = sem_push_int_from_float_cast(block, *ir, result);
        }
        else {
          sem_error_type_mismatch(sem_loc(0), to, from);
        }
      } break;
      default: {
        sem_error_type_mismatch(sem_loc(0), to, from);
      }
      }
    }
    else {
      sem_error_type_mismatch(sem_loc(0), to, from);
    }
  }
  return result;
}

void type_of_var_put(Block* block, Ir** ir, Var* var, Type* type) {
  if (var->kind == Var_Kind_constant) {
    sem_error_name(Diag_Code_assign_to_constant, sem_loc(0), var->name);
  }
  if (var->kind == Var_Kind_none || var->kind == Var_Kind_pointee) {
    var->declared = type_join(var->declared, type);
    var->block_types[block->id] = type;
  }
  else {
    if (type_is_subtype(block, type, var->declared)) {
      type = type_auto_cast(block, ir, type, var->declared);

      if (type->kind == Type_Kind_record) {
        for (I32 i = 0; i < type->record->length; i++) {
          Field field = type_record_get_by_position(type->record, i);
          Field field_var;
          if (field.name) {
            field_var = type_record_get_by_name(var->declared->record, field.name);
            if (field_var.pos < 0) {
              sem_error_name(Diag_Code_unknown_field, sem_loc(0), field.name);
            }
          }
          else {
            if (i >= var->declared->record->length) {
              sem_error_type_mismatch(sem_loc(0), var->declared, type);
            }
            field_var = type_record_get_by_position(var->declared->record, i);
          }
          type_of_var_put(block, 0, var->vars[field_var.pos], field.type);
        }
      }
      else if (type->kind == Type_Kind_array) {
        Array* array = type->array;
        I32 length = array->length;
        for (I32 i = 0; i < length; i++) {
          type_of_var_put(block, 0, var->vars[i], array->types[i]);
        }
      }
      var->block_types[block->id] = type;
    }
    else {
      sem_error_type_mismatch(sem_loc(0), var->declared, type);
    }
  }
}

Type* type_of_fun(Fun* fun) {
  return fun->type;
}

void sem_block(Block* block);
void sem_init_block_preds(Block* block);

Var* sem_var_init(I32 index, Str* name, Var* parent, Type* type) {
  Var* var = &new(irgen.vars);
       var->global   = parent->global;
       var->index    = index;
       var->name     = name;
       var->parent   = parent;
       var->declared = type;
       var->state    = Var_State_resolved;
       var->kind     = Var_Kind_declared;
  sem_var_kind(var);
  I32 blocks_length = 0;
  if (var->global) {
    blocks_length = irgen.block_length_max;
  }
  else {
    blocks_length = sem.current_fun->blocks->length;
  }
  var->block_types = arena_push_zero(sem.perm_arena, blocks_length * sizeof(Type*));
  sem_record_declare_fields(var, type);

  return var;
}

void sem_record_declare_fields(Var* var, Type* type) {
  if (type->kind == Type_Kind_record) {
    var->vars = arena_push(sem.perm_arena, type->record->length*sizeof(Var*));
    for (I32 i = 0; i < type->record->length; i++) {
      var->vars[i] = sem_var_init(i, type->record->names[i], var, type->record->types[i]);
    }
  }
  else if (type->kind == Type_Kind_join_records) {
    Join* join = type->join;
    I32 total_length = 0;
    for (I32 i = 0; i < join->records.len; i++) {
      Type* element = join->records.list[i];
      total_length += element->record->length;
    }
    var->vars = arena_push(sem.perm_arena, join->records.len*sizeof(Var*));
    var->join_vars = hash_map_init(sem.perm_arena, max(total_length, 1));
    for (I32 i = 0; i < join->records.len; i++) {
      var->vars[i] = sem_var_init(i, irgen.str_nil, var, join->records.list[i]);
    }
  }
  else if (type->kind == Type_Kind_array) {
    Array* array = type->array;
    I32 length = array->length;
    var->vars = arena_push(sem.perm_arena, (length+1) * sizeof(Var*));
    var->vars += 1; // NOTE: at -1 we have legnth
    var->vars[-1] = sem_var_init(-1, irgen.str_len, var, type_int(length));
    Type* type_item = array->of_type;
    if (type_item->kind == Type_Kind_type) {
      type_item = type_item->type;
    }
    for (I32 i = 0; i < length; i++) {
      var->vars[i] = sem_var_init(i, irgen.str_nil, var, type_item);
    }
  }
  else if (type->kind == Type_Kind_span) {
    Span* span = type->span;
    var->vars = arena_push(sem.perm_arena, 2 * sizeof(Var*));
    var->vars[0] = sem_var_init(0, irgen.str_len, var, sem.type_len);
    var->vars[1] = sem_var_init(1, irgen.str_nil, var, type_pointer_to(span->of_type));
  }
  return;
}

Type* sem_var_declare(Var* var) {
  if (var->state == Var_State_resolved) return var->declared;
  if (var->state == Var_State_resolving) {
    sem_error_name(Diag_Code_cyclic_declaration, sem_loc_for_var(var), var->name);
  }
  var->state = Var_State_resolving;

  if (var->blocks) {
    for (I32 i = 0; i < var->blocks->length; i++) {
      Block* block = var->blocks->base[i];
      sem_init_block_preds(block);
      sem_block(block);
    }
  }
  Type* type = type_of_ir(var->declared_ir);
  if (type) {
    if (type->kind == Type_Kind_type) {
      type = type->type;
    }
    var->declared = type;
    sem_var_kind(var);
    sem_record_declare_fields(var, type);
  }

  var->state = Var_State_resolved;
  return type;
}

Type* sem_fun(Fun* fun);

void sem_ir(Block* block, Ir* ir) {
  // printf("  r%ld\n", ir-irgen.irs.base);
  Ir* saved_ir = sem.current_ir;
  Loc saved_loc = sem.current_loc;
  sem.current_ir = ir;
  if (loc_is_valid(ir->loc)) sem.current_loc = ir->loc;
  Type* result = sem.type_none;
  switch (ir->kind) {
  case Ir_Kind_none: {
    result = sem.type_none;
  } break;
  case Ir_Kind_len: {
    result = sem.fun_len;
  } break;
  case Ir_Kind_bits: {
    result = sem.fun_bits;
  } break;
  case Ir_Kind_foreign: {
    result = sem.rec_foreign;
  } break;
  case Ir_Kind_int: {
    I64 i64 = ir->i64;
    result  = type_int(i64);
  } break;
  case Ir_Kind_float: {
    F64 f64 = ir->f64;
    result  = type_float(f64);
  } break;
  case Ir_Kind_str: {
    Array* str_array = type_array_init(ir->str->length);
    str_array->of_type = sem.bytes_range;
    I32 length = 0;
    for (I32 i = 0; i < ir->str->length; i++) {
      C8 raw = ir->str->base[i];
      C8 val = 0;
      if (raw == '\\') {
        i++;
        if (i >= ir->str->length) {
          sem_error_text(Diag_Code_invalid_escape, ir->loc, "\\");
        }
        raw = ir->str->base[i];
        if (raw == 'x') {
          sem_error_text(Diag_Code_unsupported_semantics, ir->loc,
                         "hex escapes in string literals");
        }
        else {
          C8 escape_to_char[256] = {
            ['0'] = '\0',
            ['\''] = '\'',
            ['"'] = '"',
            ['\\'] = '\\',
            ['n'] = '\n',
            ['r'] = '\r',
            ['t'] = '\t',
            ['v'] = '\v',
            ['b'] = '\b',
            ['a'] = '\a',
          };
          I8 index = raw;
          val = escape_to_char[index];
          if (val == 0 && raw != '0') {
            C8* text = arena_push(sem.perm_arena, 3);
            text[0] = '\\';
            text[1] = raw;
            text[2] = '\0';
            sem_error_text(Diag_Code_invalid_escape, ir->loc, text);
          }
        }
      }
      else {
        val = raw;
      }
      Type* int_type = type_int(val);
      str_array->types[length++] = type_define_size(8, int_type);
    }

    str_array->length = length;
    result = type_array(str_array);
  } break;
  case Ir_Kind_declare: {
    result = sem_var_declare(ir->declare.var);
  } break;
  case Ir_Kind_arg: {
    result = type_pointer_var(ir->var);
  } break;
  case Ir_Kind_var: {
    ir->var->declared = sem.type_none;
    result = type_pointer_var(ir->var);
  } break;
  case Ir_Kind_join: {
    Type_Pair pair = type_of_ir_binary(ir);
    result = type_join(pair.one, pair.two);
  } break;
  case Ir_Kind_add: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_scalar && types.two->kind == Type_Kind_scalar) {
      if (type_is_float(types.one) && type_is_float(types.two)) {
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_int(types.one) && type_is_int(types.two)) {
        Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
        Range one = scalar_limits(pair.one);
        Range two = scalar_limits(pair.two);
        Range range = { one.lo+two.lo, one.hi+two.hi };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
      else if (type_is_float(types.one)) {
        type_auto_cast(block, &ir->binary.two, types.two, types.one);
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_float(types.two)) {
        type_auto_cast(block, &ir->binary.one, types.one, types.two);
        result = type_float_full(types.two->scalar->bits_exp, types.two->scalar->bits_man);
      }
    }
    else {
      sem_error_binary_operand(ir, "+");
    }
  } break;
  case Ir_Kind_sub: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_scalar && types.two->kind == Type_Kind_scalar) {
      if (type_is_float(types.one) && type_is_float(types.two)) {
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_int(types.one) && type_is_int(types.two)) {
        Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
        Range one = scalar_limits(pair.one);
        Range two = scalar_limits(pair.two);
        I64 a = one.hi - two.hi;
        I64 b = one.hi - two.lo;
        I64 c = one.lo - two.hi;
        I64 d = one.lo - two.lo;
        Range range = { min(min(min(a, b), c), d), max(max(max(a, b), c), d) };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
      else if (type_is_float(types.one)) {
        type_auto_cast(block, &ir->binary.two, types.two, types.one);
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_float(types.two)) {
        type_auto_cast(block, &ir->binary.one, types.one, types.two);
        result = type_float_full(types.two->scalar->bits_exp, types.two->scalar->bits_man);
      }
    }
    else {
      sem_error_binary_operand(ir, "-");
    }
  } break;
  case Ir_Kind_mul: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_scalar && types.two->kind == Type_Kind_scalar) {
      if (type_is_float(types.one) && type_is_float(types.two)) {
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_int(types.one) && type_is_int(types.two)) {
        Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
        Range one = scalar_limits(pair.one);
        Range two = scalar_limits(pair.two);
        I64 a = 0;
        I64 b = 0;
        I64 c = 0;
        I64 d = 0;
        B8 overflow = false;
        overflow |= __builtin_mul_overflow(one.lo, two.lo, &a);
        overflow |= __builtin_mul_overflow(one.hi, two.lo, &b);
        overflow |= __builtin_mul_overflow(one.lo, two.hi, &c);
        overflow |= __builtin_mul_overflow(one.hi, two.hi, &d);
        Range range;
        if (overflow) {
          range = (Range){ .lo = I64_MIN, .hi = I64_MAX };
        }
        else {
          range = (Range){ .lo = min(a, min(b, min(c, d))), .hi = max(a, max(b, max(c, d))) };
        }
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
      else if (type_is_float(types.one)) {
        type_auto_cast(block, &ir->binary.two, types.two, types.one);
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_float(types.two)) {
        type_auto_cast(block, &ir->binary.one, types.one, types.two);
        result = type_float_full(types.two->scalar->bits_exp, types.two->scalar->bits_man);
      }
    }
    else {
      sem_error_binary_operand(ir, "*");
    }
  } break;
  case Ir_Kind_div: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_scalar && types.two->kind == Type_Kind_scalar) {
      if (type_is_float(types.one) && type_is_float(types.two)) {
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_int(types.one) && type_is_int(types.two)) {
        Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
        I64 max_one = scalar_max(pair.one);
        I64 max_two = scalar_max(pair.two);
        I64 min_one = scalar_min(pair.one);
        I64 min_two = scalar_min(pair.two);
        I64 max = max_one / min_two;
        I64 min = min_one / max_two;
        Range range = { .lo = min, .hi = max };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
      else if (type_is_float(types.one)) {
        type_auto_cast(block, &ir->binary.two, types.two, types.one);
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_float(types.two)) {
        type_auto_cast(block, &ir->binary.one, types.one, types.two);
        result = type_float_full(types.two->scalar->bits_exp, types.two->scalar->bits_man);
      }
    }
    else {
      sem_error_binary_operand(ir, "/");
    }
  } break;
  case Ir_Kind_rem: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_scalar && types.two->kind == Type_Kind_scalar) {
      if (type_is_float(types.one) && type_is_float(types.two)) {
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_int(types.one) && type_is_int(types.two)) {
        Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
        I64 max_two = scalar_max(pair.two);
        Range range  = { .lo = 0, .hi = max_two-1 };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
      else if (type_is_float(types.one)) {
        type_auto_cast(block, &ir->binary.two, types.two, types.one);
        result = type_float_full(types.one->scalar->bits_exp, types.one->scalar->bits_man);
      }
      else if (type_is_float(types.two)) {
        type_auto_cast(block, &ir->binary.one, types.one, types.two);
        result = type_float_full(types.two->scalar->bits_exp, types.two->scalar->bits_man);
      }
    }
    else {
      sem_error_binary_operand(ir, "%");
    }
  } break;
  case Ir_Kind_or: {
    Type_Pair types = type_of_ir_binary(ir);
    if (types.one->kind == Type_Kind_scalar && types.two->kind == Type_Kind_scalar) {
      if (scalar_is_single(types.one->scalar) && scalar_is_single(types.one->scalar)) {
        I64 one = scalar_min(types.one->scalar);
        I64 two = scalar_min(types.two->scalar);
        result = type_int(one | two);
      }
      else {
        Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
        Range one = scalar_limits(pair.one);
        Range two = scalar_limits(pair.two);
        I64   or_one = bits_or_range(one.lo, one.hi);
        I64   or_two = bits_or_range(two.lo, two.hi);
        Range range  = { .lo = 0, .hi = or_one | or_two };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
    }
    else {
      sem_error_binary_operand(ir, "|");
    }
  } break;
  case Ir_Kind_xor: {
    sem_error_text(Diag_Code_unsupported_semantics, ir->loc,
                   "the bitwise xor operator");
  } break;
  case Ir_Kind_and: {
    sem_error_text(Diag_Code_unsupported_semantics, ir->loc,
                   "the bitwise and operator");
  } break;
  case Ir_Kind_eq: case Ir_Kind_ne:
  case Ir_Kind_lt: case Ir_Kind_le:
  case Ir_Kind_gt: case Ir_Kind_ge: {
    Type_Pair types = type_of_ir_binary(ir);
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_scalar)) {
      if (type_is_float(types.one) && type_is_float(types.two)) {
        result = type_float_full(types.one->scalar->bits_exp, types.two->scalar->bits_man);
      }
      Scalar_Pair pair = scalar_pair_of_ir_binary(ir);
      if (scalar_is_single(pair.one) && scalar_is_single(pair.two)) {
        I64 val = 0;
        if (type_is_float(types.one)) {
          I16 bits_exp = types.one->scalar->bits_exp;
          F64 x = sem_float_of_key(scalar_min(pair.one), bits_exp);
          F64 y = sem_float_of_key(scalar_min(pair.two), bits_exp);
          switch (ir->kind) {
          case Ir_Kind_eq: val = x == y; break;
          case Ir_Kind_ne: val = x != y; break;
          case Ir_Kind_lt: val = x <  y; break;
          case Ir_Kind_le: val = x <= y; break;
          case Ir_Kind_gt: val = x >  y; break;
          case Ir_Kind_ge: val = x >= y; break;
          default: break;
          }
        }
        else {
          I64 x = scalar_min(pair.one);
          I64 y = scalar_min(pair.two);
          switch (ir->kind) {
          case Ir_Kind_eq: val = x == y; break;
          case Ir_Kind_ne: val = x != y; break;
          case Ir_Kind_lt: val = x <  y; break;
          case Ir_Kind_le: val = x <= y; break;
          case Ir_Kind_gt: val = x >  y; break;
          case Ir_Kind_ge: val = x >= y; break;
          default: break;
          }
        }
        Range range = { .lo = val, .hi = val };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
      else {
        Range range = { .lo = 0, .hi = 1 };
        result = sem_scalar_reflow_binary(block, ir, range, types);
      }
    }
    else {
      Cstr op = "comparison";
      switch (ir->kind) {
      case Ir_Kind_eq: op = "=="; break;
      case Ir_Kind_ne: op = "!="; break;
      case Ir_Kind_lt: op = "<";  break;
      case Ir_Kind_le: op = "<="; break;
      case Ir_Kind_gt: op = ">";  break;
      case Ir_Kind_ge: op = ">="; break;
      default: break;
      }
      sem_error_binary_operand(ir, op);
    }
  } break;
  case Ir_Kind_pos: {
    Type* one = type_of_ir(ir->unary);
    if (one->kind == Type_Kind_scalar) {
      result = one;
    }
    else {
      sem_error_unary_operand(ir, "+");
    }
  } break;
  case Ir_Kind_neg: {
    Type* one = type_of_ir(ir->unary);
    if (type_is_float(one)) {
      result = type_float_full(one->scalar->bits_exp, one->scalar->bits_man);
    }
    else if (one->kind == Type_Kind_scalar) {
      // TODO: overflow/underflow
      Range limits = scalar_limits(one->scalar);
      if (limits.lo == I64_MIN) {
        sem_error_unary_operand(ir, "-");
      }
      Type* unsized = type_range(-limits.hi, -limits.lo);
      if (has_flag(one->flag, Type_Flag_size_defined)) {
        result = type_define_size(one->bits_size, unsized);
      }
      else {
        result = unsized;
      }
    }
    else {
      sem_error_unary_operand(ir, "-");
    }
  } break;
  case Ir_Kind_record: {
    Record* new_record  = arena_push_zero(sem.perm_arena, sizeof(Record));
    new_record->length  = ir->rec->length;
    new_record->names   = ir->rec->names;
    new_record->types   = arena_push(sem.perm_arena, new_record->length * sizeof(Type*));
    new_record->offsets = arena_push(sem.perm_arena, new_record->length * sizeof(I32));
    for (I32 i = 0; i < ir->rec->length; i++) {
      Type* type = type_of_ir(ir->rec->irs[i]);
      if (type->kind == Type_Kind_type) {
        type = type->type;
      }
      new_record->types[i] = type;
    }
    new_record->position_from_name   = ir->rec->position_from_name;
    new_record->position_from_offset = hash_map_init(sem.perm_arena, new_record->length);

    result = type_record(new_record);
  } break;
  case Ir_Kind_meet: {
    Type* type_one = type_of_ir(ir->binary.one);
    Type* type_two = type_of_ir(ir->binary.two);
    result = type_meet(type_one, type_two);
  } break;
  case Ir_Kind_span: {
    Type* unary = type_of_ir(ir->unary);
    Span* span = arena_push(sem.perm_arena, sizeof(Span));
    span->length = sem.type_len;
    if (unary->kind == Type_Kind_type) {
      unary = unary->type;
    }
    span->of_type = unary;
    result = type_span(span);
  } break;
  case Ir_Kind_array: {
    Type* one_type = type_of_ir(ir->binary.one);
    Type* two_type = type_of_ir(ir->binary.two);
    if (one_type->kind == Type_Kind_scalar) {
      if (scalar_is_single(one_type->scalar)) {
        I64 length = scalar_min(one_type->scalar);
        if (length < 0 || length > I32_MAX) {
          sem_error_type(Diag_Code_invalid_array_length, ir->loc, one_type);
        }
        Array* array = type_array_init(length);
        if (two_type->kind == Type_Kind_type) {
          two_type = two_type->type;
        }
        array->of_type = two_type;
        for (I32 i = 0; i < length; i++) {
          array->types[i] = two_type;
        }
        result = type_array(array);
      }
      else {
        sem_error_type(Diag_Code_invalid_array_length, ir->loc, one_type);
      }
    }
    else {
      sem_error_type(Diag_Code_invalid_array_length, ir->loc, one_type);
    }
  } break;
  case Ir_Kind_ptr_offset: {
    Type* of_type = type_of_ir(ir->binary.one);
    Type* at_type = type_of_ir(ir->binary.two);
    if (of_type->kind == Type_Kind_ptr) {
      if (at_type->kind == Type_Kind_scalar) {
        Type* declared = type_pointer_declared(of_type->pointer);
        Type* declared_of_type;
        if (declared->kind == Type_Kind_ptr) {
          declared_of_type = type_pointer_declared(declared->pointer);
        }
        else if (declared->kind == Type_Kind_array) {
          declared_of_type = declared->array->of_type;
        }
        else if (declared->kind == Type_Kind_span) {
          declared_of_type = declared->span->of_type;
        }
        else {
          sem_error_type(Diag_Code_invalid_field_base, ir->loc, declared);
        }
        Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
        pointer->stack_vars = hash_set_init(sem.perm_arena, of_type->pointer->stack_vars.len);
        pointer->declared = declared_of_type;
        result = type_pointer(pointer);
      }
      else {
        sem_error_operand(ir->loc, "[]", at_type);
      }
    }
    else if (of_type->kind == Type_Kind_array) {
      result = type_pointer_to(of_type->array->of_type);
    }
    else if (of_type->kind == Type_Kind_span) {
      result = type_pointer_to(of_type->span->of_type);
    }
    else {
      sem_error_operand(ir->loc, "[]", of_type);
    }
  } break;
  case Ir_Kind_position_offset: {
    Type* of_type = type_of_ir(ir->position.of);
    I64   at      = ir->position.at;
    if (of_type->kind == Type_Kind_ptr) {
      Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
      if (of_type->pointer->stack_vars.len > 0) {
        pointer->stack_vars = hash_set_init(sem.perm_arena, of_type->pointer->stack_vars.len);

        Var* first_var        = of_type->pointer->stack_vars.list[0];
        if (!first_var->declared || first_var->declared->kind != Type_Kind_record
            || at < 0 || at >= first_var->declared->record->length) {
          sem_error_operand(ir->loc, ".", of_type);
        }
        I32  first_offset     = first_var->declared->record->offsets[at];
        Var* first_var_field  = first_var->vars[at];
            pointer->declared = first_var_field->declared;
        hash_set_put(&pointer->stack_vars, first_var_field);
        for (I32 i = 1; i < of_type->pointer->stack_vars.len; i++) {
          Var* var    = of_type->pointer->stack_vars.list[i];
          I32  offset = var->declared->record->offsets[at];

          if (offset != first_offset) {
            sem_error_text(Diag_Code_join_layout_mismatch, ir->loc, 0);
          }

          Var* field_var = var->vars[at];
          pointer->declared = type_meet(pointer->declared, field_var->declared);
          hash_set_put(&pointer->stack_vars, field_var);
        }
      }
      else {
        sem_error_operand(ir->loc, ".", of_type);
      }
      result = type_pointer(pointer);
    }
    else if (of_type->kind == Type_Kind_record) {
      if (at < 0 || at >= of_type->record->length) {
        sem_error_operand(ir->loc, ".", of_type);
      }
      result = of_type->record->types[at];
    }
    else {
      sem_error_operand(ir->loc, ".", of_type);
    }
  } break;
  case Ir_Kind_name_offset: {
    Type* of_type = type_of_ir(ir->name_offset.of);
    if (of_type->kind == Type_Kind_ptr) {
      if (of_type->pointer->stack_vars.len > 0) {
        Var* first_var = of_type->pointer->stack_vars.list[0];
        I32  first_offset = sem_get_offset_by_name(block, ir, first_var, ir->name_offset.at);
        Var* first_var_field = sem_get_var_by_name(block, first_var, ir->name_offset.at);
        for (I32 i = 1; i < of_type->pointer->stack_vars.len; i++) {
          Var* var = of_type->pointer->stack_vars.list[i];
          I32  offset = sem_get_offset_by_name(block, ir, var, ir->name_offset.at);
          if (offset != first_offset) {
            sem_error_name(Diag_Code_join_field_mismatch, ir->loc, ir->name_offset.at);
          }
        }

        Pointer* pointer = arena_push(sem.perm_arena, sizeof(Pointer));
        pointer->stack_vars = hash_set_init(sem.perm_arena, of_type->pointer->stack_vars.len);
        pointer->declared = first_var_field->declared;
        hash_set_put(&pointer->stack_vars, first_var_field);

        for (I32 i = 1; i < of_type->pointer->stack_vars.len; i++) {
          Var* var = of_type->pointer->stack_vars.list[i];
          Var* field_var = sem_get_var_by_name(block, var, ir->name_offset.at);
          pointer->declared = type_meet(pointer->declared, field_var->declared);
          hash_set_put(&pointer->stack_vars, field_var);
        }
        result = type_pointer(pointer);
      }
      else {
        Type* declared = type_pointer_declared(of_type->pointer);
        if (declared->kind == Type_Kind_record) {
          Field field = type_record_get_by_name(declared->record, ir->name_offset.at);
          if (field.pos < 0) {
            sem_error_name(Diag_Code_unknown_field, ir->loc, ir->name_offset.at);
          }
          result = type_pointer_to(field.type);
        }
        else {
          sem_error_type(Diag_Code_invalid_field_base, ir->loc, declared);
        }
      }

    }
    else if (of_type->kind == Type_Kind_record) {
      Field field = type_record_get_by_name(of_type->record, ir->name_offset.at);
      if (field.pos < 0) {
        sem_error_name(Diag_Code_unknown_field, ir->loc, ir->name_offset.at);
      }
      result = field.type;
    }
    else {
      sem_error_operand(ir->loc, ".", of_type);
    }
  } break;
  case Ir_Kind_load: {
    Type* ptr_type = type_of_ir(ir->unary);
    if (ptr_type->kind == Type_Kind_ptr) {
      Pointer* pointer = ptr_type->pointer;
      Hash_Set stack_vars = pointer->stack_vars;
      if (stack_vars.len >= 1) {
        for (I32 i = 0; i < stack_vars.len; i++) {
          sem_var_declare(stack_vars.list[i]);
        }
        result = type_of_var(block, stack_vars.list[0]);
        for (I32 i = 1; i < stack_vars.len; i++) {
          Type* type = type_of_var(block, stack_vars.list[i]);
          result = type_join(result, type);
        }
      }
      if (result == sem.type_none) {
        result = type_pointer_declared(ptr_type->pointer);
      }
    }
    else if (ptr_type->kind == Type_Kind_array) {
      if (ptr_type->array->length == 0) {
        sem_error_type(Diag_Code_invalid_dereference, ir->loc, ptr_type);
      }
      result = ptr_type->array->types[0];
    }
    else {
      sem_error_type(Diag_Code_invalid_dereference, ir->loc, ptr_type);
    }
  } break;
  case Ir_Kind_store: {
    Type* lhs = type_of_ir(ir->binary.one);
    Type* rhs = type_of_ir(ir->binary.two);
    if (lhs->kind == Type_Kind_ptr) {
      Pointer* pointer = lhs->pointer;
      Hash_Set stack_vars = pointer->stack_vars;
      if (stack_vars.len == 1) {
        Var* var = stack_vars.list[0];
        type_of_var_put(block, &ir->binary.two, var, rhs);
      }
      else if (stack_vars.len > 1) {
        for (I32 i = 0; i < stack_vars.len; i++) {
          Var* var = stack_vars.list[i];
          Type* old_type = type_of_var(block, var);
          Type* new_type = type_join(old_type, rhs);
          type_of_var_put(block, &ir->binary.two, var, new_type);
        }
      }
      else {
        Type* declared = type_pointer_declared(lhs->pointer);
        if (type_is_subtype(block, rhs, declared)) {
          type_auto_cast(block, &ir->binary.two, rhs, declared);
        }
        else {
          sem_error_type_mismatch(sem_loc(0), declared, rhs);
        }
      }
    }
    else {
      sem_error_type(Diag_Code_invalid_store, ir->loc, lhs);
    }
    result = rhs;
  } break;
  case Ir_Kind_ptr: {
    Type* type_unary = type_of_ir(ir->unary);

    if (type_unary->kind == Type_Kind_type) {
      result = type_pointer_to(type_unary->type);
    }
    else if (ir->unary->kind == Ir_Kind_load) {
      Type* ptr_type = type_of_ir(ir->unary->unary);
      if (ptr_type->kind == Type_Kind_ptr) {
        result = ptr_type;
      }
      else {
        sem_error_operand(ir->loc, "&", ptr_type);
      }
    }
    else {
      Var* var = &new(irgen.vars);
           var->kind   = Var_Kind_pointee;
           var->state  = Var_State_unresolved;
           var->global = irgen.scope_stack.length == 1;
           var->name   = irgen.str_anon;
           var->declared_ir = ir->unary;
           var->declared = type_unary;
      result = type_pointer_var(ir->var);
      type_of_var_put(block, 0, var, type_unary);
    }
  } break;
  case Ir_Kind_fun: {
    if (ir->fun->kind == Fun_Kind_macro) {
      Function* function = arena_push(sem.perm_arena, sizeof(Function));
      function->fun = ir->fun;
      function->foreign_name = irgen.str_nil;
      function->foreign_id   = 0;
      function->arg = sem.type_none;
      function->ret = sem.type_none;
      // ir->fun->type = type_fun(function);
      result = type_function(function);
    }
    else {
      Fun* save_fun = sem.current_fun;
      result = sem_fun(ir->fun);
      sem.current_fun = save_fun;
    }
  } break;
  case Ir_Kind_call: {
    Type_Pair types = type_of_ir_binary(ir);
    Type* fun_type = types.one;
    Type* arg_type = types.two;
    if (fun_type == sem.fun_bits) {
      if (arg_type->kind == Type_Kind_scalar) {
        if (scalar_is_single(arg_type->scalar)) {
          I64 val = scalar_min(arg_type->scalar);
          if (0 <= val && val <= I16_MAX) {
            Type* type = &new(sem.types);
            type->kind = Type_Kind_none;
            type->flag = Type_Flag_size_defined;
            type->bits_size = val;
            type->bits_align = align_up(val, 8);
            type->value = 0;
            result = type_in_set(type);
          }
          else {
            sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                            "an integer constant between 0 and 32767",
                            sem_type_name(arg_type));
          }
        }
        else {
          sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                          "a single integer constant", sem_type_name(arg_type));
        }
      }
      else {
        sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                        "a single integer constant", sem_type_name(arg_type));
      }
    }
    else if (fun_type == sem.fun_float) {
      if (arg_type->kind == Type_Kind_record && arg_type->record->length == 2) {
        Type* exp_type  = arg_type->record->types[0];
        Type* mant_type = arg_type->record->types[1];
        if (exp_type->kind == Type_Kind_scalar && mant_type->kind == Type_Kind_scalar
         && scalar_is_single(exp_type->scalar) && scalar_is_single(mant_type->scalar)) {
          I64 exp  = scalar_min(exp_type->scalar);
          I64 mant = scalar_min(mant_type->scalar);
          if (0 < exp && exp < I16_MAX && 0 < mant && mant < I16_MAX) {
            result = type_float_full((I16)exp, (I16)mant);
          }
          else {
            sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                            "positive integer constants below 32767",
                            sem_type_name(arg_type));
          }
        }
        else {
          sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                          "a record of two single integer constants",
                          sem_type_name(arg_type));
        }
      }
      else {
        sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                        "a record of two integer constants",
                        sem_type_name(arg_type));
      }
    }
    else if (fun_type == sem.fun_len) {
      if (arg_type->kind == Type_Kind_record) {
        result = type_int(arg_type->record->length);
      }
      else if (arg_type->kind == Type_Kind_array) {
        result = type_int(arg_type->array->length);
      }
      else {
        sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                        "a record or array", sem_type_name(arg_type));
      }
    }
    else if (fun_type == sem.fun_foreign_c) {
      if (arg_type->kind == Type_Kind_array) {
        Array* array = arg_type->array;
        I32 length = array->length;
        if (array->of_type == sem.bytes_range) {
          C8* chars = arena_push(sem.perm_arena, length * sizeof(C8));
          for (I32 i = 0; i < length; i++) {
            Type* char_type = array->types[i];
            if (char_type->kind != Type_Kind_scalar || !scalar_is_single(char_type->scalar)) {
              sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                              "a single integer constant", sem_type_name(char_type));
            }
            I64 val = scalar_min(char_type->scalar);
            if (I8_MIN <= val && val <= I8_MAX) {
              chars[i] = (I8)val;
            }
            else {
              sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                              "a byte value between -128 and 127", sem_type_name(char_type));
            }
          }
          Str* str = str_from_range(chars, chars+length);
          Type* type = &new(sem.types);
          type->kind = Type_Kind_none;
          type->str = str;
          result = type_in_set(type);
        }
        else {
          sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                          "a byte array", sem_type_name(arg_type));
        }
      }
      else {
        sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                        "a byte array", sem_type_name(arg_type));
      }
    }
    else if (fun_type == sem.fun_type_type) {
      result = type_type(type_type(arg_type));
    }
    else if (fun_type->kind == Type_Kind_type) {
      result = type_cast(block, &ir->binary.two, arg_type, fun_type->type);
    }
    else if (fun_type->kind == Type_Kind_fun) {
      if (fun_type->function->fun->kind == Fun_Kind_macro) {
        Fun*   fun         = fun_type->function->fun;
        Block* entry_block = fun->blocks->base[0];
        Ir     macro_par   = *entry_block->irs->base[1];
        Fun*   save_fun    = sem.current_fun;

        *entry_block->irs->base[1] = *ir->binary.two;
        fun_type = sem_fun(fun);

        *entry_block->irs->base[1] = macro_par;
        sem.current_fun            = save_fun;

        result = fun_type->function->ret;
      }
      else {
        Function* fun = fun_type->function;
        if (type_is_subtype(block, arg_type, fun->arg)) {
          type_auto_cast(block, &ir->binary.two, arg_type, fun->arg);
          result = fun->ret;
        }
        else {
          sem_error_call_mismatch(ir->loc, fun->arg, arg_type);
        }
      }
    }
    else if (fun_type->kind == Type_Kind_record) {
      sem_error_text(Diag_Code_unsupported_semantics, ir->loc, "calling a record");
    }
    else if (fun_type->kind == Type_Kind_none) {
      if (fun_type->flag == Type_Flag_size_defined) { // NOTE: bits function
        if (arg_type->kind == Type_Kind_type) {
          arg_type = arg_type->type;
        }
        result = type_define_size(fun_type->bits_size, arg_type);
      }
      else if (fun_type->str) {
        if (arg_type->kind == Type_Kind_fun) { // NOTE: foreign.c "c_name"
          result = type_function_foreign(arg_type->function, fun_type->str);
        }
        else {
          sem_error_types(Diag_Code_call_argument_mismatch, ir->loc,
                          "a function type", sem_type_name(arg_type));
        }
      }
      else {
        sem_error_operand(ir->loc, "()", fun_type);
      }
    }
    else {
      sem_error_operand(ir->loc, "()", fun_type);
    }
  } break;
  case Ir_Kind_run: {
    Type* type = type_of_ir(ir->unary);
    if (type_is_const(type)) {
      result = type;
    }
    else {
      sem_error_text(Diag_Code_unsupported_semantics, ir->loc,
                     "#run on a non-constant expression");
    }
  } break;
  case Ir_Kind_range: {
    if (type_kind_of_ir_binary_operands_equal(ir, Type_Kind_scalar)) {
      Scalar_Pair pair = scalar_pair_of_ir_binary(ir);

      if (scalar_is_single(pair.one) && scalar_is_single(pair.two)) {
        I64 val_one = scalar_min(pair.one);
        I64 val_two = scalar_min(pair.two);
        if (val_one < val_two) {
          result = type_range(val_one, val_two);
        }
        else {
          sem_error_binary_operand(ir, "..");
        }
      }
      else {
        sem_error_binary_operand(ir, "..");
      }
    }
    else {
      sem_error_binary_operand(ir, "..");
    }
  } break;
  default: {
    sem_error_text(Diag_Code_unsupported_semantics, ir->loc, "this expression");
  }
  }
  fa_add(block->sem_irs, ir);
  type_of_ir_put(ir, result);
  if (!result) {
    sem_error_text(Diag_Code_unsupported_semantics, ir->loc, "this expression");
  }
  sem.current_ir = saved_ir;
  sem.current_loc = saved_loc;
}

void sem_block(Block* block) {
  Block* saved_block = sem.current_block;
  sem.current_block = block;
  for (I32 i = 0; i < block->irs->length; i++) {
    sem_ir(block, block->irs->base[i]);
  }
  if (block->state == Block_State_reachable) {
    if (block->kind == Block_Kind_branch) {
      Type* cond_type = type_of_ir(block->branch.cond);
      if (!cond_type || cond_type->kind != Type_Kind_scalar) {
        sem_error_type(Diag_Code_invalid_condition, block->branch.cond->loc, cond_type);
      }
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
  sem.current_block = saved_block;
}

void sem_worklist_push(Block* block) {
  if (!block->is_present_in_worklist) {
    block->is_present_in_worklist = true;
    fa_add(sem.current_fun->worklist, block);
  }
}

B8 sem_worklist_is_not_empty() {
  return sem.current_fun->worklist->length != 0;
}

Block* sem_worklist_pop(void) {
  Block* block = fa_pop(sem.current_fun->worklist);
  block->is_present_in_worklist = false;
  return block;
}

void sem_scc_block(Block* block);
I32 sem_scc_block_jump(Block* parent, Block* kid) {
  if (!kid->is_scc_visited) {
    sem_scc_block(kid);
    parent->low_sccid = min(parent->low_sccid, kid->low_sccid);
  }
  else if (kid->is_on_scc_stack) {
    kid->is_loop_head = true;
    parent->low_sccid = min(parent->low_sccid, kid->sccid);
  }
  return parent->low_sccid;
}

void sem_scc_block(Block* block) {
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
      sem_worklist_push(pop_block);
      // printf("b%ld, sccid: %d\n", pop_block-irgen.blocks.base, pop_block->sccid);
    } while (block != pop_block);
    sem_worklist_push(block);
    // printf("b%ld, sccid: %d\n", block-irgen.blocks.base, block->sccid);
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
  if (block->preds.length == 0) {
    block->state = Block_State_unreachable;
  }
}

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

Type* sem_fun(Fun* fun) {
  sem.current_fun = fun;
  Type* fun_type = type_of_fun(fun);
  if (fun_type) return fun_type;
  fun->worklist = arena_push(sem.temp_arena, sizeof(Blocks) + sizeof(Block*)*fun->blocks->length);
  fun->worklist->length = 0;
  printf("fun: %s\n", fun->name->base);

  if (fun->kind != Fun_Kind_macro) {
    sem_var_declare(fun->arg_var->var);
  }

  for (I32 b = 0; b < fun->blocks->length; b++) {
    Block* block = fun->blocks->base[b];
    sem_init_block_preds(block);
  }
  Block* entry_block = fun->blocks->base[0];
  sem_scc_block(entry_block);
  entry_block->state = Block_State_reachable;

  while (sem_worklist_is_not_empty()) {
    Block* block = sem_worklist_pop();
    // if (block->is_loop_head) {
    //   printf(" *b%ld, scc: %d\n", block-irgen.blocks.base, block->sccid);
    // }
    // else {
    //   printf("  b%ld, scc: %d\n", block-irgen.blocks.base, block->sccid);
    // }
    sem_block(block);
  }
  // Blocks*  rpo = sem_cfg_rpo(fun);
  //                sem_cfg_doms(fun, rpo);
  // for (I32 i = 0; i < rpo->length; i++) {
  //   Block* block = rpo->base[i];
  //   sem_block(block);
  // }

  Function* function = arena_push(sem.perm_arena, sizeof(Function));
  function->foreign_name = irgen.str_nil;
  function->foreign_id   = 0;
  function->fun = fun;
  function->arg = fun->arg_var->var->declared;
  function->ret = type_of_var(fun->ret_block, fun->ret_ir->var);

  if (function->ret->kind == Type_Kind_type) {
    function->ret = function->ret->type;
  }

  fun->ret_ir->var->declared = function->ret;
  Type* ret_type = type_of_ir(fun->ret_ir);
  assert(ret_type->kind == Type_Kind_ptr);
  ret_type->pointer->declared = function->ret;
  fun->type = type_function(function);

  {
    Type* type = function->ret;
    Var* var = fun->ret_ir->var;
    var->declared = type;
    sem_var_kind(var);
    sem_record_declare_fields(var, type);
  }

  {
    Type* type = function->arg;
    Var* var = fun->arg_var->var;
    var->declared = type;
    sem_var_kind(var);
    sem_record_declare_fields(var, type);
  }

  arena_release_mark(sem.temp_arena, fun->worklist);
  printf("end: %s\n", fun->name->base);
  return fun->type;
}

void sem_funs(Arena* arena, Funs funs, Diags* diags) {
  Arena temp = arena_init(arena->capacity);
  sem.perm_arena = arena;
  sem.temp_arena = &temp;
  sem.diags = diags;
  sem.current_ir = 0;
  sem.current_loc = LOC_NONE;
  sem.current_block = 0;
  sem.funs = funs;
  sem.sccid = 0;
  sem.scc_stack = arena_push(arena, sizeof(Blocks) + sizeof(Block*)*irgen.blocks.length);
  sem.scc_stack->length = 0;
  sem.type_of_irs = hash_map_init(arena, 3 * irgen.irs.length);
  sem.types.base = arena_push(arena, irgen.irs.length * sizeof(Type));
  sem.types.length = 0;

  sem.elements = arena_push(arena, 3 * irgen.irs.length * sizeof(I32));

  fa_init(sem.perm_arena, sem.foreign_funs, irgen.irs.length);
  fa_add(sem.foreign_funs, 0);

  sem.type_set      = hash_set_init(arena, irgen.irs.length);
  sem.type_type_set = hash_set_init(arena, irgen.irs.length);
  sem.record_set    = hash_set_init(arena, irgen.irs.length);
  sem.scalar_set    = hash_set_init(arena, irgen.irs.length);
  sem.pointer_set   = hash_set_init(arena, irgen.irs.length);
  sem.array_set     = hash_set_init(arena, irgen.irs.length);
  sem.span_set      = hash_set_init(arena, irgen.irs.length);
  sem.function_set  = hash_set_init(arena, irgen.irs.length);
  sem.join_set      = hash_set_init(arena, irgen.irs.length);

  sem.type_none = &new(sem.types);
  sem.type_none->kind = Type_Kind_none;

  sem.type_empty_record = &new(sem.types);
  sem.type_empty_record->kind = Type_Kind_record;
  sem.type_empty_record->record = arena_push(sem.perm_arena, sizeof(Record));
  sem.type_empty_record->record->length = 0;

  sem.fun_bits = &new(sem.types);
  sem.fun_bits->kind = Type_Kind_none;
  type_of_ir_put(irgen.irid_bits, sem.fun_bits);

  sem.fun_float = &new(sem.types);
  sem.fun_float->kind = Type_Kind_none;
  type_of_ir_put(irgen.irid_float, sem.fun_float);

  sem.fun_len = &new(sem.types);
  sem.fun_len->kind = Type_Kind_none;
  type_of_ir_put(irgen.irid_len, sem.fun_len);

  sem.fun_type_type = &new(sem.types);
  sem.fun_type_type->kind = Type_Kind_none;
  type_of_ir_put(irgen.irid_type, sem.fun_type_type);

  {
    sem.str_c = str_from_cstr("c");

    sem.fun_foreign_c = &new(sem.types);
    sem.fun_foreign_c->kind = Type_Kind_none;
    Record* record_foreign = type_record_init(1);
    record_foreign->types[0] = sem.fun_foreign_c;
    record_foreign->names[0] = sem.str_c;
    hash_map_put_i32(&record_foreign->position_from_name, sem.str_c, 0);
    sem.rec_foreign = type_record(record_foreign);
    type_of_ir_put(irgen.irid_foreign, sem.rec_foreign);
  }

  sem.bytes_range = type_range(I8_MIN, I8_MAX);

  sem.type_len = type_define_size(64, type_range(0, I64_MAX));
  sem.type_zero = type_int(0);

  sem.str_ptr = str_from_cstr("ptr");

  sem.str_from_i8_array = arena_push(arena, sizeof(Fun));
  {
    Fun* fun = &new(irgen.funs);
    sem.str_from_i8_array = fun;
  }

  // for (I32 i = 0; i < irgen.builtins->len; i++) {
  //   Str* str = irgen.builtins->list[i];
  //   Symbol* sym = hash_map_get(irgen.builtins, str);
  //   sem_ensure_declared(sym->var_ir->var);
  // }

  if (setjmp(sem.fatal) != 0) {
    sem.fatal_ready = false;
    arena_free(&temp);
    return;
  }
  sem.fatal_ready = true;

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    if (fun->kind != Fun_Kind_macro) {
      sem_fun(fun);
    }
  }
  sem.fatal_ready = false;
  arena_free(&temp);
}

void _test_sem(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Cstr builtin        = file_read("basic.i");
  I32  builtin_length = (I32)file_size("basic.i");
  I32  source_length  = strlen(source);
  I32  length         = source_length + builtin_length + 32;

  Arena arena         = arena_init(KB(4) * length);
                        str_init(&arena, 2*length);

  Source_Map sources = {0};
  Diags diags = diags_init(&arena, 256);
  File_Id builtin_file = source_add(&sources, &arena, "basic.i", builtin, builtin_length);

  Tokens builtin_tokens = lex_source(&arena, &sources, builtin_file, &diags);
  Ast_Block ast         = parse_tokens(&arena, builtin_tokens, &diags);

  File_Id source_file = source_add(&sources, &arena, file_name, source, strlen(source));
  Tokens source_tokens = lex_source(&arena, &sources, source_file, &diags);
  Ast_Block source_ast = parse_tokens(&arena, source_tokens, &diags);

  Ast_Node* node = ast_new_node(&arena, Ast_Kind_block);
  node->block    = source_ast;

  fa_init(&arena, ast.list, 1);
  fa_add(ast.list, node);

  Funs funs            = irgen_ast(&arena, ast, length, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stdout, false);
    assert(0);
  }
  {
    Cstr result          = cstr_from_funs(&arena, funs);
    printf("\n%s", result);
  }

                         sem_funs(&arena, funs, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stdout, false);
    assert(0);
  }
  {
    Cstr result          = cstr_from_sem(&arena, funs);
    test_at_source(result, expected, file_name, line, source);
  }
  arena_free(&arena);
}

void _test_pipeline_diag(Cstr source, Diag_Code code, Cstr file_name, I32 line) {
  Cstr builtin        = file_read("basic.i");
  I32  builtin_length = (I32)file_size("basic.i");
  I32  source_length  = strlen(source);
  I32  length         = source_length + builtin_length + 32;

  Arena arena         = arena_init(KB(4) * length);
                        str_init(&arena, 2*length);

  Source_Map sources = {0};
  Diags diags = diags_init(&arena, 256);
  File_Id builtin_file = source_add(&sources, &arena, "basic.i", builtin, builtin_length);

  Tokens builtin_tokens = lex_source(&arena, &sources, builtin_file, &diags);
  Ast_Block ast         = parse_tokens(&arena, builtin_tokens, &diags);

  File_Id source_file = source_add(&sources, &arena, file_name, source, strlen(source));
  Tokens source_tokens = lex_source(&arena, &sources, source_file, &diags);
  Ast_Block source_ast = parse_tokens(&arena, source_tokens, &diags);

  Ast_Node* node = ast_new_node(&arena, Ast_Kind_block);
  node->block    = source_ast;

  fa_init(&arena, ast.list, 1);
  fa_add(ast.list, node);

  Funs funs = irgen_ast(&arena, ast, length, &diags);
  if (!diags_has_errors(&diags)) {
    sem_funs(&arena, funs, &diags);
  }

  B8 found = false;
  for (I32 i = 0; i < diags.length; i++) {
    if (diags.base[i].code == code) {
      found = true;
      break;
    }
  }
  if (!found) {
    printf("pipeline diagnostic test failed at %s:%d: expected %s\n",
           file_name, line, cstr_from_diag_code(code));
    diag_print_all(&diags, &sources, stdout, false);
    fflush(stdout);
    assert(0);
  }
  arena_free(&arena);
}

#define test_diag(source, code) _test_pipeline_diag(source, code, __FILE__, __LINE__)

#define test(source, expected) _test_sem(source, expected, __FILE__, __LINE__)

void sem_test(void) {
  // test("a:(0..2); if a && 1 do a else a", "");
  // test("a:(0..2); c:(0..1) = a && 1; if c do a else a", "");
  // test("a:(0..2) = 0; b:(0..2); if (0..1) do a = 1; c:B8 = a && b; c = 0..1; if c do a+b else a+b", "");
  // test("a:(0..2) = 0; b:(0..2); if (0..1) do a = 1; c:B8 = a && b; c = 1; if c do a+b else a+b", "");
  // test("c:0..1 = 0; x:0..2; while 1 do { if c do x; c = 0..1 }", "");
  // test("a:(0..2); b:(0..2); c:(0..2); if (a && b) && c do a+b+c else a+b+c", "");
  test("c:0..1 = 0; a:2..5; if (0..1) do { c = 1; a = 3 } else { c = 0; a = 4 }; c = 1; if c do a else a", ""); // FIX: this is incorrect a should be 3..4 not 3
  // test_diag("a", Diag_Code_undefined_name);
  // test_diag("a : 1\n{ a : 2 }", Diag_Code_shadowed_declaration);
  // test_diag("a : 1\na = 2", Diag_Code_assign_to_constant);
  // test_diag("a : (x:I32)\na.y", Diag_Code_unknown_field);
  // test_diag("a : (x:I32)\na = (y:I32)", Diag_Code_type_mismatch);
  // test_diag("a : \"\\q\"", Diag_Code_invalid_escape);
  // test_diag("f : (x:I32) -> x\nf(\"s\")", Diag_Code_call_argument_mismatch);
  // test_diag("a : 1 + \"s\"", Diag_Code_invalid_operand);
  // test_diag("if (x:1) do 1", Diag_Code_invalid_condition);
  // test_diag("a : b\nb : a", Diag_Code_cyclic_declaration);
  // test_diag("1 = 2", Diag_Code_invalid_assignment_target);
  // test_diag("a : (if 1 do 2)", Diag_Code_unsupported_construct);
  // test_diag("break", Diag_Code_unsupported_construct);
  // test("a: (x:I32 1; y: I32 2) \\ (x:I32 2; z: I32 3); if a.x == 1 do a.y else { if 4 do a.z }", "");
  // test("a: (x:I32 1; y: I32 2) \\ (x:I32 2; z: I32 3); if a.x == 1 do a.x + a.y else a.x + a.z", "");
  // test("A:(a:I32 0; b:I32 5); B:(a:I32 1; c:I32 4); ab: A\\B; if ab.a == 1 do ab.c else ab.b", "");
  // test("A:(a:I32 0; b:I32 5); B:(a:I32 1; c:I32 4); ab: A\\B; if ab.a == 1 do ab.c", "");
  // test("A:(a:I32 0; b:I32 5); B:(a:I32 1; c:I32 4); ab: A\\B; ab.a", "");
  // test("A:(a:(x:I32 1); b:I32 5); B:(a:(x:I32 2); c:I32 4); ab: A\\B; if ab.a.x == 1 do ab.b else ab.c", "");
  // test("A:(a:(b:(x:I32 1); p:I32 5); q:I32 7); B:(a:(b:(x:I32 2); r:I32 4); s:I32 6); ab: A\\B; if ab.a.b.x == 1 do ab.q else ab.s", "");
  // test("A:(a:(x:I32 1); r:(y:I32 5)); B:(a:(x:I32 2); r:(y:I32 6)); ab: A\\B; ab.r; if ab.a.x == 1 do ab.r.y else ab.r.y", "");
  // test("a:(x:F32); a = (10;); a.x", "");
  // test("A:(a:I32; b:I32); ab: A; ab.a ", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32", "");
  // test("a: Str = \"Hi\"; a.len; a[0]", "");
  // test("a: I32, (x:I32); a = 1; a", "");
  // test("a: @I32, (x:I32); b: I32; a = @b; a@ = 7; a@; a.x = 4; a.x + b + a@; a", "");
  // test("a: Str; a.len = 10; a.len;", "");
  // test("a: @I32, (x:I32); a = @b; a.x = 4; a", "");
  // test("a: Str; a.len = 1", "");
  // test("a:(x:I32; y:I16); a = (1; 2); a.x + a.x; a.y+a.y; a", "");
  // test("a:(x:I32; y:I32); a = (y:1; x:2); a.x", "");
  // test("a: 8'bits 0..100 = 30; a+10", "");
  // test("a: I32 = 3; a+a", "");
  // test("B8: type 8'bits (0\\1); a: B8 = 0; a = 1; if a do {c: B8 = 0; a+c}; a+a", "");
  // test("foo:#c foo () -> 1", "");
  // test("a:I32 = 0; while 0\\1 do {a = a+1; a}; a", "");
  // test("n:I32; i:I32 = 0; while n > 0 do { n = n / 10; i = i + 1 }; i+n", "");
  // test("a:[2]I32; a[0] = 1; a[0] + a[1]", "");
  // test("a:12\\13 = 12; b:I32; b = a", "");
  // test("a:I32 = 70; c:@a\\@a; a + c@", "");
  // test("a:I32 = 70; b:I32 = 50; a+b; c:@a\\@b; c@ = 30; a+b", "");
  // test("a:I32 = 70; b:@I32 = @a;", "");
  // test("putchar: #c putchar (char:I32) -> I32; a:(x:66; y:I32); putchar(a.x)", "");
  // test("a:(x:I32; y:I32); a.x = 1; a.x + a.x", "");
  // test("a: (x:I32; y:I32); b:(x:I32; y:I32); c:@a\\@b; a = (x:1; y:2); b = (x:3; y:4); c@.x", "");
  // test("a: (x:1\\2; y:I32); b:(x:2\\3; y:I32); c:@a\\@b; a = (x:1; y:2); b = (x:3; y:4); c@.x = 2", "");
  // test("a: (x:I32; y:I32); b:(y:I32; x:I32); c:@a\\@b; a = (x:1; y:2); b = (x:3; y:4); c@.x", "");
  // test("a: (x:0\\1\\3; y:2\\4\\5); b: @(0\\1\\3); a=(x:1; y:2); b=@a.x; if b@ do { a = (x:0; y:5); b@=3; a.y = 4; }", "");
  // test("a: (x:0\\1); a.x=1; if 2 do { a.x = 0 }; if a.x == 0 do a.x", "");
  // test("a: 0\\1; a=1; if 2 do { a = 0 }; a", "");
  // test("a: 0\\1; a=1; if 2 do { a = 0 }; if a do {a+a}; a+a", "");
  // test("a: (x:0\\1; y:2\\3); a = (x:0; y:2); if 5 do { a = (x:1; y:3) }; if a.x == 1 do {a.x}", "");
  // test("a: 0\\1; b: 1\\2; c: @(0\\1\\2); a = 0; b = 2; c = @a; if 3 do { c = @b }; c @= 1; a+b", "");
  // test("a: 0\\1; b: 1\\2; c: @(0\\1\\2); a = 0; b = 2; c = @a; if 3 do { c = @b }; if c == @a do {c @= 1}; a+b", "");
  // test("a: 1\\2; while 2 do { a = 1 }; a+a", "");
  // test("a: 1\\2\\3; a = 2; while 2 do { if 3 do { a = 1 } }; a+a", "");
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
  // test("a:I32; b: B8; a = b; a", "");
  // test("str: Str; str[0]", "");
  // test("a:I32; a=0; while a < 8 do {a = a + 1}; a", "");
  // test("a:I32; a=0; while a != 8 do {a = a + 2}; a", "");
  // test("a:I32; a=0; while a != 8 do {a = a + 3}; a", "");
  // test("a:I32; a=5; while a > 0 do {a = a - 1}; a", "");
  // test("a:I32; b:I32; a=0; while a < 8 do {b=0; while b != 3 do { b = b + 1 }; a = a + 1}; a", "");
  // test("a:I32; b:I32; a=0; b=0; while a < 8 do {while b != 3 do { b = b + 1 }; a = a + 3}; a+b", "");
  // test("a:I32; if a != 10 do {a = 1}; a", "");
  // test("I32 = 0; I32", "");
  // test("Vec2 : (x:I32; y:I32); Vec2 = (x:1+2; y:2+3); Vec2.x + Vec2.y", "");
  // test("a:I32 = 2; a=3; a+a", "");
  // test("foo:(a:I32; b:I32) -> a+b; foo(b:1; a:2)", "");
  // test("foo:(a:I32) -> { if 1\\2 return 2 else return 3 }; foo(2)", "");
  // test("foo:() I32 -> I32 bar(); bar:()->foo()", "");
  // test("if 1\\2 do 3 else 4;", "");
  // test("a:I32; b:@I32; b = @a; b@ = 1", "");
  // test("a:I32; foo:() -> a;", "");
  // test("putchar: #c putchar (char:I32) -> I32", "");
  // test("a : 1; a = 1", "");
  // test("a:I32; b:I32; p:@I32; p = @a; p = @b", "");
  // test("putchar: #c putchar (char:I32) -> I32; putchar(60)", "");
  // test("f:#c foo ()->1", "");
  // test("foo : (a:I32; b:I32) -> a+b", "");
  // test("foo : (a:I32) -> a+2; foo(1)", "");
}

#undef test
