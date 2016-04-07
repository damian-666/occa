#include "occa/Serial.hpp"

namespace occa {
  namespace serial {
    template <>
    memory_t<Serial>::memory_t(){
      strMode = "Serial";

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

    template <>
    memory_t<Serial>::memory_t(const memory_t<Serial> &m){
      *this = m;
    }

    template <>
    memory_t<Serial>& memory_t<Serial>::operator = (const memory_t<Serial> &m){
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

    template <>
    memory_t<Serial>::~memory_t(){}

    template <>
    void* memory_t<Serial>::getMemoryHandle(){
      return handle;
    }

    template <>
    void* memory_t<Serial>::getTextureHandle(){
      return textureInfo.arg;
    }

    template <>
    void memory_t<Serial>::copyFrom(const void *src,
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

    template <>
    void memory_t<Serial>::copyFrom(const memory_v *src,
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

    template <>
    void memory_t<Serial>::copyTo(void *dest,
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

    template <>
    void memory_t<Serial>::copyTo(memory_v *dest,
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
      const void *srcPtr = ((char*) (isATexture()       ? textureInfo.arg       : handle))       + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    template <>
    void memory_t<Serial>::asyncCopyFrom(const void *src,
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

    template <>
    void memory_t<Serial>::asyncCopyFrom(const memory_v *src,
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

      void *destPtr      = ((char*) (isATexture()      ? textureInfo.arg      : handle))         + destOffset;
      const void *srcPtr = ((char*) (src->isATexture() ? src->textureInfo.arg : src->handle)) + srcOffset;;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    template <>
    void memory_t<Serial>::asyncCopyTo(void *dest,
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

    template <>
    void memory_t<Serial>::asyncCopyTo(memory_v *dest,
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
      const void *srcPtr = ((char*) (isATexture()       ? textureInfo.arg       : handle))       + srcOffset;

      ::memcpy(destPtr, srcPtr, bytes_);
    }

    template <>
    void memory_t<Serial>::mappedFree(){
      sys::free(handle);
      handle    = NULL;
      mappedPtr = NULL;

      size = 0;
    }

    template <>
    void memory_t<Serial>::free(){
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
