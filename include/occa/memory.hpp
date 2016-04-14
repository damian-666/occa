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

#ifndef OCCA_MEMORY_HEADER
#define OCCA_MEMORY_HEADER

#include <iostream>

#include "occa/defines.hpp"

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;

  namespace uvaFlag {
    static const int none         = 0;
    static const int isManaged    = (1 << 0);
    static const int inDevice     = (1 << 1);
    static const int leftInDevice = (1 << 2);
    static const int isDirty      = (1 << 3);
  }

  //---[ memory_v ]---------------------
  class memory_v {
  public:
    int memInfo;
    occa::properties properties;

    void *handle, *uvaPtr;
    occa::device_v *dHandle;

    uintptr_t size;

    memory_v(const occa::properties &properties_);

    void initFrom(const memory_v &m);

    bool isManaged() const;
    bool inDevice() const;
    bool leftInDevice() const;
    bool isDirty() const;

    void* uvaHandle();

    //---[ Virtual Methods ]------------
    virtual ~memory_v() = 0;

    virtual void* getHandle(const std::string &type) = 0;

    virtual void copyTo(const void *src,
                        const uintptr_t bytes = 0,
                        const uintptr_t offset = 0,
                        const bool async = false) = 0;

    virtual void copyFrom(const void *src,
                          const uintptr_t bytes = 0,
                          const uintptr_t offset = 0,
                          const bool async = false) = 0;

    virtual void copyFrom(const memory_v *src,
                          const uintptr_t bytes = 0,
                          const uintptr_t destOffset = 0,
                          const uintptr_t srcOffset = 0,
                          const bool async = false) = 0;

    virtual void free() = 0;
    //==================================

    //---[ Friend Functions ]-----------
    // Let [memcpy] use private info
    friend void memcpy(void *dest, void *src,
                       const uintptr_t bytes,
                       const int flags);

    friend void asyncMemcpy(void *dest, void *src,
                            const uintptr_t bytes,
                            const int flags);

    friend void memcpy(void *dest, void *src,
                       const uintptr_t bytes,
                       const int flags,
                       const bool isAsync);

    friend void startManaging(void *ptr);
    friend void stopManaging(void *ptr);

    friend void syncToDevice(void *ptr, const uintptr_t bytes);
    friend void syncFromDevice(void *ptr, const uintptr_t bytes);

    friend void syncMemToDevice(occa::memory_v *mem,
                                const uintptr_t bytes,
                                const uintptr_t offset);

    friend void syncMemFromDevice(occa::memory_v *mem,
                                  const uintptr_t bytes,
                                  const uintptr_t offset);
  };
  //====================================

  //---[ memory ]-----------------------
  class memory {
    friend class occa::device;
    friend class occa::kernelArg;

  private:
    memory_v *mHandle;

  public:
    memory();
    memory(void *uvaPtr);
    memory(memory_v *mHandle_);

    memory(const memory &m);
    memory& operator = (const memory &m);

    memory& swap(memory &m);

    void checkIfInitialized() const;

    memory_v* getMHandle();
    device_v* getDHandle();

    const std::string& mode();

    uintptr_t bytes() const;

    bool isManaged() const;
    bool inDevice() const;
    bool leftInDevice() const;
    bool isDirty() const;

    void* getHandle(const std::string &type);

    void placeInUva();
    void manage();

    void syncToDevice(const uintptr_t bytes, const uintptr_t offset);
    void syncFromDevice(const uintptr_t bytes, const uintptr_t offset);

    bool uvaIsDirty();
    void uvaMarkDirty();
    void uvaMarkClean();

    void copyFrom(const void *src,
                  const uintptr_t bytes = 0,
                  const uintptr_t offset = 0,
                  const bool async = false);

    void copyFrom(const memory src,
                  const uintptr_t bytes = 0,
                  const uintptr_t destOffset = 0,
                  const uintptr_t srcOffset = 0,
                  const bool async = false);

    void copyTo(void *dest,
                const uintptr_t bytes = 0,
                const uintptr_t offset = 0,
                const bool async = false);

    void copyTo(memory dest,
                const uintptr_t bytes = 0,
                const uintptr_t destOffset = 0,
                const uintptr_t srcOffset = 0,
                const bool async = false);

    void asyncCopyFrom(const void *src,
                       const uintptr_t bytes = 0,
                       const uintptr_t offset = 0);

    void asyncCopyFrom(const memory src,
                       const uintptr_t bytes = 0,
                       const uintptr_t destOffset = 0,
                       const uintptr_t srcOffset = 0);

    void asyncCopyTo(void *dest,
                     const uintptr_t bytes = 0,
                     const uintptr_t offset = 0);

    void asyncCopyTo(memory dest,
                     const uintptr_t bytes = 0,
                     const uintptr_t destOffset = 0,
                     const uintptr_t srcOffset = 0);

    void free();
  };
  //====================================
}

#endif
