# itt_example_context-manager.py
import time
import ittapi

print("Starting ITT example...")

# Use context managers that emit ITT begin/end around the block.
with ittapi.task("Task 1", domain="Example.Domain"):
    for _ in range(10):
        # Do "Task 1" work here
        pass

with ittapi.task("Task 2", domain="Example.Domain"):
    for _ in range(20):
        # Do "Task 2" work here
        pass

print("Done.")

