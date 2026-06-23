/*
 * Helper for the iprof stdout/stderr streaming tests.
 *
 * It uses plain stdio (printf/fprintf) with NO explicit fflush, so the C
 * library decides the buffering policy from whether the fd is a tty:
 *   - tty       -> line-buffered  -> each line is emitted as it is printed
 *   - pipe/file -> block-buffered -> everything is flushed only at exit
 */
#include <stdio.h>
#include <unistd.h>

#define SLEEP_SECONDS 4

int main(void) {
  printf("THAPI_STREAM_FIRST\n");
  fprintf(stderr, "THAPI_STREAM_STDERR\n");
  sleep(SLEEP_SECONDS);
  printf("THAPI_STREAM_SECOND\n");
  return 0;
}
