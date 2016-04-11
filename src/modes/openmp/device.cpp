#include "occa/defines.hpp"

#if OCCA_OPENMP_ENABLED

#include "occa/Serial.hpp"
#include "occa/OpenMP.hpp"

#include <omp.h>


namespace occa {
  namespace openmp {
    device_t<OpenMP>::device_t(){
      strMode = "OpenMP";

      data = NULL;

      uvaEnabled_ = false;

      bytesAllocated = 0;

      getEnvironmentVariables();

      sys::addSharedBinaryFlagsTo(compiler, compilerFlags);
    }

    device_t<OpenMP>::device_t(const device_t<OpenMP> &d){
      *this = d;
    }

    device_t<OpenMP>& device_t<OpenMP>::operator = (const device_t<OpenMP> &d){
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

    void* device_t<OpenMP>::getContextHandle(){
      return NULL;
    }

    void device_t<OpenMP>::setup(argInfoMap &aim){
      properties = aim;

      // Generate an OpenMP library dependency (so it doesn't crash when dlclose())
      omp_get_num_threads();

      data = new OpenMPDeviceData_t;

      OCCA_EXTRACT_DATA(OpenMP, Device);

      data_.vendor         = sys::compilerVendor(compiler);
      data_.OpenMPFlag     = omp::compilerFlag(data_.vendor, compiler);
      data_.supportsOpenMP = (data_.OpenMPFlag != omp::notSupported);

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device_t<OpenMP>::addOccaHeadersToInfo(kernelInfo &info_){
      info_.mode = OpenMP;
    }

    std::string device_t<OpenMP>::getInfoSalt(const kernelInfo &info_){
      std::stringstream salt;

      salt << "OpenMP"
           << info_.salt()
           << parserVersion
           << compilerEnvScript
           << compiler
           << compilerFlags;

      return salt.str();
    }

    deviceIdentifier device_t<OpenMP>::getIdentifier() const {
      deviceIdentifier dID;

      dID.mode_ = OpenMP;

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

    void device_t<OpenMP>::getEnvironmentVariables(){
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
      compilerFlags = " /Od /openmp";
#  else
      compilerFlags = " /O2 /openmp";
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

    void device_t<OpenMP>::appendAvailableDevices(std::vector<device> &dList){
      device d;
      d.setup("OpenMP");

      dList.push_back(d);
    }

    void device_t<OpenMP>::setCompiler(const std::string &compiler_){
      compiler = compiler_;

      OCCA_EXTRACT_DATA(OpenMP, Device);

      data_.vendor         = sys::compilerVendor(compiler);
      data_.OpenMPFlag     = omp::compilerFlag(data_.vendor, compiler);
      data_.supportsOpenMP = (data_.OpenMPFlag != omp::notSupported);

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device_t<OpenMP>::setCompilerEnvScript(const std::string &compilerEnvScript_){
      compilerEnvScript = compilerEnvScript_;
    }

    void device_t<OpenMP>::setCompilerFlags(const std::string &compilerFlags_){
      OCCA_EXTRACT_DATA(OpenMP, Device);

      compilerFlags = compilerFlags_;

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device_t<OpenMP>::flush(){}

    void device_t<OpenMP>::finish(){}

    bool device_t<OpenMP>::fakesUva(){
      return false;
    }

    void device_t<OpenMP>::waitFor(streamTag tag){}

    stream_t device_t<OpenMP>::createStream(){
      return NULL;
    }

    void device_t<OpenMP>::freeStream(stream_t s){}

    stream_t device_t<OpenMP>::wrapStream(void *handle_){
      return NULL;
    }

    streamTag device_t<OpenMP>::tagStream(){
      streamTag ret;

      ret.tagTime = currentTime();

      return ret;
    }

    double device_t<OpenMP>::timeBetween(const streamTag &startTag, const streamTag &endTag){
      return (endTag.tagTime - startTag.tagTime);
    }

    std::string device_t<OpenMP>::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    kernel_v* device_t<OpenMP>::buildKernelFromSource(const std::string &filename,
                                                      const std::string &functionName,
                                                      const kernelInfo &info_){
      OCCA_EXTRACT_DATA(OpenMP, Device);

      kernel_v *k;

      if(data_.supportsOpenMP){
        k = new kernel_t<OpenMP>;
      }
      else{
        std::cout << "Compiler [" << compiler << "] does not support OpenMP, defaulting to [Serial] mode\n";
        k = new kernel_t<Serial>;
      }

      k->dHandle = this;

      k->buildFromSource(filename, functionName, info_);

      return k;
    }

    kernel_v* device_t<OpenMP>::buildKernelFromBinary(const std::string &filename,
                                                      const std::string &functionName){
      OCCA_EXTRACT_DATA(OpenMP, Device);

      kernel_v *k;

      if(data_.supportsOpenMP){
        k = new kernel_t<OpenMP>;
      }
      else{
        std::cout << "Compiler [" << compiler << "] does not support OpenMP, defaulting to [Serial] mode\n";
        k = new kernel_t<Serial>;
      }

      k->dHandle = this;

      k->buildFromBinary(filename, functionName);

      return k;
    }

    void device_t<OpenMP>::cacheKernelInLibrary(const std::string &filename,
                                                const std::string &functionName,
                                                const kernelInfo &info_){
#if 0
      //---[ Creating shared library ]----
      kernel tmpK = occa::device(this).buildKernelFromSource(filename, functionName, info_);
      tmpK.free();

      kernelInfo info = info_;

      addOccaHeadersToInfo(info);

      std::string cachedBinary = getCachedName(filename, getInfoSalt(info));

#if (OCCA_OS == WINDOWS_OS)
      // Windows requires .dll extension
      cachedBinary = cachedBinary + ".dll";
#endif
      //==================================

      library::infoID_t infoID;

      infoID.modelID    = modelID_;
      infoID.kernelName = functionName;

      library::infoHeader_t &header = library::headerMap[infoID];

      header.fileID = -1;
      header.mode   = OpenMP;

      const std::string flatDevID = getIdentifier().flattenFlagMap();

      header.flagsOffset = library::addToScratchPad(flatDevID);
      header.flagsBytes  = flatDevID.size();

      header.contentOffset = library::addToScratchPad(cachedBinary);
      header.contentBytes  = cachedBinary.size();

      header.kernelNameOffset = library::addToScratchPad(functionName);
      header.kernelNameBytes  = functionName.size();
#endif
    }

    kernel_v* device_t<OpenMP>::loadKernelFromLibrary(const char *cache,
                                                      const std::string &functionName){
#if 0
      kernel_v *k = new kernel_t<OpenMP>;
      k->dHandle = this;
      k->loadFromLibrary(cache, functionName);
      return k;
#endif
      return NULL;
    }

    memory_v* device_t<OpenMP>::wrapMemory(void *handle_,
                                           const uintptr_t bytes){
      memory_v *mem = new memory_t<OpenMP>;

      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = handle_;

      mem->memInfo |= memFlag::isAWrapper;

      return mem;
    }

    memory_v* device_t<OpenMP>::wrapTexture(void *handle_,
                                            const int dim, const occa::dim &dims,
                                            occa::formatType type, const int permissions){
      memory_v *mem = new memory_t<OpenMP>;

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

    memory_v* device_t<OpenMP>::malloc(const uintptr_t bytes,
                                       void *src){
      memory_v *mem = new memory_t<OpenMP>;

      mem->dHandle = this;
      mem->size    = bytes;

      mem->handle = sys::malloc(bytes);

      if(src != NULL)
        ::memcpy(mem->handle, src, bytes);

      return mem;
    }

    memory_v* device_t<OpenMP>::textureAlloc(const int dim, const occa::dim &dims,
                                             void *src,
                                             occa::formatType type, const int permissions){
      memory_v *mem = new memory_t<OpenMP>;

      mem->dHandle = this;
      mem->size    = ((dim == 1) ? dims.x : (dims.x * dims.y)) * type.bytes();

      mem->memInfo |= memFlag::isATexture;

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->handle = sys::malloc(mem->size);

      ::memcpy(mem->textureInfo.arg, src, mem->size);

      mem->handle = &(mem->textureInfo);

      return mem;
    }

    memory_v* device_t<OpenMP>::mappedAlloc(const uintptr_t bytes,
                                            void *src){
      memory_v *mem = malloc(bytes, src);

      mem->mappedPtr = mem->handle;

      return mem;
    }

    uintptr_t device_t<OpenMP>::memorySize(){
      return sys::installedRAM();
    }

    void device_t<OpenMP>::free(){}

    int device_t<OpenMP>::simdWidth(){
      simdWidth_ = OCCA_SIMD_WIDTH;
      return OCCA_SIMD_WIDTH;
    }
  }
}

#endif
