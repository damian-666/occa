#if OCCA_OPENCL_ENABLED
#  ifndef OCCA_OPENCL_DEVICE_HEADER
#  define OCCA_OPENCL_DEVICE_HEADER

#include "occa/defines.hpp"
#include "occa/base.hpp"

#if   (OCCA_OS & LINUX_OS)
#  include <CL/cl.h>
#  include <CL/cl_gl.h>
#elif (OCCA_OS & OSX_OS)
#  include <OpenCL/OpenCl.h>
#else
#  include "CL/opencl.h"
#endif

namespace occa {
  namespace opencl {
    //---[ Data Structs ]-----------------
    struct OpenCLKernelData_t {
      int platform, device;

      cl_platform_id platformID;
      cl_device_id   deviceID;
      cl_context     context;
      cl_program     program;
      cl_kernel      kernel;
    };

    struct OpenCLDeviceData_t {
    };
    //====================================

    class device : public occa::device_v {
      int platform, device;

      cl_platform_id platformID;
      cl_device_id   deviceID;
      cl_context     context;

      device();
      device(const device &k);
      device& operator = (const device &k);
      void free();

      void* getContextHandle();

      void setup(argInfoMap &aim);

      void addOccaHeadersToInfo(kernelInfo &info_);

      std::string getInfoSalt(const kernelInfo &info_);

      void getEnvironmentVariables();

      void appendAvailableDevices(std::vector<occa::device> &dList);

      void flush();
      void finish();

      //  |---[ Stream ]----------------
      stream createStream();
      void freeStream(stream s);

      streamTag tagStream();
      void waitFor(streamTag tag);
      double timeBetween(const streamTag &startTag, const streamTag &endTag);

      stream wrapStream(void *handle_);
      //  |=============================

      //  |---[ Kernel ]----------------
      std::string fixBinaryName(const std::string &filename);

      kernel_v* buildKernelFromSource(const std::string &filename,
                                      const std::string &functionName,
                                      const kernelInfo &info_);

      kernel_v* buildKernelFromBinary(const std::string &filename,
                                      const std::string &functionName);
      //  |=============================

      //  |---[ Kernel ]----------------
      memory_v* wrapMemory(void *handle_,
                           const uintptr_t bytes);

      memory_v* malloc(const uintptr_t bytes,
                       void *src);

      memory_v* mappedAlloc(const uintptr_t bytes,
                            void *src);

      uintptr_t memorySize();
      //  |=============================
    };
  }
}

#  endif
#endif
