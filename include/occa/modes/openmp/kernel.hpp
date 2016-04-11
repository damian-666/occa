#include "occa/defines.hpp"

#if OCCA_OPENMP_ENABLED
#  ifndef OCCA_OPENMP_KERNEL_HEADER
#  define OCCA_OPENMP_KERNEL_HEADER

#include "occa/modes/serial/kernel.hpp"

namespace occa {
  namespace openmp {
    class kernel : public serial::kernel {
    private:
      void *dlHandle;
      handleFunction_t handle;

      void *vArgs[2*OCCA_MAX_ARGS];

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

#  endif
#endif
