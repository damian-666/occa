#ifndef OCCA_MEMORY_HEADER
#define OCCA_MEMORY_HEADER

#include <iostream>

#include "occa/defines.hpp"

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;
  class kernelArg;

  namespace memFlag {
    static const int none         = 0;
    static const int isManaged    = (1 << 0);
    static const int isMapped     = (1 << 1);
    static const int isAWrapper   = (1 << 2);
  }

  //---[ memory_v ]---------------------
  class memory_v {
  public:
    int memInfo;

    void *handle, *mappedPtr, *uvaPtr;
    occa::device_v *dHandle;

    uintptr_t size;

    memory_v();
    virtual ~memory_v() = 0;

    void initFrom(const memory_v &m);

    bool isManaged() const;
    bool isMapped() const;
    bool isAWrapper() const;

    bool inDevice() const;
    bool leftInDevice() const;
    bool isDirty() const;

    virtual void* getMemoryHandle() = 0;

    virtual void copyFrom(const void *src,
                          const uintptr_t bytes = 0,
                          const uintptr_t offset = 0,
                          const bool async = false) = 0;

    virtual void copyFrom(const memory_v *src,
                          const uintptr_t bytes = 0,
                          const uintptr_t destOffset = 0,
                          const uintptr_t srcOffset = 0,
                          const bool async = false) = 0;

    virtual void copyTo(void *dest,
                        const uintptr_t bytes = 0,
                        const uintptr_t offset = 0,
                        const bool async = false) = 0;

    virtual void copyTo(memory_v *dest,
                        const uintptr_t bytes = 0,
                        const uintptr_t destOffset = 0,
                        const uintptr_t srcOffset = 0,
                        const bool async = false) = 0;

    virtual void free() = 0;

    //---[ Friend Functions ]---------------------

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
    bool isMapped() const;
    bool isAWrapper() const;

    bool inDevice() const;
    bool leftInDevice() const;
    bool isDirty() const;

    void* getMappedPointer();
    void* getMemoryHandle();

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
