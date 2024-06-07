CFLAGS = -Wall -Wextra $(WERROR) -DCL_TARGET_OPENCL_VERSION=220 -DCL_USE_DEPRECATED_OPENCL_1_2_APIS -I$(THAPI_INSTALL_DIR)/include
LDFLAGS = /usr/lib/x86_64-linux-gnu/libOpenCL.so -L$(THAPI_INSTALL_DIR)/lib -lthapi-ctl -llttng-ctl


.PHONY: all
all: hello-cl-startstop

hello-cl-startstop: hello-cl-startstop.c cl_utils.c
	gcc $(CFLAGS) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f hello-cl-startstop
