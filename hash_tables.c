#define BTC_IMPLEMENTATION
#include "btc.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum Read_Entire_File_Return {
  Read_Entire_File_Return__SUCCESS,
  Read_Entire_File_Return__FILE_OPEN_FAILURE,
  Read_Entire_File_Return__FSEEK_FAILURE,
  Read_Entire_File_Return__FREAD_FAILURE,
  Read_Entire_File_Return__ALLOCATION_FAILURE,
} Read_Entire_File_Return;

Read_Entire_File_Return read_entire_file(const char *filepath, String* contents, Allocator* allocator) {
  String tmp = {0};

  FILE *file = fopen(filepath, "r");
  if (NULL == file) {
    return Read_Entire_File_Return__FILE_OPEN_FAILURE;
  }
  
  Read_Entire_File_Return rc = Read_Entire_File_Return__SUCCESS;
  if (0 != fseek(file, 0, SEEK_END)) {
    rc = Read_Entire_File_Return__FSEEK_FAILURE;
    goto close_file;
  }
  long file_length = ftell(file);
  if (0 != fseek(file, 0, SEEK_SET)) {
    rc = Read_Entire_File_Return__FSEEK_FAILURE;
    goto close_file;
  }

  tmp.data = allocator_alloc(allocator, cast(u64)file_length);
  if (NULL == tmp.data) {
    rc = Read_Entire_File_Return__ALLOCATION_FAILURE;
    goto close_file;
  }
  tmp.len = file_length;

  isize bytes_read = fread(tmp.data, 1, file_length, file);
  if (bytes_read != file_length) {
    allocator_free(allocator, tmp.data);
    rc = Read_Entire_File_Return__FREAD_FAILURE;
    goto close_file;
  }

  *contents = tmp;

close_file:
  fclose(file);

  return rc;
}

double get_duration_s(struct timespec* start, struct timespec* end) {
  double duration_s = (end->tv_sec - start->tv_sec);
  double duration_ns = cast(double)(end->tv_nsec - start->tv_nsec) / cast(double)1000000000;
  return duration_s + duration_ns;
}

typedef struct String_Hash_Table_Item {
  String key;
  int count;
} String_Hash_Table_Item;

typedef struct Dumb_Hash_Table {
  String_Hash_Table_Item *items;
  usize len;
  usize capacity;
} Dumb_Hash_Table;

void dumb_hash_table_init(Dumb_Hash_Table* ht, usize buckets, Allocator* allocator) {
  da_reserve(ht, buckets, allocator);
  assert(NULL != ht->items && "failed to allocate dumb ht");
  ht->len = 0;
}

void dumb_hash_table_insert(Dumb_Hash_Table* ht, String key) {
  for (u64 i=0; i<ht->capacity; ++i) {
    String_Hash_Table_Item* item = &ht->items[i];
    if (item->count == 0) {
      item->key = key;
      item->count = 1;
      return;
    }
    if (string_compare(item->key, key)) {
      item->count++;
      return;
    }
  }
  assert(0 && "not enough space in dumb ht");
};

typedef struct String_Hash_Table {
  String_Hash_Table_Item* buckets;
  u64 capacity;
} String_Hash_Table;


void string_hash_table_init(String_Hash_Table* table, u32 buckets, Allocator* allocator) {
  table->buckets = allocator_alloc(allocator, sizeof(*table->buckets)*cast(u64)buckets);
  assert(NULL != table->buckets && "failed to allocate table");
  table->capacity = buckets;
}

u32 hash(String key) {
  u32 hash = 5381;
  int c;
  for (u64 i=0; i<key.len; ++i) {
    hash = ((hash << 5) + hash) + cast(u32)key.data[i];
  }
  return hash;
}

void string_hash_table_insert(String_Hash_Table* table, String key) {
  u32 bucket = hash(key) % table->capacity;

  // open addressing, need to walk buckets if not empty
  u32 i = 0;
  for (; i<table->capacity; ++i) {
    u32 b = (bucket+i)%table->capacity;
    String_Hash_Table_Item* item = &table->buckets[b];
    if (item->count == 0) {
      item->key = key;
      item->count = 1;
      return;
    }

    if (string_compare(item->key, key)) {
      item->count++;
      return;
    }
  }
  
  // no buckets left, table completely full
  assert(0 && "table completely full");
}


// void qsort(void *base, size_t nmemb, size_t size,
//       int (*compar)(const void *, const void *));
int shti_greater_than(const void *a, const void *b) {
  String_Hash_Table_Item* _a = cast(String_Hash_Table_Item*)a;
  String_Hash_Table_Item* _b = cast(String_Hash_Table_Item*)b;

  return _a->count < _b->count;
}

int main(int argc, char **argv) {

  Arena arena = {0};
  arena_init(&arena, 10000000);
  Allocator allocator = arena_make_allocator(&arena);

  String contents = {0};
  Read_Entire_File_Return rc = read_entire_file("shakespeare.txt", &contents, &allocator);
  if (Read_Entire_File_Return__SUCCESS != rc) {
    fprintf(stderr, "read_entire_file failed: %d\n", cast(u32)rc);
    return -1;
  }

  u32 buckets = 10000;
  Dumb_Hash_Table dumb_ht = {0};
  dumb_hash_table_init(&dumb_ht, buckets, &allocator);
  String contents_copy = contents;

  String token = {0};


  struct timespec start_dumb = {0};
  struct timespec end_dumb = {0};
  clock_gettime(CLOCK_MONOTONIC, &start_dumb);
  while (true) {
    token = str_chop_by_whitespace(&contents_copy);
    if (0 != token.len) {
      dumb_hash_table_insert(&dumb_ht, token);
    } else {
      break;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end_dumb);
  double dumb_duration_s = get_duration_s(&start_dumb, &end_dumb);

  qsort(dumb_ht.items, dumb_ht.capacity, sizeof(*dumb_ht.items), shti_greater_than);

  String_Hash_Table ht = {0};
  string_hash_table_init(&ht, buckets, &allocator);

  struct timespec start_ht = {0};
  struct timespec end_ht = {0};
  clock_gettime(CLOCK_MONOTONIC, &start_ht);
  while (true) {
    token = str_chop_by_whitespace(&contents);
    if (0 != token.len) {
      string_hash_table_insert(&ht, token);
    } else {
      break;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end_ht);
  double ht_duration_s = get_duration_s(&start_ht, &end_ht);


  for (u64 i=0; i<ht.capacity; ++i) {
    String_Hash_Table_Item* bucket = &ht.buckets[i];
    if (0 == bucket->count) {
      printf("%d: -\n", cast(u32)i);
    } else {
      printf("%d:\tcount: %d\tkey: '%.*s'\n", cast(u32)i, bucket->count, cast(u32)bucket->key.len, bucket->key.data);
    }
  }

  String_Hash_Table ht2;
  string_hash_table_init(&ht2, ht.capacity, &allocator);
  memcpy(ht2.buckets, ht.buckets, sizeof(*ht.buckets)*ht2.capacity);
  qsort(ht2.buckets, ht2.capacity, sizeof(*ht2.buckets), shti_greater_than);

  printf("\n\n\n");
  printf("Hash Table\n");
  for (u64 i=0; i<20; ++i) {
    String_Hash_Table_Item* bucket = &ht2.buckets[i];
    if (0 == bucket->count) {
      printf("%d: -\n", cast(u32)i);
    } else {
      printf("%d:\tcount: %d\tkey: '%.*s'\n", cast(u32)i, bucket->count, cast(u32)bucket->key.len, bucket->key.data);
    }
  }

  printf("\n\n\n");
  printf("Dumb Hash Table\n");
  for (u64 i=0; i<20; ++i) {
    String_Hash_Table_Item* bucket = &dumb_ht.items[i];
    if (0 == bucket->count) {
      printf("%d: -\n", cast(u32)i);
    } else {
      printf("%d:\tcount: %d\tkey: '%.*s'\n", cast(u32)i, bucket->count, cast(u32)bucket->key.len, bucket->key.data);
    }
  }

  printf("\n\n\n");
  printf("Dumb Duration(s): %0.7f\n", dumb_duration_s);
  printf("Ht   Duration(s): %0.7f\n", ht_duration_s);


  return 0;
}
