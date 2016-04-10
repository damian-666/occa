#include "occa/device.hpp"
#include "occa/base.hpp"
#include "occa/parser/parser.hpp"

namespace occa {
  //---[ argInfoMap ]-------------------
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

  std::string& argInfoMap::operator [] (const std::string &info) {
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
  //====================================

  //---[ device_v ]---------------------
  device_v::device_v(){}
  //====================================

  //---[ device ]-----------------------
  device::device() {
    dHandle = NULL;
  }

  device::device(device_v *dHandle_) :
    dHandle(dHandle_) {}

  device::device(deviceInfo &dInfo) {
    setup(dInfo);
  }

  device::device(const std::string &infos) {
    setup(infos);
  }

  device::device(const device &d) :
    dHandle(d.dHandle) {}

  device& device::operator = (const device &d) {
    dHandle = d.dHandle;

    return *this;
  }

  void* device::getContextHandle() {
    return dHandle->getContextHandle();
  }

  device_v* device::getDHandle() {
    return dHandle;
  }

  void device::setup(deviceInfo &dInfo) {
    setup(dInfo.infos);
  }

  void device::setup(const std::string &infos) {
    argInfoMap aim(infos);

    OCCA_CHECK(aim.has("mode"),
               "OCCA mode not given");

    dHandle = occa::newModeDevice(aim["mode"]);
    dHandle->setup(aim);

    if(aim.has("UVA")) {
      if(upStringCheck(aim["UVA"], "enabled"))
        dHandle->uvaEnabled_ = true;
      else
        dHandle->uvaEnabled_ = false;
    }
    else
      dHandle->uvaEnabled_ = uvaEnabledByDefault_f;

    stream newStream = createStream();
    dHandle->currentStream = newStream.handle;
  }

  uintptr_t device::memorySize() const {
    checkIfInitialized();
    return dHandle->memorySize();
  }

  uintptr_t device::memoryAllocated() const {
    checkIfInitialized();
    return dHandle->bytesAllocated;
  }

  const std::string& device::mode() {
    checkIfInitialized();
    return dHandle->properties["mode"];
  }

  void device::setCompiler(const std::string &compiler_) {
    setProperty("compiler", compiler_);
  }

  void device::setCompilerEnvScript(const std::string &compilerEnvScript_) {
    setProperty("compilerEnvScript", compilerEnvScript_);
  }

  void device::setCompilerFlags(const std::string &compilerFlags_) {
    setProperty("compilerFlags", compilerFlags_);
  }

  std::string device::getCompiler() {
    return getProperty<std::string>("compiler");
  }

  std::string device::getCompilerEnvScript() {
    return getProperty<std::string>("compilerEnvScript");
  }

  std::string device::getCompilerFlags() {
    return getProperty<std::string>("compilerFlags");
  }

  void device::flush() {
    checkIfInitialized();
    dHandle->flush();
  }

  void device::finish() {
    checkIfInitialized();

    if(dHandle->fakesUva()) {
      const size_t dirtyEntries = uvaDirtyMemory.size();

      if(dirtyEntries) {
        for(size_t i = 0; i < dirtyEntries; ++i) {
          occa::memory_v *mem = uvaDirtyMemory[i];

          mem->copyTo(mem->uvaPtr, 0, 0, true);

          mem->memInfo &= ~uvaFlag::inDevice;
          mem->memInfo &= ~uvaFlag::isDirty;
        }

        uvaDirtyMemory.clear();
      }
    }

    dHandle->finish();
  }

  void device::waitFor(streamTag tag) {
    checkIfInitialized();
    dHandle->waitFor(tag);
  }

  stream device::createStream() {
    checkIfInitialized();

    stream newStream(dHandle, dHandle->createStream());

    dHandle->streams.push_back(newStream.handle);

    return newStream;
  }

  stream device::getStream() {
    checkIfInitialized();
    return stream(dHandle, dHandle->currentStream);
  }

  void device::setStream(stream s) {
    checkIfInitialized();
    dHandle->currentStream = s.handle;
  }

  stream device::wrapStream(void *handle_) {
    checkIfInitialized();
    return stream(dHandle, dHandle->wrapStream(handle_));
  }

  streamTag device::tagStream() {
    checkIfInitialized();
    return dHandle->tagStream();
  }

  double device::timeBetween(const streamTag &startTag, const streamTag &endTag) {
    checkIfInitialized();
    return dHandle->timeBetween(startTag, endTag);
  }

  void device::freeStream(stream s) {
    checkIfInitialized();

    const int streamCount = dHandle->streams.size();

    for(int i = 0; i < streamCount; ++i) {
      if(dHandle->streams[i] == s.handle) {
        if(dHandle->currentStream == s.handle)
          dHandle->currentStream = NULL;

        dHandle->freeStream(dHandle->streams[i]);
        dHandle->streams.erase(dHandle->streams.begin() + i);

        break;
      }
    }
  }

  kernel device::buildKernel(const std::string &str,
                             const std::string &functionName,
                             const kernelInfo &info_) {
    checkIfInitialized();

    if(sys::fileExists(str, flags::checkCacheDir))
      return buildKernelFromSource(str, functionName, info_);
    else
      return buildKernelFromString(str, functionName, info_);
  }

  kernel device::buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const int language) {
    checkIfInitialized();

    return buildKernelFromString(content,
                                 functionName,
                                 defaultKernelInfo,
                                 language);
  }

  kernel device::buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const kernelInfo &info_,
                                       const int language) {
    checkIfInitialized();

    kernelInfo info = info_;

    dHandle->addOccaHeadersToInfo(info);

    const std::string hash = getContentHash(content,
                                            dHandle->getInfoSalt(info));

    const std::string hashDir = hashDirFor("", hash);

    std::string stringSourceFile = hashDir;

    if(language & occa::usingOKL)
      stringSourceFile += "stringSource.okl";
    else if(language & occa::usingOFL)
      stringSourceFile += "stringSource.ofl";
    else
      stringSourceFile += "stringSource.occa";

    if(!haveHash(hash, 1)) {
      waitForHash(hash, 1);

      return buildKernelFromBinary(hashDir +
                                   dHandle->fixBinaryName(kc::binaryFile),
                                   functionName);
    }

    writeToFile(stringSourceFile, content);

    kernel k = buildKernelFromSource(stringSourceFile,
                                     functionName,
                                     info_);

    releaseHash(hash, 1);

    return k;
  }

  kernel device::buildKernelFromSource(const std::string &filename,
                                       const std::string &functionName,
                                       const kernelInfo &info_) {
    checkIfInitialized();

    const std::string realFilename = sys::getFilename(filename);
    const bool usingParser         = fileNeedsParser(filename);

    kernel ker;

    kernel_v *&k = ker.kHandle;

    if(usingParser) {
      if (mode() != "OpenMP") {
        k          = newModeKernel("Serial");
        k->dHandle = newModeDevice("Serial");
      }
      else {
        k          = newModeKernel("OpenMP");
        k->dHandle = dHandle;
      }

      const std::string hash = getFileContentHash(realFilename,
                                                  dHandle->getInfoSalt(info_));

      const std::string hashDir    = hashDirFor(realFilename, hash);
      const std::string parsedFile = hashDir + "parsedSource.occa";

      k->metaInfo = parseFileForFunction(mode(),
                                         realFilename,
                                         parsedFile,
                                         functionName,
                                         info_);

      kernelInfo info = defaultKernelInfo;
      info.addDefine("OCCA_LAUNCH_KERNEL", 1);

      k->buildFromSource(parsedFile, functionName, info);
      k->nestedKernels.clear();

      if (k->metaInfo.nestedKernels) {
        std::stringstream ss;

        const int vc_f = verboseCompilation_f;

        for(int ki = 0; ki < k->metaInfo.nestedKernels; ++ki) {
          ss << ki;

          const std::string sKerName = k->metaInfo.baseName + ss.str();

          ss.str("");

          kernel sKer;
          sKer.kHandle = dHandle->buildKernelFromSource(parsedFile,
                                                        sKerName,
                                                        info_);

          sKer.kHandle->metaInfo               = k->metaInfo;
          sKer.kHandle->metaInfo.name          = sKerName;
          sKer.kHandle->metaInfo.nestedKernels = 0;
          sKer.kHandle->metaInfo.removeArg(0); // remove nestedKernels **
          k->nestedKernels.push_back(sKer);

          // Only show compilation the first time
          if(ki == 0)
            verboseCompilation_f = false;
        }

        verboseCompilation_f = vc_f;
      }
    }
    else{
      k = dHandle->buildKernelFromSource(realFilename,
                                         functionName,
                                         info_);
      k->dHandle = dHandle;
    }

    return ker;
  }

  kernel device::buildKernelFromBinary(const std::string &filename,
                                       const std::string &functionName) {
    checkIfInitialized();

    kernel ker;
    ker.kHandle = dHandle->buildKernelFromBinary(filename, functionName);
    ker.kHandle->dHandle = dHandle;

    return ker;
  }

  memory device::wrapMemory(void *handle_,
                            const uintptr_t bytes) {
    checkIfInitialized();

    memory mem;
    mem.mHandle = dHandle->wrapMemory(handle_, bytes);
    mem.mHandle->dHandle = dHandle;

    return mem;
  }

  void device::wrapManagedMemory(void *handle_,
                                 const uintptr_t bytes) {
    checkIfInitialized();
    memory mem = wrapMemory(handle_, bytes);
    mem.manage();
  }

  memory device::malloc(const uintptr_t bytes,
                        void *src) {
    checkIfInitialized();

    memory mem;
    mem.mHandle          = dHandle->malloc(bytes, src);
    mem.mHandle->dHandle = dHandle;

    dHandle->bytesAllocated += bytes;

    return mem;
  }

  void* device::managedAlloc(const uintptr_t bytes,
                             void *src) {
    checkIfInitialized();

    memory mem = malloc(bytes, src);
    mem.manage();

    return mem.mHandle->uvaPtr;
  }

  memory device::mappedAlloc(const uintptr_t bytes,
                             void *src) {
    checkIfInitialized();

    memory mem;

    mem.mHandle          = dHandle->mappedAlloc(bytes, src);
    mem.mHandle->dHandle = dHandle;

    dHandle->bytesAllocated += bytes;

    return mem;
  }

  void* device::managedMappedAlloc(const uintptr_t bytes,
                                   void *src) {
    checkIfInitialized();

    memory mem = mappedAlloc(bytes, src);

    mem.manage();

    return mem.mHandle->uvaPtr;
  }

  void device::free() {
    checkIfInitialized();

    const int streamCount = dHandle->streams.size();

    for(int i = 0; i < streamCount; ++i)
      dHandle->freeStream(dHandle->streams[i]);

    dHandle->free();

    delete dHandle;
    dHandle = NULL;
  }
  //====================================

  //---[ stream ]-----------------------
  void stream::free() {
    if(dHandle == NULL)
      return;

    device(dHandle).freeStream(*this);
  }
  //====================================
}
