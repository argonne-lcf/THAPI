#pragma once

#include "time.h"
void thapi_register_sampling(void (*pfn_run)(void), struct timespec *interval,
                             void (*pfn_final)(void));
