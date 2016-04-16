#ifndef OCCA_TOOLS_CRYPTO_HEADER
#define OCCA_TOOLS_CRYPTO_HEADER

#include <iostream>

namespace occa {
  // Uses FNV hashing
  class hash_t {
  public:
    int h[8];

    hash_t();
    hash_t(const hash_t &hash);
    hash_t& operator = (const hash_t &hash);

    bool operator == (const hash_t &fo);
    bool operator != (const hash_t &fo);

    hash_t operator ^ (const hash_t hash);
    hash_t& operator ^= (const hash_t hash);

    operator std::string ();
  };

  hash_t hash(const void *ptr, uintptr_t bytes);

  template <class TM>
  hash_t hash(const TM &t){
    return hash(&t, sizeof(TM));
  }

  hash_t hash(const std::string &str);
  hash_t hashFile(const std::string &filename);
}

#endif
