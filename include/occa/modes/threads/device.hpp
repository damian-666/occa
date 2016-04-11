#ifndef OCCA_THREADS_DEVICE_HEADER
#define OCCA_THREADS_DEVICE_HEADER

#include "occa/modes/threads/headers.hpp"
#include "occa/modes/threads/utils.hpp"
#include "occa/device.hpp"

namespace occa {
  namespace threads {
    class device : public occa::device_v {
    public:
      int vendor;
      std::string compiler, compilerFlags, compilerEnvScript;

      int coreCount;

      int threads;
      schedule_t schedule;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      pthread_t tid[50];
#else
      DWORD tid[50];
#endif

      std::queue<job_t> jobs;

      mutex_t jobMutex, kernelMutex;

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

      bool fakesUva();

      //---[ Stream ]-------------------
      stream_t createStream();
      void freeStream(stream_t s);

      streamTag tagStream();
      void waitFor(streamTag tag);
      double timeBetween(const streamTag &startTag, const streamTag &endTag);

      stream_t wrapStream(void *handle_);
      //================================

      //---[ Kernel ]-------------------
      std::string fixBinaryName(const std::string &filename);

      kernel_v* buildKernelFromSource(const std::string &filename,
                                      const std::string &functionName,
                                      const kernelInfo &info_);

      kernel_v* buildKernelFromBinary(const std::string &filename,
                                      const std::string &functionName);
      //================================

      //---[ Memory ]-------------------
      memory_v* wrapMemory(void *handle_,
                           const uintptr_t bytes);

      memory_v* malloc(const uintptr_t bytes,
                       void *src);

      memory_v* mappedAlloc(const uintptr_t bytes,
                            void *src);

      uintptr_t memorySize();
      //================================

      //---[ Custom ]-------------------
      void addJob(job_t &job);
      //================================
    };
  }
}

#endif
