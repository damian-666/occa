/* The MIT License (MIT)
 * 
 * Copyright (c) 2014 David Medina and Tim Warburton
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

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

    bool haveHash(hash_t &hash, const int depth = 0);
    void waitForHash(hash_t &hash, const int depth = 0);
    void releaseHash(hash_t &hash, const int depth = 0);
    void releaseHashLock(const std::string &lockDir);

    bool fileNeedsParser(const std::string &filename);

    kernelMetadata parseFileForFunction(const std::string &deviceMode,
                                        const std::string &filename,
                                        const std::string &cachedBinary,
                                        const std::string &functionName,
                                        const properties &props);

    std::string removeSlashes(const std::string &str);

    void cacheFile(const std::string &filename,
                   std::string source,
                   hash_t &hash);

    void cacheFile(const std::string &filename,
                   const char *source,
                   hash_t &hash,
                   const bool deleteSource = true);

    void createSourceFileFrom(const std::string &filename,
                              hash_t &hash,
                              const properties &props);

    std::string getLibraryName(const std::string &filename);

    std::string hashFrom(const std::string &filename);

    std::string hashDir(hash_t hash);
    std::string hashDir(const std::string &filename,
                        hash_t hash = hash_t());
  }
}

#endif