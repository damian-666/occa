#if OCCA_OPENMP_ENABLED

#include "occa/modes/openmp/registration.hpp"

namespace occa {
  namespace openmp {
    occa::mode<openmp::device, openmp::kernel, serial::memory> mode("OpenMP");
  }
}

#endif
