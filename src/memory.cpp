#include "occa/memory.hpp"

namespace occa {
  //---[ memory_v ]---------------------
  bool memory_v::isATexture() const {
    return (memInfo & memFlag::isATexture);
  }

  bool memory_v::isManaged() const {
    return (memInfo & memFlag::isManaged);
  }

  bool memory_v::isMapped() const {
    return (memInfo & memFlag::isMapped);
  }

  bool memory_v::isAWrapper() const {
    return (memInfo & memFlag::isAWrapper);
  }

  bool memory_v::inDevice() const {
    return (memInfo & uvaFlag::inDevice);
  }

  bool memory_v::leftInDevice() const {
    return (memInfo & uvaFlag::leftInDevice);
  }

  bool memory_v::isDirty() const {
    return (memInfo & uvaFlag::isDirty);
  }

  //---[ memory ]-----------------------
  memory::memory() :
    mHandle(NULL) {}

  memory::memory(void *uvaPtr) {
    // Default to uvaPtr is actually a memory_v*
    memory_v *mHandle_ = (memory_v*) uvaPtr;

    ptrRangeMap_t::iterator it = uvaMap.find(uvaPtr);

    if(it != uvaMap.end())
      mHandle_ = it->second;

    mHandle = mHandle_;
  }

  memory::memory(memory_v *mHandle_) :
    mHandle(mHandle_) {}

  memory::memory(const memory &m) :
    mHandle(m.mHandle) {}

  memory& memory::swap(memory &m) {
    memory_v *tmp = mHandle;
    mHandle       = m.mHandle;
    m.mHandle     = tmp;

    return *this;
  }

  memory& memory::operator = (const memory &m) {
    mHandle = m.mHandle;
    return *this;
  }

  void memory::checkIfInitialized() const {
    OCCA_CHECK(mHandle != NULL,
               "Memory is not initialized");
  }

  memory_v* memory::getMHandle() {
    checkIfInitialized();
    return mHandle;
  }

  device_v* memory::getDHandle() {
    checkIfInitialized();
    return mHandle->dHandle;
  }

  const std::string& memory::mode() {
    checkIfInitialized();
    return mHandle->strMode;
  }

  uintptr_t memory::bytes() const {
    if(mHandle == NULL)
      return 0;
    return mHandle->size;
  }

  bool memory::isATexture() const {
    return (mHandle->memInfo & memFlag::isATexture);
  }

  bool memory::isManaged() const {
    return (mHandle->memInfo & memFlag::isManaged);
  }

  bool memory::isMapped() const {
    return (mHandle->memInfo & memFlag::isMapped);
  }

  bool memory::isAWrapper() const {
    return (mHandle->memInfo & memFlag::isAWrapper);
  }

  bool memory::inDevice() const {
    return (mHandle->memInfo & uvaFlag::inDevice);
  }

  bool memory::leftInDevice() const {
    return (mHandle->memInfo & uvaFlag::leftInDevice);
  }

  bool memory::isDirty() const {
    return (mHandle->memInfo & uvaFlag::isDirty);
  }

  void* memory::textureArg1() const {
    checkIfInitialized();

#if !OCCA_CUDA_ENABLED
    return (void*) mHandle;
#else
    if(mHandle->mode() != CUDA)
      return (void*) mHandle;
    else
      return &(((CUDATextureData_t*) mHandle->handle)->surface);
#endif
  }

  void* memory::textureArg2() const {
    checkIfInitialized();
    return (void*) ((mHandle->textureInfo).arg);
  }

  void* memory::getMappedPointer() {
    checkIfInitialized();
    return mHandle->mappedPtr;
  }

  void* memory::getMemoryHandle() {
    checkIfInitialized();
    return mHandle->getMemoryHandle();
  }

  void* memory::getTextureHandle() {
    checkIfInitialized();
    return mHandle->getTextureHandle();
  }

  void memory::placeInUva() {
    checkIfInitialized();

    if( !(mHandle->dHandle->fakesUva()) ) {
      mHandle->uvaPtr = mHandle->handle;
    }
    else if(mHandle->isMapped()) {
      mHandle->uvaPtr = mHandle->mappedPtr;
    }
    else{
      mHandle->uvaPtr = sys::malloc(mHandle->size);
    }

    ptrRange_t uvaRange;

    uvaRange.start = (char*) (mHandle->uvaPtr);
    uvaRange.end   = (uvaRange.start + mHandle->size);

    uvaMap[uvaRange]                   = mHandle;
    mHandle->dHandle->uvaMap[uvaRange] = mHandle;

    // Needed for kernelArg.void_ -> mHandle checks
    if(mHandle->uvaPtr != mHandle->handle)
      uvaMap[mHandle->handle] = mHandle;
  }

  void memory::manage() {
    checkIfInitialized();
    placeInUva();
    mHandle->memInfo |= memFlag::isManaged;
  }

  void memory::syncToDevice(const uintptr_t bytes,
                            const uintptr_t offset) {
    checkIfInitialized();

    if(mHandle->dHandle->fakesUva()) {
      uintptr_t bytes_ = ((bytes == 0) ? mHandle->size : bytes);

      copyTo(mHandle->uvaPtr, bytes_, offset);

      mHandle->memInfo |=  uvaFlag::inDevice;
      mHandle->memInfo &= ~uvaFlag::isDirty;

      removeFromDirtyMap(mHandle);
    }
  }

  void memory::syncFromDevice(const uintptr_t bytes,
                              const uintptr_t offset) {
    checkIfInitialized();

    if(mHandle->dHandle->fakesUva()) {
      uintptr_t bytes_ = ((bytes == 0) ? mHandle->size : bytes);

      copyFrom(mHandle->uvaPtr, bytes_, offset);

      mHandle->memInfo &= ~uvaFlag::inDevice;
      mHandle->memInfo &= ~uvaFlag::isDirty;

      removeFromDirtyMap(mHandle);
    }
  }

  bool memory::uvaIsDirty() {
    checkIfInitialized();
    return (mHandle && mHandle->isDirty());
  }

  void memory::uvaMarkDirty() {
    checkIfInitialized();
    if(mHandle != NULL)
      mHandle->memInfo |= uvaFlag::isDirty;
  }

  void memory::uvaMarkClean() {
    checkIfInitialized();
    if(mHandle != NULL)
      mHandle->memInfo &= ~uvaFlag::isDirty;
  }

  void memory::copyFrom(const void *src,
                        const uintptr_t bytes,
                        const uintptr_t offset) {
    checkIfInitialized();
    mHandle->copyFrom(src, bytes, offset);
  }

  void memory::copyFrom(const memory src,
                        const uintptr_t bytes,
                        const uintptr_t destOffset,
                        const uintptr_t srcOffset) {
    checkIfInitialized();

    if(mHandle->dHandle == src.mHandle->dHandle) {
      mHandle->copyFrom(src.mHandle, bytes, destOffset, srcOffset);
    }
    else{
      memory_v *srcHandle  = src.mHandle;
      memory_v *destHandle = mHandle;

      const occa::mode modeS = srcHandle->mode();
      const occa::mode modeD = destHandle->mode();

      if(modeS & onChipMode) {
        destHandle->copyFrom(srcHandle->getMemoryHandle(),
                             bytes, destOffset);
      }
      else if(modeD & onChipMode) {
        srcHandle->copyTo(destHandle->getMemoryHandle(),
                          bytes, srcOffset);
      }
      else{
        OCCA_CHECK(((modeS == CUDA) && (modeD == CUDA)),
                   "Peer-to-peer is not supported between ["
                   << modeToStr(modeS) << "] and ["
                   << modeToStr(modeD) << "]");

#if OCCA_CUDA_ENABLED
        CUDADeviceData_t &srcDevData  =
          *((CUDADeviceData_t*) srcHandle->dHandle->data);

        CUDADeviceData_t &destDevData =
          *((CUDADeviceData_t*) destHandle->dHandle->data);

        CUdeviceptr srcMem  = *(((CUdeviceptr*) srcHandle->handle)  + srcOffset);
        CUdeviceptr destMem = *(((CUdeviceptr*) destHandle->handle) + destOffset);

        if(!srcDevData.p2pEnabled)
          cuda::enablePeerToPeer(srcDevData.context);

        if(!destDevData.p2pEnabled)
          cuda::enablePeerToPeer(destDevData.context);

        cuda::checkPeerToPeer(destDevData.device,
                              srcDevData.device);

        cuda::peerToPeerMemcpy(destDevData.device,
                               destDevData.context,
                               destMem,

                               srcDevData.device,
                               srcDevData.context,
                               srcMem,

                               bytes,
                               *((CUstream*) srcHandle->dHandle->currentStream));
#endif
      }
    }
  }

  void memory::copyTo(void *dest,
                      const uintptr_t bytes,
                      const uintptr_t offset) {
    checkIfInitialized();
    mHandle->copyTo(dest, bytes, offset);
  }

  void memory::copyTo(memory dest,
                      const uintptr_t bytes,
                      const uintptr_t destOffset,
                      const uintptr_t srcOffset) {
    checkIfInitialized();

    if(mHandle->dHandle == dest.mHandle->dHandle) {
      mHandle->copyTo(dest.mHandle, bytes, destOffset, srcOffset);
    }
    else{
      memory_v *srcHandle  = mHandle;
      memory_v *destHandle = dest.mHandle;

      const occa::mode modeS = srcHandle->mode();
      const occa::mode modeD = destHandle->mode();

      if(modeS & onChipMode) {
        destHandle->copyFrom(srcHandle->getMemoryHandle(),
                             bytes, srcOffset);
      }
      else if(modeD & onChipMode) {
        srcHandle->copyTo(destHandle->getMemoryHandle(),
                          bytes, destOffset);
      }
      else{
        OCCA_CHECK(((modeS == CUDA) && (modeD == CUDA)),
                   "Peer-to-peer is not supported between ["
                   << modeToStr(modeS) << "] and ["
                   << modeToStr(modeD) << "]");

#if OCCA_CUDA_ENABLED
        CUDADeviceData_t &srcDevData  =
          *((CUDADeviceData_t*) srcHandle->dHandle->data);

        CUDADeviceData_t &destDevData =
          *((CUDADeviceData_t*) destHandle->dHandle->data);

        CUdeviceptr srcMem  = *(((CUdeviceptr*) srcHandle->handle)  + srcOffset);
        CUdeviceptr destMem = *(((CUdeviceptr*) destHandle->handle) + destOffset);

        cuda::peerToPeerMemcpy(destDevData.device,
                               destDevData.context,
                               destMem,

                               srcDevData.device,
                               srcDevData.context,
                               srcMem,

                               bytes,
                               *((CUstream*) srcHandle->dHandle->currentStream));
#endif
      }
    }
  }

  void memory::asyncCopyFrom(const void *src,
                             const uintptr_t bytes,
                             const uintptr_t offset) {
    checkIfInitialized();
    mHandle->asyncCopyFrom(src, bytes, offset);
  }

  void memory::asyncCopyFrom(const memory src,
                             const uintptr_t bytes,
                             const uintptr_t destOffset,
                             const uintptr_t srcOffset) {
    checkIfInitialized();

    if(mHandle->dHandle == src.mHandle->dHandle) {
      mHandle->asyncCopyFrom(src.mHandle, bytes, destOffset, srcOffset);
    }
    else{
      memory_v *srcHandle  = src.mHandle;
      memory_v *destHandle = mHandle;

      const occa::mode modeS = srcHandle->mode();
      const occa::mode modeD = destHandle->mode();

      if(modeS & onChipMode) {
        destHandle->asyncCopyFrom(srcHandle->getMemoryHandle(),
                             bytes, destOffset);
      }
      else if(modeD & onChipMode) {
        srcHandle->asyncCopyTo(destHandle->getMemoryHandle(),
                          bytes, srcOffset);
      }
      else{
        OCCA_CHECK(((modeS == CUDA) && (modeD == CUDA)),
                   "Peer-to-peer is not supported between ["
                   << modeToStr(modeS) << "] and ["
                   << modeToStr(modeD) << "]");

#if OCCA_CUDA_ENABLED
        CUDADeviceData_t &srcDevData  =
          *((CUDADeviceData_t*) srcHandle->dHandle->data);

        CUDADeviceData_t &destDevData =
          *((CUDADeviceData_t*) destHandle->dHandle->data);

        CUdeviceptr srcMem  = *(((CUdeviceptr*) srcHandle->handle)  + srcOffset);
        CUdeviceptr destMem = *(((CUdeviceptr*) destHandle->handle) + destOffset);

        cuda::asyncPeerToPeerMemcpy(destDevData.device,
                                    destDevData.context,
                                    destMem,

                                    srcDevData.device,
                                    srcDevData.context,
                                    srcMem,

                                    bytes,
                                    *((CUstream*) srcHandle->dHandle->currentStream));
#endif
      }
    }
  }

  void memory::asyncCopyTo(void *dest,
                           const uintptr_t bytes,
                           const uintptr_t offset) {
    checkIfInitialized();
    mHandle->asyncCopyTo(dest, bytes, offset);
  }

  void memory::asyncCopyTo(memory dest,
                           const uintptr_t bytes,
                           const uintptr_t destOffset,
                           const uintptr_t srcOffset) {
    checkIfInitialized();

    if(mHandle->dHandle == dest.mHandle->dHandle) {
      mHandle->asyncCopyTo(dest.mHandle, bytes, destOffset, srcOffset);
    }
    else{
      memory_v *srcHandle  = mHandle;
      memory_v *destHandle = dest.mHandle;

      const occa::mode modeS = srcHandle->mode();
      const occa::mode modeD = destHandle->mode();

      if(modeS & onChipMode) {
        destHandle->asyncCopyFrom(srcHandle->getMemoryHandle(),
                                  bytes, destOffset);
      }
      else if(modeD & onChipMode) {
        srcHandle->asyncCopyTo(destHandle->getMemoryHandle(),
                               bytes, srcOffset);
      }
      else{
        OCCA_CHECK(((modeS == CUDA) && (modeD == CUDA)),
                   "Peer-to-peer is not supported between ["
                   << modeToStr(modeS) << "] and ["
                   << modeToStr(modeD) << "]");

#if OCCA_CUDA_ENABLED
        CUDADeviceData_t &srcDevData  =
          *((CUDADeviceData_t*) srcHandle->dHandle->data);

        CUDADeviceData_t &destDevData =
          *((CUDADeviceData_t*) destHandle->dHandle->data);

        CUdeviceptr srcMem  = *(((CUdeviceptr*) srcHandle->handle)  + srcOffset);
        CUdeviceptr destMem = *(((CUdeviceptr*) destHandle->handle) + destOffset);

        cuda::asyncPeerToPeerMemcpy(destDevData.device,
                                    destDevData.context,
                                    destMem,

                                    srcDevData.device,
                                    srcDevData.context,
                                    srcMem,

                                    bytes,
                                    *((CUstream*) srcHandle->dHandle->currentStream));
#endif
      }
    }
  }

  void memory::free() {
    checkIfInitialized();

    mHandle->dHandle->bytesAllocated -= (mHandle->size);

    if(mHandle->uvaPtr) {
      uvaMap.erase(mHandle->uvaPtr);
      mHandle->dHandle->uvaMap.erase(mHandle->uvaPtr);

      // CPU case where memory is shared
      if(mHandle->uvaPtr != mHandle->handle) {
        uvaMap.erase(mHandle->handle);
        mHandle->dHandle->uvaMap.erase(mHandle->uvaPtr);

        ::free(mHandle->uvaPtr);
        mHandle->uvaPtr = NULL;
      }
    }

    if(!mHandle->isMapped())
      mHandle->free();
    else
      mHandle->mappedFree();

    delete mHandle;
    mHandle = NULL;
  }
}