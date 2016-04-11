#if OCCA_OPENMP_ENABLED
#  ifndef OCCA_OPENMP_REGISTRATION_HEADER
#  define OCCA_OPENMP_REGISTRATION_HEADER

#include "occa/modes/openmp/device.hpp"
#include "occa/modes/openmp/kernel.hpp"
#include "occa/modes/serial/memory.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace openmp {
    extern occa::mode<openmp::device, openmp::kernel, serial::memory> mode;
  }
}

#  endif
#endif
