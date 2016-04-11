#if OCCA_CUDA_ENABLED
#  ifndef OCCA_CUDA_REGISTRATION_HEADER
#  define OCCA_CUDA_REGISTRATION_HEADER

#include "occa/modes/cuda/device.hpp"
#include "occa/modes/cuda/kernel.hpp"
#include "occa/modes/cuda/memory.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace cuda {
    extern occa::mode<cuda::device, cuda::kernel, cuda::memory> mode;
  }
}

#  endif
#endif