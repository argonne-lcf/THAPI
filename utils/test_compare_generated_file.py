import os

# We suggest to install `pytest-icdiff` to get better diff
import pytest
import yaml

# Please put the corrects paths
stems = [os.environ["THAPI_REF"], os.environ["THAPI_NEW"]]

filenames = []
filenames += [
    "backend_mpi/tracer_mpi.c",
    "backend_mpi/mpi_library.rb",
    "backend_mpi/btx_mpi_model.yaml",
    "backend_mpi/mpi_tracepoints.tp",
    "backend_mpi/mpi_type.tp",
]

# filenames += ["mpi/mpi_api.yaml"]

filenames += [
    "backend_omp/tracer_ompt.c",
    "backend_omp/omp_library.rb",
    "backend_omp/btx_omp_model.yaml",
    "backend_omp/ompt_tracepoints.tp",
]
# filenames += ["omp/ompt_api.yaml"]

filenames += [
    "backend_hip/tracer_hip.c",
    "backend_hip/hip_library.rb",
    "backend_hip/btx_hip_model.yaml",
    "backend_hip/hip_tracepoints.tp",
]
# filenames += ["hip/hip_api.yaml"]

filenames += [
    "backend_cuda/tracer_cuda.c",
    "backend_cuda/cuda_library.rb",
    "backend_cuda/btx_cuda_model.yaml",
    "backend_cuda/cuda_tracepoints.tp",
    "backend_cuda/cuda_properties.tp",
]
# filenames += ["cuda/cuda_api.yaml"]

# filenames += ["cuda/tracer_cudart.c", "cuda/cudart_api.yaml"]

filenames += [
    "backend_ze/tracer_ze.c",
    "backend_ze/ze_library.rb",
    "backend_ze/btx_ze_model.yaml",
    "backend_ze/ze_tracepoints.tp",
    "backend_ze/ze_structs_tracepoints.tp",
    "backend_ze/ze_properties.tp",
]

# filenames += [
#    "ze/ze_api.yaml",
#    "ze/zes_api.yaml",
#    "ze/zel_api.yaml",
#    "ze/zex_api.yaml",
# ]

filenames += [
    "backend_opencl/opencl_profiling.tp",
]

filenames += [
    "backend_cxi/cxi_sampling.tp",
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
