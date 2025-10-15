import os

# We suggest to install `pytest-icdiff` to get better diff
import pytest
import yaml
import re

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
]

# filenames += ["cuda/tracer_cudart.c", "cuda/cudart_api.yaml"]

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
    "backends/ze/zex_api.yaml",
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


def load_file(path):
    with open(path, "r") as f:
        if path.endswith("yaml"):
            return yaml.safe_load(f)
        return f.readlines()


all_tuples = [tuple(os.path.join(stem, n) for stem in stems) for n in filenames]


@pytest.mark.parametrize("path_ref,path_new", all_tuples)
def test_code(path_ref, path_new):
    ref_ = load_file(path_ref)
    new_ = load_file(path_new)
    assert ref_ == new_
