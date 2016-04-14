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

#ifndef OCCA_DEVICE_HEADER
#define OCCA_DEVICE_HEADER

#include <iostream>
#include <sstream>

#include "occa/defines.hpp"

#include "occa/parser/tools.hpp"
#include "occa/uva.hpp"
#include "occa/kernel.hpp"

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;
  class deviceInfo;

  typedef void* stream_t;
  class stream;
  class streamTag;

  //---[ device_v ]---------------------
  class device_v {
  public:
    std::string mode;
    occa::properties properties;

    bool uvaEnabled_;
    ptrRangeMap_t uvaMap;
    memoryVector_t uvaDirtyMemory;

    stream_t currentStream;
    std::vector<stream_t> streams;

    uintptr_t bytesAllocated;

    device_v(const occa::properties &properties_);

    void initFrom(const device_v &m);
    bool hasUvaEnabled();

    //---[ Virtual Methods ]------------
    virtual ~device_v() = 0;

    virtual void* getHandle(const std::string &type) = 0;

    virtual void appendAvailableDevices(std::vector<occa::device> &dList) = 0;

    virtual void flush()  = 0;
    virtual void finish() = 0;

    virtual bool fakesUva() = 0;

    //  |---[ Stream ]------------------
    virtual stream_t createStream() = 0;
    virtual void freeStream(stream_t s) = 0;

    virtual stream_t wrapStream(void *handle_) = 0;

    virtual streamTag tagStream() = 0;
    virtual void waitFor(streamTag tag) = 0;
    virtual double timeBetween(const streamTag &startTag, const streamTag &endTag) = 0;
    //  |===============================

    //  |---[ Kernel ]------------------
    virtual kernel_v* buildKernelFromSource(const std::string &filename,
                                            const std::string &functionName,
                                            const properties &props = defaultProperties) = 0;

    virtual kernel_v* buildKernelFromBinary(const std::string &filename,
                                            const std::string &functionName) = 0;
    //  |===============================

    //  |---[ Memory ]------------------
    virtual memory_v* malloc(const uintptr_t bytes,
                             void* src,
                             const properties &props) = 0;

    virtual memory_v* wrapMemory(void *handle_,
                                 const uintptr_t bytes) = 0;
    //  |===============================

    virtual void free() = 0;

    virtual uintptr_t memorySize() = 0;
    //==================================
  };
  //====================================

  //---[ device ]-----------------------
  class device {
    friend class occa::kernel_v;
    friend class occa::memory_v;
    friend class occa::device_v;

    friend class occa::memory;

  private:
    device_v *dHandle;

  public:
    device();
    device(device_v *dHandle_);

    device(const properties &props);
    device(const std::string &props);

    device(const occa::device &d);
    device& operator = (const occa::device &d);

    void checkIfInitialized();

    const std::string& mode();
    const occa::properties& properties();

    void* getHandle(const std::string &type);
    device_v* getDHandle();

    void setup(properties &props);
    void setup(const std::string &props);

    uintptr_t memorySize() const;
    uintptr_t memoryAllocated() const;

    bool hasUvaEnabled();

    void flush();
    void finish();

    //  |---[ Stream ]------------------
    stream createStream();
    void freeStream(stream s);

    stream getStream();
    void setStream(stream s);
    stream wrapStream(void *handle_);

    streamTag tagStream();
    void waitFor(streamTag tag);
    double timeBetween(const streamTag &startTag, const streamTag &endTag);
    //  |===============================

    //  |---[ Kernel ]------------------
    occa::kernel buildKernel(const std::string &str,
                             const std::string &functionName,
                             const properties &props = defaultProperties);

    occa::kernel buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const int language = usingOKL);

    occa::kernel buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const properties &props = defaultProperties,
                                       const int language = usingOKL);

    occa::kernel buildKernelFromSource(const std::string &filename,
                                       const std::string &functionName,
                                       const properties &props = defaultProperties);

    occa::kernel buildKernelFromBinary(const std::string &filename,
                                       const std::string &functionName);
    //  |===============================

    //  |---[ Memory ]------------------
    occa::memory malloc(const uintptr_t bytes,
                        void *src = NULL,
                        const properties &props = defaultProperties);

    void* managedAlloc(const uintptr_t bytes,
                       void *src = NULL,
                        const properties &props = defaultProperties);
    //  |===============================

    void free();
  };
  //====================================

  //---[ stream ]-----------------------
  class stream {
  public:
    device_v *dHandle;
    stream_t handle;

    stream();
    stream(device_v *dHandle_, stream_t handle_);
    stream(const stream &s);
    stream& operator = (const stream &s);

    void* getHandle(const std::string &type);

    void free();
  };

  /*
   * CUDA   : handle = CUevent*
   * OpenCL : handle = cl_event*
   */
  class streamTag {
  public:
    double tagTime;
    void *handle;
  };
  //====================================
}
#endif
