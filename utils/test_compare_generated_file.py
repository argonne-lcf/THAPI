import os

# We suggest to install `pytest-icdiff` to get better diff
import pytest
import yaml

# Please put the corrects paths
stems = [os.environ["THAPI_REF"], os.environ["THAPI_NEW"]]

filenames = []
filenames += [
    "mpi/tracer_mpi.c",
    "mpi/mpi_library.rb",
    "mpi/btx_mpi_model.yaml",
    "mpi/mpi_tracepoints.tp",
]
# filenames += ["mpi/mpi_api.yaml"]

filenames += [
    "omp/tracer_ompt.c",
    "omp/omp_library.rb",
    "omp/btx_omp_model.yaml",
    "omp/ompt_tracepoints.tp",
]
# filenames += ["omp/ompt_api.yaml"]

filenames += [
    "hip/tracer_hip.c",
    "hip/hip_library.rb",
    "hip/btx_hip_model.yaml",
    "hip/hip_tracepoints.tp",
]
# filenames += ["hip/hip_api.yaml"]

filenames += [
    "cuda/tracer_cuda.c",
    "cuda/cuda_library.rb",
    "cuda/btx_cuda_model.yaml",
    "cuda/cuda_tracepoints.tp",
]
# filenames += ["cuda/cuda_api.yaml"]

# filenames += ["cuda/tracer_cudart.c", "cuda/cudart_api.yaml"]

filenames += [
    "ze/tracer_ze.c",
    "ze/ze_library.rb",
    "ze/btx_ze_model.yaml",
    "ze/ze_tracepoints.tp",
    "ze/ze_structs_tracepoints.tp",
    "ze/ze_properties.tp"
]

# filenames += [
#    "ze/ze_api.yaml",
#    "ze/zes_api.yaml",
#    "ze/zel_api.yaml",
#    "ze/zex_api.yaml",
# ]


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
