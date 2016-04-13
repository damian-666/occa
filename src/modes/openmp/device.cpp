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

#include "occa/defines.hpp"

#if OCCA_OPENMP_ENABLED

#include "occa/Serial.hpp"
#include "occa/OpenMP.hpp"

#include <omp.h>


namespace occa {
  namespace openmp {
    device::device() : occa::device_v() {
      getEnvironmentVariables();
      sys::addSharedBinaryFlagsTo(compiler, compilerFlags);
    }

    device::device(const device &d){
      *this = d;
    }

    device& device::operator = (const device &d){
      initFrom(d);

      vendor = d.vendor;
      compiler = d.compiler;
      compilerFlags = d.compilerFlags;
      compilerEnvScript = d.compilerEnvScript;

      return *this;
    }

    device::~device(){}

    void* device::getContextHandle(){
      return NULL;
    }

    void device::setup(argInfoMap &aim){
      properties = aim;

      // Generate an OpenMP library dependency (so it doesn't crash when dlclose())
      omp_get_num_threads();

      vendor         = sys::compilerVendor(compiler);
      ompFlag        = omp::compilerFlag(data_.vendor, compiler);
      supportsOpenMP = (data_.OpenMPFlag != omp::notSupported);

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    // [REFACTOR]
    void device::addOccaHeadersToInfo(kernelInfo &info_){
    }

    std::string device::getInfoSalt(const kernelInfo &info_){
      std::stringstream salt;

      salt << "OpenMP"
           << info_.salt()
           << parserVersion
           << compilerEnvScript
           << compiler
           << compilerFlags;

      return salt.str();
    }

    deviceIdentifier device::getIdentifier() const {
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

    void device::getEnvironmentVariables(){
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

      properties["compiler"]          = compiler;
      properties["compilerFlags"]     = compilerFlags;
      properties["compilerEnvScript"] = compilerEnvScript;
    }

    void device::appendAvailableDevices(std::vector<device> &dList){
      device d;
      d.setup("OpenMP");

      dList.push_back(d);
    }

    void device::setCompiler(const std::string &compiler_){
      compiler = compiler_;

      OCCA_EXTRACT_DATA(OpenMP, Device);

      data_.vendor         = sys::compilerVendor(compiler);
      data_.OpenMPFlag     = omp::compilerFlag(data_.vendor, compiler);
      data_.supportsOpenMP = (data_.OpenMPFlag != omp::notSupported);

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device::setCompilerEnvScript(const std::string &compilerEnvScript_){
      compilerEnvScript = compilerEnvScript_;
    }

    void device::setCompilerFlags(const std::string &compilerFlags_){
      OCCA_EXTRACT_DATA(OpenMP, Device);

      compilerFlags = compilerFlags_;

      sys::addSharedBinaryFlagsTo(data_.vendor, compilerFlags);
    }

    void device::flush(){}

    void device::finish(){}

    bool device::fakesUva(){
      return false;
    }

    void device::waitFor(streamTag tag){}

    stream_t device::createStream(){
      return NULL;
    }

    void device::freeStream(stream_t s){}

    stream_t device::wrapStream(void *handle_){
      return NULL;
    }

    streamTag device::tagStream(){
      streamTag ret;

      ret.tagTime = currentTime();

      return ret;
    }

    double device::timeBetween(const streamTag &startTag, const streamTag &endTag){
      return (endTag.tagTime - startTag.tagTime);
    }

    std::string device::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    kernel_v* device::buildKernelFromSource(const std::string &filename,
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

    kernel_v* device::buildKernelFromBinary(const std::string &filename,
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

    void device::cacheKernelInLibrary(const std::string &filename,
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

    kernel_v* device::loadKernelFromLibrary(const char *cache,
                                                      const std::string &functionName){
#if 0
      kernel_v *k = new kernel_t<OpenMP>;
      k->dHandle = this;
      k->loadFromLibrary(cache, functionName);
      return k;
#endif
      return NULL;
    }

    memory_v* device::wrapMemory(void *handle_,
                                           const uintptr_t bytes){
      memory_v *mem = new memory_t<OpenMP>;

      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = handle_;

      mem->memInfo |= memFlag::isAWrapper;

      return mem;
    }

    memory_v* device::wrapTexture(void *handle_,
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

    memory_v* device::malloc(const uintptr_t bytes,
                                       void *src){
      memory_v *mem = new memory_t<OpenMP>;

      mem->dHandle = this;
      mem->size    = bytes;

      mem->handle = sys::malloc(bytes);

      if(src != NULL)
        ::memcpy(mem->handle, src, bytes);

      return mem;
    }

    memory_v* device::textureAlloc(const int dim, const occa::dim &dims,
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

    memory_v* device::mappedAlloc(const uintptr_t bytes,
                                            void *src){
      memory_v *mem = malloc(bytes, src);

      mem->mappedPtr = mem->handle;

      return mem;
    }

    uintptr_t device::memorySize(){
      return sys::installedRAM();
    }

    void device::free(){}

    int device::simdWidth(){
      simdWidth_ = OCCA_SIMD_WIDTH;
      return OCCA_SIMD_WIDTH;
    }
  }
}

#endif
