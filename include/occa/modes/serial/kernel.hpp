#ifndef OCCA_SERIAL_KERNEL_HEADER
#define OCCA_SERIAL_KERNEL_HEADER

#include "occa/kernel.hpp"

namespace occa {
  namespace serial {
    class kernel : public occa::kernel_v {
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
#endif
