#include "occa/modes/serial/memory.hpp"
#include "occa/device.hpp"

namespace occa {
  namespace serial {
    memory::memory() : occa::memory_v() {}

    memory::memory(const memory &m){
      *this = m;
    }

    memory& memory::operator = (const memory &m){
      initFrom(m);
      return *this;
    }

    memory::~memory(){}

    void* memory::getMemoryHandle(){
      return handle;
    }

    void memory::copyFrom(const void *src,
                          const uintptr_t bytes,
                          const uintptr_t offset,
                          const bool asycn){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      void *destPtr      = ((char*) handle) + offset;
      const void *srcPtr = src;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory::copyFrom(const memory_v *src,
                          const uintptr_t bytes,
                          const uintptr_t destOffset,
                          const uintptr_t srcOffset,
                          const bool asycn){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + destOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + srcOffset) <= src->size,
                 "Source has size [" << src->size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      void *destPtr      = ((char*) handle)      + destOffset;
      const void *srcPtr = ((char*) src->handle) + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory::copyTo(void *dest,
                        const uintptr_t bytes,
                        const uintptr_t offset,
                        const bool asycn){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      void *destPtr      = dest;
      const void *srcPtr = ((char*) handle) + offset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory::copyTo(memory_v *dest,
                        const uintptr_t bytes,
                        const uintptr_t destOffset,
                        const uintptr_t srcOffset,
                        const bool asycn){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + srcOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + destOffset) <= dest->size,
                 "Destination has size [" << dest->size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      void *destPtr      = ((char*) dest->handle) + destOffset;
      const void *srcPtr = ((char*) handle)       + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory::free(){
      sys::free(handle);
      handle = NULL;
      mappedPtr = NULL;

      size = 0;
    }
  }
}
