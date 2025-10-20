# itt_example_c-style.py
from ittapi import compat as itt

print("Starting ITT example...")

# __itt_domain_create("Example.Domain")
domain = itt.domain_create("Example.Domain")

# Prefer ITT C-style string handles if present; otherwise just use strings.
def _handle(name: str):
    create = getattr(itt, "string_handle_create", None)
    return create(name) if callable(create) else name

task1 = _handle("Task 1")
task2 = _handle("Task 2")

itt.task_begin(domain, task1)
for _ in range(10):
    # Do "Task 1" work here
    pass
itt.task_end(domain)

itt.task_begin(domain, task2)
for _ in range(20):
    # Do "Task 2" work here
    pass
itt.task_end(domain)

print("Done.")

