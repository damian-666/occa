#include "occa/Serial.hpp"
#include "occa/Pthreads.hpp"

namespace occa {
  namespace threads {
    device_t<Pthreads>::device_t(){
      strMode = "Pthreads";

      data = NULL;

      uvaEnabled_ = false;

      bytesAllocated = 0;

      getEnvironmentVariables();

      sys::addSharedBinaryFlagsTo(compiler, compilerFlags);
    }

    device_t<Pthreads>::device_t(const device_t<Pthreads> &d){
      *this = d;
    }

    device_t<Pthreads>& device_t<Pthreads>::operator = (const device_t<Pthreads> &d){
      modelID_ = d.modelID_;
      id_      = d.id_;

      data = d.data;

      uvaEnabled_    = d.uvaEnabled_;
      uvaMap         = d.uvaMap;
      uvaDirtyMemory = d.uvaDirtyMemory;

      compiler      = d.compiler;
      compilerFlags = d.compilerFlags;

      bytesAllocated = d.bytesAllocated;

      return *this;
    }

    void* device_t<Pthreads>::getContextHandle(){
      return NULL;
    }

    void device_t<Pthreads>::setup(argInfoMap &aim){
      properties = aim;

      data = new PthreadsDeviceData_t;

      OCCA_EXTRACT_DATA(Pthreads, Device);

      data_.vendor = sys::compilerVendor(compiler);

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);

      data_.pendingJobs = 0;

      data_.coreCount = sys::getCoreCount();

      std::vector<int> pinnedCores;

      if(!aim.has("threadCount"))
        data_.pThreadCount = 1;
      else
        data_.pThreadCount = aim.iGet("threadCount");

      if(!aim.has("schedule") ||
         (aim.get("schedule") == "compact")){

        data_.schedule = occa::compact;
      }
      else{
        data_.schedule = occa::scatter;
      }

      if(aim.has("pinnedCores")){
        aim.iGets("pinnedCores", pinnedCores);

        if(pinnedCores.size() != (size_t) data_.pThreadCount){
          std::cout << "[Pthreads]: Mismatch between thread count and pinned cores\n"
                    << "            Defaulting to ["
                    << ((data_.schedule == occa::compact) ?
                        "compact" : "scatter")
                    << "] scheduling\n"
                    << "  Thread Count: " << data_.pThreadCount << '\n'
                    << "  Pinned Cores: [";

          if(pinnedCores.size()){
            std::cout << pinnedCores[0];

            for(size_t i = 1; i < pinnedCores.size(); ++i)
              std::cout << ", " << pinnedCores[i];
          }

          std::cout << "]\n";

          pinnedCores.clear();
        }
        else{
          for(size_t i = 0; i < pinnedCores.size(); ++i)
            if(pinnedCores[i] < 0){
              const int newPC = (((pinnedCores[i] % data_.coreCount)
                                  + pinnedCores[i]) % data_.coreCount);

              std::cout << "Trying to pin thread on core ["
                        << pinnedCores[i] << "], changing it to ["
                        << newPC << "]\n";

              pinnedCores[i] = newPC;
            }
            else if(data_.coreCount <= pinnedCores[i]){
              const int newPC = (pinnedCores[i] % data_.coreCount);

              std::cout << "Trying to pin thread on core ["
                        << pinnedCores[i] << "], changing it to ["
                        << newPC << "]\n";

              pinnedCores[i] = newPC;
            }

          data_.schedule = occa::manual;
        }
      }

      for(int p = 0; p < data_.pThreadCount; ++p){
        PthreadWorkerData_t *args = new PthreadWorkerData_t;

        args->rank  = p;
        args->count = data_.pThreadCount;

        // [-] Need to know number of sockets
        if(data_.schedule & occa::compact)
          args->pinnedCore = (p % data_.coreCount);
        else if(data_.schedule & occa::scatter)
          args->pinnedCore = (p % data_.coreCount);
        else // Manual
          args->pinnedCore = pinnedCores[p];

        args->pendingJobs = &(data_.pendingJobs);

        args->pendingJobsMutex = &(data_.pendingJobsMutex);
        args->kernelMutex      = &(data_.kernelMutex);

        args->pKernelInfo = &(data_.pKernelInfo[p]);

#if (OCCA_OS & (LINUX_OS | OSX_OS))
        pthread_create(&data_.tid[p], NULL, pthreads::limbo, args);
#else
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) pthreads::limbo, args, 0, &data_.tid[p]);
#endif
      }
    }

    void device_t<Pthreads>::addOccaHeadersToInfo(kernelInfo &info_){
      info_.mode = Pthreads;
    }

    std::string device_t<Pthreads>::getInfoSalt(const kernelInfo &info_){
      std::stringstream salt;

      salt << "Pthreads"
           << info_.salt()
           << parserVersion
           << compilerEnvScript
           << compiler
           << compilerFlags;

      return salt.str();
    }

    deviceIdentifier device_t<Pthreads>::getIdentifier() const {
      deviceIdentifier dID;

      dID.mode_ = Pthreads;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      const bool debugEnabled = (compilerFlags.find("-g") != std::string::npos);
#else
      const bool debugEnabled = (compilerFlags.find("/Od") != std::string::npos);
#endif

      dID.flagMap["compiler"]     = compiler;
      dID.flagMap["debugEnabled"] = (debugEnabled ? "true" : "false");

      for(int i = 0; i <= 3; ++i){
        std::string flag = "-O";
        flag += '0' + i;

        if(compilerFlags.find(flag) != std::string::npos){
          dID.flagMap["optimization"] = '0' + i;
          break;
        }

        if(i == 3)
          dID.flagMap["optimization"] = "None";
      }

      return dID;
    }

    void device_t<Pthreads>::getEnvironmentVariables(){
      char *c_compiler = getenv("OCCA_CXX");

      if(c_compiler != NULL){
        compiler = std::string(c_compiler);
      }
      else{
        c_compiler = getenv("CXX");

        if(c_compiler != NULL){
          compiler = std::string(c_compiler);
        }
        else{
#if (OCCA_OS & (LINUX_OS | OSX_OS))
          compiler = "g++";
#else
          compiler = "cl.exe";
#endif
        }
      }

      char *c_compilerFlags = getenv("OCCA_CXXFLAGS");

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      if(c_compilerFlags != NULL)
        compilerFlags = std::string(c_compilerFlags);
      else{
#  if OCCA_DEBUG_ENABLED
        compilerFlags = "-g";
#  else
        compilerFlags = "";
#  endif
      }
#else
#  if OCCA_DEBUG_ENABLED
      compilerFlags = " /Od";
#  else
      compilerFlags = " /O2";
#  endif

      std::string byteness;

      if(sizeof(void*) == 4)
        byteness = "x86 ";
      else if(sizeof(void*) == 8)
        byteness = "amd64";
      else
        OCCA_CHECK(false, "sizeof(void*) is not equal to 4 or 8");

#  if      (OCCA_VS_VERSION == 1800)
      char *visualStudioTools = getenv("VS120COMNTOOLS");   // MSVC++ 12.0 - Visual Studio 2013
#  elif    (OCCA_VS_VERSION == 1700)
      char *visualStudioTools = getenv("VS110COMNTOOLS");   // MSVC++ 11.0 - Visual Studio 2012
#  else // (OCCA_VS_VERSION == 1600)
      char *visualStudioTools = getenv("VS100COMNTOOLS");   // MSVC++ 10.0 - Visual Studio 2010
#  endif

      if(visualStudioTools != NULL){
        setCompilerEnvScript("\"" + std::string(visualStudioTools) + "..\\..\\VC\\vcvarsall.bat\" " + byteness);
      }
      else{
        std::cout << "WARNING: Visual Studio environment variable not found -> compiler environment (vcvarsall.bat) maybe not correctly setup." << std::endl;
      }
#endif
    }

    void device_t<Pthreads>::appendAvailableDevices(std::vector<device> &dList){
      device d;

      d.setup("Pthreads", sys::getCoreCount(), occa::compact);

      dList.push_back(d);
    }

    void device_t<Pthreads>::setCompiler(const std::string &compiler_){
      compiler = compiler_;

      OCCA_EXTRACT_DATA(Pthreads, Device);

      data_.vendor = sys::compilerVendor(compiler);

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device_t<Pthreads>::setCompilerEnvScript(const std::string &compilerEnvScript_){
      compilerEnvScript = compilerEnvScript_;
    }

    void device_t<Pthreads>::setCompilerFlags(const std::string &compilerFlags_){
      OCCA_EXTRACT_DATA(Pthreads, Device);

      compilerFlags = compilerFlags_;

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device_t<Pthreads>::flush(){}

    void device_t<Pthreads>::finish(){
      OCCA_EXTRACT_DATA(Pthreads, Device);

      // Fence local data (incase of out-of-socket updates)
      while(data_.pendingJobs){
        OCCA_LFENCE;
      }
    }

    bool device_t<Pthreads>::fakesUva(){
      return false;
    }

    void device_t<Pthreads>::waitFor(streamTag tag){
      finish(); // [-] Not done
    }

    stream_t device_t<Pthreads>::createStream(){
      return NULL;
    }

    void device_t<Pthreads>::freeStream(stream_t s){}

    stream_t device_t<Pthreads>::wrapStream(void *handle_){
      return NULL;
    }

    streamTag device_t<Pthreads>::tagStream(){
      streamTag ret;

      ret.tagTime = currentTime();

      return ret;
    }

    double device_t<Pthreads>::timeBetween(const streamTag &startTag, const streamTag &endTag){
      return (endTag.tagTime - startTag.tagTime);
    }

    std::string device_t<Pthreads>::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    kernel_v* device_t<Pthreads>::buildKernelFromSource(const std::string &filename,
                                                        const std::string &functionName,
                                                        const kernelInfo &info_){
      kernel_v *k = new kernel_t<Pthreads>;
      k->dHandle  = this;

      k->buildFromSource(filename, functionName, info_);

      return k;
    }

    kernel_v* device_t<Pthreads>::buildKernelFromBinary(const std::string &filename,
                                                        const std::string &functionName){
      kernel_v *k = new kernel_t<Pthreads>;
      k->dHandle  = this;
      k->buildFromBinary(filename, functionName);
      return k;
    }

    void device_t<Pthreads>::cacheKernelInLibrary(const std::string &filename,
                                                  const std::string &functionName,
                                                  const kernelInfo &info_){
#if 0
      //---[ Creating shared library ]----
      kernel tmpK = occa::device(this).buildKernelFromSource(filename, functionName, info_);
      tmpK.free();

      kernelInfo info = info_;

      addOccaHeadersToInfo(info);

      std::string cachedBinary = getCachedName(filename, getInfoSalt(info));

#if (OCCA_OS & WINDOWS_OS)
      // Windows requires .dll extension
      cachedBinary = cachedBinary + ".dll";
#endif
      //==================================

      library::infoID_t infoID;

      infoID.modelID    = modelID_;
      infoID.kernelName = functionName;

      library::infoHeader_t &header = library::headerMap[infoID];

      header.fileID = -1;
      header.mode   = Pthreads;

      const std::string flatDevID = getIdentifier().flattenFlagMap();

      header.flagsOffset = library::addToScratchPad(flatDevID);
      header.flagsBytes  = flatDevID.size();

      header.contentOffset = library::addToScratchPad(cachedBinary);
      header.contentBytes  = cachedBinary.size();

      header.kernelNameOffset = library::addToScratchPad(functionName);
      header.kernelNameBytes  = functionName.size();
#endif
    }

    kernel_v* device_t<Pthreads>::loadKernelFromLibrary(const char *cache,
                                                        const std::string &functionName){
      kernel_v *k = new kernel_t<Pthreads>;
      k->dHandle  = this;
      k->loadFromLibrary(cache, functionName);
      return k;
    }

    memory_v* device_t<Pthreads>::wrapMemory(void *handle_,
                                             const uintptr_t bytes){
      memory_v *mem = new memory_t<Pthreads>;

      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = handle_;

      mem->memInfo |= memFlag::isAWrapper;

      return mem;
    }

    memory_v* device_t<Pthreads>::wrapTexture(void *handle_,
                                              const int dim, const occa::dim &dims,
                                              occa::formatType type, const int permissions){
      memory_v *mem = new memory_t<Pthreads>;

      mem->dHandle = this;
      mem->size    = ((dim == 1) ? dims.x : (dims.x * dims.y)) * type.bytes();

      mem->memInfo |= (memFlag::isATexture |
                       memFlag::isAWrapper);

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->textureInfo.arg = handle_;

      mem->handle = &(mem->textureInfo);

      return mem;
    }

    memory_v* device_t<Pthreads>::malloc(const uintptr_t bytes,
                                         void *src){
      memory_v *mem = new memory_t<Pthreads>;

      mem->dHandle = this;
      mem->size    = bytes;

      mem->handle = sys::malloc(bytes);

      if(src != NULL)
        ::memcpy(mem->handle, src, bytes);

      return mem;
    }

    memory_v* device_t<Pthreads>::textureAlloc(const int dim, const occa::dim &dims,
                                               void *src,
                                               occa::formatType type, const int permissions){
      memory_v *mem = new memory_t<Pthreads>;

      mem->dHandle = this;
      mem->size    = ((dim == 1) ? dims.x : (dims.x * dims.y)) * type.bytes();

      mem->memInfo |= memFlag::isATexture;

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->handle = sys::malloc(mem->size);

      ::memcpy(mem->handle, src, mem->size);

      mem->textureInfo.arg = mem->handle;
      mem->handle = &(mem->textureInfo);

      return mem;
    }

    memory_v* device_t<Pthreads>::mappedAlloc(const uintptr_t bytes,
                                              void *src){
      memory_v *mem = malloc(bytes, src);

      mem->mappedPtr = mem->handle;

      return mem;
    }

    uintptr_t device_t<Pthreads>::memorySize(){
      return sys::installedRAM();
    }

    void device_t<Pthreads>::free(){
      finish();

      OCCA_EXTRACT_DATA(Pthreads, Device);

      data_.pendingJobsMutex.free();
      data_.kernelMutex.free();

      delete (PthreadsDeviceData_t*) data;
    }

    int device_t<Pthreads>::simdWidth(){
      simdWidth_ = OCCA_SIMD_WIDTH;
      return OCCA_SIMD_WIDTH;
    }
  }
}
