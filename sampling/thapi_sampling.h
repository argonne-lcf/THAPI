#include <time.h>

typedef void * thapi_sampling_handle_t;

extern int thapi_sampling_init();

extern thapi_sampling_handle_t
thapi_register_sampling(
	void (*pfn)(void),
	struct timespec *interval);

extern void
thapi_unregister_sampling(
	thapi_sampling_handle_t handle);
