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

#include "occa/base.hpp"

namespace occa {
  // [--] Add uintptr_t support to occa::kernels

  template <class TM>
  void memset(void *ptr, const TM &value, uintptr_t count){
    OCCA_CHECK(false,
               "memset is only implemented for POD-type data (bool, char, short, int, long, float, double)");
  }

  template <>
  void memset<bool>(void *ptr, const bool &value, uintptr_t count);

  template <>
  void memset<char>(void *ptr, const char &value, uintptr_t count);

  template <>
  void memset<short>(void *ptr, const short &value, uintptr_t count);

  template <>
  void memset<int>(void *ptr, const int &value, uintptr_t count);

  template <>
  void memset<long>(void *ptr, const long &value, uintptr_t count);

  template <>
  void memset<float>(void *ptr, const float &value, uintptr_t count);

  template <>
  void memset<double>(void *ptr, const double &value, uintptr_t count);
}
