#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, "cc", "main.c", "-Wall", "-Wextra", "-Werror", "-o",
                 "google");

  if (!nob_cmd_run(&cmd))
    return 1;

  return 0;
}
