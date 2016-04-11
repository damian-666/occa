#include "occa/modes/serial/registration.hpp"

namespace occa {
  namespace serial {
    occa::mode<serial::device, serial::kernel, serial::memory> mode("Serial");
  }
}
