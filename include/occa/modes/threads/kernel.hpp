#ifndef OCCA_PTHREADS_KERNEL_HEADER
#define OCCA_PTHREADS_KERNEL_HEADER

#if (OCCA_OS & (LINUX_OS | OSX_OS))
#  if (OCCA_OS != WINUX_OS)
#    include <sys/sysctl.h>
#  endif
#  include <pthread.h>
#  include <dlfcn.h>
#else
#  include <intrin.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include <queue>

#include "occa/base.hpp"

namespace occa {
  namespace threads {
    class kernel : public occa::kernel_v {
    private:
      int rank, count;

    public:
      kernel();
      kernel(const kernel &k);
      kernel& operator = (const kernel &k);
      ~kernel();

      void* getKernelHandle();
      void* getProgramHandle();

      std::string fixBinaryName(const std::string &filename);

      void buildFromSource(const std::string &filename,
                           const std::string &functionName,
                           const kernelInfo &info_);

      void buildFromBinary(const std::string &filename,
                           const std::string &functionName);

      int maxDims();
      dim maxOuterDims();
      dim maxInnerDims();

      void runFromArguments(const int kArgc, const kernelArg *kArgs);

      void free();
    };
  }
}

#endif
