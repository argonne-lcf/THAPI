#include <time.h>

extern int thapi_sampling_init();

extern void thapi_register_sampling(void (*pfn)(void), struct timespec *interval);
