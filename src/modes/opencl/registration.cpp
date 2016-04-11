#if OCCA_OPENCL_ENABLED

#include "occa/modes/opencl/registration.hpp"

namespace occa {
  namespace opencl {
    occa::mode<opencl::device, opencl::kernel, opencl::memory> mode("OpenCL");
  }
}

#endif
