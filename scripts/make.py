import sys, os, imp, re
import numpy as np
from distutils import sysconfig
import os.path as osp

occadir = osp.abspath(osp.join(osp.abspath(osp.dirname(__file__)), '..'))

def occapath(*path):
    return osp.join(occadir, *path)

def makefile(argv):
    def ext_files(path, ext):
        return [osp.join(dp, f) for dp, dn, fn in os.walk(occapath(path)) for f in fn if f.endswith(ext)]

    def dirs(files):
        return set(osp.dirname(f) for f in files)

    def generalize(lst):
        return [re.sub(occadir, '$(OCCA_DIR)', l) for l in lst]

    def objpath(obj):
        return re.sub('\$\(OCCA_DIR\)/obj', 'src', obj)

    with open(occapath('scripts', 'libocca-makefile'), 'r') as f:
        contents = f.read()

    cpp_files = ext_files('src', 'cpp')
    f90_files = ext_files('src', 'f90')
    src_dirs  = dirs(cpp_files).union(dirs(f90_files))
    obj_dirs  = [re.sub(occapath('src'), occapath('obj'), src) for src in src_dirs]

    cpp_files = generalize(cpp_files)
    f90_files = generalize(f90_files)
    src_dirs  = generalize(src_dirs)
    obj_dirs  = generalize(obj_dirs)

    cpp_rule = """{obj}/%.o:$(OCCA_DIR)/{path}/%.cpp
\t@mkdir -p {obj}
\t$(compiler) $(compilerFlags) -o $@ $(flags) -c $(paths) $<"""

    f90_rule=""
    """     f90_rule = {obj}/%.o:$(OCCA_DIR)/obj/{path}/%.f90 {obj}
\t@mkdir -p {obj}
\t$(fCompiler) $(fCompilerFlags) $(fModDirFlag) $(lPath) -o $@ $(fFlags) -c $<"""

    cpp_rules = '\n'.join(cpp_rule.format(path=objpath(obj), obj=obj) for obj in obj_dirs)
    f90_rules = '\n'.join(f90_rule.format(path=objpath(obj), obj=obj) for obj in obj_dirs)

    contents = re.sub('{{cpp_rules}}', cpp_rules, contents)
    contents = re.sub('{{f90_rules}}', f90_rules, contents)

    with open(occapath('makefile'), 'w') as f:
        f.write(contents)

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

    if cmd == 'makefile': makefile(argv)
    if cmd == 'build'   : build(argv)