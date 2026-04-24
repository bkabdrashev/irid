#define false 0
#define true 1

typedef  char     C8;

typedef int8_t    I8;
typedef int16_t   I16;
typedef int32_t   I32;
typedef int64_t   I64;

typedef int8_t    B8;
typedef int16_t   B16;
typedef int32_t   B32;
typedef int64_t   B64;

typedef uint8_t   U8;
typedef uint16_t  U16;
typedef uint32_t  U32;
typedef uint64_t  U64;

typedef float    F32;
typedef double   F64;

typedef size_t    Umi; // unsigned memory index
typedef ssize_t   Smi; //   signed memory index
typedef uintptr_t Ump; // unsigned memory pointer
typedef intptr_t  Smp; //   signed memory pointer

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define I32_MAX INT32_MAX
#define I32_MIN INT32_MIN

#define KB(a) ((a)*1024llu)
#define MB(a) (KB(a)*1024llu)
#define GB(a) (MB(a)*1024llu)
#define TB(a) (GB(a)*1024llu)

typedef struct {
  C8* base;
  C8* top;
  Umi   capacity;
  I8    alignment;
} Arena; 

Arena arena_init(Umi capacity) {
  Arena arena = {};
  arena.base = malloc(capacity);
  arena.top = arena.base;
  arena.capacity = capacity;
  arena.alignment = 8;
  return arena;
}

void* arena_push(Arena* arena, Umi size) {
  assert(arena->top + size <= arena->base + arena->capacity);
  void* result = arena->top;
  Umi first_bits_off_mask = ~(arena->alignment - 1);
  Umi overshoot_up        = ((Umi)arena->top + size + arena->alignment - 1);
  arena->top = (void*)(overshoot_up & first_bits_off_mask);
  return result;
}

void* arena_push_zero(Arena* arena, Umi size) {
  void* mem = arena_push(arena, size);
  memset(mem, 0, size);
  return mem;
}

void arena_free_all(Arena* arena) {
  arena->top = arena->base;
}

U64 hash_bytes(const void* ptr, U64 len) {
  U64 x = 0xcbf29ce484222325;
  C8* buf = (C8 *)ptr;
  for (U64 i = 0; i < len; i++) {
    x ^= buf[i];
    x *= 0x100000001b3;
    x ^= x >> 32;
  }
  return x;
}

void* xmalloc(Umi num_bytes) {
  void* ptr = malloc(num_bytes);
  if (!ptr) {
    perror("xmalloc failed");
    exit(1);
  }
  return ptr;
}

void* xcalloc(Umi size, Umi length) {
  void* ptr = calloc(size, length);
  if (!ptr) {
    perror("xcalloc failed");
    exit(1);
  }
  return ptr;
}

Umi power_of_2_up(Umi v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;
  return v;
}

typedef struct {
  I32* base;
  I32 length;
} Slice_S32;

#define empty(slice) ((slice).length == 0)
#define push(slice, item) ((slice).base[(slice).length++] = (item), ((slice).length)-1)
#define add(slice, item) do {(slice).base[(slice).length++] = (item);} while(0);
#define new(slice) ((slice).base[(slice).length++])
#define pop(slice) ((slice).base[--(slice).length])
#define del(slice) (--(slice).length)
#define top(slice) ((slice).base[(slice).length-1])
#define get(slice, index) ((slice).base[(index)])

I32 slice_s32_push(Slice_S32* slice, I32 item) {
  I32 index = slice->length;
  slice->base[slice->length++] = item;
  return index;
}
I32 slice_s32_pop(Slice_S32* slice) {
  return slice->base[--slice->length];
}
I32 slice_s32_top(Slice_S32* slice) {
  return slice->base[slice->length-1];
}
B8 slice_s32_is_empty(Slice_S32* slice) {
  return slice->length == 0;
}

