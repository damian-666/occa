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

#if (OCCA_OS & LINUX_OS)
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/dir.h>
#elif (OCCA_OS & OSX_OS)
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

#include <fstream>
#include <stddef.h>

#include "occa/tools/env.hpp"
#include "occa/tools/io.hpp"

namespace occa {
  // Kernel Caching
  namespace kc {
    std::string sourceFile = "source.occa";
    std::string binaryFile = "binary";
  }

  namespace io {
    strToBoolMap_t fileLocks;

    std::string dirname(const std::string &filename) {
      const int chars = (int) filename.size();
      const char *c   = filename.c_str();

      int lastSlash = 0;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      for(int i = 0; i < chars; ++i)
        if (c[i] == '/')
          lastSlash = i;
#else
      for(int i = 0; i < chars; ++i)
        if ((c[i] == '/') || (c[i] == '\\'))
          lastSlash = i;
#endif

      if (lastSlash || (c[0] == '/'))
        ++lastSlash;

      return filename.substr(0, lastSlash);
    }

    std::string extension(const std::string &filename) {
      const char *c = filename.c_str();
      const char *i = NULL;

      while(*c != '\0') {
        if (*c == '.')
          i = c;

        ++c;
      }

      if (i != NULL)
        return filename.substr(i - filename.c_str() + 1);

      return "";
    }

    std::string shortname(const std::string &filename) {
      if (filename.find(env::OCCA_CACHE_DIR) != 0)
        return filename;

      const std::string libPath = env::OCCA_CACHE_DIR + "libraries/";
      const std::string kerPath = env::OCCA_CACHE_DIR + "kernels/";

      if (filename.find(libPath) == 0) {
        std::string libName = getLibraryName(filename);
        std::string theRest = filename.substr(libPath.size() + libName.size() + 1);

        return ("[" + libName + "]/" + theRest);
      }
      else if (filename.find(kerPath) == 0) {
        return filename.substr(kerPath.size());
      }

      return filename;
    }

    // NBN: handle binary mode and EOL chars on Windows
    std::string read(const std::string &filename, const bool readingBinary) {
      FILE *fp = NULL;

      if (!readingBinary) {
        fp = fopen(filename.c_str(), "r");
      }
      else{
        fp = fopen(filename.c_str(), "rb");
      }

      OCCA_CHECK(fp != 0,
                 "Failed to open [" << io::shortname(filename) << "]");

      struct stat statbuf;
      stat(filename.c_str(), &statbuf);

      const size_t nchars = statbuf.st_size;

      char *buffer = (char*) calloc(nchars + 1, sizeof(char));
      size_t nread = fread(buffer, sizeof(char), nchars, fp);

      fclose(fp);
      buffer[nread] = '\0';

      std::string contents(buffer, nread);

      free(buffer);

      return contents;
    }

    void write(const std::string &filename,
               const std::string &content) {

      sys::mkpath(dirname(filename));

      FILE *fp = fopen(filename.c_str(), "w");

      OCCA_CHECK(fp != 0,
                 "Failed to open [" << io::shortname(filename) << "]");

      fputs(content.c_str(), fp);

      fclose(fp);
    }

    std::string getFileLock(hash_t &hash, const int depth) {
      std::string ret = (env::OCCA_CACHE_DIR + "locks/" + hash.toString());

      ret += '_';
      ret += (char) ('0' + depth);

      return ret;
    }

    void clearLocks() {
      strToBoolMapIterator it = fileLocks.begin();
      while (it != fileLocks.end()) {
        releaseHash(it->first);
        ++it;
      }
      fileLocks.clear();
    }

    bool haveHash(hash_t &hash, const int depth) {
      std::string lockDir = getFileLock(hash, depth);

      sys::mkpath(env::OCCA_CACHE_DIR + "locks/");

      int mkdirStatus = sys::mkdir(lockDir);

      if (mkdirStatus && (errno == EEXIST))
        return false;

      fileLocks[lockDir] = true;

      return true;
    }

    void waitForHash(hash_t &hash, const int depth) {
      struct stat buffer;

      std::string lockDir   = getFileLock(hash, depth);
      const char *c_lockDir = lockDir.c_str();

      while(stat(c_lockDir, &buffer) == 0)
        ; // Do Nothing
    }

    void releaseHash(hash_t &hash, const int depth) {
      releaseHashLock(getFileLock(hash, depth));
    }

    void releaseHashLock(const std::string &lockDir) {
      sys::rmdir(lockDir);
      fileLocks.erase(lockDir);
    }

    bool fileNeedsParser(const std::string &filename) {
      std::string ext = extension(filename);

      return ((ext == "okl") ||
              (ext == "ofl") ||
              (ext == "cl") ||
              (ext == "cu"));
    }

    kernelMetadata parseFileForFunction(const std::string &deviceMode,
                                        const std::string &filename,
                                        const std::string &parsedFile,
                                        const std::string &functionName,
                                        const properties &props) {

      parser fileParser;

      const std::string extension = extension(filename);

      flags_t parserFlags = info.getParserFlags();

      parserFlags["mode"]     = deviceMode;
      parserFlags["language"] = ((extension != "ofl") ? "C" : "Fortran");

      if ((extension == "oak") ||
          (extension == "oaf")) {

        parserFlags["magic"] = "enabled";
      }

      std::string parsedContent = fileParser.parseFile(info.header,
                                                       filename,
                                                       parserFlags);

      if (!sys::fileExists(parsedFile)) {
        sys::mkpath(dirname(parsedFile));

        std::ofstream fs;
        fs.open(parsedFile.c_str());

        fs << parsedContent;

        fs.close();
      }

      kernelInfoIterator kIt = fileParser.kernelInfoMap.find(functionName);

      if (kIt != fileParser.kernelInfoMap.end())
        return (kIt->second)->metadata();

      OCCA_CHECK(false,
                 "Could not find function ["
                 << functionName << "] in file ["
                 << io::shortname(filename) << "]");

      return kernelMetadata();
    }

    std::string removeSlashes(const std::string &str) {
      std::string ret = str;
      const size_t chars = str.size();

      for(size_t i = 0; i < chars; ++i) {
        if (ret[i] == '/')
          ret[i] = '_';
      }

      return ret;
    }

    void cache(const std::string &filename,
               std::string source,
               hash_t &hash) {

      cache(filename, source.c_str(), hash, false);
    }

    void cache(const std::string &filename,
               const char *source,
               hash_t &hash,
               const bool deleteSource) {

      std::string shash = hash.toString();
      if(!haveHash(shash)){
        waitForHash(shash);
      } else {
        if (!sys::fileExists(filename)) {
          sys::mkpath(dirname(filename));

          std::ofstream fs2;
          fs2.open(filename.c_str());
          fs2 << source;
          fs2.close();
        }
        releaseHash(shash);
      }
      if (deleteSource)
        delete [] source;
    }

    void cacheFile(const std::string &filename,
                   hash_t &hash,
                   const std::string &header,
                   const std::string &footer) {

      const std::string hashDir = io::hashDir(filename, hash);
      const std::string sourceFile = hashDir + kc::sourceFile;

      if (sys::fileExists(sourceFile))
        return;

      sys::mkpath(hashDir);

      std::ofstream fs;
      fs.open(sourceFile.c_str());

      fs << header             << '\n'
         << readFile(filename) << '\n'
         << footer;

      fs.close();
    }

    std::string getLibraryName(const std::string &filename) {
      const std::string cacheLibraryPath = (env::OCCA_CACHE_DIR + "libraries/");

      if (filename.find(cacheLibraryPath) != 0)
        return "";

      const int chars = (int) filename.size();
      const char *c   = filename.c_str();

      int start = (int) cacheLibraryPath.size();
      int end;

      for (end = start; end < chars; ++end) {
        if (c[end] == '/')
          break;
      }

      return filename.substr(start, end - start);
    }

    std::string hashFrom(const std::string &filename) {
      std::string dir = hashDir(filename);

      const int chars = (int) filename.size();
      const char *c   = filename.c_str();

      int start = (int) dir.size();
      int end;

      for (end = (start + 1); end < chars; ++end) {
        if (c[end] == '/')
          break;
      }

      return filename.substr(start, end - start);
    }

    std::string hashDir(hash_t hash) {
      return hashDir("", hash);
    }

    std::string hashDir(const std::string &filename,
                        hash_t hash) {

      if (filename.size() == 0) {
        if (hash.initialized)
          return (env::OCCA_CACHE_DIR + "kernels/" + hash.toString() + "/");
        else
          return (env::OCCA_CACHE_DIR + "kernels/");
      }

      std::string occaLibName = getLibraryName(sys::filename(filename));

      if (occaLibName.size() == 0) {
        if (hash.initialized)
          return (env::OCCA_CACHE_DIR + "kernels/" + hash.toString() + "/");
        else
          return (env::OCCA_CACHE_DIR + "kernels/");
      }

      return (env::OCCA_CACHE_DIR + "libraries/" + occaLibName + "/" + hash.toString() + "/");
    }
  }
}
