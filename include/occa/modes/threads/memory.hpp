#ifndef OCCA_PTHREADS_MEMORY_HEADER
#define OCCA_PTHREADS_MEMORY_HEADER

#if (OCCA_OS & (LINUX_OS | OSX_OS))
#  if (OCCA_OS != WINUX_OS)
#    include <sys/sysctl.h>
#  endif
#  include <pthread.h>
#  include <dlfcn.h>
#else
#  include <intrin.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include <queue>

#include "occa/base.hpp"

namespace occa {
  namespace threads {
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

    struct PthreadsKernelData_t {
      void *dlHandle;
      handleFunction_t handle;

      int pThreadCount;
      int *pendingJobs;

      std::queue<PthreadKernelInfo_t*> *pKernelInfo[50];

      mutex_t *pendingJobsMutex, *kernelMutex;
    };

    struct PthreadWorkerData_t {
      int rank, count;
      int pinnedCore;

      int *pendingJobs;

      std::queue<PthreadKernelInfo_t*> *pKernelInfo;

      mutex_t *pendingJobsMutex, *kernelMutex;
    };

    struct PthreadKernelInfo_t {
      int rank, count;

      handleFunction_t kernelHandle;

      int dims;
      occa::dim inner, outer;

      int argc;
      void **args;
    };

    static const int compact = (1 << 10);
    static const int scatter = (1 << 11);
    static const int manual  = (1 << 12);
    //====================================

    template <>
    memory_t<Pthreads>::memory_t();

    template <>
    memory_t<Pthreads>::memory_t(const memory_t &m);

    template <>
    memory_t<Pthreads>& memory_t<Pthreads>::operator = (const memory_t &m);

    template <>
    void* memory_t<Pthreads>::getMemoryHandle();

    template <>
    void* memory_t<Pthreads>::getTextureHandle();

    template <>
    void memory_t<Pthreads>::copyFrom(const void *src,
                                      const uintptr_t bytes,
                                      const uintptr_t offset);

    template <>
    void memory_t<Pthreads>::copyFrom(const memory_v *src,
                                      const uintptr_t bytes,
                                      const uintptr_t destOffset,
                                      const uintptr_t srcOffset);

    template <>
    void memory_t<Pthreads>::copyTo(void *dest,
                                    const uintptr_t bytes,
                                    const uintptr_t destOffset);

    template <>
    void memory_t<Pthreads>::copyTo(memory_v *dest,
                                    const uintptr_t bytes,
                                    const uintptr_t srcOffset,
                                    const uintptr_t offset);

    template <>
    void memory_t<Pthreads>::asyncCopyFrom(const void *src,
                                           const uintptr_t bytes,
                                           const uintptr_t destOffset);

    template <>
    void memory_t<Pthreads>::asyncCopyFrom(const memory_v *src,
                                           const uintptr_t bytes,
                                           const uintptr_t srcOffset,
                                           const uintptr_t offset);

    template <>
    void memory_t<Pthreads>::asyncCopyTo(void *dest,
                                         const uintptr_t bytes,
                                         const uintptr_t offset);

    template <>
    void memory_t<Pthreads>::asyncCopyTo(memory_v *dest,
                                         const uintptr_t bytes,
                                         const uintptr_t destOffset,
                                         const uintptr_t srcOffset);

    template <>
    void memory_t<Pthreads>::mappedFree();

    template <>
    void memory_t<Pthreads>::free();
  }
}

#endif
