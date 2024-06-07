if STRICT
  WERROR = -Werror
else
  WERROR =
endif

include_HEADERS = thapi-ctl.h

lib_LTLIBRARIES = libthapi-ctl.la

libthapi_ctl_la_SOURCES = startstop.c
libthapi_ctl_la_CFLAGS = -Wall -Wextra $(WERROR)

check_PROGRAMS = test-thapi-ctl
test_thapi_ctl_SOURCES = tests/test-thapi-ctl.c
test_thapi_ctl_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir) -L$(builddir) -Wl,-rpath,$(builddir) -lthapi-ctl -llttng-ctl

TESTS = $(check_PROGRAMS)
