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
#define I16_MAX INT16_MAX
#define I16_MIN INT16_MIN

#define KB(a) ((a)*1024llu)
#define MB(a) (KB(a)*1024llu)
#define GB(a) (MB(a)*1024llu)
#define TB(a) (GB(a)*1024llu)

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

U64 bit_width(U64 x) {
  U64 w = 0;
  if (x >= (1ULL << 32)) { w += 32; x >>= 32; }
  if (x >= (1ULL << 16)) { w += 16; x >>= 16; }
  if (x >= (1ULL << 8))  { w += 8;  x >>= 8;  }
  if (x >= (1ULL << 4))  { w += 4;  x >>= 4;  }
  if (x >= (1ULL << 2))  { w += 2;  x >>= 2;  }
  if (x >= (1ULL << 1))  { w += 1;           }
  return w + 1;
}

I32 bits_needed(I64 min, I64 max) {
  // if (min == max) return 0;

  if (min >= 0) {
    return bit_width((U64)max);
  }

  if (max <= 0) {
    U64 x = (U64)(-min);
    return bit_width(x - 1) + 1;
  }

  unsigned a = (unsigned)max;
  unsigned b = (unsigned)(-min);
  unsigned w = bit_width(a);
  unsigned v = bit_width(b - 1);
  return (w > v ? w : v) + 1;
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

Umi align_up(Umi val, Umi alignment) {
  Umi over_up             = val + alignment - 1;
  Umi first_bits_off_mask = ~(alignment - 1);
  Umi masked              = over_up & first_bits_off_mask;
  return masked;
}

typedef struct {
  C8* base;
  C8* top;
  Umi   capacity;
  I8    alignment;
} Arena;

Arena arena_init(Umi capacity) {
  Arena arena = {};
  arena.base = xmalloc(capacity);
  arena.top = arena.base;
  arena.capacity = capacity;
  arena.alignment = 8;
  return arena;
}

void* arena_push(Arena* arena, Umi size) {
  assert(arena->top + size <= arena->base + arena->capacity);
  void* result = arena->top;
  Umi aligned_up = (Umi)arena->top + align_up(size, arena->alignment);
  arena->top  = (void*)(aligned_up);
  return result;
}

void* arena_push_zero(Arena* arena, Umi size) {
  void* mem = arena_push(arena, size);
  memset(mem, 0, size);
  return mem;
}

void arena_release_all(Arena* arena) {
  arena->top = arena->base;
}

void arena_release_mark(Arena* arena, void* mark) {
  arena->top = mark;
}

C8* arena_mark(Arena* arena) {
  return arena->top;
}

void arena_free(Arena* arena) {
  free(arena->base);
  arena->top = NULL;
  arena->base = NULL;
}

void arena_test() {
  Arena arena = arena_init(KB(64));
  C8* buf0 = arena_push(&arena, 16);
  assert(arena.base + 16 <= arena.top);
  assert(buf0 + 16 <= arena.top);
  C8* buf1 = arena_push(&arena, 32);
  assert(arena.base + 16 + 32 <= arena.top);
  assert(buf0 + 16 <= buf1);
  C8* buf2 = arena_push(&arena, 64);
  assert(arena.base + 16 + 32 + 64 <= arena.top);
  assert(buf1 + 32 <= buf2);
  arena_free(&arena);
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

U64 hash_u64(U64 x) {
  x *= 0xff51afd7ed558ccd;
  x ^= x >> 32;
  return x;
}

// array
#define empty(slice) ((slice).length == 0)
#define push(slice, item) ((slice).base[(slice).length++] = (item), ((slice).length)-1)
#define add(slice, item) do {(slice).base[(slice).length++] = (item);} while(0);
#define new(slice) ((slice).base[(slice).length++])
#define pop(slice) ((slice).base[--(slice).length])
#define del(slice) (--(slice).length)
#define top(slice) ((slice).base[(slice).length-1])
#define get(slice, index) ((slice).base[(index)])
#define put(slice, index, value) ((slice).base[(index)] = (value))

#define min(x, y) ((x) <= (y) ? (x) : (y))
#define max(x, y) ((x) >= (y) ? (x) : (y))

// flexible array
#define fa_empty(fa) ((fa)->length == 0)
#define fa_push(fa, item) ((fa)->base[(fa)->length++] = (item), ((fa)->length)-1)
#define fa_add(fa, item) do {(fa)->base[(fa)->length++] = (item);} while(0);
#define fa_new(fa) ((fa)->base[(fa)->length++])
#define fa_pop(fa) ((fa)->base[--(fa)->length])
#define fa_del(fa) (--(fa)->length)
#define fa_top(fa) ((fa)->base[(fa)->length-1])
#define fa_get(fa, index) ((fa)->base[(index)])
#define fa_put(fa, index, value) ((fa)->base[(index)] = (value))

typedef struct Hash_Set Hash_Set;
struct Hash_Set {
  I32  cap;
  I32  len;
  void** list;
  void** keys;
};

Hash_Set hash_set_init(Arena* arena, Umi capacity) {
  Hash_Set set = {};
  set.cap  = 2*power_of_2_up(capacity);
  set.len  = 0;
  set.keys = arena_push_zero(arena, sizeof(void*)*set.cap);
  set.list = arena_push_zero(arena, sizeof(void*)*capacity);
  return set;
}

Hash_Set hash_set_copy(Arena* arena, Hash_Set set) {
  Hash_Set new_set = {};
  I32 size = sizeof(void*)*set.cap;
  new_set.cap  = set.cap;
  new_set.len  = set.len;
  new_set.keys = arena_push(arena, size);
  new_set.list = arena_push(arena, sizeof(void*) * set.len);
  memcpy(new_set.keys, set.keys, size);
  memcpy(new_set.list, set.list, sizeof(void*) * set.len);

  return new_set;
}

B8 hash_set_put(Hash_Set* set, void* key) {
  I32 i = hash_u64((U64)key);
  for (;;) {
    i &= set->cap - 1;
    if (!set->keys[i]) {
      set->keys[i] = key;
      set->list[set->len++] = key;
      return true;
    }
    else if (set->keys[i] == key) {
      return false;
    }
    i++;
  }
}

B8 hash_set_exists(Hash_Set* set, void* key) {
  I32 i = hash_u64((U64)key);
  for (;;) {
    i &= set->cap - 1;
    if (!set->keys[i]) {
      return false;
    }
    else if (set->keys[i] == key) {
      return true;
    }
    i++;
  }
}

Hash_Set hash_set_meet(Arena* arena, Hash_Set* one, Hash_Set* two) {
  if (one->len > two->len) {
    return hash_set_meet(arena, two, one);
  }
  Hash_Set new_set = hash_set_init(arena, one->len);
  for (I32 i = 0; i < one->len; i++) {
    void* key = one->list[i];
    if (hash_set_exists(two, key)) {
      hash_set_put(&new_set, key);
    }
  }
  return new_set;
}

Hash_Set hash_set_join(Arena* arena, Hash_Set* one, Hash_Set* two) {
  Hash_Set new_set = hash_set_init(arena, one->len + two->len);
  for (I32 i = 0; i < one->len; i++) {
    hash_set_put(&new_set, one->list[i]);
  }
  for (I32 i = 0; i < two->len; i++) {
    hash_set_put(&new_set, two->list[i]);
  }
  return new_set;
}

Hash_Set hash_set_exclude(Arena* arena, Hash_Set* set, void* key) {
  Hash_Set new_set = hash_set_init(arena, set->len);
  for (I32 i = 0; i < set->len; i++) {
    if (set->list[i] != key) {
      hash_set_put(&new_set, set->list[i]);
    }
  }
  return new_set;
}

B8 hash_set_is_equal(Hash_Set one, Hash_Set two) {
  if (one.len != two.len) return false;
  for (I32 i = 0; i < one.len; i++) {
    void* key = one.list[i];
    B8 exists = hash_set_exists(&two, key);
    if (!exists) {
      return false;
    }
  }
  return true;
}

typedef struct Hash_Map Hash_Map;
struct Hash_Map {
  I32  cap;
  I32  len;
  void** list;
  void** keys;
  void** vals;
};

Hash_Map hash_map_init(Arena* arena, Umi capacity) {
  Hash_Map map = {};
  map.cap  = 2*power_of_2_up(capacity);
  map.len  = 0;
  map.keys = arena_push_zero(arena, sizeof(void*)*map.cap);
  map.list = arena_push_zero(arena, sizeof(void*)*capacity);
  map.vals = arena_push(arena, sizeof(void*)*map.cap);
  return map;
}

Hash_Map hash_map_copy(Arena* arena, Hash_Map map) {
  Hash_Map new_map = {};
  I32 size = sizeof(void*)*new_map.cap;
  new_map.cap  = map.cap;
  new_map.len  = map.len;
  new_map.keys = arena_push(arena, size);
  new_map.list = arena_push(arena, map.len);
  new_map.vals = arena_push(arena, size);

  memcpy(new_map.keys, map.keys, size);
  memcpy(new_map.vals, map.vals, size);
  memcpy(new_map.list, map.list, map.len);

  return new_map;
}

B8 hash_map_change_if_exists(Hash_Map* map, void* key, void* val) {
  I32 i = hash_u64((U64)key);
  for (;;) {
    i &= map->cap - 1;
    if (!map->keys[i]) {
      return false;
    }
    else if (map->keys[i] == key) {
      map->vals[i] = val;
      return true;
    }
    i++;
  }
}

void hash_map_put(Hash_Map* map, void* key, void* val) {
  I32 i = hash_u64((U64)key);
  for (;;) {
    i &= map->cap - 1;
    if (!map->keys[i]) {
      map->keys[i] = key;
      map->vals[i] = val;
      map->list[map->len++] = key;
      break;
    }
    else if (map->keys[i] == key) {
      map->vals[i] = val;
      break;
    }
    i++;
  }
}

void* hash_map_get(Hash_Map* map, void* key) {
  I32 i = hash_u64((U64)key);
  for (;;) {
    i &= map->cap - 1;
    if (!map->keys[i]) {
      return 0;
    }
    else if (map->keys[i] == key) {
      return map->vals[i];
    }
    i++;
  }
}

void** hash_map_get_ptr(Hash_Map* map, void* key) {
  I32 i = hash_u64((U64)key);
  for (;;) {
    i &= map->cap - 1;
    if (!map->keys[i]) {
      return 0;
    }
    else if (map->keys[i] == key) {
      return &map->vals[i];
    }
    i++;
  }
}

void hash_map_put_i32(Hash_Map* map, void* key, I32 val) {
  I64 cast = val;
  hash_map_put(map, key, (void*)cast);
}

I32 hash_map_get_i32(Hash_Map* map, void* key) {
  void* val = hash_map_get(map, key);
  I64 i64 = (I64)val;
  return i64;
}

void* hash_map_i32_get(Hash_Map* map, I32 key) {
  I64 cast = key;
  void* val = hash_map_get(map, (void*)cast);
  return val;
}

void hash_map_i32_put(Hash_Map* map, I32 key, void* val) {
  I64 cast = key;
  hash_map_put(map, (void*)cast, val);
}

B8 hash_map_is_equal(Hash_Map one, Hash_Map two) {
  if (one.len != two.len) return false;
  for (I32 i = 0; i < one.len; i++) {
    void* key = one.list[i];
    void* one_val = hash_map_get(&one, key);
    void* two_val = hash_map_get(&two, key);
    if (one_val != two_val) {
      return false;
    }
  }
  return true;
}
