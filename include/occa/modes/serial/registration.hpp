#ifndef OCCA_SERIAL_REGISTRATION_HEADER
#define OCCA_SERIAL_REGISTRATION_HEADER

#include "occa/modes/serial/device.hpp"
#include "occa/modes/serial/kernel.hpp"
#include "occa/modes/serial/memory.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace serial {
    extern occa::mode<serial::device, serial::kernel, serial::memory> mode;
  }
}

#endif
