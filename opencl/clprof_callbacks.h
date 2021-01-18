#include "babeltrace_cl.h"
#include "babeltrace_cl_callbacks.h"
#ifndef CPP_H
#define CPP_H
#ifdef __cplusplus
extern "C" {
#endif
extern void
init_callbacks(struct opencl_dispatch *opencl_dispatch);
extern void
finalize_callbacks();
extern const bt_value *display_mode;
#ifdef __cplusplus
}
#endif
#endif
