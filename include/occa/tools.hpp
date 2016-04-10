#ifndef OCCA_TOOLS_HEADER
#define OCCA_TOOLS_HEADER

#include <iostream>

#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include "occa/defines.hpp"
#include "occa/sys.hpp"
#include "occa/parser/types.hpp"

#if   (OCCA_OS & LINUX_OS)
#  include <sys/time.h>
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/dir.h>
#elif (OCCA_OS & OSX_OS)
#  ifdef __clang__
#    include <CoreServices/CoreServices.h>
#    include <mach/mach_time.h>
#  else
#    include <mach/clock.h>
#    include <mach/mach.h>
#  endif
#  include <sys/types.h>
#  include <sys/dir.h>
#else
#  ifndef NOMINMAX
#    define NOMINMAX     // NBN: clear min/max macros
#  endif
#  include <windows.h>
#  include <string>
#  include <direct.h>    // NBN: rmdir _rmdir
#endif

namespace occa {
  class kernelInfo;

  extern strToBoolMap_t fileLocks;

  //---[ Helper Info ]----------------
  namespace env {
    extern bool isInitialized;

    extern std::string HOME, PWD;
    extern std::string PATH, LD_LIBRARY_PATH;

    extern std::string OCCA_DIR, OCCA_CACHE_DIR;
    extern size_t OCCA_MEM_BYTE_ALIGN;
    extern stringVector_t OCCA_INCLUDE_PATH;

    void initialize();
    void initCachePath();
    void initIncludePath();

    void signalExit(int sig);

    void endDirWithSlash(std::string &dir);

    class envInitializer_t {
    public: envInitializer_t();
    };
    extern envInitializer_t envInitializer;
  }

  // Kernel Caching
  namespace kc {
    extern std::string sourceFile;
    extern std::string binaryFile;
  }
  //==================================

  double currentTime();

  //---[ File Functions ]-------------------------
  std::string getOnlyFilename(const std::string &filename);
  std::string getPlainFilename(const std::string &filename);
  std::string getFileDirectory(const std::string &filename);
  std::string getFileExtension(const std::string &filename);

  std::string compressFilename(const std::string &filename);

  std::string readFile(const std::string &filename,
                       const bool readingBinary = false);

  void writeToFile(const std::string &filename,
                   const std::string &content);

  std::string getFileLock(const std::string &filename, const int n);
  void clearLocks();

  bool haveHash(const std::string &hash, const int depth = 0);
  void waitForHash(const std::string &hash, const int depth = 0);
  void releaseHash(const std::string &hash, const int depth = 0);
  void releaseHashLock(const std::string &lockDir);

  bool fileNeedsParser(const std::string &filename);

  parsedKernelInfo parseFileForFunction(const std::string &deviceMode,
                                        const std::string &filename,
                                        const std::string &cachedBinary,
                                        const std::string &functionName,
                                        const kernelInfo &info);

  std::string removeSlashes(const std::string &str);

  void setupOccaHeaders(const kernelInfo &info);

  void cacheFile(const std::string &filename,
                 std::string source,
                 const std::string &hash);

  void cacheFile(const std::string &filename,
                 const char *source,
                 const std::string &hash,
                 const bool deleteSource = true);

  void createSourceFileFrom(const std::string &filename,
                            const std::string &hashDir,
                            const kernelInfo &info);
  //==============================================


  //---[ Hash Functions ]-------------------------
  class fnvOutput_t {
  public:
    int h[8];

    fnvOutput_t();

    bool operator == (const fnvOutput_t &fo);
    bool operator != (const fnvOutput_t &fo);

    void mergeWith(const fnvOutput_t &fo);

    operator std::string ();
  };

  fnvOutput_t fnv(const void *ptr, uintptr_t bytes);

  template <class TM>
  fnvOutput_t fnv(const TM &t){
    return fnv(&t, sizeof(TM));
  }

  template <>
  fnvOutput_t fnv(const std::string &saltedString);

  std::string getContentHash(const std::string &content,
                             const std::string &salt);

  std::string getFileContentHash(const std::string &content,
                                 const std::string &salt);

  std::string getLibraryName(const std::string &filename);

  std::string hashFrom(const std::string &filename);

  std::string hashDirFor(const std::string &filename,
                         const std::string &hash);
  //==============================================


  //---[ String Functions ]-----------------------
  uintptr_t atoi(const char *c);
  uintptr_t atoiBase2(const char *c);
  uintptr_t atoi(const std::string &str);

  double atof(const char *c);
  double atof(const std::string &str);

  double atod(const char *c);
  double atod(const std::string &str);

  std::string stringifyBytes(uintptr_t bytes);
  //==============================================


  //---[ Misc Functions ]-------------------------
  int maxBase2Bit(const int value);
  int maxBase2(const int value);

  uintptr_t ptrDiff(void *start, void *end);
  void* ptrOff(void *ptr, uintptr_t offset);
  //==============================================


  //---[ Helper Classes ]-------------------------
  class dim {
  public:
    uintptr_t x, y, z;

    dim();
    dim(uintptr_t x_);
    dim(uintptr_t x_, uintptr_t y_);
    dim(uintptr_t x_, uintptr_t y_, uintptr_t z_);

    dim(const dim &d);

    dim& operator = (const dim &d);

    dim operator + (const dim &d);
    dim operator - (const dim &d);
    dim operator * (const dim &d);
    dim operator / (const dim &d);

    bool hasNegativeEntries();

    uintptr_t& operator [] (int i);
    uintptr_t  operator [] (int i) const;
  };
  //==============================================

  template <class TM>
  void ignoreResult(const TM &t){
    (void) t;
  }
}

#endif
