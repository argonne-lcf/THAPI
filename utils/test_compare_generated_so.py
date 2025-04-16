import os
import subprocess

# We suggest to install `pytest-icdiff` to get better diff
import pytest

# Please put the corrects paths
stems = [os.environ["THAPI_REF"], os.environ["THAPI_NEW"]]

filenames = []
filenames += ["ze/.libs/libze_loader.so"]
filenames += ["opencl/.libs/libOpenCL.so"]
filenames += ["hip/.libs/libamdhip64.so"]
filenames += ["cuda/.libs/libcudart.so"]
filenames += ["cuda/.libs/libcuda.so"]
filenames += ["mpi/.libs/libmpi.so"]


def load_so(path):
    # nm output
    #                 U __cxa_atexit
    #000000000003d690 T zeDeviceGet
    # code similar too | awk '$2 ~ /^T|W$/ {print $3}'
    output = subprocess.run(
        ["nm", "-D", path], capture_output=True, text=True, check=True
    ).stdout
    symbols = (line.rsplit(" ", 2) for line in output.splitlines())
    return sorted(name for _address, t, name in symbols if t in ["T","W"] )


all_tuples = [tuple(os.path.join(stem, n) for stem in stems) for n in filenames]


@pytest.mark.parametrize("path_ref,path_new", all_tuples)
def test_symbol(path_ref, path_new):
    ref_ = load_so(path_ref)
    new_ = load_so(path_new)
    assert ref_ == new_
