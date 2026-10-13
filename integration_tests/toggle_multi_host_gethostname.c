#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Preloaded into an LTTng session daemon so that the traces it writes carry
 * the hostname $THAPI_FAKE_HOSTNAME, to fake a multi-host run on one host.
 * LTTng sees the gethostname() defined below, not the actual libc version. */

int gethostname(char *name, size_t len) {
  const char *hostname = getenv("THAPI_FAKE_HOSTNAME");

  if (!hostname)
    hostname = "localhost";
  strncpy(name, hostname, len);
  if (len)
    name[len - 1] = '\0';
  return 0;
}
