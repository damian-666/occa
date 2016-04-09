#include "occa/device.hpp"

namespace occa {
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

  void device::setupHandle(occa::mode m) {
    switch(m) {

    case Serial:{
      dHandle = new device_t<Serial>();
      break;
    }
    case OpenMP:{
#if OCCA_OPENMP_ENABLED
      dHandle = new device_t<OpenMP>();
#else
      std::cout << "OCCA mode [OpenMP] is not enabled, defaulting to [Serial] mode\n";
      dHandle = new device_t<Serial>();
#endif
      break;
    }
    case OpenCL:{
#if OCCA_OPENCL_ENABLED
      dHandle = new device_t<OpenCL>();
#else
      std::cout << "OCCA mode [OpenCL] is not enabled, defaulting to [Serial] mode\n";
      dHandle = new device_t<Serial>();
#endif
      break;
    }
    case CUDA:{
#if OCCA_CUDA_ENABLED
      dHandle = new device_t<CUDA>();
#else
      std::cout << "OCCA mode [CUDA] is not enabled, defaulting to [Serial] mode\n";
      dHandle = new device_t<Serial>();
#endif
      break;
    }
    case Pthreads:{
      std::cout << "OCCA mode [Pthreads] is still in development-mode (unstable)\n";
      dHandle = new device_t<Pthreads>();
      break;
    }
    default:{
      std::cout << "Unsupported OCCA mode given, defaulting to [Serial] mode\n";
      dHandle = new device_t<Serial>();
    }
    }
  }

  void device::setupHandle(const std::string &m) {
    setupHandle( strToMode(m) );
  }

  void device::setup(deviceInfo &dInfo) {
    setup(dInfo.infos);
  }

  void device::setup(const std::string &infos) {
    argInfoMap aim(infos);

    OCCA_CHECK(aim.has("mode"),
               "OCCA mode not given");

    // Load [mode] from aim
    occa::mode m = strToMode(aim.get("mode"));

    setupHandle(m);

    dHandle->setup(aim);

    // [REFACTOR]
    dHandle->modelID_ = 0;
    dHandle->id_      = 0;

    if(aim.has("UVA")) {
      if(upStringCheck(aim.get("UVA"), "enabled"))
        dHandle->uvaEnabled_ = true;
      else
        dHandle->uvaEnabled_ = false;
    }
    else
      dHandle->uvaEnabled_ = uvaEnabledByDefault_f;

    stream newStream = createStream();
    dHandle->currentStream = newStream.handle;
  }

  void device::setup(occa::mode m,
                     const int arg1, const int arg2) {
    setupHandle(m);

    argInfoMap aim;

    switch(m) {
    case Serial:{
      // Do Nothing
      break;
    }
    case OpenMP:{
      // Do Nothing, maybe add thread order next, dynamic static, etc
      break;
    }
    case OpenCL:{
      aim.set("platformID", arg1);
      aim.set("deviceID"  , arg2);
      break;
    }
    case CUDA:{
      aim.set("deviceID", arg1);
      break;
    }
    case Pthreads:{
      aim.set("threadCount", arg1);
      aim.set("pinningInfo", arg2);
      break;
    }
    }

    dHandle->setup(aim);

    // [REFACTOR]
    dHandle->modelID_ = 0;
    dHandle->id_      = 0;

    stream newStream = createStream();
    dHandle->currentStream = newStream.handle;
  }


  void device::setup(const std::string &m,
                     const int arg1, const int arg2) {
    setup(strToMode(m), arg1, arg2);
  }

  uintptr_t device::memorySize() const {
    checkIfInitialized();
    return dHandle->memorySize();
  }

  uintptr_t device::memoryAllocated() const {
    checkIfInitialized();
    return dHandle->bytesAllocated;
  }

  void device::setCompiler(const std::string &compiler_) {
    checkIfInitialized();
    dHandle->setCompiler(compiler_);
  }

  void device::setCompilerEnvScript(const std::string &compilerEnvScript_) {
    checkIfInitialized();
    dHandle->setCompilerEnvScript(compilerEnvScript_);
  }

  void device::setCompilerFlags(const std::string &compilerFlags_) {
    checkIfInitialized();
    dHandle->setCompilerFlags(compilerFlags_);
  }

  std::string& device::getCompiler() {
    checkIfInitialized();
    return dHandle->compiler;
  }

  std::string& device::getCompilerEnvScript() {
    checkIfInitialized();
    return dHandle->compilerEnvScript;
  }

  std::string& device::getCompilerFlags() {
    checkIfInitialized();
    return dHandle->compilerFlags;
  }

  int device::modelID() {
    checkIfInitialized();
    return dHandle->modelID_;
  }

  int device::id() {
    checkIfInitialized();
    return dHandle->id_;
  }

  int device::modeID() {
    checkIfInitialized();
    return dHandle->mode();
  }

  const std::string& device::mode() {
    checkIfInitialized();
    return dHandle->strMode;
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

          mem->asyncCopyTo(mem->uvaPtr);

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
#if OCCA_OPENMP_ENABLED
      if(dHandle->mode() != OpenMP) {
        k          = new kernel_t<Serial>;
        k->dHandle = new device_t<Serial>;
      }
      else {
        k          = new kernel_t<OpenMP>;
        k->dHandle = dHandle;
      }
#else
      k          = new kernel_t<Serial>;
      k->dHandle = new device_t<Serial>;
#endif

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

  memory device::wrapTexture(void *handle_,
                             const int dim, const occa::dim &dims,
                             occa::formatType type, const int permissions) {
    checkIfInitialized();

    OCCA_CHECK((dim == 1) || (dim == 2),
               "Textures of [" << dim << "D] are not supported,"
               << "only 1D or 2D are supported at the moment");

    memory mem;
    mem.mHandle = dHandle->wrapTexture(handle_,
                                       dim, dims,
                                       type, permissions);
    mem.mHandle->dHandle = dHandle;

    return mem;
  }

  void device::wrapManagedTexture(void *handle_,
                                  const int dim, const occa::dim &dims,
                                  occa::formatType type, const int permissions) {
    checkIfInitialized();
    memory mem = wrapTexture(handle_, dim, dims, type, permissions);
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

  memory device::textureAlloc(const int dim, const occa::dim &dims,
                              void *src,
                              occa::formatType type, const int permissions) {
    checkIfInitialized();

    OCCA_CHECK((dim == 1) || (dim == 2),
               "Textures of [" << dim << "D] are not supported,"
               << "only 1D or 2D are supported at the moment");

    OCCA_CHECK(src != NULL,
               "Non-NULL source is required for [textureAlloc] (texture allocation)");

    memory mem;

    mem.mHandle      = dHandle->textureAlloc(dim, dims, src, type, permissions);
    mem.mHandle->dHandle = dHandle;

    dHandle->bytesAllocated += (type.bytes() *
                                ((dim == 2) ?
                                 (dims[0] * dims[1]) :
                                 (dims[0]          )));

    return mem;
  }

  void* device::managedTextureAlloc(const int dim, const occa::dim &dims,
                                    void *src,
                                    occa::formatType type, const int permissions) {
    checkIfInitialized();

    memory mem = textureAlloc(dim, dims, src, type, permissions);

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

  int device::simdWidth() {
    checkIfInitialized();

    return dHandle->simdWidth();
  }

  void stream::free() {
    if(dHandle == NULL)
      return;

    device(dHandle).freeStream(*this);
  }
};