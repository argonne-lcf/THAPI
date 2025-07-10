#pragma once

#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void * thapi_sampling_handle_t;

extern thapi_sampling_handle_t
thapi_register_sampling(
	void (*pfn)(void),
	struct timespec *interval);

extern void
thapi_unregister_sampling(
	thapi_sampling_handle_t handle);

#ifdef __cplusplus
}
#endif

