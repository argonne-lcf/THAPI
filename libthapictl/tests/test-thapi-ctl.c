#include <stdio.h>

#include "thapi-ctl.h"


int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  // TODO: is there something we can do here that requires no "backed"?
  // maybe can do omp CPU?
  // for now just makes sure can compile and run without blowing up
  for (int i = 0; i < 10; i++) {
    if (i % 2 == 1)
      thapi_ctl_start();
    printf("loop %d\n", i);
    if (i % 2 == 1)
      thapi_ctl_stop();
  }
}
