#pragma once

extern volatile int thapi_sampling_finished;

int thapi_sampling_init();
void *thapi_sampling_loop(void *args);
