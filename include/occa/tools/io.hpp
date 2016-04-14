#ifndef OCCA_TOOLS_IO_HEADER
#define OCCA_TOOLS_IO_HEADER

#include <iostream>

namespace occa {
  extern strToBoolMap_t fileLocks;

  // Kernel Caching
  namespace kc {
    extern std::string sourceFile;
    extern std::string binaryFile;
  }

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
}

#endif
