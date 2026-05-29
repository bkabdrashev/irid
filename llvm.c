#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Types.h>

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
  LLVMValueRef*      irs;
  LLVMBasicBlockRef* blocks;
  LLVMTypeRef*       types;
};

LLVM_Gen llvm_gen;

LLVMTypeRef llvm_of_type(LLVMContextRef context, Type* type) {
  I32 typeid = type - sem.types.base;
  LLVMTypeRef result = llvm_gen.types[typeid];
  if (result) return result;

  switch (type->kind) {
  case Type_Kind_none: {
    result = LLVMVoidTypeInContext(context);
  } break;
  case Type_Kind_int: {
    result = LLVMIntTypeInContext(context, 32);
  } break;
  case Type_Kind_ptr: {
    LLVMTypeRef pointer_to = llvm_of_type(context, type->pointer->declared);
    result = LLVMPointerType(pointer_to, 0);
  } break;
  case Type_Kind_record: {
    result = LLVMStructCreateNamed(context, "");
    LLVMTypeRef field_types[2] = {
      LLVMIntTypeInContext(context, 32),
      LLVMIntTypeInContext(context, 32),
    };
    LLVMStructSetBody(result, field_types, 2, 0);
  } break;
  case Type_Kind_fun: {
    LLVMTypeRef* arg_types;
    I32 arg_count = 0;
    if (type->fun->arg->kind == Type_Kind_none) {
      arg_types = 0;
      arg_count = 0;
    }
    else if (type->fun->arg->kind == Type_Kind_record) {
      Record* record = type->fun->arg->record;
      arg_count = record->length;
      assert(0);
    }
    else {
      LLVMTypeRef arg_type = llvm_of_type(context, type->fun->arg);
      arg_types = (LLVMTypeRef[1]){ arg_type };
    }
    LLVMTypeRef ret_type = llvm_of_type(context, type->fun->ret);
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
  return llvm_of_type(context, ir_type);
}

I32 llvm_funs(Arena* arena, Funs funs) {
  llvm_gen.blocks = arena_push(arena, irgen.blocks.length * sizeof(LLVMBasicBlockRef));
  llvm_gen.irs    = arena_push(arena, irgen.irs.length    * sizeof(LLVMValueRef));
  llvm_gen.types  = arena_push(arena, sem.types.length    * sizeof(LLVMTypeRef));
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef  module  = LLVMModuleCreateWithNameInContext("contex_name", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    LLVMTypeRef llvm_fun_type = llvm_of_type(context, fun->type);
    LLVMValueRef function = LLVMAddFunction(module, cstr_from_str(fun->name), llvm_fun_type);

    for (I32 b = 0; b < fun->blocks->length; b++) {
      Block* block = fun->blocks->base[b];
      LLVMBasicBlockRef llvm_block = LLVMAppendBasicBlockInContext(context, function, "block");
      llvm_of_block_put(block, llvm_block);
    }

    for (I32 b = 0; b < fun->blocks->length; b++) {
      Block* block = fun->blocks->base[b];
      LLVMBasicBlockRef llvm_block = llvm_of_block(block);
      LLVMPositionBuilderAtEnd(builder, llvm_block);
      LLVMValueRef one; LLVMValueRef two; LLVMValueRef unary;

      for (I32 i = 0; i < block->irs->length; i++) {
        Ir* ir = fa_get(block->irs, i);
        if (ir->kind & Ir_Flag_binary) {
          one = llvm_of_ir(ir->binary.one);
          two = llvm_of_ir(ir->binary.two);
        }
        else if (ir->kind & Ir_Flag_unary) {
          unary = llvm_of_ir(ir->unary);
        }

        LLVMValueRef llvm_ir;
        switch (ir->kind) {
        case Ir_Kind_var: {
          if (ir->var->declared->kind != Type_Kind_none) {
            if (ir->var->global) {
              LLVMTypeRef llvm_var_type = llvm_of_type(context, ir->var->declared);
              llvm_ir = LLVMAddGlobal(module, llvm_var_type, ir->var->name->base);
              // LLVMSetInitializer(llvm_ir, LLVMConstInt(llvm_var_type, 0, 0));
              LLVMSetGlobalConstant(llvm_ir, 0);
            }
            else {
              LLVMTypeRef llvm_var_type = llvm_of_type(context, ir->var->declared);
              llvm_ir = LLVMBuildAlloca(builder, llvm_var_type, ir->var->name->base);
            }
          }
        } break;
        case Ir_Kind_int: {
          Type* type = type_of_ir(ir);
          LLVMTypeRef llvm_type = llvm_of_type(context, type);
          llvm_ir = LLVMConstInt(llvm_type, ir->i64, 0);
        } break;

        case Ir_Kind_add: llvm_ir = LLVMBuildAdd(builder, one, two, ""); break;
        case Ir_Kind_sub: llvm_ir = LLVMBuildSub(builder, one, two, ""); break;
        case Ir_Kind_mul: llvm_ir = LLVMBuildMul(builder, one, two, ""); break;
        case Ir_Kind_eq:  llvm_ir = LLVMBuildICmp(builder, LLVMIntEQ, one, two, ""); break;
        case Ir_Kind_ne:  llvm_ir = LLVMBuildICmp(builder, LLVMIntNE, one, two, ""); break;
        case Ir_Kind_lt:  llvm_ir = LLVMBuildICmp(builder, LLVMIntSLT, one, two, ""); break;
        case Ir_Kind_le:  llvm_ir = LLVMBuildICmp(builder, LLVMIntSLE, one, two, ""); break;
        case Ir_Kind_gt:  llvm_ir = LLVMBuildICmp(builder, LLVMIntSGT, one, two, ""); break;
        case Ir_Kind_ge:  llvm_ir = LLVMBuildICmp(builder, LLVMIntSGE, one, two, ""); break;
        case Ir_Kind_neg: llvm_ir = LLVMBuildNeg(builder, unary, ""); break;

        case Ir_Kind_name_offset: {
          Type* of_type = type_of_ir(ir->name.of);
          Ir_Kind ir_kind = ir->name.of->kind;
          if (ir_kind == Ir_Kind_load) {
            LLVMValueRef ptr = llvm_of_ir(ir->name.of->unary);
            I32 position = hash_map_get_i32(&of_type->record->position_from_name, ir->name.at);
            LLVMTypeRef llvm_type = llvm_of_type(context, of_type);
            llvm_ir = LLVMBuildStructGEP2(builder, llvm_type, ptr, position, "");
          }
        } break;
        case Ir_Kind_load:  {
          Type* type = type_of_ir(ir);
          LLVMTypeRef llvm_type = llvm_of_type(context, type);
          llvm_ir = LLVMBuildLoad2(builder, llvm_type, unary, "");
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
                LLVMValueRef llvm_assigned = llvm_of_ir(record_one->assigned[position]);
                LLVMTypeRef llvm_type = llvm_of_type(context, record_type);
                LLVMValueRef llvm_gep = LLVMBuildStructGEP2(builder, llvm_type, one, position, "");
                LLVMBuildStore(builder, llvm_assigned, llvm_gep);
              }
              else {
                assert(0);
              }
            }
          }
          else {
            llvm_ir = LLVMBuildStore(builder, two, one);
          }
        } break;
        case Ir_Kind_record: {
        } break;
        case Ir_Kind_ptr: assert(0);
        case Ir_Kind_position_offset: assert(0);

        case Ir_Kind_fun:     assert(0);
        case Ir_Kind_call:    assert(0);
        case Ir_Kind_declare: break;
        case Ir_Kind_none:    break;
        case Ir_Kind_range:   break;
        case Ir_Kind_join:    break;
        case Ir_Kind_bits:    break;

        }

        llvm_of_ir_put(ir, llvm_ir);
      }
      switch (block->kind) {
      case Block_Kind_none: {
      } break;
      case Block_Kind_jump: {
        LLVMBasicBlockRef llvm_to_block = llvm_of_block(block->jump.to_block);
        LLVMBuildBr(builder, llvm_to_block);
      } break;
      case Block_Kind_branch: {
        LLVMValueRef cond = llvm_of_ir(block->branch.cond);
        LLVMBasicBlockRef then_block = llvm_of_block(block->branch.nez.to_block);
        LLVMBasicBlockRef else_block = llvm_of_block(block->branch.eqz.to_block);
        LLVMBuildCondBr(builder, cond, then_block, else_block);
      } break;
      }
    }

    Type* ret_type = fun->type->fun->ret;
    if (ret_type->kind == Type_Kind_none) {
      LLVMBuildRetVoid(builder);
    }
    else {
      LLVMTypeRef ret_var_type = llvm_of_type(context, fun->type->fun->ret);
      LLVMValueRef ret_var  = LLVMBuildAlloca(builder, ret_var_type, "my_new_test");

      LLVMTypeRef llvm_ret_type = llvm_of_type(context, ret_type);
      LLVMValueRef ret_loaded = LLVMBuildLoad2(builder, llvm_ret_type, ret_var, "loaded_ret_var");
      LLVMBuildRet(builder, ret_loaded);
    }
  }

  char *error = NULL;
  if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error)) {
    fprintf(stderr, "Error verifying module: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  printf("Generated LLVM IR:\n");
  LLVMDumpModule(module);

  if (error) {
    LLVMDisposeMessage(error);
  }
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(module);
  LLVMContextDispose(context);
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
  test_at_source("", expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_llvm(source, expected, __FILE__, __LINE__)

void llvm_test(void) {
  // test("a:I32; a=5", "");
  // test("a:(x:I32; y:I32); a = (y=1; x=2); a.x", "");
  // test("putchar: #c putchar (char:I32) -> I32", "");
}

#undef test
