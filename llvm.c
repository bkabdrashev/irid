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
  LLVMBasicBlockRef* blocks;
  LLVMTypeRef*       types;
  LLVMValueRef*      funs;

  LLVMModuleRef      module;
  LLVMContextRef     context;
  LLVMBuilderRef     builder;
  LLVMValueRef       function;
};

LLVM_Gen llvm_gen;

LLVMTypeRef llvm_of_type(Type* type) {
  I32 typeid = type - sem.types.base;
  LLVMTypeRef result = llvm_gen.types[typeid];
  if (result) return result;

  switch (type->kind) {
  case Type_Kind_none: {
    result = LLVMVoidTypeInContext(llvm_gen.context);
  } break;
  case Type_Kind_int: {
    result = LLVMIntTypeInContext(llvm_gen.context, 32);
  } break;
  case Type_Kind_ptr: {
    LLVMTypeRef pointer_to = llvm_of_type(type->pointer->declared);
    result = LLVMPointerType(pointer_to, 0);
  } break;
  case Type_Kind_record: {
    LLVMTypeRef* field_types = arena_push(llvm_gen.perm_arena, type->record->length * sizeof(LLVMTypeRef));
    for (I32 i = 0; i < type->record->length; i++) {
      Type* field_type = type_of_ir(type->record->declared[i]);
      if (type_is_const(field_type)) {
        field_types[i] = LLVMIntTypeInContext(llvm_gen.context, 0);
      }
      else {
        field_types[i] = llvm_of_type(field_type);
      }
    }
    result = LLVMStructTypeInContext(llvm_gen.context, field_types, type->record->length, false);
  } break;
  case Type_Kind_fun: {
    LLVMTypeRef* arg_types;
    I32 arg_count = 0;
    if (type->function->arg->kind == Type_Kind_none) {
      arg_types = 0;
      arg_count = 0;
    }
    else if (type->function->fun->foreign) {
      if (type->function->arg->kind == Type_Kind_record) {
        Record* record = type->function->arg->record;
        arg_types = arena_push(llvm_gen.perm_arena, record->length * sizeof(LLVMTypeRef));
        for (I32 i = 0; i < record->length; i++) {
          Type* arg_type = type_of_ir(record->declared[i]);
          arg_types[i] = llvm_of_type(arg_type);
        }
        arg_count = record->length;
      }
      else {
        LLVMTypeRef arg_type = llvm_of_type(type->function->arg);
        arg_types = (LLVMTypeRef[1]){ arg_type };
        arg_count = 1;
      }
    }
    else {
      LLVMTypeRef arg_type = llvm_of_type(type->function->arg);
      arg_types = (LLVMTypeRef[1]){ arg_type };
      arg_count = 1;
    }
    LLVMTypeRef ret_type = llvm_of_type(type->function->ret);
    result = LLVMFunctionType(ret_type, arg_types, arg_count, 0);
  } break;
  }
  llvm_gen.types[typeid] = result;
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
LLVMValueRef llvm_fun(Fun* fun);
void llvm_block(Block* block);

LLVMValueRef llvm_default_of_type(Type* type) {
  LLVMValueRef result = 0;
  switch (type->kind) {
  case Type_Kind_none: {
  } break;
  case Type_Kind_ptr: {
    if (type->pointer->stack.len == 1) {
      Var* var = type->pointer->stack.list[0];
      result = llvm_of_ir(var->declared_ir);
    }
    else {
      LLVMValueRef llvm_of_default = llvm_default_of_type(type->pointer->declared);
      LLVMTypeRef llvm_var_type = llvm_of_type(type->pointer->declared);
      LLVMValueRef llvm_global = LLVMAddGlobal(llvm_gen.module, llvm_var_type, "__internal_default_stub");
      LLVMSetInitializer(llvm_global, llvm_of_default);
      LLVMSetGlobalConstant(llvm_global, false);
      result = llvm_global;
    }
  } break;
  case Type_Kind_int: {
    LLVMTypeRef llvm_type = llvm_of_type(type);
    I64 i64 = ranges_max(type->ranges);
    for (I32 i = 0; i < type->ranges->length; i++) {
      if (type->ranges->pairs[i].lo <= 0 && 0 <= type->ranges->pairs[i].hi) {
        i64 = 0;
        break;
      }
      else if (type->ranges->pairs[i].lo >= 0) {
        i64 = type->ranges->pairs[i].lo;
      }
    }
    result = LLVMConstInt(llvm_type, i64, false);
  } break;
  case Type_Kind_fun: {
    LLVMValueRef save_function = llvm_gen.function;
    LLVMBasicBlockRef save_block = LLVMGetInsertBlock(llvm_gen.builder);
    result = llvm_fun(type->function->fun);
    llvm_gen.function = save_function;
    LLVMPositionBuilderAtEnd(llvm_gen.builder, save_block);
  } break;
  case Type_Kind_record: {
    LLVMValueRef* values = arena_push(llvm_gen.perm_arena, type->record->length * sizeof(LLVMValueRef));
    for (I32 i = 0; i < type->record->length; i++) {
      Type* field_type = type_of_ir(type->record->declared[i]);
      if (type_is_const(field_type)) {
        LLVMTypeRef llvm_zero_type = LLVMIntTypeInContext(llvm_gen.context, 0);
        values[i] = LLVMConstInt(llvm_zero_type, 0, false);
      }
      else {
        values[i] = llvm_default_of_type(field_type);
      }
    }
    result = LLVMConstStructInContext(llvm_gen.context, values, type->record->length, false);
  } break;
  }
  return result;
}

void llvm_ir(Ir* ir) {
  LLVMValueRef result = 0;
  LLVMValueRef llvm_one = 0;
  LLVMValueRef llvm_two = 0;
  LLVMValueRef llvm_unary = 0;

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
      result = 0;
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
    if (ir->var->declared->kind != Type_Kind_none) {
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
      // NOTE: declared_ir is used to store llvm value reference, so that pointer can refer to it
      if (ir->var->declared_ir) {
        llvm_of_ir_put(ir->var->declared_ir, result);
      }
    }
  } break;
  case Ir_Kind_int: {
    Type* type = type_of_ir(ir);
    LLVMTypeRef llvm_type = llvm_of_type(type);
    result = LLVMConstInt(llvm_type, ir->i64, 0);
  } break;
  case Ir_Kind_str: {
    result = LLVMConstStringInContext2(llvm_gen.context, ir->str->base, ir->str->length, false);
  } break;

  case Ir_Kind_add: result = LLVMBuildAdd(llvm_gen.builder, llvm_one, llvm_two, ""); break;
  case Ir_Kind_sub: result = LLVMBuildSub(llvm_gen.builder, llvm_one, llvm_two, ""); break;
  case Ir_Kind_mul: result = LLVMBuildMul(llvm_gen.builder, llvm_one, llvm_two, ""); break;
  case Ir_Kind_eq:  result = LLVMBuildICmp(llvm_gen.builder, LLVMIntEQ, llvm_one, llvm_two, ""); break;
  case Ir_Kind_ne:  result = LLVMBuildICmp(llvm_gen.builder, LLVMIntNE, llvm_one, llvm_two, ""); break;
  case Ir_Kind_lt:  result = LLVMBuildICmp(llvm_gen.builder, LLVMIntSLT, llvm_one, llvm_two, ""); break;
  case Ir_Kind_le:  result = LLVMBuildICmp(llvm_gen.builder, LLVMIntSLE, llvm_one, llvm_two, ""); break;
  case Ir_Kind_gt:  result = LLVMBuildICmp(llvm_gen.builder, LLVMIntSGT, llvm_one, llvm_two, ""); break;
  case Ir_Kind_ge:  result = LLVMBuildICmp(llvm_gen.builder, LLVMIntSGE, llvm_one, llvm_two, ""); break;
  case Ir_Kind_neg: result = LLVMBuildNeg(llvm_gen.builder, llvm_unary, ""); break;
  case Ir_Kind_call: {
    Type* fun_type = type_of_ir(ir->binary.one);
    LLVMTypeRef llvm_fun_type = llvm_of_type(fun_type);
    Ir*   arg_ir   = ir->binary.two;
    Type* arg_type = type_of_ir(arg_ir);
    LLVMValueRef* llvm_args;
    I32 llvm_arg_count = 0;
    if (fun_type->function->fun->foreign) {
      if (arg_type->kind == Type_Kind_none) {
        llvm_args = 0;
        llvm_arg_count = 0;
      }
      else if (arg_type->kind == Type_Kind_record) {
        Record* record = arg_type->record;
        llvm_args = arena_push(llvm_gen.perm_arena, record->length * sizeof(LLVMValueRef));
        for (I32 i = 0; i < record->length; i++) {
          llvm_args[i] = llvm_of_ir(record->declared[i]);
        }
        llvm_arg_count = record->length;
      }
      else {
        LLVMValueRef llvm_arg = llvm_of_ir(arg_ir);
        llvm_args = (LLVMValueRef[1]){ llvm_arg };
        llvm_arg_count = 1;
      }
    }
    else {
      LLVMValueRef llvm_arg = llvm_of_ir(arg_ir);
      llvm_args = (LLVMValueRef[1]){ llvm_arg };
      llvm_arg_count = 1;
    }
    result = LLVMBuildCall2(llvm_gen.builder, llvm_fun_type, llvm_one, llvm_args, llvm_arg_count, "");
  } break;

  case Ir_Kind_name_offset: {
    Type* of_type = type_of_ir(ir->name.of);
    Type* rec_type = of_type->pointer->declared;
    LLVMValueRef ptr = llvm_of_ir(ir->name.of);
    I32 position = hash_map_get_i32(&rec_type->record->position_from_name, ir->name.at);
    LLVMTypeRef llvm_type = llvm_of_type(rec_type);
    result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, ptr, position, "");
  } break;
  case Ir_Kind_position_offset: {
    Type* of_type = type_of_ir(ir->position.of);
    Type* rec_type = of_type->pointer->declared;
    LLVMValueRef ptr = llvm_of_ir(ir->position.of);
    LLVMTypeRef llvm_type = llvm_of_type(rec_type);
    result = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, ptr, ir->position.at, "");
  } break;
  case Ir_Kind_load: {
    Type* type = type_of_ir(ir);
    if (type->kind == Type_Kind_fun) {
      result = llvm_of_fun(type->function->fun);
    }
    else if (type_is_const(type)) {
      result = llvm_default_of_type(type);
    }
    else {
      LLVMTypeRef llvm_type = llvm_of_type(type);
      result = LLVMBuildLoad2(llvm_gen.builder, llvm_type, llvm_unary, "");
    }
  } break;
  case Ir_Kind_store: {
    if (ir->binary.two->kind == Ir_Kind_record) {
      Type* type_ptr = type_of_ir(ir->binary.one);
      Type* record_type = type_ptr->pointer->declared;
      Record* record_one = ir->binary.two->record;
      Record* record_two = record_type->record;
      for (I32 pos = 0; pos < record_one->length; pos++) {
        Str* name = record_one->names[pos];
        if (name) {
          I32 position = hash_map_get_i32(&record_two->position_from_name, name);
          LLVMValueRef llvm_assigned = llvm_of_ir(record_one->declared[pos]);
          LLVMTypeRef llvm_type = llvm_of_type(record_type);
          LLVMValueRef llvm_gep = LLVMBuildStructGEP2(llvm_gen.builder, llvm_type, llvm_one, position, "");
          LLVMBuildStore(llvm_gen.builder, llvm_assigned, llvm_gep);
        }
        else {
          assert(0);
        }
      }
    }
    else {
      result = LLVMBuildStore(llvm_gen.builder, llvm_two, llvm_one);
    }
  } break;
  case Ir_Kind_fun: {
    LLVMValueRef save_function = llvm_gen.function;
    LLVMBasicBlockRef save_block = LLVMGetInsertBlock(llvm_gen.builder);
    result = llvm_fun(ir->fun);
    llvm_gen.function = save_function;
    LLVMPositionBuilderAtEnd(llvm_gen.builder, save_block);
  } break;
  case Ir_Kind_record: {
    LLVMValueRef* values = arena_push(llvm_gen.perm_arena, ir->record->length * sizeof(LLVMValueRef));
    for (I32 i = 0; i < ir->record->length; i++) {
      values[i] =  llvm_of_ir(ir->record->declared[i]);
    }
    result = LLVMConstStructInContext(llvm_gen.context, values, ir->record->length, false);
  } break;
  case Ir_Kind_ptr: assert(0);

  case Ir_Kind_declare: break;
  case Ir_Kind_none:    break;
  case Ir_Kind_range:   break;
  case Ir_Kind_join:    break;
  case Ir_Kind_bits:    break;

  }

  llvm_of_ir_put(ir, result);
}

void llvm_block(Block* block) {
  LLVMBasicBlockRef llvm_block = llvm_of_block(block);
  LLVMPositionBuilderAtEnd(llvm_gen.builder, llvm_block);

  for (I32 i = 0; i < block->irs->length; i++) {
    Ir* ir = fa_get(block->irs, i);
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
    Type*        cond_type       = type_of_ir(block->branch.cond);
    LLVMTypeRef  llvm_cond_type  = llvm_of_type(cond_type);
    LLVMValueRef llvm_cond       = llvm_of_ir(block->branch.cond);
    LLVMValueRef llvm_zero       = LLVMConstInt(llvm_cond_type, 0, false);
    LLVMValueRef llvm_cond_bool  = LLVMBuildICmp(llvm_gen.builder, LLVMIntNE, llvm_cond, llvm_zero, "");
    LLVMBasicBlockRef then_block = llvm_of_block(block->branch.nez.to_block);
    LLVMBasicBlockRef else_block = llvm_of_block(block->branch.eqz.to_block);
    LLVMBuildCondBr(llvm_gen.builder, llvm_cond_bool, then_block, else_block);
  } break;
  }
}

LLVMValueRef llvm_fun(Fun* fun) {
  {
    LLVMValueRef llvm_function = llvm_of_fun(fun);
    if (llvm_function) return llvm_function;
  }
  LLVMTypeRef llvm_fun_type = llvm_of_type(fun->type);
  llvm_gen.function = LLVMAddFunction(llvm_gen.module, cstr_from_str(fun->name), llvm_fun_type);
  llvm_of_fun_put(fun, llvm_gen.function);
  if (fun->foreign) {
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
  if (ret_type->kind == Type_Kind_none) {
    LLVMBuildRetVoid(llvm_gen.builder);
  }
  else {
    LLVMValueRef llvm_ret_var = llvm_of_ir(fun->ret_ir);
    LLVMTypeRef llvm_ret_type = llvm_of_type(ret_type);
    LLVMValueRef ret_loaded = LLVMBuildLoad2(llvm_gen.builder, llvm_ret_type, llvm_ret_var, "");
    LLVMBuildRet(llvm_gen.builder, ret_loaded);
  }
  return llvm_gen.function;
}

I32 llvm_funs(Arena* arena, Funs funs) {
  llvm_gen.perm_arena = arena;
  llvm_gen.blocks = arena_push(arena, irgen.blocks.length * sizeof(LLVMBasicBlockRef));
  llvm_gen.irs    = arena_push(arena, irgen.irs.length    * sizeof(LLVMValueRef));
  llvm_gen.types  = arena_push_zero(arena, sem.types.length    * sizeof(LLVMTypeRef));
  llvm_gen.funs   = arena_push_zero(arena, irgen.funs.length   * sizeof(LLVMValueRef));

  llvm_gen.context = LLVMContextCreate();
  llvm_gen.module  = LLVMModuleCreateWithNameInContext("contex_name", llvm_gen.context);
  llvm_gen.builder = LLVMCreateBuilderInContext(llvm_gen.context);

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    llvm_fun(fun);
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

  LLVMDisposeMessage(triple);
  LLVMOrcDisposeLLJIT(jit);
  LLVMDisposeBuilder(llvm_gen.builder);
  LLVMOrcDisposeThreadSafeContext(ts_context);
  return 0;
}

void _test_llvm(Cstr source, Cstr expected, Cstr file_name, I32 line) {
  I32 source_length    = strlen(source) + 32;
  Arena arena          = arena_init(KB(4) * source_length);
  str_init(&arena, 2*source_length);
  Tokens tokens        = lex_source(&arena, source);
  Ast_Block ast        = parse_tokens(&arena, tokens);
  Funs funs            = irgen_ast(&arena, ast, source_length);
                         sem_funs(&arena, funs);
                         llvm_funs(&arena, funs);
  arena_free(&arena);
}

#define test(source, expected) _test_llvm(source, expected, __FILE__, __LINE__)

void llvm_test(void) {
  // test("a:I32 = 70; b:@I32 = @a;", "");
  // test("putchar: #c putchar (char:I32) -> I32; a:(x:66; y:I32); putchar(a.x); putchar 10", "");
  // test("a:I32; a=5", "");
  // test("a:(x:I32; y:I32); a = (y:1; x:2); a.x", "");
  // test("putchar: #c putchar (char:I32) -> I32", "");
  // test("putchar: #c putchar (char:I32) -> I32; putchar 65; putchar 10", "");
  // test("putchar: #c putchar (char:I32) -> I32; foo : (a:I32; b:I32) -> a+b; putchar(foo(30;35)); putchar 65; putchar 10", "");
  // test("f:()->1", "");
  // test("a:1; a+a", "");
}

#undef test
