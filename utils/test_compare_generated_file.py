import pathlib
# We suggest to install `pytest-icdiff` to get better diff
import pytest
import yaml

steams = [
    "/home/applenco/tmp/THAPI_old_parser/build/",
    "/home/applenco/tmp/THAPI_new_parser/build/",
]

filenames = []
# filenames += ["mpi/tracer_mpi.c","mpi/mpi_library.rb"]
filenames += ["mpi/mpi_api.yaml"]

# filenames += ["omp/tracer_ompt.c","omp/omp_library.rb"]
filenames += ["omp/ompt_api.yaml"]

# filenames += ["hip/tracer_hip.c","hip/hip_library.rb"]
filenames += ["hip/hip_api.yaml"]

# filenames += ["cuda/tracer_cuda.c","cuda/cuda_library.rb"]
filenames += ["cuda/cuda_api.yaml"]

# filenames += ["cuda/tracer_cudart.c","cuda/cudart_library.rb"]
filenames += ["cuda/cudart_api.yaml"]

# filenames += ["ze/tracer_ze.c","ze/ze_library.rb"]
filenames += [
    "ze/ze_api.yaml",
    "ze/zes_api.yaml",
    "ze/zel_api.yaml",
    "ze/zex_api.yaml",
]


def load_file(path):
    with open(path, "r") as f:
        if path.endswith("yaml"):
            return yaml.safe_load(f)
        return f.readlines()

@pytest.mark.parametrize(
    "path_ref,path_new", [(steams[0] + n, steams[1] + n) for n in filenames]
)
def test_filter_include(path_ref, path_new):
    ref_ = load_file(path_ref)
    new_ = load_file(path_new)
    assert ref_ == new_
