#ifndef OCCA_CPU_HEADER
#define OCCA_CPU_HEADER

#include <iostream>

#include "occa/defines.hpp"
#include "occa/typedefs.hpp"

namespace occa {
  typedef void (*handleFunction_t)(const int *occaKernelInfoArgs,
                                   int occaInnerId0,
                                   int occaInnerId1,
                                   int occaInnerId2, ...);

  namespace flags {
    extern const int checkCacheDir;
  }

  namespace env {
    std::string var(const std::string &var);
  }

  namespace sys {
    namespace vendor {
      static const int notFound     = 0;

      static const int b_GNU          = 0;
      static const int b_LLVM         = 1;
      static const int b_Intel        = 2;
      static const int b_Pathscale    = 3;
      static const int b_IBM          = 4;
      static const int b_PGI          = 5;
      static const int b_HP           = 6;
      static const int b_VisualStudio = 7;
      static const int b_Cray         = 8;
      static const int b_max          = 9;

      static const int GNU          = (1 << b_GNU);          // gcc    , g++
      static const int LLVM         = (1 << b_LLVM);         // clang  , clang++
      static const int Intel        = (1 << b_Intel);        // icc    , icpc
      static const int Pathscale    = (1 << b_Pathscale);    // pathCC
      static const int IBM          = (1 << b_IBM);          // xlc    , xlc++
      static const int PGI          = (1 << b_PGI);          // pgcc   , pgc++
      static const int HP           = (1 << b_HP);           // aCC
      static const int VisualStudio = (1 << b_VisualStudio); // cl.exe
      static const int Cray         = (1 << b_Cray);         // cc     , CC
    }

    //---[ System Calls ]---------------
    int call(const std::string &cmdline);
    int call(const std::string &cmdline, std::string &output);

    std::string expandEnvVariables(const std::string &str);

    void rmdir(const std::string &dir);
    int mkdir(const std::string &dir);
    void mkpath(const std::string &dir);

    bool dirExists(const std::string &dir_);
    bool fileExists(const std::string &filename_,
                    const int flags = 0);

    std::string getFilename(const std::string &filename);

    void absolutePathVec(const std::string &path_,
                         stringVector_t &pathVec);

    stringVector_t absolutePathVec(const std::string &path);

    //---[ Processor Info ]-------------
    std::string getFieldFrom(const std::string &command,
                             const std::string &field);

    std::string getProcessorName();
    int getCoreCount();
    int getProcessorFrequency();
    std::string getProcessorCacheSize(int level);
    uintptr_t installedRAM();
    uintptr_t availableRAM();

    std::string getDeviceListInfo();

    int compilerVendor(const std::string &compiler);

    std::string compilerSharedBinaryFlags(const std::string &compiler);
    std::string compilerSharedBinaryFlags(const int vendor_);

    void addSharedBinaryFlagsTo(const std::string &compiler, std::string &flags);
    void addSharedBinaryFlagsTo(const int vendor_, std::string &flags);

    //---[ Dynamic Methods ]------------
    void* malloc(uintptr_t bytes);
    void free(void *ptr);

    void* dlopen(const std::string &filename,
                 const std::string &hash = "");

    handleFunction_t dlsym(void *dlHandle,
                           const std::string &functionName,
                           const std::string &hash = "");

    void dlclose(void *dlHandle);

    void runFunction(handleFunction_t f,
                     const int *occaKernelInfoArgs,
                     int occaInnerId0, int occaInnerId1, int occaInnerId2,
                     int argc, void **args);
  }

  class mutex_t {
  public:
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    pthread_mutex_t mutexHandle;
#else
    HANDLE mutexHandle;
#endif

    mutex_t();
    void free();

    void lock();
    void unlock();
  };
}

#endif
