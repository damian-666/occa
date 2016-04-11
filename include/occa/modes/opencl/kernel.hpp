#if OCCA_OPENCL_ENABLED
#  ifndef OCCA_OPENCL_KERNEL_HEADER
#  define OCCA_OPENCL_KERNEL_HEADER

#include "occa/kernel.hpp"
#include "occa/modes/opencl/headers.hpp"
#include "occa/modes/opencl/utils.hpp"

namespace occa {
  namespace opencl {
    class device;

    class kernel : public occa::kernel_v {
      friend class device;

    private:
      int platformID, deviceID;

      cl_platform_id clPlatformID;
      cl_device_id   clDeviceID;
      cl_context     clContext;
      cl_program     clProgram;
      cl_kernel      clKernel;

    public:
      kernel();
      kernel(const kernel &k);
      kernel& operator = (const kernel &k);
      ~kernel();

      info_t makeCLInfo();

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
