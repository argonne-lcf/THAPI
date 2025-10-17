# itt_example_context-manager.py
import time
import ittapi

print("Starting ITT example...")

# Use context managers that emit ITT begin/end around the block.
with ittapi.task("Task 1", domain="Example.Domain"):
    # simulate work (tight loop like the C example)
    for _ in range(100_000_000):
        pass

with ittapi.task("Task 2", domain="Example.Domain"):
    for _ in range(200_000_000):
        pass

print("Done.")
