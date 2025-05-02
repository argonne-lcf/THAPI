import os

# We suggest to install `pytest-icdiff` to get better diff
import pytest
import yaml

# Please put the corrects paths
stems = [os.environ["THAPI_REF"], os.environ["THAPI_NEW"]]

filenames = []
filenames += ["mpi/tracer_mpi.c", "mpi/mpi_library.rb", "mpi/mpi_babeltrace_model.yaml"]
# filenames += ["mpi/mpi_api.yaml"]

filenames += ["omp/tracer_ompt.c", "omp/omp_library.rb", "omp/omp_babeltrace_model.yaml"]
# filenames += ["omp/ompt_api.yaml"]

filenames += ["hip/tracer_hip.c", "hip/hip_library.rb", "hip/hip_babeltrace_model.yaml"]
# filenames += ["hip/hip_api.yaml"]

filenames += ["cuda/tracer_cuda.c", "cuda/cuda_library.rb", "cuda/cuda_babeltrace_model.yaml"]
# filenames += ["cuda/cuda_api.yaml"]

filenames += ["cuda/tracer_cudart.c", "cuda/cudart_api.yaml"]

filenames += ["ze/tracer_ze.c", "ze/ze_library.rb", "ze/ze_babeltrace_model.yaml"]
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
