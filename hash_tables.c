#define BTC_IMPLEMENTATION
#include "btc.h"

#include <stdio.h>

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

typedef struct String_Hash_Table_Item {
  String key;
  int count;
} String_Hash_Table_Item;

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

int main(int argc, char **argv) {

  Arena arena = {0};
  arena_init(&arena, 100000);
  Allocator allocator = arena_make_allocator(&arena);

  String contents = {0};
  Read_Entire_File_Return rc = read_entire_file("hash_tables.c", &contents, &allocator);
  if (Read_Entire_File_Return__SUCCESS != rc) {
    fprintf(stderr, "read_entire_file failed: %d\n", cast(u32)rc);
    return -1;
  }


  String_Hash_Table ht = {0};
  u32 buckets = 300;
  string_hash_table_init(&ht, buckets, &allocator);

  String token = {0};
  while (true) {
    token = str_chop_by_whitespace(&contents);
    if (0 != token.len) {
      string_hash_table_insert(&ht, token);
    } else {
      break;
    }
  }

  for (u64 i=0; i<ht.capacity; ++i) {
    String_Hash_Table_Item* bucket = &ht.buckets[i];
    if (0 == bucket->count) {
      printf("%d: -\n", cast(u32)i);
    } else {
      printf("%d:\tcount: %d\tkey: '%.*s'\n", cast(u32)i, bucket->count, cast(u32)bucket->key.len, bucket->key.data);
    }
  }




  return 0;
}
