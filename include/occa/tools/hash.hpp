#ifndef OCCA_TOOLS_HASH_HEADER
#define OCCA_TOOLS_HASH_HEADER

#include <iostream>

namespace occa {
  // Uses FNV hashing
  class hash_t {
  public:
    bool initialized;
    int h[8];

    std::string h_string;
    int sh[8];

    hash_t();
    hash_t(int *h_);
    hash_t(const hash_t &hash);
    hash_t& operator = (const hash_t &hash);

    bool operator == (const hash_t &fo);
    bool operator != (const hash_t &fo);

    hash_t operator ^ (const hash_t hash);
    hash_t& operator ^= (const hash_t hash);

    std::string toString();
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
