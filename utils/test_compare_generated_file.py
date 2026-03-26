import os
import pytest
import yaml
import difflib
from deepdiff import DeepDiff

# Please put the corrects paths
stems = [os.environ["THAPI_REF"], os.environ["THAPI_NEW"]]

filenames = []
filenames += [
    "backends/mpi/tracer_mpi.c",
    "backends/mpi/mpi_library.rb",
    "backends/mpi/btx_mpi_model.yaml",
    "backends/mpi/mpi_tracepoints.tp",
    "backends/mpi/mpi_type.tp",
    "backends/mpi/mpi_api.yaml",
]

filenames += [
    "backends/omp/tracer_ompt.c",
    "backends/omp/omp_library.rb",
    "backends/omp/btx_omp_model.yaml",
    "backends/omp/ompt_tracepoints.tp",
    "backends/omp/ompt_api.yaml",
]

filenames += [
    "backends/hip/tracer_hip.c",
    "backends/hip/hip_library.rb",
    "backends/hip/btx_hip_model.yaml",
    "backends/hip/hip_tracepoints.tp",
    "backends/hip/hip_api.yaml",
]

filenames += [
    "backends/cuda/tracer_cuda.c",
    "backends/cuda/cuda_library.rb",
    "backends/cuda/btx_cuda_model.yaml",
    "backends/cuda/cuda_tracepoints.tp",
    "backends/cuda/cuda_properties.tp",
    "backends/cuda/cuda_api.yaml",
    "backends/cuda/tracer_cudart.c",
    "backends/cuda/cudart_api.yaml",
]

filenames += [
    "backends/ze/tracer_ze.c",
    "backends/ze/ze_library.rb",
    "backends/ze/btx_ze_model.yaml",
    "backends/ze/ze_tracepoints.tp",
    "backends/ze/ze_structs_tracepoints.tp",
    "backends/ze/ze_properties.tp",
    "backends/ze/ze_api.yaml",
    "backends/ze/zes_api.yaml",
    "backends/ze/zel_api.yaml",
    "backends/ze/zet_api.yaml",
    "backends/ze/zex_api.yaml",
]

filenames += [
    "backends/itt/tracer_itt.c",
    "backends/itt/itt_library.rb",
    "backends/itt/btx_itt_model.yaml",
    "backends/itt/itt_metadata.tp",
    "backends/itt/itt_tracepoints.tp",
    "backends/itt/itt_api.yaml",
]

filenames += [
    "backends/opencl/opencl_profiling.tp",
]

filenames += [
    "backends/cxi/cxi_sampling.tp",
]

filenames += [
    "sampling/sampling.tp",
]


def format_diff(diff):
    output = []
    for change in diff.get("values_changed", {}).values():
        old = change["old_value"].splitlines()
        new = change["new_value"].splitlines()
        output.append("\n".join(difflib.unified_diff(old, new, lineterm="", n=3)))
    return "\n".join(output)

def load_file(path):
    with open(path, "r") as f:
        if path.endswith("yaml"):
            return yaml.safe_load(f)
        return f.read()


all_tuples = [tuple(os.path.join(stem, n) for stem in stems) for n in filenames]


@pytest.mark.parametrize("path_ref,path_new", all_tuples)
def test_code(path_ref, path_new):
    ref_ = load_file(path_ref)
    new_ = load_file(path_new)
    diff = DeepDiff(ref_, new_, ignore_order=True)
    assert bool(diff), f"Differences found:\n{format_diff(diff)}"
