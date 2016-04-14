#include <sstream>
#include <stdint.h>

#include "occa/tools/crypto.hpp"
#include "occa/tools/env.hpp"

namespace occa {
  fnvOutput_t fnv(const void *ptr, uintptr_t bytes) {
    std::stringstream ss;

    const char *c = (char*) ptr;

    fnvOutput_t fo;
    int *h = fo.h;

    const int p[8] = {102679, 102701,
                      102761, 102763,
                      102769, 102793,
                      102797, 102811};

    for(uintptr_t i = 0; i < bytes; ++i) {
      for(int j = 0; j < 8; ++j)
        h[j] = (h[j] * p[j]) ^ c[i];
    }

    // int h2[8];

    // for(int i = 0; i < 8; ++i)
    //   h2[i] = ((h[0] & (0xFF << (8*i))) << (8*i + 0))
    //     |     ((h[1] & (0xFF << (8*i))) << (8*i + 1))
    //     |     ((h[2] & (0xFF << (8*i))) << (8*i + 2))
    //     |     ((h[3] & (0xFF << (8*i))) << (8*i + 3))
    //     |     ((h[4] & (0xFF << (8*i))) << (8*i + 4))
    //     |     ((h[5] & (0xFF << (8*i))) << (8*i + 5))
    //     |     ((h[6] & (0xFF << (8*i))) << (8*i + 6))
    //     |     ((h[7] & (0xFF << (8*i))) << (8*i + 7));

    return fo;
  }

  template <>
  fnvOutput_t fnv(const std::string &saltedString) {
    return fnv(saltedString.c_str(), saltedString.size());
  }

  std::string getContentHash(const std::string &content,
                             const std::string &salt) {

    std::string fo = fnv(content + salt);

    // Only taking the first 16 characters
    return fo.substr(0, 16);
  }

  std::string getFileContentHash(const std::string &filename,
                                 const std::string &salt) {

    return getContentHash(readFile(filename), salt);
  }

  std::string getLibraryName(const std::string &filename) {
    const std::string cacheLibraryPath = (env::OCCA_CACHE_DIR + "libraries/");

    if (filename.find(cacheLibraryPath) != 0)
      return "";

    const int chars = (int) filename.size();
    const char *c   = filename.c_str();

    int start = (int) cacheLibraryPath.size();
    int end;

    for(end = start; end < chars; ++end) {
      if (c[end] == '/')
        break;
    }

    return filename.substr(start, end - start);
  }

  std::string hashFrom(const std::string &filename) {
    std::string hashDir = hashDirFor(filename, "");

    const int chars = (int) filename.size();
    const char *c   = filename.c_str();

    int start = (int) hashDir.size();
    int end;

    for(end = (start + 1); end < chars; ++end) {
      if (c[end] == '/')
        break;
    }

    return filename.substr(start, end - start);
  }

  std::string hashDirFor(const std::string &filename,
                         const std::string &hash) {

    if (filename.size() == 0) {
      if (hash.size() != 0)
        return (env::OCCA_CACHE_DIR + "kernels/" + hash + "/");
      else
        return (env::OCCA_CACHE_DIR + "kernels/");
    }

    std::string occaLibName = getLibraryName(sys::getFilename(filename));

    if (occaLibName.size() == 0) {
      if (hash.size() != 0)
        return (env::OCCA_CACHE_DIR + "kernels/" + hash + "/");
      else
        return (env::OCCA_CACHE_DIR + "kernels/");
    }

    return (env::OCCA_CACHE_DIR + "libraries/" + occaLibName + "/" + hash + "/");
  }

  fnvOutput_t::fnvOutput_t() {
    h[0] = 101527; h[1] = 101531;
    h[2] = 101533; h[3] = 101537;
    h[4] = 101561; h[5] = 101573;
    h[6] = 101581; h[7] = 101599;
  }

  bool fnvOutput_t::operator == (const fnvOutput_t &fo) {
    for(int i = 0; i < 8; ++i) {
      if (h[i] != fo.h[i])
        return false;
    }

    return true;
  }

  bool fnvOutput_t::operator != (const fnvOutput_t &fo) {
    for(int i = 0; i < 8; ++i) {
      if (h[i] != fo.h[i])
        return true;
    }

    return false;
  }

  void fnvOutput_t::mergeWith(const fnvOutput_t &fo) {
    for(int i = 0; i < 8; ++i)
      h[i] ^= fo.h[i];
  }

  fnvOutput_t::operator std::string () {
    std::stringstream ss;

    for(int i = 0; i < 8; ++i)
      ss << std::hex << h[i];

    return ss.str();
  }
}
