#include <stdio.h>
#include <ittnotify.h>

int main(void) {
    __itt_domain* domain = __itt_domain_create("Example.Domain");
    __itt_string_handle* task1 = __itt_string_handle_create("Task 1");
    __itt_string_handle* task2 = __itt_string_handle_create("Task 2");

    printf("Starting ITT example...\n");

    __itt_task_begin(domain, __itt_null, __itt_null, task1);
    for (volatile int i = 0; i < 100000000; ++i);
    __itt_task_end(domain);

    __itt_task_begin(domain, __itt_null, __itt_null, task2);
    for (volatile int i = 0; i < 200000000; ++i);
    __itt_task_end(domain);

    printf("Done.\n");
    return 0;
}
