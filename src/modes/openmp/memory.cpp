#include "occa/defines.hpp"

#if OCCA_OPENMP_ENABLED

#include "occa/Serial.hpp"
#include "occa/OpenMP.hpp"

#include <omp.h>

namespace occa {
  namespace openmp {
    memory_t<OpenMP>::memory_t(){
      strMode = "OpenMP";

      memInfo = memFlag::none;

      handle    = NULL;
      mappedPtr = NULL;
      uvaPtr    = NULL;

      dHandle = NULL;
      size    = 0;

      textureInfo.arg = NULL;
      textureInfo.dim = 1;
      textureInfo.w = textureInfo.h = textureInfo.d = 0;
    }

    memory_t<OpenMP>::memory_t(const memory_t<OpenMP> &m){
      *this = m;
    }

    memory_t<OpenMP>& memory_t<OpenMP>::operator = (const memory_t<OpenMP> &m){
      memInfo = m.memInfo;

      handle    = m.handle;
      mappedPtr = m.mappedPtr;
      uvaPtr    = m.uvaPtr;

      dHandle = m.dHandle;
      size    = m.size;

      textureInfo.arg  = m.textureInfo.arg;
      textureInfo.dim  = m.textureInfo.dim;

      textureInfo.w = m.textureInfo.w;
      textureInfo.h = m.textureInfo.h;
      textureInfo.d = m.textureInfo.d;

      if(isATexture())
        handle = &textureInfo;

      return *this;
    }

    memory_t<OpenMP>::~memory_t(){}

    void* memory_t<OpenMP>::getMemoryHandle(){
      return handle;
    }

    void* memory_t<OpenMP>::getTextureHandle(){
      return textureInfo.arg;
    }

    void memory_t<OpenMP>::copyFrom(const void *src,
                                    const uintptr_t bytes,
                                    const uintptr_t offset){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      void *destPtr      = ((char*) (isATexture() ? textureInfo.arg : handle)) + offset;
      const void *srcPtr = src;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::copyFrom(const memory_v *src,
                                    const uintptr_t bytes,
                                    const uintptr_t destOffset,
                                    const uintptr_t srcOffset){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + destOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + srcOffset) <= src->size,
                 "Source has size [" << src->size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      void *destPtr      = ((char*) (isATexture()      ? textureInfo.arg      : handle))      + destOffset;
      const void *srcPtr = ((char*) (src->isATexture() ? src->textureInfo.arg : src->handle)) + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::copyTo(void *dest,
                                  const uintptr_t bytes,
                                  const uintptr_t offset){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      void *destPtr      = dest;
      const void *srcPtr = ((char*) (isATexture() ? textureInfo.arg : handle)) + offset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::copyTo(memory_v *dest,
                                  const uintptr_t bytes,
                                  const uintptr_t destOffset,
                                  const uintptr_t srcOffset){
      dHandle->finish();

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + srcOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + destOffset) <= dest->size,
                 "Destination has size [" << dest->size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      void *destPtr      = ((char*) (dest->isATexture() ? dest->textureInfo.arg : dest->handle)) + destOffset;
      const void *srcPtr = ((char*) (isATexture() ? textureInfo.arg : handle))       + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::asyncCopyFrom(const void *src,
                                         const uintptr_t bytes,
                                         const uintptr_t offset){
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      void *destPtr      = ((char*) (isATexture() ? textureInfo.arg : handle)) + offset;
      const void *srcPtr = src;


      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::asyncCopyFrom(const memory_v *src,
                                         const uintptr_t bytes,
                                         const uintptr_t destOffset,
                                         const uintptr_t srcOffset){
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + destOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + srcOffset) <= src->size,
                 "Source has size [" << src->size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      void *destPtr      = ((char*) (isATexture()      ? textureInfo.arg      : handle))      + destOffset;
      const void *srcPtr = ((char*) (src->isATexture() ? src->textureInfo.arg : src->handle)) + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::asyncCopyTo(void *dest,
                                       const uintptr_t bytes,
                                       const uintptr_t offset){
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      void *destPtr      = dest;
      const void *srcPtr = ((char*) (isATexture() ? textureInfo.arg : handle)) + offset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::asyncCopyTo(memory_v *dest,
                                       const uintptr_t bytes,
                                       const uintptr_t destOffset,
                                       const uintptr_t srcOffset){
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + srcOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + destOffset) <= dest->size,
                 "Destination has size [" << dest->size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      void *destPtr      = ((char*) (dest->isATexture() ? dest->textureInfo.arg : dest->handle)) + destOffset;
      const void *srcPtr = ((char*) (isATexture() ? textureInfo.arg : handle))       + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    void memory_t<OpenMP>::mappedFree(){
      sys::free(handle);
      handle    = NULL;
      mappedPtr = NULL;

      size = 0;
    }

    void memory_t<OpenMP>::free(){
      if(isATexture()){
        sys::free(textureInfo.arg);
        textureInfo.arg = NULL;
      }
      else{
        sys::free(handle);
        handle = NULL;
      }

      size = 0;
    }
  }
}

#endif
