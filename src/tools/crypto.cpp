#include <sstream>
#include <stdint.h>

#include "occa/tools/crypto.hpp"
#include "occa/tools/env.hpp"
#include "occa/tools/io.hpp"

namespace occa {
  hash_t::hash_t() {
    h[0] = 101527; h[1] = 101531;
    h[2] = 101533; h[3] = 101537;
    h[4] = 101561; h[5] = 101573;
    h[6] = 101581; h[7] = 101599;
  }

  hash_t::hash_t(const hash_t &hash) {
    *this = hash;
  }

  hash_t& hash_t::operator = (const hash_t &hash) {
    for (int i = 0; i < 8; ++i)
      h[i] = hash[i];
    return *this;
  }

  bool hash_t::operator == (const hash_t &fo) {
    for (int i = 0; i < 8; ++i) {
      if (h[i] != fo.h[i])
        return false;
    }
    return true;
  }

  bool hash_t::operator != (const hash_t &fo) {
    for (int i = 0; i < 8; ++i) {
      if (h[i] != fo.h[i])
        return true;
    }
    return false;
  }

  hash_t hash_t::operator ^ (const hash_t hash) {
    hash_t mix;
    for (int i = 0; i < 8; ++i)
      mix.h[i] = (h[i] ^ hash.h[i])
    return mix;
  }

  hash_t& hash_t::operator ^= (const hash_t hash) {
    *this = (*this ^ hash);
    return *this;
  }

  hash_t::operator std::string () {
    std::stringstream ss;
    for (int i = 0; i < 8; ++i)
      ss << std::hex << h[i];

    std::string s = ss.str();
    return (s.size() < 16) ? s : s.substr(0, 16);
  }

  hash_t hash(const void *ptr, uintptr_t bytes) {
    std::stringstream ss;
    const char *c = (char*) ptr;

    hash_t hash;
    int *h = hash.h;

    const int p[8] = {102679, 102701, 102761, 102763,
                      102769, 102793, 102797, 102811};

    for (uintptr_t i = 0; i < bytes; ++i) {
      for (int j = 0; j < 8; ++j) {
        h[j] = (h[j] * p[j]) ^ c[i];
      }
    }

    return hash;
  }

  hash_t hash(const std::string &str) {
    return hash(str.c_str(), str.size());
  }

  hash_t hashFile(const std::string &filename) {
    return hash(io::read(filename));
  }
}
