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

#if (OCCA_OS & OCCA_LINUX_OS)
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/dir.h>
#elif (OCCA_OS & OCCA_OSX_OS)
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

#include "occa/parser/parser.hpp"
#include "occa/tools/env.hpp"
#include "occa/tools/io.hpp"
#include "occa/tools/string.hpp"
#include "occa/tools/sys.hpp"

namespace occa {
  // Kernel Caching
  namespace kc {
    std::string sourceFile = "source.occa";
    std::string binaryFile = "binary";
  }

  namespace io {
    std::map<std::string, hash_t> fileLocks;

    //---[ File Openers ]---------------
    fileOpener::fileOpener() {}

    std::vector<fileOpener*>& fileOpeners() {
      static std::vector<fileOpener*> fileOpeners_;
      return fileOpeners_;
    }

    fileOpener& getFileOpener(const std::string &filename) {
      std::vector<fileOpener*> &fileOpeners = io::fileOpeners();
      for (size_t i = 0; i < fileOpeners.size(); ++i) {
        if (fileOpeners[i]->handles(filename))
          return *(fileOpeners[i]);
      }
      return defaultFileOpener;
    }

    //  ---[ Default File Opener ]------
    defaultFileOpener_t::defaultFileOpener_t() {}

    bool defaultFileOpener_t::handles(const std::string &filename) {
      return true;
    }

    std::string defaultFileOpener_t::expand(const std::string &filename) {
      return filename;
    }

    defaultFileOpener_t defaultFileOpener;
    //  ================================

    //  ---[ OCCA File Opener ]---------
    occaFileOpener_t::occaFileOpener_t() {}

    bool occaFileOpener_t::handles(const std::string &filename) {
      return ((7 <= filename.size()) &&
              (filename.substr(0, 7) == "occa://"));
    }

    std::string occaFileOpener_t::expand(const std::string &filename) {
      return (env::OCCA_CACHE_DIR + filename.substr(7));
    }
    registerFileOpener<occaFileOpener_t> occaFileOpener;
    //  ================================

    //==================================

    bool isAbsolutePath(const std::string &filename) {
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_OSX_OS))
      return ((0 < filename.size()) &&
              (filename[0] == '/'));
#else
      return ((3 <= filename.size())
              isAlpha(filename[0]) &&
              (filename[1] == ':') &&
              ((filename[2] == '\\') || (filename[2] == '/')));
#endif
    }

    std::string expandEnvVariables(const std::string &filename) {
      const char *c = filename.c_str();
      std::string expFilename = sys::expandEnvVariables(filename);

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_OSX_OS))
      if (*c == '~') {
        expFilename += env::HOME;
        c += 1 + (*c != '\0');
      }
#endif

      return expFilename;
    }

    std::string convertSlashes(const std::string &filename) {
#if (OCCA_OS == OCCA_WINDOWS_OS)
      char slash = '\\';
      if (isAbsolutePath(filename)) {
          slash = filename[2];
      }
      else {
        const char *c = filename.c_str();
        const char *c0 = c;

        while (*c != '\0') {
          if ((*c == '\\') || (*c == '/')) {
            slash = *c;
            break;
          }
          ++c;
        }
      }
      if (slash == '\\')
        return std::replace(filename.begin(), filename.end(), '\\', '/');
#endif
      return filename;
    }

    std::string filename(const std::string &filename, bool makeAbsolute) {
      std::string expFilename = convertSlashes(expandEnvVariables(filename));

      fileOpener &fo = getFileOpener(expFilename);
      expFilename = fo.expand(expFilename);

      if (makeAbsolute && !isAbsolutePath(expFilename))
        expFilename = env::PWD + expFilename;

      return expFilename;
    }

    std::string binaryName(const std::string &filename){
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    std::string dirname(const std::string &filename) {
      std::string expFilename = io::filename(filename);

      const int chars = (int) expFilename.size();
      const char *c   = expFilename.c_str();

      int lastSlash = 0;

      for(int i = 0; i < chars; ++i) {
        if (c[i] == '/')
          lastSlash = i;
      }

      if (lastSlash || (c[0] == '/'))
        ++lastSlash;

      return expFilename.substr(0, lastSlash);
    }

    std::string extension(const std::string &filename) {
      std::string expFilename = io::filename(filename);
      const char *c = expFilename.c_str();
      const char *i = NULL;

      while(*c != '\0') {
        if (*c == '.')
          i = c;
        ++c;
      }

      if (i != NULL)
        return expFilename.substr(i - filename.c_str() + 1);

      return "";
    }

    std::string shortname(const std::string &filename) {
      std::string expFilename = io::filename(filename);

      if (expFilename.find(env::OCCA_CACHE_DIR) != 0)
        return filename;

      const std::string libPath = env::OCCA_CACHE_DIR + "libraries/";
      const std::string kerPath = env::OCCA_CACHE_DIR + "kernels/";

      if (expFilename.find(libPath) == 0) {
        std::string libName = getLibraryName(expFilename);
        std::string theRest = expFilename.substr(libPath.size() + libName.size() + 1);

        return ("occa://" + libName + "/" + theRest);
      }
      else if (expFilename.find(kerPath) == 0) {
        return expFilename.substr(kerPath.size());
      }

      return expFilename;
    }

    // NBN: handle binary mode and EOL chars on Windows
    std::string read(const std::string &filename, const bool readingBinary) {
      std::string expFilename = io::filename(filename);

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

    void write(const std::string &filename, const std::string &content) {
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
      hashMap_t::iterator it = fileLocks.begin();
      while (it != fileLocks.end()) {
        releaseHash(it->second);
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

      fileLocks[lockDir] = hash;

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
      std::string ext = io::extension(filename);

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

      const std::string ext = extension(filename);

      occa::properties properties = props;

      properties["mode"]     = deviceMode;
      properties["language"] = ((ext != "ofl") ? "C" : "Fortran");

      if ((ext == "oak") || (ext == "oaf")) {
        properties["magic"] = "enabled";
      }

      std::string parsedContent = fileParser.parseFile(io::filename(filename),
                                                       properties);

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

      std::string expFilename = io::filename(filename);
      if(!io::haveHash(hash)){
        io::waitForHash(hash);
      } else {
        if (!sys::fileExists(expFilename)) {
          sys::mkpath(dirname(expFilename));

          std::ofstream fs2;
          fs2.open(expFilename.c_str());
          fs2 << source;
          fs2.close();
        }
        io::releaseHash(hash);
      }
      if (deleteSource)
        delete [] source;
    }

    void cacheFile(const std::string &filename,
                   hash_t &hash,
                   const std::string &header,
                   const std::string &footer) {
      std::string expFilename = io::filename(filename);
      const std::string hashDir = io::hashDir(expFilename, hash);
      const std::string sourceFile = hashDir + kc::sourceFile;

      if (sys::fileExists(sourceFile))
        return;

      sys::mkpath(hashDir);

      std::ofstream fs;
      fs.open(sourceFile.c_str());

      fs << header             << '\n'
         << io::read(expFilename) << '\n'
         << footer;

      fs.close();
    }

    std::string getLibraryName(const std::string &filename) {
      std::string expFilename = io::filename(filename);
      const std::string cacheLibraryPath = (env::OCCA_CACHE_DIR + "libraries/");

      if (expFilename.find(cacheLibraryPath) != 0)
        return "";

      const int chars = (int) expFilename.size();
      const char *c   = expFilename.c_str();

      int start = (int) cacheLibraryPath.size();
      int end;

      for (end = start; end < chars; ++end) {
        if (c[end] == '/')
          break;
      }

      return expFilename.substr(start, end - start);
    }

    std::string hashFrom(const std::string &filename) {
      std::string expFilename = io::filename(filename);
      std::string dir = hashDir(expFilename);

      const int chars = (int) expFilename.size();
      const char *c   = expFilename.c_str();

      int start = (int) dir.size();
      int end;

      for (end = (start + 1); end < chars; ++end) {
        if (c[end] == '/')
          break;
      }

      return expFilename.substr(start, end - start);
    }

    std::string hashDir(hash_t hash) {
      return hashDir("", hash);
    }

    std::string hashDir(const std::string &filename, hash_t hash) {
      if (filename.size() == 0) {
        if (hash.initialized)
          return (env::OCCA_CACHE_DIR + "kernels/" + hash.toString() + "/");
        else
          return (env::OCCA_CACHE_DIR + "kernels/");
      }

      std::string occaLibName = getLibraryName(io::filename(filename));

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
