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
#  ifndef OCCA_OPENCL_DEVICE_HEADER
#  define OCCA_OPENCL_DEVICE_HEADER

#include "occa/device.hpp"
#include "occa/modes/opencl/headers.hpp"

namespace occa {
  namespace opencl {
    class device : public occa::device_v {
    public:
      int platformID, deviceID;

      cl_platform_id clPlatformID;
      cl_device_id clDeviceID;
      cl_context clContext;

      std::string compilerFlags;

      device(const occa::properties &properties_);
      ~device();
      void free();

      void getEnvironmentVariables();

      void* getHandle(const occa::properties &props);

      void appendAvailableDevices(std::vector<occa::device> &dList);

      void flush();
      void finish();

      bool fakesUva();

      //  |---[ Stream ]----------------
      stream_t createStream();
      void freeStream(stream_t s);

      stream_t wrapStream(void *handle_);

      streamTag tagStream();
      void waitFor(streamTag tag);
      double timeBetween(const streamTag &startTag, const streamTag &endTag);
      //  |=============================

      //  |---[ Kernel ]----------------
      kernel_v* buildKernelFromSource(const std::string &filename,
                                      const std::string &functionName,
                                      const occa::properties &props);

      kernel_v* buildKernelFromBinary(const std::string &filename,
                                      const std::string &functionName);
      //  |=============================

      //  |---[ Memory ]----------------
      memory_v* malloc(const uintptr_t bytes,
                       void *src,
                       const occa::properties &props);

      memory_v* mappedAlloc(const uintptr_t bytes,
                            void *src);

      memory_v* wrapMemory(void *handle_,
                           const uintptr_t bytes);

      uintptr_t memorySize();
      //  |=============================
    };
  }
}

#  endif
#endif
