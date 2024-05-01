## ZE API Flow

```mermaid
graph TD

    q([ze_command_queue_desc_t]) 
    ep[[event_profiling]]
    epr[[event_profiling_result]]
    a[[zeCommandListAppend*_entry]]

    q --> zeCommandQueueCreate
    q --> zeCommandListCreateImmediate
    zeCommandListCreate          --> a
    zeCommandListCreateImmediate --> a
    k([ze_kernel_desc_t]) --> zeKernelCreate
    zeKernelCreate --> zeKernelSetGroupSize -. For LaunchKernel Variant .-> a
    zeKernelCreate --> zeKernelGetProperties --> p([ze_kernel_properties_t])
    c([ze_group_count_t]) -. For LaunchKernel Variant .-> a
    a --> ep
    ep --> zeCommandListAppend*_exit
    zeCommandQueueCreate --> zeCommandQueueExecuteCommandLists

    zeCommandListAppend*_exit -- if not Immediate --> zeCommandQueueExecuteCommandLists
    zeCommandListAppend*_exit -- if Immediate--> epr

    zeCommandQueueExecuteCommandLists --> epr
```
