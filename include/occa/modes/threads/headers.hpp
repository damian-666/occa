#include "occa/defines.hpp"

#if (OCCA_OS & (LINUX_OS | OSX_OS))
#  if (OCCA_OS != WINUX_OS)
#    include <sys/sysctl.h>
#  endif
#  include <pthread.h>
#  include <dlfcn.h>
#else
#  include <windows.h>
#  include <intrin.h>
#endif
