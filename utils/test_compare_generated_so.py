import subprocess
# We suggest to install `pytest-icdiff` to get better diff
import pytest
import yaml

# Please put the corrects paths
stems = [ os.environ['THAPI_REF'], os.environ['THAPI_NEW'] ]

filenames = []
filenames += ["lib/thapi/ze/lib_zeloader.so"]
filenames += ["lib/thapi/opencl/libOpenCL.so"]
filenames += ["lib/thapi/hip/libamdhip64.so"]
filenames += ["lib/thapi/cudart/libcudart.so"]
filenames += ["lib/thapi/cuda/libcuda.so"]
filenames += ["lib/thapi/mpi/libmpi.so"]


def load_file(path):
    symbols = subprocess.run(['nm', '-g', path], capture_output=True, text=True).stdout
    return [line[3] for line in symbols if line[2] == 'T'].join('\n')

all_tuples = [ tuple( os.path.join(stem,n) for stem in stems ) for n in filenames ]
@pytest.mark.parametrize("path_ref,path_new", all_tuples)
def test_filter_include(path_ref, path_new):
    ref_ = load_file(path_ref)
    new_ = load_file(path_new)
    assert ref_ == new_
