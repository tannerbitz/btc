#ifndef BTC_H
#define BTC_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef uint8_t b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;
#include <stdbool.h>

#define btc_offsetof(Type, field) (isize)&((cast(Type*)0)->field)

#ifndef align_of
#define align_of(Type)                                                         \
  offsetof(                                                                    \
      struct {                                                                 \
        char c;                                                                \
        Type member;                                                           \
      },                                                                       \
      member)
#endif

#include <assert.h>

b8 is_power_of_two(uintptr_t x);
uintptr_t align_forward(uintptr_t ptr, usize align);

typedef struct {
  u8 *base;
  u64 size;
  u64 prev_offset;
  u64 curr_offset;
} Arena;

void* arena_alloc_align(Arena* arena, usize size, usize align);
void* arena_alloc(Arena* arena, usize size);
void arena_init(Arena* arena, void* backing_buffer, usize buffer_size);
void arena_free(Arena* arena, void* ptr);
void* arena_resize_align(Arena* arena, void* old_memory, usize old_size, usize new_size, usize align);
void* arena_resize(Arena* arena, void* old_memory, usize old_size, usize new_size);
void arena_free_all(Arena* arena);

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2*sizeof(void*))
#endif

typedef struct {
  void *(*alloc)(void *context, usize bytes, usize alignment);
  void *(*resize)(void *context, void *ptr, usize old_size, usize new_size, usize alignment);
  void (*free)(void *context, void *ptr);
  void *context;
} Allocator;

void *allocator_alloc_align(Allocator *allocator, usize bytes, usize alignment);
void* allocator_alloc(Allocator* allocator, usize bytes);
void *allocator_resize_align(Allocator *allocator, void *ptr, usize old_size, usize new_size, usize alignment);
void *allocator_resize(Allocator *allocator, void *ptr, usize old_size, usize new_size);
void allocator_free(Allocator *allocator, void *ptr);

#define alloc_one(allocator, type)                                             \
  allocator_alloc(allocator, sizeof(type), align_of(type))

#define alloc_many(allocator, type, number)                                    \
  allocator_alloc(allocator, sizeof(type) * number, align_of(type))


void *arena_alloc_erased(void *arena, usize bytes, usize alignment);
void *arena_resize_erased(void *arena, void *ptr, usize old_size, usize new_size, usize alignment);
void arena_free_erased(void *arena, void *ptr);
Allocator arena_make_allocator(Arena *arena);

typedef struct String {
  usize len;
  char* data;
} String;

#ifdef BTC_IMPLEMENTATION
b8 is_power_of_two(uintptr_t x) {
  return (x & (x-1)) == 0;
}

uintptr_t align_forward(uintptr_t ptr, usize align) {
  assert(is_power_of_two(align));

  uintptr_t p = ptr;
  uintptr_t a = cast(uintptr_t)align;

  uintptr_t modulo = p & (a-1);

  if (modulo != 0) {
    p += (a- modulo);
  }
  return p;
}

void* arena_alloc_align(Arena* arena, usize size, usize align) {
  uintptr_t curr_ptr = cast(uintptr_t)arena->base + cast(uintptr_t)arena->curr_offset;
  uintptr_t offset = align_forward(curr_ptr, align);
  offset -= cast(uintptr_t)arena->base;

  if (offset+size <= arena->size) {
    arena->prev_offset = offset;
    arena->curr_offset = offset+size;
    void* ptr = &arena->base[offset];
    memset(ptr, 0, size);
    return ptr;
  }
  return NULL;
}

void* arena_alloc(Arena* arena, usize size) {
  return arena_alloc_align(arena, size, DEFAULT_ALIGNMENT);
}

void arena_init(Arena* arena, void* backing_buffer, usize buffer_size) {
  arena->base = cast(u8*)backing_buffer;
  arena->curr_offset = 0;
  arena->prev_offset = 0;
  arena->size = buffer_size;
}

void arena_free(Arena* arena, void* ptr) {
  UNUSED(arena);
  UNUSED(ptr);
}

void* arena_resize_align(Arena* arena, void* old_memory, usize old_size, usize new_size, usize align) {
  u8* old_mem = cast(u8*)old_memory;

  assert(is_power_of_two(align));

  if (old_mem == NULL || old_size == 0) {
    return arena_alloc_align(arena, new_size, align);
  } else if (arena->base <= old_mem && old_mem < arena->base+arena->size) {
    if (arena->base+arena->prev_offset == old_mem) {
      arena->curr_offset = arena->prev_offset + new_size;
      if (new_size > old_size) {
        memset(&arena->base[arena->curr_offset], 0, new_size);
      }
      return old_memory;
    } else {
      void* new_memory = arena_alloc_align(arena, new_size, align);
      usize copy_size = old_size < new_size ? old_size : new_size;
      memmove(new_memory, old_memory, copy_size);
      return new_memory;
    }
  } else {
    assert(0 && "Memory is out of bounds of the buffer in this arena");
    return NULL;
  }
}

void* arena_resize(Arena* arena, void* old_memory, usize old_size, usize new_size) {
  return arena_resize_align(arena, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

void arena_free_all(Arena* arena) {
  arena->curr_offset = 0;
  arena->prev_offset = 0;
}

void *allocator_alloc_align(Allocator *allocator, usize bytes, usize alignment) {
  return allocator->alloc(allocator->context, bytes, alignment);
}

void* allocator_alloc(Allocator* allocator, usize bytes) {
  return allocator_alloc_align(allocator, bytes, DEFAULT_ALIGNMENT);
}

void *allocator_resize_align(Allocator *allocator, void *ptr, usize old_size, usize new_size, usize alignment) {
  return allocator->resize(allocator->context, ptr, old_size, new_size, alignment);
}

void *allocator_resize(Allocator *allocator, void *ptr, usize old_size, usize new_size) {
  return allocator->resize(allocator->context, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void allocator_free(Allocator *allocator, void *ptr) {
  allocator->free(allocator->context, ptr);
}

#define alloc_one(allocator, type)                                             \
  allocator_alloc(allocator, sizeof(type), align_of(type))

#define alloc_many(allocator, type, number)                                    \
  allocator_alloc(allocator, sizeof(type) * number, align_of(type))


void *arena_alloc_erased(void *arena, usize bytes, usize alignment) {
  return arena_alloc_align(cast(Arena *) arena, bytes, alignment);
}

void *arena_resize_erased(void *arena, void *ptr, usize old_size, usize new_size, usize alignment) {
  return arena_resize_align(cast(Arena *) arena, ptr, old_size, new_size, alignment);
}

void arena_free_erased(void *arena, void *ptr) {
  return arena_free(cast(Arena *) arena, ptr);
}

Allocator arena_make_allocator(Arena *arena) {
  return (Allocator){
      .alloc = arena_alloc_erased,
      .resize = arena_resize_erased,
      .free = arena_free_erased,
      .context = cast(void *) arena,
  };
};

#endif


#endif // BTC_H
