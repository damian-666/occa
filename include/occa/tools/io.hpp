#ifndef OCCA_TOOLS_IO_HEADER
#define OCCA_TOOLS_IO_HEADER

#include <iostream>
#include "occa/typedefs.hpp"
#include "occa/tools/properties.hpp"
#include "occa/parser/types.hpp"

namespace occa {
  // Kernel Caching
  namespace kc {
    extern std::string sourceFile;
    extern std::string binaryFile;
  }

  namespace io {
    extern strToBoolMap_t fileLocks;

    std::string dirname(const std::string &filename);
    std::string extension(const std::string &filename);

    std::string shortname(const std::string &filename);

    std::string read(const std::string &filename,
                     const bool readingBinary = false);

    void write(const std::string &filename,
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
                                          const properties &props);

    std::string removeSlashes(const std::string &str);

    void cacheFile(const std::string &filename,
                   std::string source,
                   const std::string &hash);

    void cacheFile(const std::string &filename,
                   const char *source,
                   const std::string &hash,
                   const bool deleteSource = true);

    void createSourceFileFrom(const std::string &filename,
                              const std::string &hashDir,
                              const properties &props);

    std::string getLibraryName(const std::string &filename);

    std::string hashFrom(const std::string &filename);
    std::string hashDirFor(const std::string &filename,
                           const std::string &hash);
  }
}

#endif
