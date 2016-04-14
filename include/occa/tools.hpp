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

#ifndef OCCA_TOOLS_HEADER
#define OCCA_TOOLS_HEADER

#include "occa/tools/crypto.hpp"
#include "occa/tools/env.hpp"
#include "occa/tools/io.hpp"
#include "occa/tools/properties.hpp"
#include "occa/tools/string.hpp"

namespace occa {
  double currentTime();

  //---[ Dim ]--------------------------
  class dim {
  public:
    uintptr_t x, y, z;

    dim();
    dim(uintptr_t x_);
    dim(uintptr_t x_, uintptr_t y_);
    dim(uintptr_t x_, uintptr_t y_, uintptr_t z_);

    dim(const dim &d);

    dim& operator = (const dim &d);

    dim operator + (const dim &d);
    dim operator - (const dim &d);
    dim operator * (const dim &d);
    dim operator / (const dim &d);

    bool hasNegativeEntries();

    uintptr_t& operator [] (int i);
    uintptr_t  operator [] (int i) const;
  };
  //====================================

  //---[ Misc Functions ]---------------
  int maxBase2Bit(const int value);
  int maxBase2(const int value);

  uintptr_t ptrDiff(void *start, void *end);
  void* ptrOff(void *ptr, uintptr_t offset);
  //====================================

  template <class TM>
  void ignoreResult(const TM &t){
    (void) t;
  }
}

#endif
