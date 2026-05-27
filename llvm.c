#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

I32 llvm_funs(Funs funs) {
  // 1. Create a context and a module
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("example", context);

  // 2. Define the function signature: i32 ()*
  LLVMTypeRef return_type = LLVMInt32TypeInContext(context);
  LLVMTypeRef param_types[1] = {}; // No parameters
  LLVMTypeRef function_type = LLVMFunctionType(return_type, param_types, 0, 0);

  // 3. Add the function to the module
  LLVMValueRef function = LLVMAddFunction(module, "main", function_type);

  // 4. Create a basic block and an IR builder
  LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, function, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  LLVMPositionBuilderAtEnd(builder, entry);

  // 5. Generate the IR: return 42
  LLVMValueRef forty_two = LLVMConstInt(return_type, 42, 0);
  LLVMBuildRet(builder, forty_two);

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
  test("1", "");
}

#undef test
