#ifndef OCCA_THREADS_REGISTRATION_HEADER
#define OCCA_THREADS_REGISTRATION_HEADER

#include "occa/modes/threads/device.hpp"
#include "occa/modes/threads/kernel.hpp"
#include "occa/modes/serial/memory.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace threads {
    extern occa::mode<threads::device, threads::kernel, serial::memory> mode;
  }
}

#endif
