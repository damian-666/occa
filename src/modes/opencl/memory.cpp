#if OCCA_OPENCL_ENABLED

#include "occa/modes/opencl/memory.hpp"
#include "occa/modes/opencl/device.hpp"
#include "occa/modes/opencl/utils.hpp"

namespace occa {
  namespace opencl {
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
                          const bool async){
      cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy From",
                    clEnqueueWriteBuffer(stream, *((cl_mem*) handle),
                                         async ? CL_FALSE : CL_TRUE,
                                         offset, bytes_, src,
                                         0, NULL, NULL));
    }

    void memory::copyFrom(const memory_v *src,
                          const uintptr_t bytes,
                          const uintptr_t destOffset,
                          const uintptr_t srcOffset,
                          const bool async){
      cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + destOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + srcOffset) <= src->size,
                 "Source has size [" << src->size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy From",
                    clEnqueueCopyBuffer(stream,
                                        *((cl_mem*) src->handle),
                                        *((cl_mem*) handle),
                                        srcOffset, destOffset,
                                        bytes_,
                                        0, NULL, NULL));
    }

    void memory::copyTo(void *dest,
                        const uintptr_t bytes,
                        const uintptr_t offset,
                        const bool async){

      const cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + offset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << offset << " , " << (offset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy To",
                    clEnqueueReadBuffer(stream, *((cl_mem*) handle),
                                        async ? CL_FALSE : CL_TRUE,
                                        offset, bytes_, dest,
                                        0, NULL, NULL));
    }

    void memory::copyTo(memory_v *dest,
                        const uintptr_t bytes,
                        const uintptr_t destOffset,
                        const uintptr_t srcOffset,
                        const bool async){

      const cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);

      const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

      OCCA_CHECK((bytes_ + srcOffset) <= size,
                 "Memory has size [" << size << "],"
                 << "trying to access [ " << srcOffset << " , " << (srcOffset + bytes_) << " ]");

      OCCA_CHECK((bytes_ + destOffset) <= dest->size,
                 "Destination has size [" << dest->size << "],"
                 << "trying to access [ " << destOffset << " , " << (destOffset + bytes_) << " ]");

      OCCA_CL_CHECK("Memory: " << (async ? "Async " : "") << "Copy To",
                    clEnqueueCopyBuffer(stream,
                                        *((cl_mem*) handle),
                                        *((cl_mem*) dest->handle),
                                        srcOffset, destOffset,
                                        bytes_,
                                        0, NULL, NULL));
    }

    void memory::mappedFree(){
      cl_command_queue &stream = *((cl_command_queue*) dHandle->currentStream);
      cl_int error;

      // Un-map pointer
      error = clEnqueueUnmapMemObject(stream,
                                      *((cl_mem*) handle),
                                      mappedPtr,
                                      0, NULL, NULL);

      OCCA_CL_CHECK("Mapped Free: clEnqueueUnmapMemObject", error);

      // Free mapped-host pointer
      error = clReleaseMemObject(*((cl_mem*) handle));

      OCCA_CL_CHECK("Mapped Free: clReleaseMemObject", error);

      delete (cl_mem*) handle;
    }

    void memory::free(){
      clReleaseMemObject(*((cl_mem*) handle));

      if (isMapped()) {
        mappedFree();
      }
      else if (!isAWrapper()) {
        delete (cl_mem*) handle;
      }

      size = 0;
    }
  }
}

#endif
