#if OCCA_CUDA_ENABLED
#  ifndef OCCA_CUDA_MEMORY_HEADER
#  define OCCA_CUDA_MEMORY_HEADER

#include "occa/defines.hpp"
#include "occa/memory.hpp"

#include <cuda.h>

namespace occa {
  namespace cuda {
    class memory : public occa::memory_v {
    public:
      memory();
      memory(const memory &m);
      memory& operator = (const memory &m);
      ~memory();

      void* getMemoryHandle();

      void copyFrom(const void *src,
                    const uintptr_t bytes,
                    const uintptr_t offset,
                    const bool async);

      void copyFrom(const memory_v *src,
                    const uintptr_t bytes,
                    const uintptr_t destOffset,
                    const uintptr_t srcOffset,
                    const bool async);

      void copyTo(void *dest,
                  const uintptr_t bytes,
                  const uintptr_t destOffset,
                  const bool async);

      void copyTo(memory_v *dest,
                  const uintptr_t bytes,
                  const uintptr_t srcOffset,
                  const uintptr_t offset,
                  const bool async);

      void free();
    };
    };

    struct CUDADeviceData_t {
      CUdevice  device;
      CUcontext context;
      bool p2pEnabled;
    };

    struct CUDATextureData_t {
      CUarray array;
      CUsurfObject surface;
    };
    //==================================


    memory_t<CUDA>::memory_t();

    memory_t<CUDA>::memory_t(const memory_t &m);

    memory_t<CUDA>& memory_t<CUDA>::operator = (const memory_t &m);

    void* memory_t<CUDA>::getMemoryHandle();

    void* memory_t<CUDA>::getTextureHandle();

    void memory_t<CUDA>::copyFrom(const void *src,
                                  const uintptr_t bytes,
                                  const uintptr_t offset);

    void memory_t<CUDA>::copyFrom(const memory_v *src,
                                  const uintptr_t bytes,
                                  const uintptr_t destOffset,
                                  const uintptr_t srcOffset);

    void memory_t<CUDA>::copyTo(void *dest,
                                const uintptr_t bytes,
                                const uintptr_t offset);

    void memory_t<CUDA>::copyTo(memory_v *dest,
                                const uintptr_t bytes,
                                const uintptr_t destOffset,
                                const uintptr_t srcOffset);

    void memory_t<CUDA>::asyncCopyFrom(const void *src,
                                       const uintptr_t bytes,
                                       const uintptr_t offset);

    void memory_t<CUDA>::asyncCopyFrom(const memory_v *src,
                                       const uintptr_t bytes,
                                       const uintptr_t destOffset,
                                       const uintptr_t srcOffset);

    void memory_t<CUDA>::asyncCopyTo(void *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t offset);

    void memory_t<CUDA>::asyncCopyTo(memory_v *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t destOffset,
                                     const uintptr_t srcOffset);

    void memory_t<CUDA>::mappedFree();

    void memory_t<CUDA>::free();
  }
}

#  endif
#endif
