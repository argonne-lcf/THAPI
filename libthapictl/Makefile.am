if STRICT
  WERROR = -Werror
else
  WERROR =
endif

include_HEADERS = \
	include/thapi-ctl.h \
	include/backends.h

lib_LTLIBRARIES = libthapi-ctl.la

libthapi_ctl_la_SOURCES = src/startstop.c src/backends.c
libthapi_ctl_la_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir)/include -I$(builddir) -I$(top_srcdir)/utils/include $(LTTNG_CTL_CFLAGS)
libthapi_ctl_la_LDFLAGS = $(LTTNG_CTL_LIBS)

bin_PROGRAMS = thapi-ctl

thapi_ctl_SOURCES = src/main.c
thapi_ctl_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir)/include -I$(builddir) -I$(top_srcdir)/utils/include $(LTTNG_CTL_CFLAGS)
thapi_ctl_LDFLAGS = -L$(builddir) -lthapi-ctl $(LTTNG_CTL_LIBS)
thapi_ctl_DEPENDENCIES = libthapi-ctl.la

check_PROGRAMS = test-thapi-ctl
test_thapi_ctl_SOURCES = tests/test-thapi-ctl.c
test_thapi_ctl_CFLAGS = -Wall -Wextra $(WERROR) -I$(srcdir)/include $(LTTNG_CTL_CFLAGS)
test_thapi_ctl_LDFLAGS = -L$(builddir) -Wl,-rpath,$(builddir) -lthapi-ctl $(LTTNG_CTL_LIBS)

TESTS = $(check_PROGRAMS)
