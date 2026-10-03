#define BTC_IMPLEMENTATION
#include "btc.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

typedef struct CString_Array {
  const char **items;
  usize len;
  usize capacity;
} CString_Array;

typedef CString_Array Cmd;

b8 execute_command_sync(Cmd cmd, Allocator* allocator) {
  pid_t child = fork();
  if (-1 == child) {
    fprintf(stderr, "failed to spawn child process\n");
    return false;
  }

  if (0 == child) {
    Cmd tmp = cmd;
    da_reserve(&tmp, 16, allocator);
    da_append(&tmp, NULL, allocator);

    execvp(cmd.items[0], cast(char * const*)cmd.items);
  }
  else {
    wait(NULL);
  }
  return true;
}

void print_usage(const char *program) {
  fprintf(stderr, "Usage: %s <application>\n", program);
}

int main(int argc, const char **argv)
{
  Arena arena = {0};
  arena_init(&arena, 4096);
  Allocator allocator = arena_make_allocator(&arena);

  const char* program = argv[0];

  if (argc < 2) {
    print_usage(program);
    return -1;
  }

  if (strcmp(argv[1], "do_something") == 0) {
    Cmd cmd = {0};
    da_append(&cmd, "cc", &allocator);
    da_append(&cmd, "-o", &allocator);
    da_append(&cmd, "do_something", &allocator);
    da_append(&cmd, "do_something.c", &allocator);

    execute_command_sync(cmd, &allocator);
  } else if (strcmp(argv[1], "hash_tables") == 0) {
    Cmd cmd = {0};
    da_append(&cmd, "cc", &allocator);
    da_append(&cmd, "-o", &allocator);
    da_append(&cmd, "hash_tables", &allocator);
    da_append(&cmd, "hash_tables.c", &allocator);

    execute_command_sync(cmd, &allocator);
  } else {
    print_usage(program);
    return -1;
  }

  return 0;
}
