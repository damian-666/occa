#if OCCA_CUDA_ENABLED

#include "occa/modes/cuda/registration.hpp"

namespace occa {
  namespace cuda {
    occa::mode<cuda::device, cuda::kernel, cuda::memory> mode("CUDA");
  }
}

#endif
