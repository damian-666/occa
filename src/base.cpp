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

namespace occa {

  //---[ Globals & Flags ]--------------
  const int parserVersion = 100;

  kernelInfo defaultKernelInfo;

  const int autoDetect = (1 << 0);
  const int srcInUva   = (1 << 1);
  const int destInUva  = (1 << 2);

  bool uvaEnabledByDefault_f = false;
  bool verboseCompilation_f  = true;

  void setVerboseCompilation(const bool value) {
    verboseCompilation_f = value;
  }
  //====================================


  //---[ Registration ]-----------------
  strToModeMap_t& modeMap() {
    static strToModeMap_t modeMap_;
    return modeMap_;
  }

  bool modeExists(const std::string &mode) {
    return (modeMap().find(mode) != modeMap().end());
  }

  device_v* newModeDevice(const std::string &mode) {
    strToModeMapIterator it = modeMap().find(mode);

    if (it == modeMap().end()) {
      std::cout << "OCCA mode [" << mode << "] is not enabled, defaulting to [Serial] mode\n";
      return newModeDevice("Serial");
    }

    return it->second->newDevice();
  }

  kernel_v* newModeKernel(const std::string &mode) {
    strToModeMapIterator it = modeMap().find(mode);

    if (it == modeMap().end()) {
      std::cout << "OCCA mode [" << mode << "] is not enabled, defaulting to [Serial] mode\n";
      return newModeKernel("Serial");
    }

    return it->second->newKernel();
  }

  memory_v* newModeMemory(const std::string &mode) {
    strToModeMapIterator it = modeMap().find(mode);

    if (it == modeMap().end()) {
      std::cout << "OCCA mode [" << mode << "] is not enabled, defaulting to [Serial] mode\n";
      return newModeMemory("Serial");
    }

    return it->second->newMemory();
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

  void setDevice(const std::string &infos) {
    currentDevice = device(infos);
  }

  mutex_t deviceListMutex;
  std::vector<device> deviceList;

  std::vector<device>& getDeviceList() {
    deviceListMutex.lock();
    if(deviceList.size() == 0) {
      strToModeMapIterator it = modeMap().begin();
      while (it != modeMap().end()) {
        device_v* dHandle = it->second->newDevice();
        dHandle->appendAvailableDevices(deviceList);
        freeModeDevice(dHandle);
        ++it;
      }
    }
    deviceListMutex.unlock();

    return deviceList;
  }

  void setCompiler(const std::string &compiler_) {
    currentDevice.setCompiler(compiler_);
  }

  void setCompilerEnvScript(const std::string &compilerEnvScript_) {
    currentDevice.setCompilerEnvScript(compilerEnvScript_);
  }

  void setCompilerFlags(const std::string &compilerFlags_) {
    currentDevice.setCompilerFlags(compilerFlags_);
  }

  std::string getCompiler() {
    return currentDevice.getCompiler();
  }

  std::string getCompilerEnvScript() {
    return currentDevice.getCompilerEnvScript();
  }

  std::string getCompilerFlags() {
    return currentDevice.getCompilerFlags();
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
                     const kernelInfo &info_) {

    return currentDevice.buildKernel(str,
                                     functionName,
                                     info_);
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
                               const kernelInfo &info_,
                               const int language) {

    return currentDevice.buildKernelFromString(content,
                                               functionName,
                                               info_,
                                               language);
  }

  kernel buildKernelFromSource(const std::string &filename,
                               const std::string &functionName,
                               const kernelInfo &info_) {

    return currentDevice.buildKernelFromSource(filename,
                                               functionName,
                                               info_);
  }

  kernel buildKernelFromBinary(const std::string &filename,
                               const std::string &functionName) {

    return currentDevice.buildKernelFromBinary(filename,
                                               functionName);
  }

  //---[ Memory Functions ]-------------
  memory wrapMemory(void *handle_,
                    const uintptr_t bytes) {

    return currentDevice.wrapMemory(handle_, bytes);
  }

  void wrapManagedMemory(void *handle_,
                         const uintptr_t bytes) {

    currentDevice.wrapManagedMemory(handle_, bytes);
  }

  memory malloc(const uintptr_t bytes,
                void *src) {

    return currentDevice.malloc(bytes, src);
  }

  void* managedAlloc(const uintptr_t bytes,
                     void *src) {

    return currentDevice.managedAlloc(bytes, src);
  }

  memory mappedAlloc(const uintptr_t bytes,
                     void *src) {

    return currentDevice.mappedAlloc(bytes, src);
  }

  void* managedMappedAlloc(const uintptr_t bytes,
                           void *src) {

    return currentDevice.managedMappedAlloc(bytes, src);
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
//     std::stringstream ss;
//     ss << "==============o=======================o==========================================\n";
//     ss << sys::getDeviceListInfo();
// #if OCCA_OPENCL_ENABLED
//     ss << "==============o=======================o==========================================\n";
//     ss << cl::getDeviceListInfo();
// #endif
// #if OCCA_CUDA_ENABLED
//     ss << "==============o=======================o==========================================\n";
//     ss << cuda::getDeviceListInfo();
// #endif
//     ss << "==============o=======================o==========================================\n";

//     std::cout << ss.str();
  }
  //====================================


  //---[ Class Infos ]------------------
  iter_t properties::iter(std::string prop) {
    return props.find(prop);
  }

  iter_t properties::end() {
    return props.end();
  }

  bool properties::has(std::string prop) {
    return (iter(prop) != end());
  }

  bool properties::hasMultiple(std::string prop) {
    iter_t it = iter(prop);
    return ((it != end()) && (it->second.size() > 1));
  }

  std::string properties::get(std::string prop) {
    iter_t it = iter(prop);
    if ((it != end()) && (it->second.size()))
      return (it->second)[0];
    return "";
  }

  std::string properties::set(std::string prop, const std::string &s) {
    strVector &v = props[prop];
    v.clear();
    v.push_back(s);
  }

  void properties::append(std::string prop,
                          const std::string &separator,
                          const std::string &s) {
    std::string &p = props[prop];
    p += separator;
    p += s;
  }

  kernelInfo::kernelInfo() :
    header(""),
    flags("") {}

  kernelInfo::kernelInfo(const kernelInfo &p) :
    header(p.header),
    flags(p.flags) {}

  kernelInfo& kernelInfo::operator = (const kernelInfo &p) {
    header = p.header;
    flags  = p.flags;

    return *this;
  }

  kernelInfo& kernelInfo::operator += (const kernelInfo &p) {
    header += p.header;
    flags  += p.flags;

    return *this;
  }

  std::string kernelInfo::salt() const {
    return (header + flags);
  }

  // [REFACTOR]
  std::string kernelInfo::getModeHeaderFilename() const {
    // if(mode & Serial)   return sys::getFilename("[occa]/defines/Serial.hpp");
    // if(mode & OpenMP)   return sys::getFilename("[occa]/defines/OpenMP.hpp");
    // if(mode & OpenCL)   return sys::getFilename("[occa]/defines/OpenCL.hpp");
    // if(mode & CUDA)     return sys::getFilename("[occa]/defines/CUDA.hpp");
    // if(mode & HSA)      return sys::getFilename("[occa]/defines/HSA.hpp");
    // if(mode & Pthreads) return sys::getFilename("[occa]/defines/Pthreads.hpp");

    return "";
  }

  // [REFACTOR]
  bool kernelInfo::isAnOccaDefine(const std::string &name) {
    // if((name == "OCCA_USING_CPU") ||
    //    (name == "OCCA_USING_GPU") ||

    //    (name == "OCCA_USING_SERIAL")   ||
    //    (name == "OCCA_USING_OPENMP")   ||
    //    (name == "OCCA_USING_OPENCL")   ||
    //    (name == "OCCA_USING_CUDA")     ||
    //    (name == "OCCA_USING_PTHREADS") ||

    //    (name == "occaInnerDim0") ||
    //    (name == "occaInnerDim1") ||
    //    (name == "occaInnerDim2") ||

    //    (name == "occaOuterDim0") ||
    //    (name == "occaOuterDim1") ||
    //    (name == "occaOuterDim2"))
    //   return true;

    return false;
  }

  void kernelInfo::addIncludeDefine(const std::string &filename) {
    header += "\n#include \"";
    header += filename;
    header += "\"\n";
  }

  void kernelInfo::addInclude(const std::string &filename) {
    header += '\n';
    header += readFile(filename);
    header += '\n';
  }

  void kernelInfo::removeDefine(const std::string &macro) {
    if(!isAnOccaDefine(macro))
      header += "#undef " + macro + '\n';
  }

  void kernelInfo::addCompilerFlag(const std::string &f) {
    flags += " " + f;
  }

  void kernelInfo::addCompilerIncludePath(const std::string &path) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    flags += " -I \"" + path + "\"";
#else
    flags += " /I \"" + path + "\"";
#endif
  }

  flags_t& kernelInfo::getParserFlags() {
    return parserFlags;
  }

  const flags_t& kernelInfo::getParserFlags() const {
    return parserFlags;
  }

  void kernelInfo::addParserFlag(const std::string &flag,
                                 const std::string &value) {

    parserFlags[flag] = value;
  }

  template <>
  void kernelInfo::addDefine(const std::string &macro, const std::string &value) {
    std::stringstream ss;

    if(isAnOccaDefine(macro))
      ss << "#undef " << macro << "\n";

    // Make sure newlines are followed by escape characters
    std::string value2 = "";
    const int chars = value.size();

    for(int i = 0; i < chars; ++i) {
      if(value[i] != '\n')
        value2 += value[i];
      else{
        if((i < (chars - 1))
           && (value[i] != '\\'))
          value2 += "\\\n";
        else
          value2 += '\n';
      }
    }

    if(value2[value2.size() - 1] != '\n')
      value2 += '\n';
    //  |===============================

    ss << "#define " << macro << " " << value2 << '\n';

    header = ss.str() + header;
  }

  template <>
  void kernelInfo::addDefine(const std::string &macro, const float &value) {
    std::stringstream ss;

    if(isAnOccaDefine(macro))
      ss << "#undef " << macro << "\n";

    ss << "#define " << macro << ' '
       << std::scientific << std::setprecision(8) << value << "f\n";

    header = ss.str() + header;
  }

  template <>
  void kernelInfo::addDefine(const std::string &macro, const double &value) {
    std::stringstream ss;

    if(isAnOccaDefine(macro))
      ss << "#undef " << macro << "\n";

    ss << "#define " << macro << ' '
       << std::scientific << std::setprecision(16) << value << '\n';

    header = ss.str() + header;
  }

  //  |---[ Device Info ]---------------
  deviceInfo::deviceInfo() {}

  deviceInfo::deviceInfo(const deviceInfo &dInfo) :
    infos(dInfo.infos) {}

  deviceInfo& deviceInfo::operator = (const deviceInfo &dInfo) {
    infos = dInfo.infos;

    return *this;
  }

  void deviceInfo::append(const std::string &key,
                          const std::string &value) {
    if(infos.size() != 0)
      infos += ',';

    infos += key;
    infos += '=';
    infos += value;
  }
  //====================================
}
