#if OCCA_CUDA_ENABLED

#include "occa/CUDA.hpp"

namespace occa {
  namespace cuda {
    memory::memory(){
      strMode = "CUDA";

      memInfo = memFlag::none;

      handle    = NULL;
      mappedPtr = NULL;
      uvaPtr    = NULL;

      dHandle = NULL;
      size    = 0;

      textureInfo.arg = NULL;

      textureInfo.dim = 1;

      textureInfo.w  = textureInfo.h = textureInfo.d = 0;
    }

    memory::memory(const memory &m){
      *this = m;
    }

    memory& memory::operator = (const memory &m){
      memInfo = m.memInfo;

      handle    = m.handle;
      mappedPtr = m.mappedPtr;
      uvaPtr    = m.uvaPtr;

      dHandle = m.dHandle;
      size    = m.size;

      textureInfo.arg = m.textureInfo.arg;

      textureInfo.dim = m.textureInfo.dim;

      textureInfo.w = m.textureInfo.w;
      textureInfo.h = m.textureInfo.h;
      textureInfo.d = m.textureInfo.d;

      return *this;
    }

    memory::~memory(){}

    void* memory::getMemoryHandle(){
      return handle;
    }

    void* memory::getTextureHandle(){
      return (void*) ((CUDATextureData_t*) handle)->array;
    }

    void memory::copyFrom(const void *src,
                                  const uintptr_t bytes,
                                  const uintptr_t offset){
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      if(!isATexture())
        OCCA_CUDA_CHECK("Memory: Copy From",
                        cuMemcpyHtoD(*((CUdeviceptr*) handle) + offset, src, bytes_) );
      else{
        if(textureInfo.dim == 1)
          OCCA_CUDA_CHECK("Texture Memory: Copy From",
                          cuMemcpyHtoA(((CUDATextureData_t*) handle)->array, offset, src, bytes_) );
        else{
          CUDA_MEMCPY2D info;

          info.srcXInBytes   = 0;
          info.srcY          = 0;
          info.srcMemoryType = CU_MEMORYTYPE_HOST;
          info.srcHost       = src;
          info.srcPitch      = 0;

          info.dstXInBytes   = offset;
          info.dstY          = 0;
          info.dstMemoryType = CU_MEMORYTYPE_ARRAY;
          info.dstArray      = ((CUDATextureData_t*) handle)->array;

          info.WidthInBytes = textureInfo.w * textureInfo.bytesInEntry;
          info.Height       = (bytes_ / info.WidthInBytes);

          cuMemcpy2D(&info);

          dHandle->finish();
        }
      }
    }

    void memory::copyFrom(const memory_v *src,
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

      void *dstPtr, *srcPtr;

      if(!isATexture())
        dstPtr = handle;
      else
        dstPtr = (void*) ((CUDATextureData_t*) handle)->array;

      if( !(src->isATexture()) )
        srcPtr = src->handle;
      else
        srcPtr = (void*) ((CUDATextureData_t*) src->handle)->array;

      if(!isATexture()){
        if(!src->isATexture())
          OCCA_CUDA_CHECK("Memory: Copy From [Memory -> Memory]",
                          cuMemcpyDtoD(*((CUdeviceptr*) dstPtr) + destOffset,
                                       *((CUdeviceptr*) srcPtr) + srcOffset,
                                       bytes_) );
        else
          OCCA_CUDA_CHECK("Memory: Copy From [Texture -> Memory]",
                          cuMemcpyAtoD(*((CUdeviceptr*) dstPtr) + destOffset,
                                       (CUarray) srcPtr         , srcOffset,
                                       bytes_) );
      }
      else{
        if(!src->isATexture())
          OCCA_CUDA_CHECK("Memory: Copy From [Memory -> Texture]",
                          cuMemcpyDtoA((CUarray) dstPtr         , destOffset,
                                       *((CUdeviceptr*) srcPtr) + srcOffset,
                                       bytes_) );
        else
          OCCA_CUDA_CHECK("Memory: Copy From [Texture -> Texture]",
                          cuMemcpyAtoA((CUarray) dstPtr, destOffset,
                                       (CUarray) srcPtr, srcOffset,
                                       bytes_) );
      }
    }

    void memory::copyTo(void *dest,
                                const uintptr_t bytes,
                                const uintptr_t offset){
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      if(!isATexture())
        OCCA_CUDA_CHECK("Memory: Copy To",
                        cuMemcpyDtoH(dest, *((CUdeviceptr*) handle) + offset, bytes_) );
      else{
        if(textureInfo.dim == 1)
          OCCA_CUDA_CHECK("Texture Memory: Copy To",
                          cuMemcpyAtoH(dest, ((CUDATextureData_t*) handle)->array, offset, bytes_) );
        else{
          CUDA_MEMCPY2D info;

          info.srcXInBytes   = offset;
          info.srcY          = 0;
          info.srcMemoryType = CU_MEMORYTYPE_ARRAY;
          info.srcArray      = ((CUDATextureData_t*) handle)->array;

          info.dstXInBytes   = 0;
          info.dstY          = 0;
          info.dstMemoryType = CU_MEMORYTYPE_HOST;
          info.dstHost       = dest;
          info.dstPitch      = 0;

          info.WidthInBytes = textureInfo.w * textureInfo.bytesInEntry;
          info.Height       = (bytes_ / info.WidthInBytes);

          cuMemcpy2D(&info);

          dHandle->finish();
        }
      }
    }

    void memory::copyTo(memory_v *dest,
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

      void *dstPtr, *srcPtr;

      if(!isATexture())
        srcPtr = handle;
      else
        srcPtr = (void*) ((CUDATextureData_t*) handle)->array;

      if( !(dest->isATexture()) )
        dstPtr = dest->handle;
      else
        dstPtr = (void*) ((CUDATextureData_t*) dest->handle)->array;

      if(!isATexture()){
        if(!dest->isATexture())
          OCCA_CUDA_CHECK("Memory: Copy To [Memory -> Memory]",
                          cuMemcpyDtoD(*((CUdeviceptr*) dstPtr) + destOffset,
                                       *((CUdeviceptr*) srcPtr) + srcOffset,
                                       bytes_) );
        else
          OCCA_CUDA_CHECK("Memory: Copy To [Memory -> Texture]",
                          cuMemcpyDtoA((CUarray) dstPtr         , destOffset,
                                       *((CUdeviceptr*) srcPtr) + srcOffset,
                                       bytes_) );
      }
      else{
        if(dest->isATexture())
          OCCA_CUDA_CHECK("Memory: Copy To [Texture -> Memory]",
                          cuMemcpyAtoD(*((CUdeviceptr*) dstPtr) + destOffset,
                                       (CUarray) srcPtr         , srcOffset,
                                       bytes_) );
        else
          OCCA_CUDA_CHECK("Memory: Copy To [Texture -> Texture]",
                          cuMemcpyAtoA((CUarray) dstPtr, destOffset,
                                       (CUarray) srcPtr, srcOffset,
                                       bytes_) );
      }
    }

    void memory::asyncCopyFrom(const void *src,
                                       const uintptr_t bytes,
                                       const uintptr_t offset){
      const CUstream &stream = *((CUstream*) dHandle->currentStream);
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      if(!isATexture())
        OCCA_CUDA_CHECK("Memory: Asynchronous Copy From",
                        cuMemcpyHtoDAsync(*((CUdeviceptr*) handle) + offset, src, bytes_, stream) );
      else
        OCCA_CUDA_CHECK("Texture Memory: Asynchronous Copy From",
                        cuMemcpyHtoAAsync(((CUDATextureData_t*) handle)->array, offset, src, bytes_, stream) );
    }

    void memory::asyncCopyFrom(const memory_v *src,
                                       const uintptr_t bytes,
                                       const uintptr_t destOffset,
                                       const uintptr_t srcOffset){
      const CUstream &stream = *((CUstream*) dHandle->currentStream);
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + destOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + srcOffset) <= src->size,
                 "Source has size [" << src->size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      void *dstPtr, *srcPtr;

      if(!isATexture())
        dstPtr = handle;
      else
        dstPtr = (void*) ((CUDATextureData_t*) handle)->array;

      if( !(src->isATexture()) )
        srcPtr = src->handle;
      else
        srcPtr = (void*) ((CUDATextureData_t*) src->handle)->array;

      if(!isATexture()){
        if(!src->isATexture())
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy From [Memory -> Memory]",
                          cuMemcpyDtoDAsync(*((CUdeviceptr*) dstPtr) + destOffset,
                                            *((CUdeviceptr*) srcPtr) + srcOffset,
                                            bytes_, stream) );
        else
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy From [Texture -> Memory]",
                          cuMemcpyAtoD(*((CUdeviceptr*) dstPtr) + destOffset,
                                       (CUarray) srcPtr         , srcOffset,
                                       bytes_) );
      }
      else{
        if(src->isATexture())
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy From [Memory -> Texture]",
                          cuMemcpyDtoA((CUarray) dstPtr         , destOffset,
                                       *((CUdeviceptr*) srcPtr) + srcOffset,
                                       bytes_) );
        else
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy From [Texture -> Texture]",
                          cuMemcpyAtoA((CUarray) dstPtr, destOffset,
                                       (CUarray) srcPtr, srcOffset,
                                       bytes_) );
      }
    }

    void memory::asyncCopyTo(void *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t offset){
      const CUstream &stream = *((CUstream*) dHandle->currentStream);
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      if(!isATexture())
        OCCA_CUDA_CHECK("Memory: Asynchronous Copy To",
                        cuMemcpyDtoHAsync(dest, *((CUdeviceptr*) handle) + offset, bytes_, stream) );
      else
        OCCA_CUDA_CHECK("Texture Memory: Asynchronous Copy To",
                        cuMemcpyAtoHAsync(dest,((CUDATextureData_t*) handle)->array, offset, bytes_, stream) );
    }

    void memory::asyncCopyTo(memory_v *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t destOffset,
                                     const uintptr_t srcOffset){
      const CUstream &stream = *((CUstream*) dHandle->currentStream);
      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + srcOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + destOffset) <= dest->size,
                 "Destination has size [" << dest->size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      void *dstPtr, *srcPtr;

      if(!isATexture())
        srcPtr = handle;
      else
        srcPtr = (void*) ((CUDATextureData_t*) handle)->array;

      if( !(dest->isATexture()) )
        dstPtr = dest->handle;
      else
        dstPtr = (void*) ((CUDATextureData_t*) dest->handle)->array;

      if(!isATexture()){
        if(!dest->isATexture())
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy To [Memory -> Memory]",
                          cuMemcpyDtoDAsync(*((CUdeviceptr*) dstPtr) + destOffset,
                                            *((CUdeviceptr*) srcPtr) + srcOffset,
                                            bytes_, stream) );
        else
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy To [Memory -> Texture]",
                          cuMemcpyDtoA((CUarray) dstPtr         , destOffset,
                                       *((CUdeviceptr*) srcPtr) + srcOffset,
                                       bytes_) );
      }
      else{
        if(dest->isATexture())
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy To [Texture -> Memory]",
                          cuMemcpyAtoD(*((CUdeviceptr*) dstPtr) + destOffset,
                                       (CUarray) srcPtr         , srcOffset,
                                       bytes_) );
        else
          OCCA_CUDA_CHECK("Memory: Asynchronous Copy To [Texture -> Texture]",
                          cuMemcpyAtoA((CUarray) dstPtr, destOffset,
                                       (CUarray) srcPtr, srcOffset,
                                       bytes_) );
      }
    }

    void memory::mappedFree(){
      if(isMapped()){
        OCCA_CUDA_CHECK("Device: mappedFree()",
                        cuMemFreeHost(mappedPtr));

        delete (CUdeviceptr*) handle;

        size = 0;
      }
    }

    void memory::free(){
      if(!isATexture()){
        cuMemFree(*((CUdeviceptr*) handle));

        if(!isAWrapper())
          delete (CUdeviceptr*) handle;
      }
      else{
        CUarray &array        = ((CUDATextureData_t*) handle)->array;
        CUsurfObject &surface = ((CUDATextureData_t*) handle)->surface;

        cuArrayDestroy(array);
        cuSurfObjectDestroy(surface);

        if(!isAWrapper()){
          delete (CUDATextureData_t*) handle;
          delete (CUaddress_mode*)    textureInfo.arg;
        }
      }

      size = 0;
    }
  }
}

#endif
