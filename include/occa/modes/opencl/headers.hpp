#include "occa/defines.hpp"

#if   (OCCA_OS & LINUX_OS)
#  include <CL/cl.h>
#  include <CL/cl_gl.h>
#elif (OCCA_OS & OSX_OS)
#  include <OpenCL/OpenCl.h>
#else
#  include "CL/opencl.h"
#endif
