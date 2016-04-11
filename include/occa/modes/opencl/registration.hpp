#if OCCA_OPENCL_ENABLED
#  ifndef OCCA_OPENCL_REGISTRATION_HEADER
#  define OCCA_OPENCL_REGISTRATION_HEADER

#include "occa/modes/opencl/device.hpp"
#include "occa/modes/opencl/kernel.hpp"
#include "occa/modes/opencl/memory.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace opencl {
    extern occa::mode<opencl::device, opencl::kernel, opencl::memory> mode;
  }
}

#  endif
#endif
