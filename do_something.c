#define BTC_IMPLEMENTATION
#include "btc.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

typedef pid_t Proc;

typedef struct Blah {
  int a;
  float b;
  char c;
} Blah;

typedef struct Ints {
  int *items;
  usize len;
  usize capacity;
} Ints;

void print_ints(const Ints *ints) {
  printf("Ints\n");
  printf("\tlen: %zu\n", ints->len);
  printf("\tcap: %zu\n", ints->capacity);
  printf("\titems: [");
  if (ints->len > 0) {
    printf("%d", ints->items[0]);
  }

  for (u64 i = 1; i < ints->len; ++i) {
    printf(", %d", ints->items[i]);
  }
  printf("]\n");
}

int main(int argc, char **argv) {
  UNUSED(argc);
  UNUSED(argv);

  int offset_a = btc_offset_of(Blah, a);
  int offset_b = btc_offset_of(Blah, b);
  int offset_c = btc_offset_of(Blah, c);
  printf("offset a: %d\n", offset_a);
  printf("offset b: %d\n", offset_b);
  printf("offset c: %d\n", offset_c);
  printf("sizeof(void): %zu\n", sizeof(void));

  Arena arena = {0};
  arena_init(&arena, 256);

  Allocator allocator = arena_make_allocator(&arena);
  Blah *blah = allocator_alloc(&allocator, sizeof(Blah));
  *blah = (Blah){.a = 1, .b = 2, .c = 'a'};

  Ints ints = {0};
  da_append(&ints, 2, &allocator);
  print_ints(&ints);
  da_append(&ints, 5, &allocator);
  print_ints(&ints);
  da_append(&ints, 5, &allocator);
  da_append(&ints, 5, &allocator);
  da_append(&ints, 5, &allocator);
  print_ints(&ints);

  Proc child = fork();

  const bool child_creation_failed = (child == -1);
  if (child_creation_failed) {
    perror(strerror(errno));
    exit(1);
  }
  const bool is_child = (child == 0);
  if (is_child) {

    char *const cmd[] = {"ls", "-la", NULL};
    if (execv("/usr/bin/ls", cmd) == -1) {
      perror(strerror(errno));
      exit(1);
    }
  } else {
    int status;
    bool finished = false;
    ;
    while (!finished) {
      Proc child_proc = waitpid(-1, &status, 0);

      if (WIFEXITED(status)) {
        fprintf(stdout,
                "child process: %d exited noramlly with return code %d\n",
                child_proc, WEXITSTATUS(status));
        finished = true;
      }

      if (WIFSIGNALED(status)) {
        fprintf(stdout, "child process: %d was terminated by signal %d\n",
                child_proc, WTERMSIG(status));
        if (WCOREDUMP(status)) {
          fprintf(stdout, "core dump was produced\n");
        }
        finished = true;
      }
    }
  }

  return 0;
}
