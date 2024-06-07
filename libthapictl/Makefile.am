if STRICT
  WERROR = -Werror
else
  WERROR =
endif

include_HEADERS = include/thapi-ctl.h

lib_LTLIBRARIES = libthapi-ctl.la

libthapi_ctl_la_SOURCES = src/startstop.c src/backends.c
libthapi_ctl_la_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir)/include -I$(builddir) -I$(top_srcdir)/utils/include

bin_PROGRAMS = thapi-ctl

thapi_ctl_SOURCES = src/main.c
thapi_ctl_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir)/include -I$(builddir) -I$(top_srcdir)/utils/include -L$(builddir) -lthapi-ctl -llttng-ctl
thapi_ctl_DEPENDENCIES = libthapi-ctl.la

check_PROGRAMS = test-thapi-ctl
test_thapi_ctl_SOURCES = tests/test-thapi-ctl.c
test_thapi_ctl_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir)/include -L$(builddir) -Wl,-rpath,$(builddir) -lthapi-ctl -llttng-ctl

TESTS = $(check_PROGRAMS)
