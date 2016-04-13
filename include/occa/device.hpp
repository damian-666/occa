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

  //---[ argInfoMap ]-------------------
  class argInfoMap {
  public:
    std::map<std::string, std::string> iMap;

    argInfoMap();
    argInfoMap(const std::string &infos);
    argInfoMap(argInfoMap &aim);
    argInfoMap& operator = (const argInfoMap &aim);

    std::string& operator [] (const std::string &info);

    bool has(const std::string &info);
    void remove(const std::string &info);

    template <class TM>
    void set(const std::string &info, const TM &value) {
      iMap[info] = toString(value);
    }

    template <class TM>
    TM get(const std::string &prop) {
      if (has(prop)) {
        std::stringstream ss;
        TM t;
        ss << iMap[prop];
        ss >> t;
        return t;
      }
      return TM();
    }

    template <class TM>
    std::vector<TM> getVector(const std::string &prop) {
      std::vector<TM> ret;
      TM t;
      char c;

      if (has(prop)) {
        std::stringstream ss(iMap[prop].c_str());
        while(ss.peek() != '\0') {
          if (isWhitespace(ss.peek())) {
            ss.get(c);
          }
          else {
            if (ss.peek() == ',')
              ss.get(c);
            if (ss.peek() == '\0') {
              ss >> t;
              ret.push_back(t);
            }
          }
        }
      }
      return ret;
    }

    friend std::ostream& operator << (std::ostream &out, const argInfoMap &m);
  };
  //====================================

  //---[ device_v ]---------------------
  class device_v {
  public:
    argInfoMap properties;

    bool uvaEnabled_;
    ptrRangeMap_t uvaMap;
    memoryVector_t uvaDirtyMemory;

    stream_t currentStream;
    std::vector<stream_t> streams;

    uintptr_t bytesAllocated;

    device_v();
    virtual ~device_v() = 0;

    void initFrom(const device_v &m);

    virtual void setup(argInfoMap &aim) = 0;

    virtual void* getContextHandle() = 0;

    virtual void addOccaHeadersToInfo(kernelInfo &info) = 0;
    virtual std::string getInfoSalt(const kernelInfo &info) = 0;

    virtual void getEnvironmentVariables() = 0;

    virtual void appendAvailableDevices(std::vector<occa::device> &dList) = 0;

    virtual void flush()  = 0;
    virtual void finish() = 0;

    virtual bool fakesUva() = 0;
    bool hasUvaEnabled();

    virtual void waitFor(streamTag tag) = 0;

    virtual stream_t createStream() = 0;
    virtual void freeStream(stream_t s) = 0;
    virtual stream_t wrapStream(void *handle_) = 0;

    virtual streamTag tagStream() = 0;
    virtual double timeBetween(const streamTag &startTag, const streamTag &endTag) = 0;

    virtual std::string fixBinaryName(const std::string &filename) = 0;

    virtual kernel_v* buildKernelFromSource(const std::string &filename,
                                            const std::string &functionName,
                                            const kernelInfo &info_ = defaultKernelInfo) = 0;

    virtual kernel_v* buildKernelFromBinary(const std::string &filename,
                                            const std::string &functionName) = 0;

    virtual memory_v* wrapMemory(void *handle_,
                                 const uintptr_t bytes) = 0;

    virtual memory_v* malloc(const uintptr_t bytes,
                             void* src) = 0;
    virtual memory_v* mappedAlloc(const uintptr_t bytes,
                                  void *src) = 0;

    virtual void free() = 0;

    virtual uintptr_t memorySize() = 0;
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

    device(deviceInfo &dInfo);
    device(const std::string &infos);

    device(const occa::device &d);
    device& operator = (const occa::device &d);

    inline void checkIfInitialized() const {
      OCCA_CHECK(dHandle != NULL,
                 "Device is not initialized");
    }

    void* getContextHandle();
    device_v* getDHandle();

    void setup(deviceInfo &dInfo);
    void setup(const std::string &infos);

    uintptr_t memorySize() const;
    uintptr_t memoryAllocated() const;

    inline bool hasUvaEnabled() {
      checkIfInitialized();

      return dHandle->hasUvaEnabled();
    }

    const std::string& mode();

    template <class TM>
    TM getProperty(const std::string &prop) {
      if (dHandle->properties.has(prop)) {
        std::stringstream ss;
        TM t;
        ss << dHandle->properties[prop];
        ss >> t;
        return t;
      }
      return TM();
    }

    template <class TM>
    inline void setProperty(const std::string &info, const TM &value) {
      dHandle->properties[info] = toString(value);
    }

    void setCompiler(const std::string &compiler_);
    void setCompilerEnvScript(const std::string &compilerEnvScript_);
    void setCompilerFlags(const std::string &compilerFlags_);

    std::string getCompiler();
    std::string getCompilerEnvScript();
    std::string getCompilerFlags();

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
    //  |=============================

    //  |---[ Kernel ]------------------
    occa::kernel buildKernel(const std::string &str,
                             const std::string &functionName,
                             const kernelInfo &info_ = defaultKernelInfo);

    occa::kernel buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const int language = usingOKL);

    occa::kernel buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const kernelInfo &info_ = defaultKernelInfo,
                                       const int language = usingOKL);

    occa::kernel buildKernelFromSource(const std::string &filename,
                                       const std::string &functionName,
                                       const kernelInfo &info_ = defaultKernelInfo);

    occa::kernel buildKernelFromBinary(const std::string &filename,
                                       const std::string &functionName);
    //  |===============================

    occa::memory wrapMemory(void *handle_,
                            const uintptr_t bytes);

    void wrapManagedMemory(void *handle_,
                           const uintptr_t bytes);

    occa::memory malloc(const uintptr_t bytes,
                        void *src = NULL);

    void* managedAlloc(const uintptr_t bytes,
                       void *src = NULL);

    occa::memory mappedAlloc(const uintptr_t bytes,
                             void *src = NULL);

    void* managedMappedAlloc(const uintptr_t bytes,
                             void *src = NULL);


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

    void* getStreamHandle();

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