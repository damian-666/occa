#include "occa/modes/threads/registration.hpp"

namespace occa {
  namespace threads {
    occa::mode<threads::device, threads::kernel, serial::memory> mode("Threads");
  }
}
