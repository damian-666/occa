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

#include "occa/modes/opencl/memory.hpp"
#include "occa/modes/opencl/device.hpp"
#include "occa/modes/opencl/utils.hpp"

namespace occa {
  namespace opencl {
    memory::memory(const occa::properties &properties_) :
      occa::kernel_v(properties_) {}

    memory::memory(const memory &m){
      *this = m;
    }

    memory& memory::operator = (const memory &m){
      initFrom(m);
      return *this;
    }

    memory::~memory(){}

    void* memory::getHandle(const occa::properties &properties_){
      return handle;
    }

    void memory::copyFrom(const void *src,
                          const uintptr_t bytes,
                          const uintptr_t offset,
                          const bool async){
      cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy From",
                    clEnqueueWriteBuffer(stream, *((cl_mem*) handle),
                                         async ? CL_FALSE : CL_TRUE,
                                         offset, bytes_, src,
                                         0, NULL, NULL));
    }

    void memory::copyFrom(const memory_v *src,
                          const uintptr_t bytes,
                          const uintptr_t destOffset,
                          const uintptr_t srcOffset,
                          const bool async){
      cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + destOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + srcOffset) <= src->size,
                 "Source has size [" << src->size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy From",
                    clEnqueueCopyBuffer(stream,
                                        *((cl_mem*) src->handle),
                                        *((cl_mem*) handle),
                                        srcOffset, destOffset,
                                        bytes_,
                                        0, NULL, NULL));
    }

    void memory::copyTo(void *dest,
                        const uintptr_t bytes,
                        const uintptr_t offset,
                        const bool async){

      const cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy To",
                    clEnqueueReadBuffer(stream, *((cl_mem*) handle),
                                        async ? CL_FALSE : CL_TRUE,
                                        offset, bytes_, dest,
                                        0, NULL, NULL));
    }

    void memory::copyTo(memory_v *dest,
                        const uintptr_t bytes,
                        const uintptr_t destOffset,
                        const uintptr_t srcOffset,
                        const bool async){

      const cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + srcOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + destOffset) <= dest->size,
                 "Destination has size [" << dest->size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy To",
                    clEnqueueCopyBuffer(stream,
                                        *((cl_mem*) handle),
                                        *((cl_mem*) dest->handle),
                                        srcOffset, destOffset,
                                        bytes_,
                                        0, NULL, NULL));
    }

    void memory::free(){
      if (isMapped()) {
        cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

        OCCA_CL_CHECK("Mapped Free: clEnqueueUnmapMemObject",
                      clEnqueueUnmapMemObject(stream,
                                              *((cl_mem*) handle),
                                              mappedPtr,
                                              0, NULL, NULL));
      }

      // Free mapped-host pointer
      OCCA_CL_CHECK("Mapped Free: clReleaseMemObject",
                    clReleaseMemObject(*((cl_mem*) handle)));
      delete (cl_mem*) handle;

      handle = NULL;
      size   = 0;
    }
  }
}

#endif
