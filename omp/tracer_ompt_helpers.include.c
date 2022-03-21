#define _OMP_CALLBACKS_WARNING(msg, result) { \
    if (result != ompt_set_always)\
        printf("THAPI Warning: '%s' callback was not set (error code: %d)\n", msg, result);\
}

static void _ompt_finalize(ompt_data_t *tool_data) {
    (void) tool_data;
}

static int _ompt_initialize(ompt_function_lookup_t lookup,
                            int initial_device_num,
                            ompt_data_t *tool_data)
{
    (void) tool_data;
    (void) initial_device_num;

    ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) (uintptr_t) lookup("ompt_set_callback");
    if (ompt_set_callback == NULL)
        return 0;
/*
    _OMP_CALLBACKS_WARNING("ompt_callback_parallel_begin",
                       ompt_set_callback(ompt_callback_parallel_begin, (ompt_callback_t) (uintptr_t) ompt_callback_parallel_begin_func));

    _OMP_CALLBACKS_WARNING("ompt_callback_parallel_end",
                       ompt_set_callback(ompt_callback_parallel_end, (ompt_callback_t) (uintptr_t) ompt_callback_parallel_end_func));

    _OMP_CALLBACKS_WARNING("ompt_callback_task_create",
                       ompt_set_callback(ompt_callback_task_create, (ompt_callback_t) (uintptr_t) ompt_callback_task_create_func));
*/
    _OMP_CALLBACKS_WARNING("ompt_callback_target",
                       ompt_set_callback(ompt_callback_target, (ompt_callback_t) (uintptr_t) ompt_callback_target_func));

    _OMP_CALLBACKS_WARNING("ompt_callback_target_data_op",
                        ompt_set_callback(ompt_callback_target_data_op, (ompt_callback_t) (uintptr_t) ompt_callback_target_data_op_intel_func));
    return 1;
}


static ompt_start_tool_result_t _ompt_start_tool_result = {&_ompt_initialize, &_ompt_finalize, (ompt_data_t) {.ptr=NULL} };

ompt_start_tool_result_t* ompt_start_tool(unsigned int omp_version, const char* runtime_version) {
    (void) omp_version;
    (void) runtime_version;
    return &_ompt_start_tool_result;
}

