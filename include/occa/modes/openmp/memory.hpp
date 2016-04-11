#include "occa/defines.hpp"

#if OCCA_OPENMP_ENABLED
#  ifndef OCCA_OPENMP_MEMORY_HEADER
#  define OCCA_OPENMP_MEMORY_HEADER

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "occa/base.hpp"
#include "occa/library.hpp"

#if (OCCA_OS & (LINUX_OS | OSX_OS))
#  include <dlfcn.h>
#else
#  include <windows.h>
#endif

namespace occa {
  namespace openmp {
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

    struct OpenMPDeviceData_t {
      int vendor;
      bool supportsOpenMP;
      std::string OpenMPFlag;
    };
    //==================================


    template <>
    memory_t<OpenMP>::memory_t();

    template <>
    memory_t<OpenMP>::memory_t(const memory_t &m);

    template <>
    memory_t<OpenMP>& memory_t<OpenMP>::operator = (const memory_t &m);

    template <>
    void* memory_t<OpenMP>::getMemoryHandle();

    template <>
    void* memory_t<OpenMP>::getTextureHandle();

    template <>
    void memory_t<OpenMP>::copyFrom(const void *src,
                                    const uintptr_t bytes,
                                    const uintptr_t offset);

    template <>
    void memory_t<OpenMP>::copyFrom(const memory_v *src,
                                    const uintptr_t bytes,
                                    const uintptr_t destOffset,
                                    const uintptr_t srcOffset);

    template <>
    void memory_t<OpenMP>::copyTo(void *dest,
                                  const uintptr_t bytes,
                                  const uintptr_t destOffset);

    template <>
    void memory_t<OpenMP>::copyTo(memory_v *dest,
                                  const uintptr_t bytes,
                                  const uintptr_t srcOffset,
                                  const uintptr_t offset);

    template <>
    void memory_t<OpenMP>::asyncCopyFrom(const void *src,
                                         const uintptr_t bytes,
                                         const uintptr_t destOffset);

    template <>
    void memory_t<OpenMP>::asyncCopyFrom(const memory_v *src,
                                         const uintptr_t bytes,
                                         const uintptr_t srcOffset,
                                         const uintptr_t offset);

    template <>
    void memory_t<OpenMP>::asyncCopyTo(void *dest,
                                       const uintptr_t bytes,
                                       const uintptr_t offset);

    template <>
    void memory_t<OpenMP>::asyncCopyTo(memory_v *dest,
                                       const uintptr_t bytes,
                                       const uintptr_t destOffset,
                                       const uintptr_t srcOffset);

    template <>
    void memory_t<OpenMP>::mappedFree();

    template <>
    void memory_t<OpenMP>::free();
  }
}

#  endif
#endif
