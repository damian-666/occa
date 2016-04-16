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

#ifndef OCCA_BASE_HEADER
#define OCCA_BASE_HEADER

#include <iostream>
#include <vector>
#include <map>

#include <stdint.h>

#include "occa/defines.hpp"
#include "occa/typedefs.hpp"

#if OCCA_SSE
#  include <xmmintrin.h>
#endif

#include "occa/device.hpp"
#include "occa/kernel.hpp"
#include "occa/memory.hpp"

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;


  //---[ Globals & Flags ]--------------
  extern const int parserVersion;

  extern const int autoDetect;
  extern const int srcInUva, destInUva;

  extern bool uvaEnabledByDefault_f;
  extern bool verboseCompilation_f;

  void setVerboseCompilation(const bool value);
  //====================================


  //---[ Registration ]-----------------
  class mode_v;

  typedef std::map<std::string,mode_v*> strToModeMap_t;
  typedef strToModeMap_t::iterator      strToModeMapIterator;

  strToModeMap_t& modeMap();
  bool modeIsEnabled(const std::string &mode);

  mode_v* getMode(const occa::properties &props);
  mode_v* getMode(const std::string &mode);

  device_v* newModeDevice(const std::string &mode);
  device_v* newModeDevice(const occa::properties &props);

  kernel_v* newModeKernel(const std::string &mode);
  kernel_v* newModeKernel(const occa::properties &props);

  memory_v* newModeMemory(const std::string &mode);
  memory_v* newModeMemory(const occa::properties &props);

  void freeModeDevice(device_v *dHandle);
  void freeModeKernel(kernel_v *kHandle);
  void freeModeMemory(memory_v *mHandle);

  class mode_v {
  protected:
    std::string modeName;

  public:
    std::string& name() {
      return modeName;
    }

    virtual device_v* newDevice() = 0;
    virtual kernel_v* newKernel() = 0;
    virtual memory_v* newMemory() = 0;
  };

  template <class device_t,
            class kernel_t,
            class memory_t>
  class mode : public mode_v {

  public:
    mode(std::string modeName_) {
      modeName = modeName_;
      modeMap()[modeName] = this;
    }

    device_v* newDevice() {
      return new device_t();
    }

    kernel_v* newKernel() {
      return new kernel_t();
    }

    memory_v* newMemory() {
      return new memory_t();
    }
  };
  //====================================


  //---[ Memory ]-----------------------
  void memcpy(void *dest, void *src,
              const uintptr_t bytes,
              const int flags,
              const bool isAsync);

  void memcpy(void *dest, void *src,
              const uintptr_t bytes,
              const int flags = occa::autoDetect);

  void asyncMemcpy(void *dest, void *src,
                   const uintptr_t bytes,
                   const int flags = occa::autoDetect);

  void memcpy(memory dest,
              const void *src,
              const uintptr_t bytes = 0,
              const uintptr_t offset = 0);

  void memcpy(void *dest,
              memory src,
              const uintptr_t bytes = 0,
              const uintptr_t offset = 0);

  void memcpy(memory dest,
              memory src,
              const uintptr_t bytes = 0,
              const uintptr_t destOffset = 0,
              const uintptr_t srcOffset = 0);

  void asyncMemcpy(memory dest,
                   const void *src,
                   const uintptr_t bytes = 0,
                   const uintptr_t offset = 0);

  void asyncMemcpy(void *dest,
                   memory src,
                   const uintptr_t bytes = 0,
                   const uintptr_t offset = 0);

  void asyncMemcpy(memory dest,
                   memory src,
                   const uintptr_t bytes = 0,
                   const uintptr_t destOffset = 0,
                   const uintptr_t srcOffset = 0);


  //---[ Device Functions ]-------------
  extern device currentDevice;
  device getCurrentDevice();

  device host();
  void setDevice(device d);
  void setDevice(const std::string &props);
  void setDevice(const properties &props);

  std::vector<device>& getDeviceList();

  properties& deviceProperties();

  void flush();
  void finish();

  void waitFor(streamTag tag);

  stream createStream();
  stream getStream();
  void setStream(stream s);
  stream wrapStream(void *handle_);

  streamTag tagStream();
  //====================================

  //---[ Kernel Functions ]-------------
  kernel buildKernel(const std::string &str,
                     const std::string &functionName,
                     const properties &props = occa::properties());

  kernel buildKernelFromString(const std::string &content,
                               const std::string &functionName,
                               const int language = usingOKL);

  kernel buildKernelFromString(const std::string &content,
                               const std::string &functionName,
                               const properties &props = occa::properties(),
                               const int language = usingOKL);

  kernel buildKernelFromSource(const std::string &filename,
                               const std::string &functionName,
                               const properties &props = occa::properties());

  kernel buildKernelFromBinary(const std::string &filename,
                               const std::string &functionName);
  //====================================

  //---[ Memory Functions ]-------------
  occa::memory malloc(const uintptr_t bytes,
                      void *src = NULL,
                      const properties &props = occa::properties());

  void* managedAlloc(const uintptr_t bytes,
                     void *src = NULL,
                     const properties &props = occa::properties());
  //====================================

  //---[ Free Functions ]---------------
  void free(device d);
  void free(stream s);
  void free(kernel k);
  void free(memory m);
  //====================================

  void printAvailableDevices();
}

#endif
