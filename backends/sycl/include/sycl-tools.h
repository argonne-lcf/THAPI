#pragma once

void init_submit_start(void *state);
void init_parallel_for_start(void *state);
//void init_memcpy_start(void *state);
void init_wait_queue_start(void *state);
void init_wait_event_start(void *state);

//void init_memset_start(void *state);
//void init_fill_start(void *state);
//void init_copy_start(void *state);

void init_submit_end(void *state);
void init_parallel_for_end(void *state);
//void init_memcpy_end(void *state);
void init_wait_queue_end(void *state);
void init_wait_event_end(void *state);

void init_malloc_shared_start(void *state);
void init_malloc_shared_end(void *state);

void init_malloc_device_start(void *state);
void init_malloc_device_end(void *state);

void init_malloc_host_start(void *state);
void init_malloc_host_end(void *state);

void init_free_start(void *state);
void init_free_end(void *state);

void init_memcpy_start(void *state);
void init_memcpy_end(void *state);

void init_copy_start(void *state);
void init_copy_end(void *state);

//void init_wait_queue_start(void *state);
//void init_memset_end(void *state);
//void init_fill_end(void *state);
//void init_copy_end(void *state);

