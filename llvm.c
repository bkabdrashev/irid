#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Types.h>
#include <llvm-c/LLJIT.h>
#include <unistd.h>

typedef struct LLVM_Gen_Blocks LLVM_Gen_Blocks;
struct LLVM_Gen_Blocks {
  I32 length;
  LLVMBasicBlockRef base[];
};

typedef struct LLVM_Gen_Irs LLVM_Gen_Irs;
struct LLVM_Gen_Irs {
  I32 length;
  LLVMValueRef base[];
};

typedef struct LLVM_Gen LLVM_Gen;
struct LLVM_Gen {
  Arena*             perm_arena;
  LLVMValueRef*      irs;
  LLVMValueRef*      vars;
  LLVMBasicBlockRef* blocks;
  LLVMTypeRef*       types;
  LLVMValueRef*      funs;
  LLVMValueRef*      foreign_funs;

  LLVMModuleRef      module;
  LLVMContextRef     context;
  LLVMBuilderRef     builder;
  LLVMValueRef       function;

  LLVMValueRef       none;
};

LLVM_Gen llvm_gen;

LLVMTypeRef llvm_of_type(Type* type) {
  I32 typeid = type - sem.types.base;
  LLVMTypeRef result = llvm_gen.types[typeid];
  if (result) {
    return result;
  }
  // else if (has_flag(type->flag, Type_Flag_comptime_const)) {
  //   // TODO: introduces asan crashes
  //   result = LLVMIntTypeInContext(llvm_gen.context, 0);
  // }
  else {
    switch (type->kind) {
    case Type_Kind_none: {
      result = LLVMIntTypeInContext(llvm_gen.context, 1);
    } break;
    case Type_Kind_type: {
      result = LLVMIntTypeInContext(llvm_gen.context, 0);
    } break;
    case Type_Kind_scalar: {
      if (type_is_float(type)) {
        if (type->scalar->bits_exp == F32_EXP) {
          result = LLVMFloatTypeInContext(llvm_gen.context);
        }
        else if (type->scalar->bits_exp == F64_EXP) {
          result = LLVMDoubleTypeInContext(llvm_gen.context);
        }
        else {
          assert(0);
        }
      }
      else {
        result = LLVMIntTypeInContext(llvm_gen.context, type->bits_size);
      }
    } break;
    case Type_Kind_ptr: {
      Type* declared = type_pointer_declared(type->pointer);
      LLVMTypeRef pointer_to = llvm_of_type(declared);
      result = LLVMPointerType(pointer_to, 0);
    } break;
    case Type_Kind_join_records: {
      result = LLVMIntTypeInContext(llvm_gen.context, type->bits_size);
    } break;
    case Type_Kind_record: {
      I32 length = type->record->length;
      LLVMTypeRef* field_types = arena_push(llvm_gen.perm_arena, (length + 1) * sizeof(LLVMTypeRef));
      for (I32 i = 0; i < length; i++) {
        Type* field_type = type->record->types[i];
        if (field_type->bits_size == 0) {
          field_types[i] = LLVMIntTypeInContext(llvm_gen.context, 0);
        }
        else {
          field_types[i] = llvm_of_type(field_type);
        }
      }
      I32 bits_nopad = type_record_bits_nopad(type->record, type->bits_align);
      if (bits_nopad < type->bits_size) {
        LLVMTypeRef int_pad = LLVMIntTypeInContext(llvm_gen.context, type->bits_size - bits_nopad);
        field_types[length++] = int_pad;
      }
      result = LLVMStructTypeInContext(llvm_gen.context, field_types, length, false);
    } break;
    case Type_Kind_array: {
      LLVMTypeRef llvm_field_type = llvm_of_type(type->array->of_type);
      I32 length = type->array->length;
      result = LLVMArrayType2(llvm_field_type, length);
    } break;
    case Type_Kind_span: {
      LLVMTypeRef llvm_field_type = llvm_of_type(type->span->of_type);
      LLVMTypeRef* field_types = arena_push(llvm_gen.perm_arena, 2 * sizeof(LLVMTypeRef));
      field_types[0] = llvm_of_type(type->span->length);
      field_types[1] = LLVMPointerType(llvm_field_type, 0);
      result = LLVMStructTypeInContext(llvm_gen.context, field_types, 2, false);
    } break;
    case Type_Kind_fun: {
      LLVMTypeRef* arg_types;
      I32 arg_count = 0;
      if (type->function->arg->kind == Type_Kind_none) {
        arg_types = 0;
        arg_count = 0;
      }
      else if (type->function->foreign_name != irgen.str_nil) {
        if (type->function->arg->kind == Type_Kind_record) {
          Record* record = type->function->arg->record;
          arg_types = arena_push(llvm_gen.perm_arena, record->length * sizeof(LLVMTypeRef));
          for (I32 i = 0; i < record->length; i++) {
            Type* arg_type = record->types[i];
            arg_types[i] = llvm_of_type(arg_type);
          }
          arg_count = record->length;
        }
        else {
          LLVMTypeRef arg_type = llvm_of_type(type->function->arg);
          arg_types = arena_push(llvm_gen.perm_arena, 1 * sizeof(LLVMTypeRef));
          arg_types[0] = arg_type;
          arg_count = 1;
        }
      }
      else {
        LLVMTypeRef arg_type = llvm_of_type(type->function->arg);
        arg_types = arena_push(llvm_gen.perm_arena, 1 * sizeof(LLVMTypeRef));
        arg_types[0] = arg_type;
        arg_count = 1;
      }
      LLVMTypeRef ret_type = llvm_of_type(type->function->ret);
      result = LLVMFunctionType(ret_type, arg_types, arg_count, 0);
    } break;
    }

    llvm_gen.types[typeid] = result;
  }
  return result;
}

LLVMValueRef llvm_of_ir(Ir* ir) {
  I32 irid = ir - irgen.irs.base;
  return llvm_gen.irs[irid];
}

void llvm_of_ir_put(Ir* ir, LLVMValueRef val) {
  I32 irid = ir - irgen.irs.base;
  llvm_gen.irs[irid] = val;
}

LLVMValueRef llvm_of_var(Var* var) {
  I32 varid = var - irgen.vars.base;
  return llvm_gen.vars[varid];
}

void llvm_of_var_put(Var* var, LLVMValueRef val) {
  I32 varid = var - irgen.vars.base;
  llvm_gen.vars[varid] = val;
}

LLVMBasicBlockRef llvm_of_block(Block* block) {
  I32 blockid = block - irgen.blocks.base;
  return llvm_gen.blocks[blockid];
}

void llvm_of_block_put(Block* block, LLVMBasicBlockRef bb) {
  I32 blockid = block - irgen.blocks.base;
  llvm_gen.blocks[blockid] = bb;
}

LLVMTypeRef llvm_type_of_ir(LLVMContextRef context, Ir* ir) {
  Type* ir_type = type_of_ir(ir);
  return llvm_of_type(ir_type);
}

LLVMValueRef llvm_of_fun(Fun* fun) {
  I32 funid = fun - irgen.funs.base;
  return llvm_gen.funs[funid];
}

void llvm_of_fun_put(Fun* fun, LLVMValueRef val) {
  I32 funid = fun - irgen.funs.base;
  llvm_gen.funs[funid] = val;
}

LLVMValueRef llvm_of_foreign_fun(I32 id) {
  return llvm_gen.foreign_funs[id];
}

void llvm_of_foreign_fun_put(I32 f, LLVMValueRef llvm_fun) {
  llvm_gen.foreign_funs[f] = llvm_fun;
}


LLVMValueRef llvm_fun(Fun* fun);
void llvm_block(Block* block);

LLVMValueRef llvm_default_of_type(Type* type) {
  LLVMValueRef result = 0;
  switch (type->kind) {
  case Type_Kind_none: {
    LLVMTypeRef llvm_type = LLVMIntTypeInContext(llvm_gen.context, 1);
    result = LLVMConstInt(llvm_type, 0, false);
  } break;
  case Type_Kind_type: {
    LLVMTypeRef llvm_type = LLVMIntTypeInContext(llvm_gen.context, type->bits_size);
    result = LLVMConstInt(llvm_type, 0, false);
  } break;
  case Type_Kind_join_records: {
    LLVMTypeRef llvm_type = llvm_of_type(type);
    result = LLVMConstInt(llvm_type, 0, false);
  } break;
  case Type_Kind_ptr: {
    if (type->pointer->stack_vars.len > 0) {
      Var* var = type->pointer->stack_vars.list[0];
      result = llvm_of_ir(var->declared_ir);
    }
    else {
      Type* declared = type_pointer_declared(type->pointer);
      LLVMValueRef llvm_of_default = llvm_default_of_type(declared);
      LLVMTypeRef llvm_var_type = llvm_of_type(declared);
      LLVMValueRef llvm_global = LLVMAddGlobal(llvm_gen.module, llvm_var_type, "__stub");
      LLVMSetInitializer(llvm_global, llvm_of_default);
      LLVMSetGlobalConstant(llvm_global, false);
      result = llvm_global;
    }
  } break;
  case Type_Kind_scalar: {
    LLVMTypeRef llvm_type = llvm_of_type(type);
    I64 key = scalar_max(type->scalar);
    for (I32 i = 0; i < type->scalar->length; i++) {
      if (type->scalar->pairs[i].lo <= 0 && 0 <= type->scalar->pairs[i].hi) {
        key = 0;
        break;
      }
      else if (type->scalar->pairs[i].lo >= 0) {
        key = type->scalar->pairs[i].lo;
      }
    }
    if (type_is_float(type)) {
      F64 value;
      if (type->scalar->bits_exp == F32_EXP) {
        value = (F64)float_f32_of_key(key);
      }
      else {
        assert(type->scalar->bits_exp == F64_EXP);
        value = float_f64_of_key(key);
      }
      result = LLVMConstReal(llvm_type, value);
    }
    else {
      result = LLVMConstInt(llvm_type, key, false);
    }
  } break;
  case Type_Kind_fun: {
    LLVMValueRef save_function = llvm_gen.function;
    LLVMBasicBlockRef save_block = LLVMGetInsertBlock(llvm_gen.builder);
    if (type->function->foreign_id) {
      result = llvm_of_foreign_fun(type->function->foreign_id);
    }
    else {
      result = llvm_of_fun(type->function->fun);
    }
    llvm_gen.function = save_function;
    LLVMPositionBuilderAtEnd(llvm_gen.builder, save_block);
  } break;
  case Type_Kind_record: {
    I32 length = type->record->length;
    LLVMValueRef* values = arena_push(llvm_gen.perm_arena, (length+1) * sizeof(LLVMValueRef));
    for (I32 i = 0; i < type->record->length; i++) {
      Type* field_type = type->record->types[i];
      if (field_type->bits_size == 0) {
        LLVMTypeRef llvm_zero_type = LLVMIntTypeInContext(llvm_gen.context, 0);
        values[i] = LLVMConstInt(llvm_zero_type, 0, false);
      }
      else {
        values[i] = llvm_default_of_type(field_type);
      }
    }
    I32 bits_nopad = type_record_bits_nopad(type->record, type->bits_align);
    if (bits_nopad < type->bits_size) {
      LLVMTypeRef int_pad = LLVMIntTypeInContext(llvm_gen.context, type->bits_size - bits_nopad);
      values[length++] = LLVMConstInt(int_pad, 0, false);
    }
    result = LLVMConstStructInContext(llvm_gen.context, values, length, false);
  } break;
  case Type_Kind_array: {
    I32 length = type->array->length;
    LLVMValueRef* values = arena_push(llvm_gen.perm_arena, length * sizeof(LLVMValueRef));
    for (I32 i = 0; i < length; i++) {
      Type* field_type = type->array->types[i];
      if (field_type->bits_size == 0) {
        LLVMTypeRef llvm_zero_type = LLVMIntTypeInContext(llvm_gen.context, 0);
        values[i] = LLVMConstInt(llvm_zero_type, 0, false);
      }
      else {
        values[i] = llvm_default_of_type(field_type);
      }
    }
    LLVMTypeRef llvm_item_type = llvm_of_type(type->array->of_type);
    result = LLVMConstArray2(llvm_item_type, values, length);
  } break;
  case Type_Kind_span: {
    LLVMValueRef* values = arena_push(llvm_gen.perm_arena, 2 * sizeof(LLVMValueRef));
    values[0] = llvm_default_of_type(type->span->length);

    LLVMValueRef llvm_of_default = llvm_default_of_type(type->span->of_type);
    LLVMTypeRef llvm_var_type = llvm_of_type(type->span->of_type);
    LLVMValueRef llvm_global = LLVMAddGlobal(llvm_gen.module, llvm_var_type, "__stub");
    LLVMSetInitializer(llvm_global, llvm_of_default);
    LLVMSetGlobalConstant(llvm_global, false);
    values[1]  = llvm_global;

    result = LLVMConstStructInContext(llvm_gen.context, values, 2, false);
  } break;
  }
  return result;
}

LLVMValueRef llvm_conversion(LLVMValueRef llvm_val, Type* from, Type* to) {
  LLVMValueRef result = llvm_val;
  if (from->kind == Type_Kind_scalar) {
    LLVMTypeRef llvm_to = llvm_of_type(to);
    if (type_is_float(from)) {
      if (type_is_float(to)) {
        if (from->scalar->bits_exp == F32_EXP && to->scalar->bits_exp == F64_EXP) {
          result = LLVMBuildFPExt(llvm_gen.builder, llvm_val, llvm_to, "");
        }
        else if (from->scalar->bits_exp == F64_EXP && to->scalar->bits_exp == F32_EXP) {
          result = LLVMBuildFPTrunc(llvm_gen.builder, llvm_val, llvm_to, "");
        }
      }
      else {
        result = LLVMBuildFPToSI(llvm_gen.builder, llvm_val, llvm_to, "");
      }
    }
    else if (type_is_float(to)) {
      result = LLVMBuildSIToFP(llvm_gen.builder, llvm_val, llvm_to, "");
    }
    else {
      result = LLVMBuildIntCast2(llvm_gen.builder, llvm_val, llvm_to, true, "");
    }
  }
  return result;
}

void llvm_ir(Ir* ir) {
  LLVMValueRef result = 0;
  LLVMValueRef llvm_one = 0;
  LLVMValueRef llvm_two = 0;
  LLVMValueRef llvm_unary = 0;

  Type* type = type_of_ir(ir);

  if (ir->kind & Ir_Flag_binary) {
    llvm_one = llvm_of_ir(ir->binary.one);
    llvm_two = llvm_of_ir(ir->binary.two);
  }
  else if (ir->kind & Ir_Flag_unary) {
    llvm_unary = llvm_of_ir(ir->unary);
  }

  switch (ir->kind) {
  case Ir_Kind_arg: {
    I32 count_params = LLVMCountParams(llvm_gen.function);
    LLVMValueRef* params = arena_push(llvm_gen.perm_arena, count_params * sizeof(LLVMValueRef));
    LLVMGetParams(llvm_gen.function, params);
    if (count_params == 0) {
      result = llvm_gen.none;
    }
    else if (count_params == 1) {
      LLVMTypeRef llvm_var_type = llvm_of_type(ir->var->declared);
      result = LLVMBuildAlloca(llvm_gen.builder, llvm_var_type, ir->var->name->base);
      LLVMBuildStore(llvm_gen.builder, params[0], result);
    }
    else {
      assert(0);
    }
  } break;
  case Ir_Kind_var: {
    LLVMValueRef llvm_var_init = llvm_default_of_type(ir->var->declared);

    if (ir->var->kind == Var_Kind_constant) {
      result = llvm_var_init;
    }
    else if (ir->var->global) {
      if (ir->var->declared->kind == Type_Kind_fun) {
        result = llvm_of_fun(ir->var->declared->function->fun);
      }
      else {
        LLVMTypeRef llvm_var_type = llvm_of_type(ir->var->declared);
        result = LLVMAddGlobal(llvm_gen.module, llvm_var_type, ir->var->name->base);
        LLVMSetInitializer(result, llvm_var_init);
        LLVMSetGlobalConstant(result, false);
      }
    }
    else {
      LLVMTypeRef llvm_var_type = llvm_of_type(ir->var->declared);
      result = LLVMBuildAlloca(llvm_gen.builder, llvm_var_type, ir->var->name->base);
      // LLVMBuildStore(llvm_gen.builder, llvm_var_init, result);
    }
    llvm_of_var_put(ir->var, result);
  } break;
  case Ir_Kind_int: {
    LLVMTypeRef llvm_type = llvm_of_type(type);
    result = LLVMConstInt(llvm_type, ir->i64, 0);
  } break;
  case Ir_Kind_float: {
    LLVMTypeRef llvm_type = llvm_of_type(type);
    if (type->scalar->bits_exp == F32_EXP) {
      result = LLVMConstReal(llvm_type, (F32)ir->f64);
    }
    else {
      result = LLVMConstReal(llvm_type, ir->f64);
    }
  } break;
  case Ir_Kind_float_cast: {
    LLVMValueRef llvm_val = llvm_of_ir(ir->int_cast.value);
    Type* from = type_of_ir(ir->int_cast.value);
    LLVMTypeRef llvm_to = llvm_of_type(type);
    if (from->scalar->bits_exp == F32_EXP && type->scalar->bits_exp == F64_EXP) {
      result = LLVMBuildFPExt(llvm_gen.builder, llvm_val, llvm_to, "");
    }
    else if (from->scalar->bits_exp == F64_EXP && type->scalar->bits_exp == 8) {
      result = LLVMBuildFPTrunc(llvm_gen.builder, llvm_val, llvm_to, "");
    }
    else {
      result = llvm_val;
    }
  } break;
  case Ir_Kind_float_from_int_cast: {
    LLVMValueRef llvm_val = llvm_of_ir(ir->cast_value);
    LLVMTypeRef  llvm_to  = llvm_of_type(type);
    result = LLVMBuildSIToFP(llvm_gen.builder, llvm_val, llvm_to, "");
  } break;
  case Ir_Kind_int_from_float_cast: {
    LLVMValueRef llvm_val = llvm_of_ir(ir->cast_value);
    LLVMTypeRef  llvm_to  = llvm_of_type(type);
    result = LLVMBuildFPToSI(llvm_gen.builder, llvm_val, llvm_to, "");
  } break;
  case Ir_Kind_add: case Ir_Kind_sub: case Ir_Kind_mul: case Ir_Kind_div: case Ir_Kind_rem:
  case Ir_Kind_or: case Ir_Kind_xor: case Ir_Kind_and:
  case Ir_Kind_eq: case Ir_Kind_ne: case Ir_Kind_lt: case Ir_Kind_le: case Ir_Kind_gt: case Ir_Kind_ge:
  {
    B8 is_float = type_is_float(type_of_ir(ir->binary.one));
    switch (ir->kind) {
      case Ir_Kind_add: result = is_float
        ? LLVMBuildFAdd(llvm_gen.builder, llvm_one, llvm_two, "")
        : LLVMBuildAdd(llvm_gen.builder, llvm_one, llvm_two, ""); break;
      case Ir_Kind_sub: result = is_float
        ? LLVMBuildFSub(llvm_gen.builder, llvm_one, llvm_two, "")
        : LLVMBuildSub(llvm_gen.builder, llvm_one, llvm_two, ""); break;
      case Ir_Kind_mul: result = is_float
        ? LLVMBuildFMul(llvm_gen.builder, llvm_one, llvm_two, "")
        : LLVMBuildMul(llvm_gen.builder, llvm_one, llvm_two, ""); break;
      case Ir_Kind_div: result = is_float
        ? LLVMBuildFDiv(llvm_gen.builder, llvm_one, llvm_two, "")
        : LLVMBuildSDiv(llvm_gen.builder, llvm_one, llvm_two, ""); break;
      case Ir_Kind_rem: result = is_float
        ? LLVMBuildFRem(llvm_gen.builder, llvm_one, llvm_two, "")
        : LLVMBuildSRem(llvm_gen.builder, llvm_one, llvm_two, ""); break;

      case Ir_Kind_or:  result = LLVMBuildOr(llvm_gen.builder, llvm_one, llvm_two, ""); break;
      case Ir_Kind_xor: result = LLVMBuildXor(llvm_gen.builder, llvm_one, llvm_two, ""); break;
      case Ir_Kind_and: result = LLVMBuildAnd(llvm_gen.builder, llvm_one, llvm_two, ""); break;

      case Ir_Kind_eq: result = is_float
        ? LLVMBuildFCmp(llvm_gen.builder, LLVMRealOEQ, llvm_one, llvm_two, "")
        : LLVMBuildICmp(llvm_gen.builder, LLVMIntEQ, llvm_one, llvm_two, ""); break;
      case Ir_Kind_ne: result = is_float
        ? LLVMBuildFCmp(llvm_gen.builder, LLVMRealONE, llvm_one, llvm_two, "")
        : LLVMBuildICmp(llvm_gen.builder, LLVMIntNE, llvm_one, llvm_two, ""); break;
      case Ir_Kind_lt: result = is_float
        ? LLVMBuildFCmp(llvm_gen.builder, LLVMRealOLT, llvm_one, llvm_two, "")
        : LLVMBuildICmp(llvm_gen.builder, LLVMIntSLT, llvm_one, llvm_two, ""); break;
      case Ir_Kind_le: result = is_float
        ? LLVMBuildFCmp(llvm_gen.builder, LLVMRealOLE, llvm_one, llvm_two, "")
        : LLVMBuildICmp(llvm_gen.builder, LLVMIntSLE, llvm_one, llvm_two, ""); break;
      case Ir_Kind_gt: result = is_float
        ? LLVMBuildFCmp(llvm_gen.builder, LLVMRealOGT, llvm_one, llvm_two, "")
        : LLVMBuildICmp(llvm_gen.builder, LLVMIntSGT, llvm_one, llvm_two, ""); break;
      case Ir_Kind_ge: result = is_float
        ? LLVMBuildFCmp(llvm_gen.builder, LLVMRealOGE, llvm_one, llvm_two, "")
        : LLVMBuildICmp(llvm_gen.builder, LLVMIntSGE, llvm_one, llvm_two, ""); break;
      default: assert(0);
    }
  } break;
  case Ir_Kind_pos: {
    result = llvm_unary;
  } break;
  case Ir_Kind_neg: {
    if (type_is_float(type_of_ir(ir->unary))) {
      result = LLVMBuildFNeg(llvm_gen.builder, llvm_unary, "");
    }
    else {
      result = LLVMBuildNeg(llvm_gen.builder, llvm_unary, "");
    }
  } break;
  case Ir_Kind_call: {
    Type* fun_type = type_of_ir(ir->binary.one);
    if (fun_type == sem.fun_bits || fun_type == sem.fun_float) {
      result = llvm_gen.none;
    }
    else if (fun_type->kind == Type_Kind_fun) {
      LLVMTypeRef llvm_fun_type = llvm_of_type(fun_type);
      Ir*   ir_arg   = ir->binary.two;
      Type* arg_type = type_of_ir(ir_arg);
      LLVMValueRef* llvm_args;
      I32 llvm_arg_count = 0;
      if (arg_type->kind == Type_Kind_none) {
        llvm_args = 0;
        llvm_arg_count = 0;
      }
      else if (fun_type->function->foreign_name != irgen.str_nil && arg_type->kind == Type_Kind_record) {
        if (arg_type->kind == Type_Kind_record) {
          Record* record = arg_type->record;
          LLVMValueRef llvm_arg = llvm_of_ir(ir_arg);
          llvm_args = arena_push(llvm_gen.perm_arena, record->length * sizeof(LLVMValueRef));
          for (I32 i = 0; i < record->length; i++) {
            llvm_args[i] = LLVMBuildExtractValue(llvm_gen.builder, llvm_arg, i, "");
          }
          llvm_arg_count = record->length;
        }
      }
      else {
        LLVMValueRef llvm_arg = llvm_of_ir(ir_arg);
        llvm_arg = llvm_conversion(llvm_arg, arg_type, fun_type->function->arg);
        llvm_args = arena_push(llvm_gen.perm_arena, 1 * sizeof(LLVMValueRef));
        llvm_args[0] = llvm_arg;
        llvm_arg_count = 1;
      }
      result = LLVMBuildCall2(llvm_gen.builder, llvm_fun_type, llvm_one, llvm_args, llvm_arg_count, "");
    }
    else if (fun_type->kind == Type_Kind_type) {
      Ir* ir_arg   = ir->binary.two;
      Type* arg_type = type_of_ir(ir_arg);
      LLVMValueRef llvm_arg = llvm_of_ir(ir_arg);
      result = llvm_conversion(llvm_arg, arg_type, fun_type->type);
    }
    else if (fun_type->kind == Type_Kind_none) {
      if (has_flag(fun_type->flag, Type_Flag_size_defined)) { // NOTE: bits function
        result = llvm_default_of_type(type);
      }
      else if (fun_type->str) {
        assert(type->kind == Type_Kind_fun);
        result = llvm_of_fun(type->function->fun);
        LLVMTypeRef llvm_fun_type = llvm_of_type(type);
        result = LLVMAddFunction(llvm_gen.module, cstr_from_str(type->function->foreign_name), llvm_fun_type);
        llvm_of_fun_put(type->function->fun, result);
      }
    }
  } break;
  case Ir_Kind_ptr_offset: {
    Type* of_type = type_of_ir(ir->binary.one);
    if (of_type->kind == Type_Kind_ptr) {
      Type* ptr_to = type_pointer_declared(of_type->pointer);
      if (ptr_to->kind == Type_Kind_array) {
        LLVMValueRef llvm_ptr_to_array = llvm_of_ir(ir->binary.one);
        LLVMTypeRef  llvm_ptr_to       = llvm_of_type(ptr_to);
        LLVMTypeRef  llvm_int_type     = LLVMIntTypeInContext(llvm_gen.context, 32);
        {
          LLVMValueRef zero       = LLVMConstInt(llvm_int_type, 0, 0);
          LLVMValueRef index      = llvm_of_ir(ir->binary.two);
          LLVMValueRef indices[2] = { zero, index };
          result = LLVMBuildInBoundsGEP2(llvm_gen.builder, llvm_ptr_to, llvm_ptr_to_array, indices, 2, "");
        }
      }
      else if (ptr_to->kind == Type_Kind_span) {
        LLVMTypeRef  llvm_type_span   = llvm_of_type(ptr_to);
        LLVMValueRef llvm_ptr_to_span = llvm_of_ir(ir->binary.one);
        LLVMTypeRef  llvm_span_ptr_to = llvm_of_type(ptr_to->span->of_type);

        LLVMValueRef llvm_ptr;
        {
          LLVMValueRef llvm_span = LLVMBuildLoad2(llvm_gen.builder, llvm_type_span, llvm_ptr_to_span, "");
          llvm_ptr  = LLVMBuildExtractValue(llvm_gen.builder, llvm_span, 1, "");
        }

        {
          LLVMValueRef indices[1] = { llvm_of_ir(ir->binary.two) };
          result = LLVMBuildInBoundsGEP2(llvm_gen.builder, llvm_span_ptr_to, llvm_ptr, indices, 1, "");
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
  case Ir_Kind_name_offset: {
    Type* of_type = type_of_ir(ir->name_offset.of);
    if (of_type->kind == Type_Kind_ptr) {
      Type* object_type = type_pointer_declared(of_type->pointer);
      if (object_type->kind == Type_Kind_record) {
        LLVMValueRef ptr = llvm_of_ir(ir->name_offset.of);
        I32 position = hash_map_get_i32(&object_type->record->position_from_name, ir->name_offset.at);
        LLVMTypeRef llvm_type = llvm_of_type(object_type);
        result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, ptr, position, "");
      }
      else if (object_type->kind == Type_Kind_array) {
        assert(ir->name_offset.at == irgen.str_len);
        LLVMValueRef ptr = llvm_of_ir(ir->name_offset.of);
        LLVMTypeRef llvm_type = llvm_of_type(object_type);
        result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, ptr, 0, "");
      }
      else if (object_type->kind == Type_Kind_span) {
        LLVMValueRef ptr = llvm_of_ir(ir->name_offset.of);
        LLVMTypeRef llvm_type = llvm_of_type(object_type);
        result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, ptr, 0, "");
      }
      else if (object_type->kind == Type_Kind_join_records) {
        LLVMValueRef ptr = llvm_of_ir(ir->name_offset.of);

        Type* element = sem_element_of_join(ir);
        I32 position = hash_map_get_i32(&element->record->position_from_name, ir->name_offset.at);

        LLVMTypeRef llvm_ptr_record = LLVMPointerType(llvm_of_type(element), 0);
        LLVMValueRef casted = LLVMBuildBitCast(llvm_gen.builder, ptr, llvm_ptr_record, "");

        LLVMTypeRef llvm_type = llvm_of_type(element);
        result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, casted, position, "");
      }
    }
    else if (of_type->kind == Type_Kind_record) {
      LLVMValueRef of = llvm_of_ir(ir->position.of);
      I32 position = hash_map_get_i32(&of_type->record->position_from_name, ir->name_offset.at);
      result = LLVMBuildExtractValue(llvm_gen.builder, of, position, "");
    }
  } break;
  case Ir_Kind_position_offset: {
    Type* of_type = type_of_ir(ir->position.of);

    if (of_type->kind == Type_Kind_ptr) {
      I32 at = ir->position.at;
      Type* rec_type = type_pointer_declared(of_type->pointer);
      LLVMValueRef ptr = llvm_of_ir(ir->position.of);
      LLVMTypeRef llvm_type = llvm_of_type(rec_type);
      result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, ptr, at, "");
    }
    else {
      LLVMValueRef of = llvm_of_ir(ir->position.of);
      result = LLVMBuildExtractValue(llvm_gen.builder, of, ir->position.at, "");
    }
  } break;
  case Ir_Kind_load: {
    if (type_is_const(type)) {
      result = llvm_default_of_type(type);
    }
    else {
      LLVMTypeRef llvm_type = llvm_of_type(type);
      result = LLVMBuildLoad2(llvm_gen.builder, llvm_type, llvm_unary, "");
    }
  } break;
  case Ir_Kind_store: {
    result = LLVMBuildStore(llvm_gen.builder, llvm_two, llvm_one);
  } break;
  case Ir_Kind_fun: {
    LLVMValueRef save_function = llvm_gen.function;
    LLVMBasicBlockRef save_block = LLVMGetInsertBlock(llvm_gen.builder);
    result = llvm_of_fun(ir->fun);
    llvm_gen.function = save_function;
    LLVMPositionBuilderAtEnd(llvm_gen.builder, save_block);
  } break;
  case Ir_Kind_int_cast: {
    LLVMValueRef llvm_val = llvm_of_ir(ir->int_cast.value);
    LLVMTypeRef llvm_to = llvm_of_type(type);
    result = LLVMBuildIntCast2(llvm_gen.builder, llvm_val, llvm_to, true, "");
  } break;
  case Ir_Kind_record_cast: {
    Type* type_to = type;
    assert(type_to->kind == Type_Kind_record);

    LLVMTypeRef llvm_type = llvm_of_type(type_to);
    LLVMValueRef llvm_val = llvm_of_ir(ir->cast_value);
    Type* type_from = type_of_ir(ir->cast_value);
    assert(type_from->kind == Type_Kind_record);

    result = LLVMGetUndef(llvm_type);
    for (I32 i = 0; i < type_to->record->length; i++) {
      Str* field_name = type_to->record->names[i];
      I32  position   = i;
      if (field_name) {
        position = hash_map_get_i32(&type_from->record->position_from_name, field_name);
      }
      LLVMValueRef field_val       = LLVMBuildExtractValue(llvm_gen.builder, llvm_val, position, "");
      Type*        field_from_type = type_from->record->types[position];
      Type*        field_type      = type_to->record->types[i];
      if (field_from_type != field_type) {
        field_val = llvm_conversion(field_val, field_from_type, field_type);
      }
      result = LLVMBuildInsertValue(llvm_gen.builder, result, field_val, i, "");
    }
  } break;
  case Ir_Kind_array_cast: {
    Type* type_to = type;
    assert(type_to->kind == Type_Kind_array);

    LLVMTypeRef llvm_type = llvm_of_type(type_to);
    LLVMValueRef llvm_val = llvm_of_ir(ir->cast_value);
    Type* type_from = type_of_ir(ir->cast_value);
    assert(type_from->kind == Type_Kind_array);

    I32 length = type_to->array->length;
    result = LLVMGetUndef(llvm_type);
    for (I32 i = 0; i < length; i++) {
      Type* type_from_item = type_from->array->types[i];
      Type* type_to_item = type_to->array->types[i];
      LLVMValueRef llvm_item = LLVMBuildExtractValue(llvm_gen.builder, llvm_val, i, "");

      if (type_is_same(type_from_item, type_to_item)) {
        llvm_item = llvm_conversion(llvm_item, type_from_item, type_to_item);
      }
      result = LLVMBuildInsertValue(llvm_gen.builder, result, llvm_item, i, "");
    }
  } break;
  case Ir_Kind_span_from_array_cast: {
    // a: []I8 = "AB"
    Type* type_to = type;
    assert(type_to->kind == Type_Kind_span);
    Type* type_from = type_of_ir(ir->cast_value);
    assert(type_from->kind == Type_Kind_ptr);
    Type* declared = type_pointer_declared(type_from->pointer);
    assert(declared->kind == Type_Kind_array);
    Array* array = declared->array;

    LLVMTypeRef llvm_type_length  = llvm_of_type(type_to->span->length);
    LLVMTypeRef llvm_type_of_type = llvm_of_type(type_to->span->of_type);
    LLVMTypeRef llvm_type_ptr_of_type = LLVMPointerType(llvm_type_of_type, 0);

    LLVMValueRef* values = arena_push(llvm_gen.perm_arena, 2 * sizeof(LLVMValueRef));
    values[0] = LLVMConstInt(llvm_type_length, array->length, false);

    LLVMValueRef ptr_to_array = llvm_of_ir(ir->cast_value);
    values[1] = LLVMBuildPointerCast(llvm_gen.builder, ptr_to_array, llvm_type_ptr_of_type, "");

    result = LLVMConstStructInContext(llvm_gen.context, values, 2, false);
  } break;
  case Ir_Kind_record: {
    LLVMTypeRef llvm_type = llvm_of_type(type);
    result = LLVMGetUndef(llvm_type);
    for (I32 i = 0; i < ir->rec->length; i++) {
      LLVMValueRef value = llvm_of_ir(ir->rec->irs[i]);
      result = LLVMBuildInsertValue(llvm_gen.builder, result, value, i, "");
    }
  } break;
  case Ir_Kind_ptr: {
    Type* type_unary = type_of_ir(ir->unary);

    if (type_unary->kind == Type_Kind_type) {
      result = llvm_of_ir(ir->unary);
    }
    else if (ir->unary->kind == Ir_Kind_load) {
      result = llvm_of_ir(ir->unary->unary);
    }
    else {
      LLVMTypeRef llvm_var_type = llvm_of_type(type_unary);
      LLVMValueRef var = LLVMBuildAlloca(llvm_gen.builder, llvm_var_type, "__pointee");
      LLVMValueRef val = llvm_of_ir(ir->unary);
      LLVMBuildStore(llvm_gen.builder, val, var);
      result = var;
    }
  } break;

  case Ir_Kind_declare: result = llvm_gen.none; break;
  case Ir_Kind_none:    result = llvm_gen.none; break;
  case Ir_Kind_type:    result = llvm_gen.none; break;
  case Ir_Kind_str: case Ir_Kind_span: case Ir_Kind_array:
  case Ir_Kind_join: case Ir_Kind_meet: case Ir_Kind_range:
  {
    result = llvm_default_of_type(type);
  } break;
  case Ir_Kind_bits:    result = llvm_gen.none; break;
  case Ir_Kind_ieee:    result = llvm_gen.none; break;
  case Ir_Kind_len:     result = llvm_gen.none; break;
  case Ir_Kind_run:     result = llvm_gen.none; break;
  case Ir_Kind_foreign: result = llvm_gen.none; break;
  case Ir_Kind_macro_par: assert(0); break;
  }

  assert(result);
  llvm_of_ir_put(ir, result);
}

void llvm_block(Block* block) {
  LLVMBasicBlockRef llvm_block = llvm_of_block(block);
  LLVMPositionBuilderAtEnd(llvm_gen.builder, llvm_block);

  for (I32 i = 0; i < block->sem_irs->length; i++) {
    Ir* ir = fa_get(block->sem_irs, i);
    llvm_ir(ir);
  }
  switch (block->kind) {
  case Block_Kind_none: {
  } break;
  case Block_Kind_jump: {
    LLVMBasicBlockRef llvm_to_block = llvm_of_block(block->jump.to_block);
    LLVMBuildBr(llvm_gen.builder, llvm_to_block);
  } break;
  case Block_Kind_branch: {
    LLVMValueRef llvm_cond       = llvm_of_ir(block->branch.cond);
    LLVMTypeRef  llvm_cond_type  = LLVMTypeOf(llvm_cond);
    LLVMValueRef llvm_cond_bool;
    if (type_is_float(type_of_ir(block->branch.cond))) {
      LLVMValueRef llvm_zero = LLVMConstReal(llvm_cond_type, 0.0);
      llvm_cond_bool = LLVMBuildFCmp(llvm_gen.builder, LLVMRealUNE, llvm_cond, llvm_zero, "");
    }
    else {
      LLVMValueRef llvm_zero = LLVMConstInt(llvm_cond_type, 0, false);
      llvm_cond_bool = LLVMBuildICmp(llvm_gen.builder, LLVMIntNE, llvm_cond, llvm_zero, "");
    }
    LLVMBasicBlockRef then_block = llvm_of_block(block->branch.nez.to_block);
    LLVMBasicBlockRef else_block = llvm_of_block(block->branch.eqz.to_block);
    LLVMBuildCondBr(llvm_gen.builder, llvm_cond_bool, then_block, else_block);
  } break;
  }
}

LLVMValueRef llvm_fun(Fun* fun) {
  llvm_gen.function = llvm_of_fun(fun);
  if (fun->kind != Fun_Kind_none) {
    return llvm_gen.function;
  }
  for (I32 b = 0; b < fun->blocks->length; b++) {
    Block* block = fun->blocks->base[b];
    LLVMBasicBlockRef llvm_block = LLVMAppendBasicBlockInContext(llvm_gen.context, llvm_gen.function, "block");
    llvm_of_block_put(block, llvm_block);
  }

  for (I32 b = 0; b < fun->blocks->length; b++) {
    Block* block = fun->blocks->base[b];
    llvm_block(block);
  }

  Type* ret_type = fun->type->function->ret;
  LLVMValueRef ret_val;
  if (type_is_const(ret_type)) {
    ret_val = llvm_default_of_type(ret_type);
  }
  else {
    LLVMValueRef llvm_ret_var = llvm_of_ir(fun->ret_ir);
    LLVMTypeRef llvm_ret_type = llvm_of_type(ret_type);
    ret_val = LLVMBuildLoad2(llvm_gen.builder, llvm_ret_type, llvm_ret_var, "");
  }

  LLVMBuildRet(llvm_gen.builder, ret_val);
  return llvm_gen.function;
}

I32 llvm_funs(Arena* arena, Funs funs) {
  llvm_gen.perm_arena = arena;
  llvm_gen.blocks = arena_push(arena, irgen.blocks.length * sizeof(LLVMBasicBlockRef));
  llvm_gen.irs    = arena_push(arena, irgen.irs.length    * sizeof(LLVMValueRef));
  llvm_gen.vars   = arena_push(arena, irgen.vars.length    * sizeof(LLVMValueRef));
  llvm_gen.types  = arena_push_zero(arena, sem.types.length  * sizeof(LLVMTypeRef));
  llvm_gen.funs   = arena_push_zero(arena, irgen.funs.length * sizeof(LLVMValueRef));
  llvm_gen.foreign_funs = arena_push_zero(arena, sem.foreign_funs->length * sizeof(LLVMValueRef));

  llvm_gen.context = LLVMContextCreate();
  llvm_gen.module  = LLVMModuleCreateWithNameInContext("contex_name", llvm_gen.context);
  llvm_gen.builder = LLVMCreateBuilderInContext(llvm_gen.context);

  llvm_gen.none = llvm_default_of_type(sem.type_none);

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    if (fun->kind == Fun_Kind_none) {
      LLVMTypeRef  llvm_fun_type = llvm_of_type(fun->type);
      LLVMValueRef llvm_fun      = LLVMAddFunction(llvm_gen.module, cstr_from_str(fun->name), llvm_fun_type);
      llvm_of_fun_put(fun, llvm_fun);
    }
  }

  for (I32 f = 1; f < sem.foreign_funs->length; f++) {
    Function* function = sem.foreign_funs->base[f];
    LLVMTypeRef* arg_types;
    I32 arg_count = 0;
    if (function->arg->kind == Type_Kind_none) {
      arg_types = 0;
      arg_count = 0;
    }
    else if (function->foreign_name != irgen.str_nil) {
      if (function->arg->kind == Type_Kind_record) {
        Record* record = function->arg->record;
        arg_types = arena_push(llvm_gen.perm_arena, record->length * sizeof(LLVMTypeRef));
        for (I32 i = 0; i < record->length; i++) {
          Type* arg_type = record->types[i];
          arg_types[i] = llvm_of_type(arg_type);
        }
        arg_count = record->length;
      }
      else {
        LLVMTypeRef arg_type = llvm_of_type(function->arg);
        arg_types = arena_push(llvm_gen.perm_arena, 1 * sizeof(LLVMTypeRef));
        arg_types[0] = arg_type;
        arg_count = 1;
      }
    }
    LLVMTypeRef ret_type = llvm_of_type(function->ret);
    LLVMTypeRef llvm_fun_type = LLVMFunctionType(ret_type, arg_types, arg_count, 0);
    LLVMValueRef llvm_fun = LLVMAddFunction(llvm_gen.module, cstr_from_str(function->foreign_name), llvm_fun_type);
    llvm_of_foreign_fun_put(f, llvm_fun);
  }

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    if (fun->kind == Fun_Kind_none) {
      llvm_fun(fun);
    }
  }

  printf("Generated LLVM IR:\n");
  LLVMDumpModule(llvm_gen.module);

  char *verify_error = NULL;
  if (LLVMVerifyModule(llvm_gen.module, LLVMAbortProcessAction, &verify_error)) {
    fprintf(stderr, "Error verifying module: %s\n", verify_error);
    LLVMDisposeMessage(verify_error);
    return 1;
  }
  if (verify_error) {
    LLVMDisposeMessage(verify_error);
  }

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMInitializeNativeDisassembler();

  char *triple = LLVMGetDefaultTargetTriple();
  LLVMSetTarget(llvm_gen.module, triple);

  LLVMOrcLLJITRef jit;
  LLVMErrorRef jit_error = LLVMOrcCreateLLJIT(&jit, NULL);
  if (jit_error) {
    char *errMsg = LLVMGetErrorMessage(jit_error);
    fprintf(stderr, "JIT creation error: %s\n", errMsg);
    LLVMDisposeMessage(triple);
    LLVMOrcDisposeLLJIT(jit);
    LLVMDisposeBuilder(llvm_gen.builder);
    LLVMContextDispose(llvm_gen.context);
    LLVMDisposeModule(llvm_gen.module);
    LLVMDisposeErrorMessage(errMsg);
    return 1;
  }

  LLVMOrcThreadSafeContextRef ts_context = LLVMOrcCreateNewThreadSafeContextFromLLVMContext(llvm_gen.context);
  LLVMOrcThreadSafeModuleRef ts_module = LLVMOrcCreateNewThreadSafeModule(llvm_gen.module, ts_context);
  if (LLVMOrcLLJITAddLLVMIRModule(jit, LLVMOrcLLJITGetMainJITDylib(jit), ts_module)) {
    fprintf(stderr, "Failed to add module\n");
    LLVMOrcDisposeLLJIT(jit);
    LLVMDisposeBuilder(llvm_gen.builder);
    LLVMContextDispose(llvm_gen.context);
    return 1;
  }

  LLVMOrcDefinitionGeneratorRef sdl_generator = NULL;
  Cstr sdl_lib_path = "libSDL3.so";
  LLVMErrorRef gen_error = LLVMOrcCreateDynamicLibrarySearchGeneratorForPath(&sdl_generator, sdl_lib_path, '\0', NULL, NULL);

  if (gen_error) {
    char *errMsg = LLVMGetErrorMessage(gen_error);
    fprintf(stderr, "Failed to create SDL3 search generator: %s\n", errMsg);
    LLVMDisposeErrorMessage(errMsg);
    return 1;
  }

  LLVMOrcJITDylibAddGenerator(LLVMOrcLLJITGetMainJITDylib(jit), sdl_generator);

  LLVMOrcJITTargetAddress addr;
  if (LLVMOrcLLJITLookup(jit, &addr, "main")) {
    fprintf(stderr, "Function 'main' not found\n");
    LLVMDisposeMessage(triple);
    LLVMOrcDisposeLLJIT(jit);
    LLVMDisposeBuilder(llvm_gen.builder);
    LLVMOrcDisposeThreadSafeContext(ts_context);
    return 1;
  }

  void (*jit_main)() = (void (*)())addr;
  jit_main();

  // LLVMOrcDisposeDefinitionGenerator(sdl_generator);
  LLVMDisposeMessage(triple);
  LLVMOrcDisposeLLJIT(jit);
  LLVMDisposeBuilder(llvm_gen.builder);
  LLVMOrcDisposeThreadSafeContext(ts_context);
  return 0;
}

void _test_llvm(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  Cstr basic        = file_read("basic.i");
  I32  basic_length = (I32)file_size("basic.i");
  I32  source_length  = strlen(source);
  I32  length         = source_length + basic_length + 32;

  Arena arena          = arena_init(KB(4) * length);
                         str_init(&arena, 2*length);

  Source_Map sources = {0};
  Diags diags = diags_init(&arena, 256);
  File_Id basic_file = source_add(&sources, &arena, "basic.i", basic, basic_length);

  Tokens basic_tokens   = lex_source(&arena, &sources, basic_file, &diags);
  Ast_Block ast         = parse_tokens(&arena, basic_tokens, &diags);

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
                         sem_funs(&arena, funs, &diags);
  if (diags_has_errors(&diags)) {
    diag_print_all(&diags, &sources, stdout, false);
    assert(0);
  }
  {
    Cstr result          = cstr_from_sem(&arena, funs);
    printf("\n%s", result);
  }
                         llvm_funs(&arena, funs);

  arena_free(&arena);
}

#define test(source, expected) _test_llvm(source, expected, __FILE__, __LINE__)

void llvm_test(void) {
  // test("foo : () -> {}; foo()", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; putchar(\"AB\"[1]); putchar 10", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; a:[2]I8 = \"AB\"; putchar(a[1]); putchar 10", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; a:[]I8 = \"AB\"; putchar(a[1]); putchar 10", "");
  // test("a:[2]I8 = \"AB\"; a.len; a[1]", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; @(32'#bits 66; 32'#bits 65); putchar 10", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; p:@(I32; I32) = @(32'#bits 66; 32'#bits 65); putchar(p@.0); putchar 10", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; p:[]I8 = \"BA\"; putchar(p[0]); putchar 10", "");
  // test("a:12; b:I32; c: @12 = @12; b = c@", "");
  // test("a: Str = \"Hi\"; a.len + 4", "");
  // test("a:(x:I32; y:I16); a = (1; 2); a.x + a.x; a.y+a.y; a", "");
  // test("a:I32 = 70; b:@I32 = @a;", "");
  // test("putchar: #c putchar (char:I32) -> I32; a:(x:66; y:I32); putchar(a.x); putchar 10", "");
  // test("a:(x:0..2; y:1..2); a = (y:1; x:2); a.x", "");
  // test("putchar: #foreign.c \"putchar\" (char:I32) -> I32", "");
  // test("putchar: #foreign.c \"putchar\" type (char:I32) -> I32; putchar 65; putchar 10", "");
  // test("putchar: #c putchar (char:I32) -> I32; foo: () -> { putchar 65; putchar 10}; foo()", "");
  // test("putchar: #c putchar (char:I32) -> I32; foo : (a:I32; b:I32) -> a+b; putchar(foo(30;35)); putchar 65; putchar 10", "");
  // test("f:()->1", "");
  // test("a:1; a+a", "");
}

#undef test
