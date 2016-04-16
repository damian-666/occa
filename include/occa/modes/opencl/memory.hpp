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

#if OCCA_OPENCL_ENABLED
#  ifndef OCCA_OPENCL_MEMORY_HEADER
#  define OCCA_OPENCL_MEMORY_HEADER

#include "occa/memory.hpp"
#include "occa/modes/opencl/headers.hpp"

namespace occa {
  namespace opencl {
    class memory : public occa::memory_v {
      friend class opencl::device;

    private:
      void *mappedPtr;

    public:
      memory(const occa::properties &properties_ = occa::properties());
      ~memory();

      void* getHandle(const occa::properties &properties_);

      void copyTo(const void *dest,
                  const uintptr_t bytes = 0,
                  const uintptr_t destOffset = 0,
                  const bool async = false);

      void copyFrom(const void *src,
                    const uintptr_t bytes = 0,
                    const uintptr_t offset = 0,
                    const bool async = false);

      void copyFrom(const memory_v *src,
                    const uintptr_t bytes = 0,
                    const uintptr_t destOffset = 0,
                    const uintptr_t srcOffset = 0,
                    const bool async = false);

      void free();
    };
  }
}

#  endif
#endif
