/* The MIT License (MIT)
 * 
 * Copyright (c) 2014 David Medina and Tim Warburton
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#if OCCA_CUDA_ENABLED
#  ifndef OCCA_CUDA_UTILS_HEADER
#  define OCCA_CUDA_UTILS_HEADER

namespace occa {
  namespace cuda {
    extern bool isInitialized;

    void init();

    int getDeviceCount();

    CUdevice getDevice(const int id);

    uintptr_t getDeviceMemorySize(CUdevice device);

    std::string getDeviceListInfo();

    void enablePeerToPeer(CUcontext context);

    void checkPeerToPeer(CUdevice destDevice,
                         CUdevice srcDevice);

    void peerToPeerMemcpy(CUdevice destDevice,
                          CUcontext destContext,
                          CUdeviceptr destMemory,
                          CUdevice srcDevice,
                          CUcontext srcContext,
                          CUdeviceptr srcMemory,
                          const uintptr_t bytes,
                          CUstream usingStream);


    void asyncPeerToPeerMemcpy(CUdevice destDevice,
                               CUcontext destContext,
                               CUdeviceptr destMemory,
                               CUdevice srcDevice,
                               CUcontext srcContext,
                               CUdeviceptr srcMemory,
                               const uintptr_t bytes,
                               CUstream usingStream);

    void peerToPeerMemcpy(CUdevice destDevice,
                          CUcontext destContext,
                          CUdeviceptr destMemory,
                          CUdevice srcDevice,
                          CUcontext srcContext,
                          CUdeviceptr srcMemory,
                          const uintptr_t bytes,
                          CUstream usingStream,
                          const bool isAsync);

    occa::device wrapDevice(CUdevice device, CUcontext context);

    CUevent& event(streamTag tag);

    std::string error(const CUresult errorCode);
  }

  extern const CUarray_format cudaFormats[8];

  template <>
  void* formatType::format<occa::CUDA>() const;

  extern const int CUDA_ADDRESS_NONE;
  extern const int CUDA_ADDRESS_CLAMP;
}

#  endif
#endif