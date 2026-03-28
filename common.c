#define false (0)
#define true (1)

typedef  char     c8;

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef int8_t    b8;
typedef int16_t   b16;
typedef int32_t   b32;
typedef int64_t   b64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

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

#define KB(a) ((a)*1024llu)
#define MB(a) (KB(a)*1024llu)
#define GB(a) (MB(a)*1024llu)
#define TB(a) (GB(a)*1024llu)

typedef struct {
  c8* base;
  c8* top;
  umi   capacity;
  s8    alignment;
} Arena; 

void arena_init(Arena* arena, umi capacity) {
  arena->base = malloc(capacity);
  arena->top = arena->base;
  arena->capacity = capacity;
  arena->alignment = 8;
}

void* arena_alloc(Arena* arena, umi size) {
  assert(arena->top + size <= arena->base + arena->capacity);
  void* result = arena->top;
  umi first_bits_off_mask = ~(arena->alignment - 1);
  umi overshoot_up        = ((umi)arena->top + size + arena->alignment - 1);
  arena->top = (void*)(overshoot_up & first_bits_off_mask);
  return result;
}

void arena_free_all(Arena* arena) {
  arena->top = arena->base;
}

Arena temp_arena = {0};

u64 hash_bytes(const void* ptr, u64 len) {
  u64 x = 0xcbf29ce484222325;
  c8* buf = (c8 *)ptr;
  for (u64 i = 0; i < len; i++) {
    x ^= buf[i];
    x *= 0x100000001b3;
    x ^= x >> 32;
  }
  return x;
}

void* xmalloc(umi num_bytes) {
  void* ptr = malloc(num_bytes);
  if (!ptr) {
    perror("xmalloc failed");
    exit(1);
  }
  return ptr;
}
