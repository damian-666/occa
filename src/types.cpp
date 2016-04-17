#include "occa/typedefs.hpp"

namespace occa {
  //---[ Dim ]--------------------------
  dim::dim() :
    x(1),
    y(1),
    z(1) {}

  dim::dim(uintptr_t x_) :
    x(x_),
    y(1),
    z(1) {}

  dim::dim(uintptr_t x_, uintptr_t y_) :
    x(x_),
    y(y_),
    z(1) {}

  dim::dim(uintptr_t x_, uintptr_t y_, uintptr_t z_) :
    x(x_),
    y(y_),
    z(z_) {}

  dim::dim(const dim &d) :
    x(d.x),
    y(d.y),
    z(d.z) {}

  dim& dim::operator = (const dim &d) {
    x = d.x;
    y = d.y;
    z = d.z;

    return *this;
  }

  dim dim::operator + (const dim &d) {
    return dim(x + d.x,
               y + d.y,
               z + d.z);
  }

  dim dim::operator - (const dim &d) {
    return dim(x - d.x,
               y - d.y,
               z - d.z);
  }

  dim dim::operator * (const dim &d) {
    return dim(x * d.x,
               y * d.y,
               z * d.z);
  }

  dim dim::operator / (const dim &d) {
    return dim(x / d.x,
               y / d.y,
               z / d.z);
  }

  bool dim::hasNegativeEntries() {
    return ((x & (1 << (sizeof(uintptr_t) - 1))) ||
            (y & (1 << (sizeof(uintptr_t) - 1))) ||
            (z & (1 << (sizeof(uintptr_t) - 1))));
  }

  uintptr_t& dim::operator [] (int i) {
    switch(i) {
    case 0 : return x;
    case 1 : return y;
    default: return z;
    }
  }

  uintptr_t dim::operator [] (int i) const {
    switch(i) {
    case 0 : return x;
    case 1 : return y;
    default: return z;
    }
  }
  //====================================
}
