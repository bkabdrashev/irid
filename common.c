#include <sys/types.h>
#include <stdint.h>

#define false (0)
#define true (1)

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef int8_t    b8;
typedef int16_t   b16;
typedef int32_t   b32;
typedef int64_t   b64;

typedef u_int8_t   u8;
typedef u_int16_t  u16;
typedef u_int32_t  u32;
typedef u_int64_t  u64;

typedef float    f32;
typedef double   f64;

typedef size_t    umi; // unsigned memory index
typedef ssize_t   smi; //   signed memory index
typedef uintptr_t ump; // unsigned memory pointer
typedef intptr_t  smp; //   signed memory pointer

#define S64_MAX INT64_MAX
#define S64_MIN INT64_MIN
#define S32_MAX INT32_MAX
#define S32_MIN INT32_MIN

#define KB(a) ((a)*1024)
#define MB(a) (KB(a)*1024)
#define GB(a) (MB(a)*1024)
#define TB(a) (GB(a)*1024)

typedef struct {
  void* base;
  void* top;
  umi   size;
} Arena; 

void arena_init(Arena* arena, umi size) {
  arena->base = malloc(size);
  arena->top = arena->base;
  arena->size = size;
  arena->alignment = 8;
}

void* arena_alloc(Arena* arena, umi size) {
  assert(top+size <= base+arena->size);
  arena->top = (arena->top + size + arena->alignment - 1) & ~(arena->alignment - 1);
  return arena->top;
}

Arena temp_arena = {0};

