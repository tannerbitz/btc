#include "btc.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

typedef pid_t Proc;

int main(int argc, char **argv) {

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
