#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

LLVMTypeRef llvm_type_map(LLVMContextRef context, Type* type) {
  LLVMTypeRef result;
  switch (type->kind) {
  case Type_Kind_none: {
    result = LLVMVoidTypeInContext(context);
  } break;
  case Type_Kind_int: {
    result = LLVMIntTypeInContext(context, type->bits_size);
  } break;
  case Type_Kind_ptr: {
    LLVMTypeRef pointer_to = llvm_type_map(context, type->pointer->declared);
    result = LLVMPointerType(pointer_to, 0);
  } break;
  case Type_Kind_record: {
    assert(0);
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
      LLVMTypeRef arg_type = llvm_type_map(context, type->fun->arg);
      arg_types = (LLVMTypeRef[1]){ arg_type };
    }
    LLVMTypeRef ret_type = llvm_type_map(context, type->fun->ret);
    result = LLVMFunctionType(ret_type, arg_types, arg_count, 0);
  } break;
  }
  return result;
}

LLVMTypeRef llvm_type_of_ir(LLVMContextRef context, Ir* ir) {
  Type* ir_type = type_of_ir(ir);
  return llvm_type_map(context, ir_type);
}

I32 llvm_funs(Funs funs) {
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("contex_name", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  for (I32 f = 0; f < funs.length; f++) {
    Fun* fun = &funs.base[f];
    LLVMTypeRef llvm_fun_type = llvm_type_map(context, fun->type);
    LLVMValueRef function = LLVMAddFunction(module, cstr_from_str(fun->name), llvm_fun_type);

    for (I32 b = 0; b < fun->blocks->length; b++) {
      Block* block = fun->blocks->base[b];
      block->llvm_block = LLVMAppendBasicBlockInContext(context, function, "block");
    }

    for (I32 b = 0; b < fun->blocks->length; b++) {
      Block* block = fun->blocks->base[b];
      LLVMPositionBuilderAtEnd(builder, block->llvm_block);
      for (I32 i = 0; i < block->irs->length; i++) {
        // Ir* ir = fa_get(block->irs, i);
        // Type* type = type_of_ir(ir);
        // LLVMTypeRef llvm_type = llvm_type_map(context, type);
      }
      switch (block->kind) {
      case Block_Kind_none: {
      } break;
      case Block_Kind_jump: {
        LLVMBuildBr(builder, block->jump.to_block->llvm_block);
      } break;
      case Block_Kind_branch: {
      } break;
      }
    }

    Type* ret_type = fun->type->fun->ret;
    if (ret_type->kind == Type_Kind_none) {
      LLVMBuildRetVoid(builder);
    }
    else {
      LLVMTypeRef ret_var_type = llvm_type_map(context, fun->type->fun->ret);
      LLVMValueRef ret_var  = LLVMBuildAlloca(builder, ret_var_type, "my_new_test");

      LLVMTypeRef llvm_ret_type = llvm_type_map(context, ret_type);
      LLVMValueRef ret_loaded = LLVMBuildLoad2(builder, llvm_ret_type, ret_var, "loaded_ret_var");
      LLVMBuildRet(builder, ret_loaded);
    }
  }

  // 6. Verify the module is correct
  char *error = NULL;
  if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error)) {
    fprintf(stderr, "Error verifying module: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  // 7. Print the generated IR to stdout
  printf("Generated LLVM IR:\n");
  LLVMDumpModule(module);

  // 8. (Optional) Write the module to a bitcode file
  if (LLVMWriteBitcodeToFile(module, "example.bc") != 0) {
    fprintf(stderr, "Error writing bitcode to file\n");
  }

  // 9. Clean up
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
                         llvm_funs(funs);
  test_at_source("", expected, file_name, line, source);
  arena_free(&arena);
}

#define test(source, expected) _test_llvm(source, expected, __FILE__, __LINE__)

void llvm_test(void) {
  test("()->1; ()->2", "");
}

#undef test
