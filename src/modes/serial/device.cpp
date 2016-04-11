#include "occa/modes/serial/device.hpp"
#include "occa/modes/serial/kernel.hpp"
#include "occa/modes/serial/memory.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace serial {
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

    void* device::getContextHandle(){
      return NULL;
    }

    void device::setup(argInfoMap &aim){
      properties = aim;

      vendor = sys::compilerVendor(compiler);

      sys::addSharedBinaryFlagsTo(vendor, compilerFlags);
    }

    // [REFACTOR]
    void device::addOccaHeadersToInfo(kernelInfo &info_){
    }

    std::string device::getInfoSalt(const kernelInfo &info_){
      std::stringstream salt;

      salt << "Serial"
           << info_.salt()
           << parserVersion
           << compilerEnvScript
           << compiler
           << compilerFlags;

      return salt.str();
    }

    void device::getEnvironmentVariables(){
      if (properties.has("compiler")) {
        compiler = properties["compiler"];
      }
      else if (env::var("OCCA_CXX").size()) {
        compiler = env::var("OCCA_CXX");
      }
      else if (env::var("CXX").size()) {
        compiler = env::var("CXX");
      }
      else{
#if (OCCA_OS & (LINUX_OS | OSX_OS))
        compiler = "g++";
#else
        compiler = "cl.exe";
#endif
      }

      if (properties.has("compilerFlags")) {
        compilerFlags = properties["compilerFlags"];
      }
      else if (env::var("OCCA_CXXFLAGS").size()) {
        compilerFlags = env::var("OCCA_CXXFLAGS");
      }
      else if (env::var("CXXFLAGS").size()) {
        compilerFlags = env::var("CXXFLAGS");
      }
      else{
#if (OCCA_OS & (LINUX_OS | OSX_OS))
        compilerFlags = "-g";
#else
#  if OCCA_DEBUG_ENABLED
        compilerFlags = " /Od";
#  else
        compilerFlags = " /O2";
#  endif
#endif
      }

      if (properties.has("compilerEnvScript")) {
        compilerEnvScript = properties["compilerEnvScript"];
      }
      else {
#if (OCCA_OS == WINDOWS_OS)
        std::string byteness;

        if(sizeof(void*) == 4)
          byteness = "x86 ";
        else if(sizeof(void*) == 8)
          byteness = "amd64";
        else
          OCCA_CHECK(false, "sizeof(void*) is not equal to 4 or 8");

#  if   (OCCA_VS_VERSION == 1800)
        // MSVC++ 12.0 - Visual Studio 2013
        char *visualStudioTools = getenv("VS120COMNTOOLS");
#  elif (OCCA_VS_VERSION == 1700)
        // MSVC++ 11.0 - Visual Studio 2012
        char *visualStudioTools = getenv("VS110COMNTOOLS");
#  else (OCCA_VS_VERSION < 1700)
        // MSVC++ 10.0 - Visual Studio 2010
        char *visualStudioTools = getenv("VS100COMNTOOLS");
#  endif

        if(visualStudioTools != NULL){
          compilerEnvScript = "\"" + std::string(visualStudioTools) + "..\\..\\VC\\vcvarsall.bat\" " + byteness;
        }
        else{
          std::cout << "WARNING: Visual Studio environment variable not found -> compiler environment (vcvarsall.bat) maybe not correctly setup." << std::endl;
        }
#endif
      }
    }

    void device::appendAvailableDevices(std::vector<occa::device> &dList){
      dList.push_back(occa::device(this));
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
      kernel *k = new kernel();
      k->dHandle = this;
      k->buildFromSource(filename, functionName, info_);
      return k;
    }

    kernel_v* device::buildKernelFromBinary(const std::string &filename,
                                            const std::string &functionName){
      kernel *k = new kernel();
      k->dHandle = this;
      k->buildFromBinary(filename, functionName);
      return k;
    }

    memory_v* device::wrapMemory(void *handle_,
                                 const uintptr_t bytes){
      memory *mem = new memory();

      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = handle_;

      mem->memInfo |= memFlag::isAWrapper;

      return mem;
    }

    memory_v* device::malloc(const uintptr_t bytes,
                             void *src){
      memory *mem = new memory();

      mem->dHandle = this;
      mem->size    = bytes;

      mem->handle = sys::malloc(bytes);

      if(src != NULL)
        ::memcpy(mem->handle, src, bytes);

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
  }
}
