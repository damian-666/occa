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

#include "occa/base.hpp"
#include "occa/tools/sys.hpp"

namespace occa {

  //---[ Globals & Flags ]--------------
  properties settings;

  const int autoDetect = (1 << 0);
  const int srcInUva   = (1 << 1);
  const int destInUva  = (1 << 2);
  //====================================


  //---[ Registration ]-----------------
  strToModeMap_t& modeMap() {
    static strToModeMap_t modeMap_;
    return modeMap_;
  }

  bool modeIsEnabled(const std::string &mode) {
    return (modeMap().find(mode) != modeMap().end());
  }

  mode_v* getMode(const occa::properties &props) {
    if (!props.has("mode")) {
      std::cout << "No OCCA mode given, defaulting to [Serial] mode\n";
      return getMode("Serial");
    }
    return getMode(props["mode"]);
  }

  mode_v* getMode(const std::string &mode) {
    if (!modeIsEnabled(mode)) {
      std::cout << "OCCA mode [" << mode << "] is not enabled, defaulting to [Serial] mode\n";
      return modeMap()["Serial"];
    }
    return modeMap()[mode];
  }

  device_v* newModeDevice(const std::string &mode) {
    return getMode(mode)->newDevice();
  }

  device_v* newModeDevice(const occa::properties &props) {
    return getMode(props)->newDevice();
  }

  kernel_v* newModeKernel(const std::string &mode) {
    return getMode(mode)->newKernel();
  }

  kernel_v* newModeKernel(const occa::properties &props) {
    return getMode(props)->newKernel();
  }

  memory_v* newModeMemory(const std::string &mode) {
    return getMode(mode)->newMemory();
  }

  memory_v* newModeMemory(const occa::properties &props) {
    return getMode(props)->newMemory();
  }

  void freeModeDevice(device_v *dHandle) {
    delete dHandle;
  }

  void freeModeKernel(kernel_v *kHandle) {
    delete kHandle;
  }

  void freeModeMemory(memory_v *mHandle) {
    delete mHandle;
  }
  //====================================


  //---[ Memory ]-----------------------
  void memcpy(void *dest, void *src,
              const uintptr_t bytes,
              const int flags) {

    memcpy(dest, src, bytes, flags, false);
  }

  void asyncMemcpy(void *dest, void *src,
                   const uintptr_t bytes,
                   const int flags) {

    memcpy(dest, src, bytes, flags, true);
  }

  void memcpy(void *dest, void *src,
              const uintptr_t bytes,
              const int flags,
              const bool isAsync) {

    ptrRangeMap_t::iterator srcIt  = uvaMap.end();
    ptrRangeMap_t::iterator destIt = uvaMap.end();

    if(flags & occa::autoDetect) {
      srcIt  = uvaMap.find(src);
      destIt = uvaMap.find(dest);
    }
    else{
      if(flags & srcInUva)
        srcIt  = uvaMap.find(src);

      if(flags & destInUva)
        destIt  = uvaMap.find(dest);
    }

    occa::memory_v *srcMem  = ((srcIt != uvaMap.end())  ? (srcIt->second)  : NULL);
    occa::memory_v *destMem = ((destIt != uvaMap.end()) ? (destIt->second) : NULL);

    const uintptr_t srcOff  = (srcMem  ? (((char*) src)  - ((char*) srcMem->uvaPtr))  : 0);
    const uintptr_t destOff = (destMem ? (((char*) dest) - ((char*) destMem->uvaPtr)) : 0);

    const bool usingSrcPtr  = ((srcMem  == NULL) || srcMem->isManaged());
    const bool usingDestPtr = ((destMem == NULL) || destMem->isManaged());

    if(usingSrcPtr && usingDestPtr) {
      ::memcpy(dest, src, bytes);
    }
    else if(usingSrcPtr) {
      destMem->copyFrom(src, bytes, destOff, isAsync);
    }
    else if(usingDestPtr) {
      srcMem->copyTo(dest, bytes, srcOff, isAsync);
    }
    else {
      // Auto-detects peer-to-peer stuff
      occa::memory srcMemory(srcMem);
      occa::memory destMemory(destMem);
      srcMemory.copyTo(destMemory, bytes, destOff, srcOff, isAsync);
    }
  }

  void memcpy(memory dest,
              const void *src,
              const uintptr_t bytes,
              const uintptr_t offset) {

    dest.copyFrom(src, bytes, offset);
  }

  void memcpy(void *dest,
              memory src,
              const uintptr_t bytes,
              const uintptr_t offset) {

    src.copyTo(dest, bytes, offset);
  }

  void memcpy(memory dest,
              memory src,
              const uintptr_t bytes,
              const uintptr_t destOffset,
              const uintptr_t srcOffset) {

    src.copyTo(dest, bytes, destOffset, srcOffset);
  }

  void asyncMemcpy(memory dest,
                   const void *src,
                   const uintptr_t bytes,
                   const uintptr_t offset) {

    dest.copyFrom(src, bytes, offset, true);
  }

  void asyncMemcpy(void *dest,
                   memory src,
                   const uintptr_t bytes,
                   const uintptr_t offset) {

    src.copyTo(dest, bytes, offset, true);
  }

  void asyncMemcpy(memory dest,
                   memory src,
                   const uintptr_t bytes,
                   const uintptr_t destOffset,
                   const uintptr_t srcOffset) {

    src.copyTo(dest, bytes, destOffset, srcOffset, true);
  }
  //====================================


  //---[ Device Functions ]-------------
  device currentDevice;

  device getCurrentDevice() {
    if (currentDevice.getDHandle() == NULL) {
      currentDevice = host();
    }
    return currentDevice;
  }

  device host() {
    static device _host;
    if (_host.getDHandle() == NULL) {
      _host = occa::device(newModeDevice("Serial"));
    }
    return _host;
  }

  void setDevice(device d) {
    currentDevice = d;
  }

  void setDevice(const std::string &props) {
    currentDevice = device(props);
  }

  void setDevice(const properties &props) {
    currentDevice = device(props);
  }

  std::vector<device>& getDeviceList() {
    static std::vector<device> deviceList;
    static mutex_t mutex;

    mutex.lock();
    if(deviceList.size() == 0) {
      strToModeMapIterator it = modeMap().begin();
      while (it != modeMap().end()) {
        device_v* dHandle = it->second->newDevice();
        dHandle->appendAvailableDevices(deviceList);
        freeModeDevice(dHandle);
        ++it;
      }
    }
    mutex.unlock();

    return deviceList;
  }

  properties& deviceProperties() {
    return currentDevice.properties();
  }

  void flush() {
    currentDevice.flush();
  }

  void finish() {
    currentDevice.finish();
  }

  void waitFor(streamTag tag) {
    currentDevice.waitFor(tag);
  }

  stream createStream() {
    return currentDevice.createStream();
  }

  stream getStream() {
    return currentDevice.getStream();
  }

  void setStream(stream s) {
    currentDevice.setStream(s);
  }

  stream wrapStream(void *handle_) {
    return currentDevice.wrapStream(handle_);
  }

  streamTag tagStream() {
    return currentDevice.tagStream();
  }

  //---[ Kernel Functions ]-------------
  kernel buildKernel(const std::string &str,
                     const std::string &functionName,
                     const properties &props) {

    return currentDevice.buildKernel(str,
                                     functionName,
                                     props);
  }

  kernel buildKernelFromString(const std::string &content,
                               const std::string &functionName,
                               const int language) {

    return currentDevice.buildKernelFromString(content,
                                               functionName,
                                               language);
  }

  kernel buildKernelFromString(const std::string &content,
                               const std::string &functionName,
                               const properties &props,
                               const int language) {

    return currentDevice.buildKernelFromString(content,
                                               functionName,
                                               props,
                                               language);
  }

  kernel buildKernelFromSource(const std::string &filename,
                               const std::string &functionName,
                               const properties &props) {

    return currentDevice.buildKernelFromSource(filename,
                                               functionName,
                                               props);
  }

  kernel buildKernelFromBinary(const std::string &filename,
                               const std::string &functionName) {

    return currentDevice.buildKernelFromBinary(filename,
                                               functionName);
  }

  //---[ Memory Functions ]-------------
  occa::memory malloc(const uintptr_t bytes,
                      void *src,
                      const properties &props) {

    return currentDevice.malloc(bytes, src, props);
  }

  void* managedAlloc(const uintptr_t bytes,
                     void *src,
                     const properties &props) {

    return currentDevice.managedAlloc(bytes, src, props);
  }
  //====================================

  //---[ Free Functions ]---------------
  void free(device d) {
    d.free();
  }

  void free(stream s) {
    currentDevice.freeStream(s);
  }

  void free(kernel k) {
    k.free();
  }

  void free(memory m) {
    m.free();
  }
  //====================================

  // [REFORMAT]
  void printAvailableDevices() {
  }
  //====================================
}
