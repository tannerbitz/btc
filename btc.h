#ifndef BTC_H
#define BTC_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define cast(type) (type)
#define UNUSED(arg) cast(void) arg

// clang-format off
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef size_t    isize;
typedef ptrdiff_t usize;

#ifndef align_of
#define align_of(Type)                                                         \
  offsetof(                                                                    \
      struct {                                                                 \
        char c;                                                                \
        Type member;                                                           \
      },                                                                       \
      member)
#endif

#define align_pow_2(base_addr, alignment)                                      \
  (((base_addr) + (alignment) - 1) & (~((alignment) - 1)))

typedef struct {
  void *(*alloc)(void *context, size_t bytes, size_t alignment);
  void *(*realloc)(void *context, void *ptr, size_t bytes, size_t alignment);
  void (*free)(void *context, void *ptr);
  void *context;
} Allocator;

void *allocator_alloc(Allocator *allocator, size_t bytes, size_t alignment) {
  return allocator->alloc(allocator->context, bytes, alignment);
}

void *allocator_realloc(Allocator *allocator, void *ptr, size_t bytes,
                        size_t alignment) {
  return allocator->realloc(allocator->context, ptr, bytes, alignment);
}

void allocator_free(Allocator *allocator, void *ptr) {
  allocator->free(allocator->context, ptr);
}

#define alloc_one(allocator, type)                                             \
  allocator_alloc(allocator, sizeof(type), align_of(type))

#define alloc_many(allocator, type, number)                                    \
  allocator_alloc(allocator, sizeof(type) * number, align_of(type))

typedef struct {
  void *base;
  u64 size;
  void *pos;
} Arena;

Arena make_arena() {
  isize sz = sysconf(_SC_PAGESIZE);
  Arena arena = {.base = NULL, .pos = NULL, .size = 0};
  arena.base = malloc(sz);
  arena.size = sz;
  arena.pos = arena.base;
  return arena;
};

void *arena_alloc(Arena *arena, size_t bytes, size_t alignment) {
  usize available = arena->base + arena->size - arena->pos;
  void *start = cast(void *) align_pow_2(cast(u64) arena->pos, alignment);
  usize alloc_size = (start - arena->pos) + bytes;
  if (alloc_size > available) {
    fprintf(stderr, "Out of arena memory!");
    exit(1);
  }

  arena->pos += alloc_size;
  arena->size -= alloc_size;
  return arena->pos;
}
void *arena_realloc(Arena *arena, void *ptr, size_t bytes, size_t alignment) {
  // unimplemented
  return NULL;
}
void arena_free(Arena *arena, void *ptr) {
  // no-op
}

void *arena_alloc_erased(void *arena, size_t bytes, size_t alignment) {
  return arena_alloc(cast(Arena *) arena, bytes, alignment);
}

void *arena_realloc_erased(void *arena, void *ptr, size_t bytes,
                           size_t alignment) {
  return arena_realloc(cast(Arena *) arena, ptr, bytes, alignment);
}

void arena_free_erased(void *arena, void *ptr) {
  return arena_free(cast(Arena *) arena, ptr);
}

Allocator arena_make_allocator(Arena *arena) {
  return (Allocator){
      .alloc = arena_alloc_erased,
      .realloc = arena_realloc_erased,
      .free = arena_free_erased,
      .context = cast(void *) arena,
  };
};


#endif // BTC_H
