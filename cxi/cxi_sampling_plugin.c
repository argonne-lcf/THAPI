/*
 * THAPI - CXI backend plugin wrapper
 *
 * This plugin integrates CXI hardware counters and basic process
 * resource-usage metrics into the THAPI sampling framework.
 *
 * When the environment variable LTTNG_UST_CXI_SAMPLING_CXI is set, it will:
 *
 *   1. Load a list of CXI counters (built-in defaults or from a file
 *      pointed to by LTTNG_UST_CXI_SAMPLING_CXI_COUNTERS_FILE).
 *   2. Initialize base paths for CXI telemetry and "RH" (resource handler) counters,
 *      overridable via LTTNG_UST_CXI_SAMPLING_RH_BASE and
 *      LTTNG_UST_CXI_SAMPLING_CXI_BASE.
 *   3. Enumerate all CXI devices under /sys/class/cxi and open one
 *      read-only file descriptor per counter per device.
 *   4. On each sample callback (default period 100 ms, or
 *      LTTNG_UST_CXI_SAMPLING_CXI_PERIOD_MS), read each counter plus
 *      process context-switch statistics (ru_nvcsw, ru_nivcsw).
 *   5. Emit an LTTng-UST tracepoint (lttng_ust_cxi_sampling) for each
 *      value collected, tagged by interface and counter name.
 *
 * The plugin automatically registers itself in
 * thapi_initialize_sampling_plugin() and cleans up (unregister &
 * close descriptors) in thapi_finalize_sampling_plugin().
 *
 * Detailed context:
 *   - CXI counters are on-chip telemetry metrics exposed under
 *     /sys/class/cxi/<iface>/device/telemetry.
 *   - RH counters come from the NIC's resource-handler interface (e.g.
 *     files under /run/cxi/<iface>/...).
 *   - PROC counters capture software-level process metrics
 *     (context-switch counts) via getrusage().
 *
 *   This separation lets the plugin correlate NIC-level hardware
 *   telemetry with host CPU context switches for end-to-end
 *   performance tracing.
 */

#include "cxi_sampling.h"
#include "thapi_sampling.h"

#include "cxi_default_counters.h"
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*                0.  ENV vars & defaults                             */
/* ------------------------------------------------------------------ */
#define COUNTER_FILE_ENV         "LTTNG_UST_CXI_SAMPLING_CXI_COUNTERS_FILE"
#define COUNTER_PERIOD_ENV       "LTTNG_UST_CXI_SAMPLING_CXI_PERIOD_MS"
#define CXI_ENV_SWITCH           "LTTNG_UST_CXI_SAMPLING_CXI"

#define CXI_RH_PATH_ENV          "LTTNG_UST_CXI_SAMPLING_RH_BASE"
#define CXI_TELEMETRY_PATH_ENV   "LTTNG_UST_CXI_SAMPLING_CXI_BASE"

static const char *const DEFAULT_RH_BASE        = "/run/cxi";
static const char *const DEFAULT_TELEMETRY_BASE = "/sys/class/cxi";

static const char *rh_base  = NULL;
static const char *cxi_base = NULL;

/* ------------------------------------------------------------------ */
/*           helper to load base‐path overrides                       */
/* ------------------------------------------------------------------ */
static void load_base_paths(void) {
    const char *p;

    p = getenv(CXI_RH_PATH_ENV);
    rh_base = (p && *p) ? p : DEFAULT_RH_BASE;

    p = getenv(CXI_TELEMETRY_PATH_ENV);
    cxi_base = (p && *p) ? p : DEFAULT_TELEMETRY_BASE;
}

/* ------------------------------------------------------------------ */
/*           1.  Counter list (built-in or user provided)             */
/* ------------------------------------------------------------------ */

static const char *const *counter_names = cxi_default_counters;
static size_t             n_counters    = 0;

static void count_default(void) {
  for (n_counters = 0; cxi_default_counters[n_counters]; ++n_counters);
}

static void load_counter_list(void) {
  const char *file = getenv(COUNTER_FILE_ENV);
  if (!file || !*file) {
    return count_default();                       /* keep defaults */
  }

  FILE *fp = fopen(file, "r");
  if (!fp) {
    return count_default();
  }

  char  **list  = NULL;
  size_t  used  = 0, cap = 0;
  char   *line  = NULL;
  size_t  lcap  = 0;

  while (getline(&line, &lcap, fp) > 0) {
    char *p = line;
    while (*p && isspace((unsigned char)*p)) ++p; /* ltrim          */
    if (*p == '#' || *p == '\n' || *p == '\0') {
        continue;
    }

    char *e = p + strcspn(p, "\r\n");
    *e = '\0';

    if (used == cap) {
        cap  = cap ? cap * 2 : 32;
        list = realloc(list, cap * sizeof(*list));
        if (!list) {
            used = 0;
            break;
        }
    }
    list[used++] = strdup(p);
  }
  free(line);
  fclose(fp);

  if (!used) {                                         /* fallback   */
    return count_default();
  }

  list = realloc(list, (used + 1) * sizeof(*list));
  list[used] = NULL;

  counter_names = (const char *const *)list;
  n_counters    = used;
}

/* ------------------------------------------------------------------ */
/*                  2.  FD table (opened once)                        */
/* ------------------------------------------------------------------ */

struct fd_entry {
  const char *counter;          /* name string (points into list) */
  char        interface_name[16];       /* cxi0 / proc                    */
  int         fd;               /* open() result                  */
  enum { C_CXI, C_RH, C_PROC } kind;
};

static struct fd_entry *fds      = NULL;
static size_t            n_fds   = 0;

static int add_fd(const char *path,
                  const char *interface_name,
                  const char *counter,
                  int kind) {
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
      return -1;
  }

  struct fd_entry *tmp = realloc(fds, (n_fds + 1)*sizeof(*fds));
  if (!tmp) {
    close(fd);
    return -1;
  }
  fds = tmp;

  fds[n_fds++] = (struct fd_entry){
      .counter = counter,
      .fd      = fd,
      .kind    = kind,
  };
  snprintf(fds[n_fds-1].interface_name, sizeof fds[n_fds-1].interface_name, "%s", interface_name);

  return 0;
}

static void open_all_fds(void) {
  /* enumerate CXI devices once */
  DIR *d = opendir(cxi_base);
  if (!d) {
    return;
  }

  struct dirent *de;
  char path[PATH_MAX];

  while ((de = readdir(d))) {
    if (de->d_type != DT_LNK || de->d_name[0] == '.') {
      continue;
    }

    const char *interface_name = de->d_name;

    for (size_t i = 0; i < n_counters; ++i) {
      const char *cn = counter_names[i];

      if (!strncmp(cn, "proc:", 5)) {
        continue;                               /* later       */
      }

      if (!strncmp(cn, "rh:", 3)) {             /* RH counter  */
        snprintf(path, sizeof(path),
                 "%s/%s/%s",
                 rh_base, interface_name, cn + 3);
        add_fd(path, interface_name, cn, C_RH);
      } else {                                  /* CXI counter */
        snprintf(path, sizeof(path),
                 "%s/%s/device/telemetry/%s",
                 cxi_base, interface_name, cn);
        add_fd(path, interface_name, cn, C_CXI);
      }
    }
  }
  closedir(d);

  /* PROC counters - single per process */
  for (size_t i = 0; i < n_counters; ++i) {
    if (!strncmp(counter_names[i], "proc:", 5)) {
      add_fd("/dev/null", "proc", counter_names[i], C_PROC); /* dummy fd */
    }
  }
}

static void close_all_fds(void) {
  for (size_t i = 0; i < n_fds; ++i) {
    if (fds[i].kind != C_PROC) {
      close(fds[i].fd);
    }
  }
  free(fds);
  fds   = NULL;
  n_fds = 0;
}

/* ------------------------------------------------------------------ */
/*                3.  Fast sampling function                          */
/* ------------------------------------------------------------------ */

static int read_u64_pread(int fd, uint64_t *val) {
  char buf[64];
  ssize_t n = pread(fd, buf, sizeof(buf)-1, 0);
  if (n <= 0) {
    return -1;
  }
  buf[n] = '\0';

  if (buf[0] == '@') {
    return -1;         /* shouldn't happen */
  }

  /* CXI counters have “value@sec.nsec” - cut at '@' if present */
  char *at = strchr(buf, '@');
  if (at) {
    *at = '\0';
  }

  errno = 0;
  unsigned long long tmp = strtoull(buf, NULL, 0);
  if (errno) {
    return -1;
  }
  *val = (uint64_t)tmp;
  return 0;
}

void thapi_sampling_cxi(void) {
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);      /* PROC values once per loop */

  for (size_t i = 0; i < n_fds; ++i) {
    struct fd_entry *e = &fds[i];
    uint64_t v = 0;

    if (e->kind == C_PROC) {      /* chearper open (no sysfs) */
      if (!strcmp(e->counter, "proc:ivcsw")) {
          v = ru.ru_nivcsw;
      } else if (!strcmp(e->counter, "proc:vcsw")) {
          v = ru.ru_nvcsw;
      } else {
        continue;
      }
    } else if (read_u64_pread(e->fd, &v) < 0) {
      continue;                 /* silent drop on error */
    }

    do_tracepoint(lttng_ust_cxi_sampling,
                  cxi, e->interface_name, e->counter, v);
  }
}

/* ------------------------------------------------------------------ */
/*            4.  Init  /  cleanup helpers                            */
/* ------------------------------------------------------------------ */

static void *plugin_handle = NULL;

void thapi_initialize_sampling_plugin(void) {
    /* master switch */
    if (!getenv(CXI_ENV_SWITCH)) {
        return;
    }

    /* one‑time set‑up of /sys/class/cxi/… file descriptors */
    /* set up counters + paths + fds */
    load_counter_list();
    load_base_paths();       /* <-- initialize rh_base & cxi_base */
    open_all_fds();

    /* default 100 ms period unless the user overrides */
    struct timespec period = {.tv_sec = 0, .tv_nsec = 100 * 1000000L};
    const char *s = getenv(COUNTER_PERIOD_ENV);
    if (s) {
        char *end;
        long v = strtol(s, &end, 10);
        if (!*end && v > 0) {
            period.tv_sec  = v / 1000;
            period.tv_nsec = (v % 1000) * 1000000L;
        }
    }
    plugin_handle = thapi_register_sampling(&thapi_sampling_cxi, &period);
}

void thapi_finalize_sampling_plugin(void) {
    if (plugin_handle) {
        thapi_unregister_sampling(plugin_handle);
    }
    close_all_fds();
}

