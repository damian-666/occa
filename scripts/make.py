import sys, os, imp, re
import numpy as np
from distutils import sysconfig
import os.path as osp

occadir = osp.abspath(osp.join(osp.abspath(osp.dirname(__file__)), '..'))

def compile(argv):
    py_major = str(sys.version_info.major)
    py_minor = str(sys.version_info.minor)
    python_name = 'python' + py_major + '.' + py_minor
    if py_major == '3':
        python_name += 'm'

    py_header_dir = sys.prefix + '/include/' + python_name + '/'

    numpy_header_dir = np.get_include() + '/'

    libpython_dir = sysconfig.get_config_var("LIBDIR") + '/'
    libpython     = python_name

    while occadir[-1] == '/':
        occadir = occadir[:-1]

    cmd_args = ' '.join(argv[1:])

    cmd = ('make'                                    +\
           ' OCCA_COMPILE_PYTHON=1'                  +\
           ' OCCA_LIBPYTHON='     + libpython        +\
           ' OCCA_LIBPYTHON_DIR=' + libpython_dir    +\
           ' OCCA_PYTHON_DIR='    + py_header_dir    +\
           ' OCCA_NUMPY_DIR='     + numpy_header_dir +\
           ' ' + cmd_args                            +\
           ' -f ' + occadir + '/makefile')

    os.system(cmd)

    try:
        imp.find_module('occa')
    except ImportError:
        print("""
    ---[ Note ]-----------------------------
     Remember to:
       export PYTHONPATH=$PYTHONPATH:{}/lib
    ========================================
    """.format(occadir))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(0)

    cmd  = sys.argv[1]
    argv = sys.argv[2:]

    if cmd == 'build': build(argv)