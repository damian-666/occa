#include "occa/base.hpp"
#include "occa/parser/parser.hpp"

namespace occa {

  //---[ Globals & Flags ]------------------------
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
  //==============================================

  //---[ Helper Classes ]-------------------------
  argInfoMap::argInfoMap() {}

  argInfoMap::argInfoMap(const std::string &infos) {
    if(infos.size() == 0)
      return;

    parserNS::expNode expRoot = parserNS::createOrganizedExpNodeFrom(infos);

    parserNS::expNode &csvFlatRoot = *(expRoot.makeCsvFlatHandle());

    for(int i = 0; i < csvFlatRoot.leafCount; ++i) {
      parserNS::expNode &leaf = csvFlatRoot[i];

      std::string &info = (leaf.leafCount ? leaf[0].value : leaf.value);

      if(leaf.value != "=") {
        std::cout << "Flag [" << info << "] was not set, skipping it\n";
        continue;
      }

      iMap[info] = leaf[1].toString();
    }

    parserNS::expNode::freeFlatHandle(csvFlatRoot);
  }

  argInfoMap::argInfoMap(argInfoMap &aim) {
    *this = aim;
  }

  argInfoMap& argInfoMap::operator = (argInfoMap &aim) {
    iMap = aim.iMap;
    return *this;
  }

  std::string& operator [] (const std::string &info) {
    return iMap[info];
  }

  bool argInfoMap::has(const std::string &info) {
    return (iMap.find(info) != iMap.end());
  }

  void argInfoMap::remove(const std::string &info) {
    std::map<std::string, std::string>::iterator it = iMap.find(info);

    if(it != iMap.end())
      iMap.erase(it);
  }

  std::string argInfoMap::get(const std::string &info) {
    std::map<std::string,std::string>::iterator it = iMap.find(info);

    if(it != iMap.end())
      return it->second;

    return "";
  }

  void argInfoMap::iGets(const std::string &info, std::vector<int> &entries) {
    std::map<std::string,std::string>::iterator it = iMap.find(info);

    if(it == iMap.end())
      return;

    const char *c = (it->second).c_str();

    while(*c != '\0') {
      skipWhitespace(c);

      if(isANumber(c)) {
        entries.push_back(atoi(c));
        skipNumber(c);
      }
      else
        ++c;
    }
  }

  dim::dim() :
    x(1),
    y(1),
    z(1) {}

  dim::dim(uintptr_t x_) :
    x(x_),
    y(1),
    z(1) {}

  dim::dim(uintptr_t x_, uintptr_t y_) :
    x(x_),
    y(y_),
    z(1) {}

  dim::dim(uintptr_t x_, uintptr_t y_, uintptr_t z_) :
    x(x_),
    y(y_),
    z(z_) {}

  dim::dim(const dim &d) :
    x(d.x),
    y(d.y),
    z(d.z) {}

  dim& dim::operator = (const dim &d) {
    x = d.x;
    y = d.y;
    z = d.z;

    return *this;
  }

  dim dim::operator + (const dim &d) {
    return dim(x + d.x,
               y + d.y,
               z + d.z);
  }

  dim dim::operator - (const dim &d) {
    return dim(x - d.x,
               y - d.y,
               z - d.z);
  }

  dim dim::operator * (const dim &d) {
    return dim(x * d.x,
               y * d.y,
               z * d.z);
  }

  dim dim::operator / (const dim &d) {
    return dim(x / d.x,
               y / d.y,
               z / d.z);
  }

  bool dim::hasNegativeEntries() {
    return ((x & (1 << (sizeof(uintptr_t) - 1))) ||
            (y & (1 << (sizeof(uintptr_t) - 1))) ||
            (z & (1 << (sizeof(uintptr_t) - 1))));
  }

  uintptr_t& dim::operator [] (int i) {
    switch(i) {
    case 0 : return x;
    case 1 : return y;
    default: return z;
    }
  }

  uintptr_t dim::operator [] (int i) const {
    switch(i) {
    case 0 : return x;
    case 1 : return y;
    default: return z;
    }
  }

  kernelArg_t::kernelArg_t() {
    dHandle = NULL;
    mHandle = NULL;

    ::memset(&data, 0, sizeof(data));
    size = 0;
    info = kArgInfo::none;
  }

  kernelArg_t::kernelArg_t(const kernelArg_t &k) {
    *this = k;
  }

  kernelArg_t& kernelArg_t::operator = (const kernelArg_t &k) {
    dHandle = k.dHandle;
    mHandle = k.mHandle;

    ::memcpy(&data, &(k.data), sizeof(data));
    size = k.size;
    info = k.info;

    return *this;
  }

  kernelArg_t::~kernelArg_t() {}

  void* kernelArg_t::ptr() const {
    return ((info & kArgInfo::usePointer) ? data.void_ : (void*) &data);
  }

  kernelArg::kernelArg() {
    argc = 0;
  }

  kernelArg::~kernelArg() {}

  kernelArg::kernelArg(kernelArg_t &arg_) {
    argc = 1;

    args[0] = arg_;
  }

  kernelArg::kernelArg(const kernelArg &k) {
    argc = k.argc;

    args[0] = k.args[0];
    args[1] = k.args[1];
  }

  kernelArg& kernelArg::operator = (const kernelArg &k) {
    argc = k.argc;

    args[0] = k.args[0];
    args[1] = k.args[1];

    return *this;
  }

  template <> kernelArg::kernelArg(const int &arg_) {
    argc = 1; args[0].data.int_ = arg_; args[0].size = sizeof(int);
  }
  template <> kernelArg::kernelArg(const char &arg_) {
    argc = 1; args[0].data.char_ = arg_; args[0].size = sizeof(char);
  }
  template <> kernelArg::kernelArg(const short &arg_) {
    argc = 1; args[0].data.short_ = arg_; args[0].size = sizeof(short);
  }
  template <> kernelArg::kernelArg(const long &arg_) {
    argc = 1; args[0].data.long_ = arg_; args[0].size = sizeof(long);
  }

  template <> kernelArg::kernelArg(const unsigned int &arg_) {
    argc = 1; args[0].data.uint_ = arg_; args[0].size = sizeof(unsigned int);
  }
  template <> kernelArg::kernelArg(const unsigned char &arg_) {
    argc = 1; args[0].data.uchar_ = arg_; args[0].size = sizeof(unsigned char);
  }
  template <> kernelArg::kernelArg(const unsigned short &arg_) {
    argc = 1; args[0].data.ushort_ = arg_; args[0].size = sizeof(unsigned short);
  }

  template <> kernelArg::kernelArg(const float &arg_) {
    argc = 1; args[0].data.float_ = arg_; args[0].size = sizeof(float);
  }
  template <> kernelArg::kernelArg(const double &arg_) {
    argc = 1; args[0].data.double_ = arg_; args[0].size = sizeof(double);
  }

#if OCCA_64_BIT
  // 32 bit: uintptr_t == unsigned int
  template <> kernelArg::kernelArg(const uintptr_t &arg_) {
    argc = 1; args[0].data.uintptr_t_ = arg_; args[0].size = sizeof(uintptr_t);
  }
#endif

  occa::device kernelArg::getDevice() const {
    return occa::device(args[0].dHandle);
  }

  void kernelArg::setupForKernelCall(const bool isConst) const {
    occa::memory_v *mHandle = args[0].mHandle;

    if(mHandle                      &&
       mHandle->isManaged()         &&
       !mHandle->leftInDevice()     &&
       mHandle->dHandle->fakesUva() &&
       mHandle->dHandle->hasUvaEnabled()) {

      if(!mHandle->inDevice()) {
        mHandle->copyFrom(mHandle->uvaPtr);
        mHandle->memInfo |= uvaFlag::inDevice;
      }

      if(!isConst && !mHandle->isDirty()) {
        uvaDirtyMemory.push_back(mHandle);
        mHandle->memInfo |= uvaFlag::isDirty;
      }
    }
  }

  int kernelArg::argumentCount(const int kArgc, const kernelArg *kArgs) {
    int argc = 0;
    for(int i = 0; i < kArgc; ++i){
      argc += kArgs[i].argc;
    }
    return argc;
  }

  std::ostream& operator << (std::ostream &out, const argInfoMap &m) {
    std::map<std::string,std::string>::const_iterator it = m.iMap.begin();

    while(it != m.iMap.end()) {
      out << it->first << " = " << it->second << '\n';
      ++it;
    }

    return out;
  }

  //  |---[ Kernel Info ]-------------------------
  kernelInfo::kernelInfo() :
    mode(NoMode),
    header(""),
    flags("") {}

  kernelInfo::kernelInfo(const kernelInfo &p) :
    mode(p.mode),
    header(p.header),
    flags(p.flags) {}

  kernelInfo& kernelInfo::operator = (const kernelInfo &p) {
    mode   = p.mode;
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

  std::string kernelInfo::getModeHeaderFilename() const {
    if(mode & Serial)   return sys::getFilename("[occa]/defines/Serial.hpp");
    if(mode & OpenMP)   return sys::getFilename("[occa]/defines/OpenMP.hpp");
    if(mode & OpenCL)   return sys::getFilename("[occa]/defines/OpenCL.hpp");
    if(mode & CUDA)     return sys::getFilename("[occa]/defines/CUDA.hpp");
    if(mode & HSA)      return sys::getFilename("[occa]/defines/HSA.hpp");
    if(mode & Pthreads) return sys::getFilename("[occa]/defines/Pthreads.hpp");

    return "";
  }

  bool kernelInfo::isAnOccaDefine(const std::string &name) {
    if((name == "OCCA_USING_CPU") ||
       (name == "OCCA_USING_GPU") ||

       (name == "OCCA_USING_SERIAL")   ||
       (name == "OCCA_USING_OPENMP")   ||
       (name == "OCCA_USING_OPENCL")   ||
       (name == "OCCA_USING_CUDA")     ||
       (name == "OCCA_USING_PTHREADS") ||

       (name == "occaInnerDim0") ||
       (name == "occaInnerDim1") ||
       (name == "occaInnerDim2") ||

       (name == "occaOuterDim0") ||
       (name == "occaOuterDim1") ||
       (name == "occaOuterDim2"))
      return true;

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

  void kernelInfo::addSource(const std::string &content) {
    header += content;
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
    //  |=========================================

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

  //  |---[ Device Info ]-------------------------
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
  //==============================================


  //---[ Memory ]---------------------------------
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
      if(!isAsync)
        destMem->copyFrom(src, bytes, destOff);
      else
        destMem->asyncCopyFrom(src, bytes, destOff);
    }
    else if(usingDestPtr) {
      if(!isAsync)
        srcMem->copyTo(dest, bytes, srcOff);
      else
        srcMem->asyncCopyTo(dest, bytes, srcOff);
    }
    else {
      // Auto-detects peer-to-peer stuff
      occa::memory srcMemory(srcMem);
      occa::memory destMemory(destMem);

      if(!isAsync)
        srcMemory.copyTo(destMemory, bytes, destOff, srcOff);
      else
        srcMemory.asyncCopyTo(destMemory, bytes, destOff, srcOff);
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

    dest.asyncCopyFrom(src, bytes, offset);
  }

  void asyncMemcpy(void *dest,
                   memory src,
                   const uintptr_t bytes,
                   const uintptr_t offset) {

    src.asyncCopyTo(dest, bytes, offset);
  }

  void asyncMemcpy(memory dest,
                   memory src,
                   const uintptr_t bytes,
                   const uintptr_t destOffset,
                   const uintptr_t srcOffset) {

    src.asyncCopyTo(dest, bytes, destOffset, srcOffset);
  }
  //==============================================


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
      _host = device(new device_t<Serial>());
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

    if(deviceList.size()) {
      deviceListMutex.unlock();
      return deviceList;
    }

    device_t<Serial>::appendAvailableDevices(deviceList);

#if OCCA_OPENMP_ENABLED
    device_t<OpenMP>::appendAvailableDevices(deviceList);
#endif
#if OCCA_PTHREADS_ENABLED
    device_t<Pthreads>::appendAvailableDevices(deviceList);
#endif
#if OCCA_OPENCL_ENABLED
    device_t<OpenCL>::appendAvailableDevices(deviceList);
#endif
#if OCCA_CUDA_ENABLED
    device_t<CUDA>::appendAvailableDevices(deviceList);
#endif

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

  std::string& getCompiler() {
    return currentDevice.getCompiler();
  }

  std::string& getCompilerEnvScript() {
    return currentDevice.getCompilerEnvScript();
  }

  std::string& getCompilerFlags() {
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

  memory wrapTexture(void *handle_,
                     const int dim, const occa::dim &dims,
                     occa::formatType type, const int permissions) {

    return currentDevice.wrapTexture(handle_,
                                     dim, dims,
                                     type, permissions);
  }

  void wrapManagedTexture(void *handle_,
                          const int dim, const occa::dim &dims,
                          occa::formatType type, const int permissions) {

    currentDevice.wrapManagedTexture(handle_,
                                     dim, dims,
                                     type, permissions);
  }

  memory malloc(const uintptr_t bytes,
                void *src) {

    return currentDevice.malloc(bytes, src);
  }

  void* managedAlloc(const uintptr_t bytes,
                     void *src) {

    return currentDevice.managedAlloc(bytes, src);
  }

  memory textureAlloc(const int dim, const occa::dim &dims,
                      void *src,
                      occa::formatType type, const int permissions) {

    return currentDevice.textureAlloc(dim, dims,
                                      src,
                                      type, permissions);
  }

  void* managedTextureAlloc(const int dim, const occa::dim &dims,
                            void *src,
                            occa::formatType type, const int permissions) {

    return currentDevice.managedTextureAlloc(dim, dims,
                                             src,
                                             type, permissions);
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
  //==============================================
}
