#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thapi-ctl.h"


void print_usage(const char* argv0) {
  fprintf(stderr, "Usage: %s init|start|stop\n", argv0);
}


int main(int argc, char **argv) {
  if (argc != 2) {
    print_usage(argv[0]);
    exit(1);
  }

  const char* cmd = argv[1];
  printf("cmd '%s'\n", cmd);
  fflush(stdout);
  if (strcmp(cmd, "init") == 0) {
    thapi_ctl_init(); // enable bookkeeping
  } else if (strcmp(cmd, "start") == 0) {
    thapi_ctl_start();
  } else if (strcmp(cmd, "stop") == 0) {
    thapi_ctl_stop();
  } else if (strcmp(cmd, "list") == 0) {
    thapi_ctl_print_events();
  } else {
    print_usage(argv[0]);
    fprintf(stderr, "Error: unknown command '%s'\n", cmd);
    return 1;
  }
  return 0;
}
