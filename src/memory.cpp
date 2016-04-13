#include <map>

#include "occa/memory.hpp"
#include "occa/device.hpp"
#include "occa/uva.hpp"

namespace occa {
  //---[ memory_v ]---------------------
  memory_v::memory_v(){
    memInfo = memFlag::none;

    handle    = NULL;
    mappedPtr = NULL;
    uvaPtr    = NULL;

    dHandle = NULL;
    size    = 0;
  }

  memory_v::~memory_v(){}

  void memory_v::initFrom(const memory_v &m) {
    memInfo = m.memInfo;

    handle    = m.handle;
    mappedPtr = m.mappedPtr;
    uvaPtr    = m.uvaPtr;

    dHandle = m.dHandle;
    size    = m.size;
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
    return device(mHandle->dHandle).mode();
  }

  uintptr_t memory::bytes() const {
    if(mHandle == NULL)
      return 0;
    return mHandle->size;
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

  void* memory::getMappedPointer() {
    checkIfInitialized();
    return mHandle->mappedPtr;
  }

  void* memory::getMemoryHandle() {
    checkIfInitialized();
    return mHandle->getMemoryHandle();
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
                        const uintptr_t offset,
                        const bool async) {
    checkIfInitialized();
    mHandle->copyFrom(src, bytes, offset, async);
  }

  // [REFACTOR]
  void memory::copyFrom(const memory src,
                        const uintptr_t bytes,
                        const uintptr_t destOffset,
                        const uintptr_t srcOffset,
                        const bool async) {
    checkIfInitialized();
#if 0
    if(mHandle->dHandle == src.mHandle->dHandle) {
      mHandle->copyFrom(src.mHandle, bytes, destOffset, srcOffset, async);
    }
    else{
      memory_v *srcHandle  = src.mHandle;
      memory_v *destHandle = mHandle;

      const occa::mode modeS = srcHandle->mode();
      const occa::mode modeD = destHandle->mode();

      if(modeS & onChipMode) {
        destHandle->copyFrom(srcHandle->getMemoryHandle(),
                             bytes, destOffset,
                             async);
      }
      else if(modeD & onChipMode) {
        srcHandle->copyTo(destHandle->getMemoryHandle(),
                          bytes, srcOffset,
                             async);
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

        CUdeviceptr srcMem  = *((CUdeviceptr*) srcHandle->handle)  + srcOffset;
        CUdeviceptr destMem = *((CUdeviceptr*) destHandle->handle) + destOffset;

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
#endif
  }

  void memory::copyTo(void *dest,
                      const uintptr_t bytes,
                      const uintptr_t offset,
                      const bool async) {
    checkIfInitialized();
    mHandle->copyTo(dest, bytes, offset, async);
  }

  void memory::copyTo(memory dest,
                      const uintptr_t bytes,
                      const uintptr_t destOffset,
                      const uintptr_t srcOffset,
                      const bool async) {

    dest.copyFrom(*this, bytes, destOffset, srcOffset, async);
  }

  void memory::asyncCopyFrom(const void *src,
                             const uintptr_t bytes,
                             const uintptr_t offset) {

    copyFrom(src, bytes, offset, true);
  }

  void memory::asyncCopyFrom(const memory src,
                             const uintptr_t bytes,
                             const uintptr_t destOffset,
                             const uintptr_t srcOffset) {

    copyFrom(src, bytes, destOffset, srcOffset, true);
  }

  void memory::asyncCopyTo(void *dest,
                           const uintptr_t bytes,
                           const uintptr_t offset) {

    copyFrom(dest, bytes, offset, true);
  }

  void memory::asyncCopyTo(memory dest,
                           const uintptr_t bytes,
                           const uintptr_t destOffset,
                           const uintptr_t srcOffset) {

    copyFrom(dest, bytes, destOffset, srcOffset, true);
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

    mHandle->free();

    delete mHandle;
    mHandle = NULL;
  }
}
