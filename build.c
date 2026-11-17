#define BTC_IMPLEMENTATION
#include "btc.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct CString_Array {
  const char **items;
  usize len;
  usize capacity;
} CString_Array;

typedef CString_Array Cmd;

b8 execute_command_sync(Cmd cmd, Allocator *allocator) {
  pid_t child = fork();
  if (-1 == child) {
    fprintf(stderr, "failed to spawn child process\n");
    return false;
  }

  if (0 == child) {
    Cmd tmp = cmd;
    da_reserve(&tmp, 16, allocator);
    da_append(&tmp, NULL, allocator);

    execvp(cmd.items[0], cast(char *const *) cmd.items);
  } else {
    wait(NULL);
  }
  return true;
}

int main(int argc, const char **argv) {
  Arena arena = {0};
  arena_init(&arena, 4096);
  Allocator allocator = arena_make_allocator(&arena);

  const char *program = argv[0];

  Cmd cmd = {0};
  da_append(&cmd, "cc", &allocator);
  da_append(&cmd, "-o", &allocator);
  da_append(&cmd, "do_something", &allocator);
  da_append(&cmd, "do_something.c", &allocator);
  execute_command_sync(cmd, &allocator);

  cmd = cast(Cmd){0};
  da_append(&cmd, "cc", &allocator);
  da_append(&cmd, "-g", &allocator);
  da_append(&cmd, "-o", &allocator);
  da_append(&cmd, "hash_tables", &allocator);
  da_append(&cmd, "hash_tables.c", &allocator);
  execute_command_sync(cmd, &allocator);

  cmd = cast(Cmd){0};
  da_append(&cmd, "cc", &allocator);
  da_append(&cmd, "-g", &allocator);
  da_append(&cmd, "-o", &allocator);
  da_append(&cmd, "json_parser", &allocator);
  da_append(&cmd, "json_parser.c", &allocator);
  execute_command_sync(cmd, &allocator);

  return 0;
}
