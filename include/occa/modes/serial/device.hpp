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

#ifndef OCCA_SERIAL_DEVICE_HEADER
#define OCCA_SERIAL_DEVICE_HEADER

#include "occa/device.hpp"

namespace occa {
  namespace serial {
    class device : public occa::device_v {
    private:
      int vendor;
      std::string compiler, compilerFlags, compilerEnvScript;

    public:
      device();
      device(const device &k);
      device& operator = (const device &k);
      ~device();
      void free();

      void* getContextHandle();

      void setup(argInfoMap &aim);

      void addOccaHeadersToInfo(kernelInfo &info_);

      std::string getInfoSalt(const kernelInfo &info_);

      void getEnvironmentVariables();

      void appendAvailableDevices(std::vector<occa::device> &dList);

      void flush();
      void finish();

      bool fakesUva();

      //  |---[ Stream ]----------------
      stream_t createStream();
      void freeStream(stream_t s);

      streamTag tagStream();
      void waitFor(streamTag tag);
      double timeBetween(const streamTag &startTag, const streamTag &endTag);

      stream_t wrapStream(void *handle_);
      //  |=============================

      //  |---[ Kernel ]----------------
      std::string fixBinaryName(const std::string &filename);

      kernel_v* buildKernelFromSource(const std::string &filename,
                                      const std::string &functionName,
                                      const kernelInfo &info_);

      kernel_v* buildKernelFromBinary(const std::string &filename,
                                      const std::string &functionName);
      //  |=============================

      //  |---[ Kernel ]----------------
      memory_v* wrapMemory(void *handle_,
                           const uintptr_t bytes);

      memory_v* malloc(const uintptr_t bytes,
                       void *src);

      memory_v* mappedAlloc(const uintptr_t bytes,
                            void *src);

      uintptr_t memorySize();
      //  |=============================
    };
  }
}

#endif
