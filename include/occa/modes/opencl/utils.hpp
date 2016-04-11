#if OCCA_OPENCL_ENABLED
#  ifndef OCCA_OPENCL_UTILS_HEADER
#  define OCCA_OPENCL_UTILS_HEADER

#include <iostream>

#include "occa/modes/opencl/headers.hpp"
#include "occa/device.hpp"

namespace occa {
  class streamTag;

  namespace opencl {
    struct info_t {
      cl_device_id clDeviceID;
      cl_context   clContext;
      cl_program   clProgram;
      cl_kernel    clKernel;
    };

    namespace info {
      static const int CPU     = (1 << 0);
      static const int GPU     = (1 << 1);
      static const int FPGA    = (1 << 3);
      static const int XeonPhi = (1 << 2);
      static const int anyType = (CPU | GPU | FPGA | XeonPhi);

      static const int Intel     = (1 << 4);
      static const int AMD       = (1 << 5);
      static const int Altera    = (1 << 6);
      static const int NVIDIA    = (1 << 7);
      static const int anyVendor = (Intel | AMD | Altera | NVIDIA);

      static const int any = (anyType | anyVendor);

      std::string deviceType(int type);
      std::string vendor(int type);
    }

    cl_device_type deviceType(int type);

    int getPlatformCount();

    cl_platform_id platformID(int pID);

    int getDeviceCount(int type = info::any);
    int getDeviceCountInPlatform(int pID, int type = info::any);

    cl_device_id deviceID(int pID, int dID, int type = info::any);

    std::string deviceStrInfo(cl_device_id clDID,
                              cl_device_info clInfo);

    std::string deviceName(int pID, int dID);

    int deviceType(int pID, int dID);

    int deviceVendor(int pID, int dID);

    int deviceCoreCount(int pID, int dID);

    uintptr_t getDeviceMemorySize(cl_device_id dID);
    uintptr_t getDeviceMemorySize(int pID, int dID);

    std::string getDeviceListInfo();

    void buildKernelFromSource(info_t &info_,
                               const char *content,
                               const size_t contentBytes,
                               const std::string &functionName,
                               const std::string &flags = "",
                               const std::string &hash = "",
                               const std::string &sourceFile = "");

    void buildKernelFromBinary(info_t &info_,
                               const unsigned char *content,
                               const size_t contentBytes,
                               const std::string &functionName,
                               const std::string &flags = "");

    void saveProgramBinary(info_t &info_,
                           const std::string &binaryFile,
                           const std::string &hash = "");

    occa::device wrapDevice(cl_platform_id platformID,
                            cl_device_id deviceID,
                            cl_context context);

    cl_event& event(streamTag tag);

    std::string error(int e);
  }
}

#  endif
#endif
