#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "build"

int main(int argc, char **argv) {

  NOB_GO_REBUILD_URSELF(argc, argv);

  nob_mkdir_if_not_exists(BUILD_DIR);
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, "cc", "-g", "-Wall", "-Wextra", "-o",
                 BUILD_DIR "/do_something", "do_something.c");
  if (!nob_cmd_run_sync(cmd)) {
    return 1;
  }
  return 0;
}
