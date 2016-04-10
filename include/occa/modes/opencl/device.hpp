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
      int platform, device;

      cl_platform_id platformID;
      cl_device_id   deviceID;
      cl_context     context;
    };
    //====================================


    template <>
    device_t<OpenCL>::device_t();

    template <>
    device_t<OpenCL>::device_t(const device_t<OpenCL> &k);

    template <>
    device_t<OpenCL>& device_t<OpenCL>::operator = (const device_t<OpenCL> &k);

    template <>
    void* device_t<OpenCL>::getContextHandle();

    template <>
    void device_t<OpenCL>::setup(argInfoMap &aim);

    template <>
    void device_t<OpenCL>::addOccaHeadersToInfo(kernelInfo &info_);

    template <>
    std::string device_t<OpenCL>::getInfoSalt(const kernelInfo &info_);

    template <>
    deviceIdentifier device_t<OpenCL>::getIdentifier() const;

    template <>
    void device_t<OpenCL>::getEnvironmentVariables();

    template <>
    void device_t<OpenCL>::appendAvailableDevices(std::vector<device> &dList);

    template <>
    void device_t<OpenCL>::setCompiler(const std::string &compiler_);

    template <>
    void device_t<OpenCL>::setCompilerEnvScript(const std::string &compilerEnvScript_);

    template <>
    void device_t<OpenCL>::setCompilerFlags(const std::string &compilerFlags_);

    template <>
    void device_t<OpenCL>::flush();

    template <>
    void device_t<OpenCL>::finish();

    template <>
    void device_t<OpenCL>::waitFor(streamTag tag);

    template <>
    stream_t device_t<OpenCL>::createStream();

    template <>
    void device_t<OpenCL>::freeStream(stream_t s);

    template <>
    stream_t device_t<OpenCL>::wrapStream(void *handle_);

    template <>
    streamTag device_t<OpenCL>::tagStream();

    template <>
    double device_t<OpenCL>::timeBetween(const streamTag &startTag, const streamTag &endTag);

    template <>
    std::string device_t<OpenCL>::fixBinaryName(const std::string &filename);

    template <>
    kernel_v* device_t<OpenCL>::buildKernelFromSource(const std::string &filename,
                                                      const std::string &functionName,
                                                      const kernelInfo &info_);

    template <>
    kernel_v* device_t<OpenCL>::buildKernelFromBinary(const std::string &filename,
                                                      const std::string &functionName);

    template <>
    void device_t<OpenCL>::cacheKernelInLibrary(const std::string &filename,
                                                const std::string &functionName,
                                                const kernelInfo &info_);

    template <>
    kernel_v* device_t<OpenCL>::loadKernelFromLibrary(const char *cache,
                                                      const std::string &functionName);

    template <>
    memory_v* device_t<OpenCL>::wrapMemory(void *handle_,
                                           const uintptr_t bytes);

    template <>
    memory_v* device_t<OpenCL>::wrapTexture(void *handle_,
                                            const int dim, const occa::dim &dims,
                                            occa::formatType type, const int permissions);

    template <>
    memory_v* device_t<OpenCL>::malloc(const uintptr_t bytes,
                                       void *src);

    template <>
    memory_v* device_t<OpenCL>::textureAlloc(const int dim, const occa::dim &dims,
                                             void *src,
                                             occa::formatType type, const int permissions);

    template <>
    memory_v* device_t<OpenCL>::mappedAlloc(const uintptr_t bytes,
                                            void *src);

    template <>
    uintptr_t device_t<OpenCL>::memorySize();

    template <>
    void device_t<OpenCL>::free();

    template <>
    int device_t<OpenCL>::simdWidth();
  }
}

#  endif
#endif
