#if OCCA_OPENCL_ENABLED
#  ifndef OCCA_OPENCL_MEMORY_HEADER
#  define OCCA_OPENCL_MEMORY_HEADER

#include "occa/memory.hpp"
#include "occa/modes/opencl/headers.hpp"

namespace occa {
  namespace opencl {
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
  }
}

#  endif
#endif
