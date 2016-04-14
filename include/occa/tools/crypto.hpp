#ifndef OCCA_TOOLS_CRYPTO_HEADER
#define OCCA_TOOLS_CRYPTO_HEADER

#include <iostream>

namespace occa {
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
}

#endif
