#ifndef OCCA_BASE_HEADER
#define OCCA_BASE_HEADER

#include <iostream>
#include <vector>
#include <map>

#include <stdint.h>

#include "occa/defines.hpp"

#if OCCA_SSE
#  include <xmmintrin.h>
#endif

#include "occa/device.hpp"
#include "occa/kernel.hpp"
#include "occa/memory.hpp"

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;

  class kernelInfo;
  class deviceInfo;

  //---[ Typedefs ]---------------------
  typedef std::vector<int>          intVector_t;
  typedef std::vector<intVector_t>  intVecVector_t;
  typedef std::vector<std::string>  stringVector_t;
  //====================================


  //---[ Globals & Flags ]--------------
  extern const int parserVersion;

  extern kernelInfo defaultKernelInfo;

  extern const int autoDetect;
  extern const int srcInUva, destInUva;

  extern bool uvaEnabledByDefault_f;
  extern bool verboseCompilation_f;

  void setVerboseCompilation(const bool value);
  //====================================


  //---[ Registration ]-----------------
  class mode_v;

  typedef std::map<std::string,mode_v*> strToModeMap_t;
  typedef strToModeMap_t::iterator      strToModeMapIterator;

  strToModeMap_t& modeMap();
  bool modeExists(const std::string &mode);

  device_v* newModeDevice(const std::string &mode);
  kernel_v* newModeKernel(const std::string &mode);
  memory_v* newModeMemory(const std::string &mode);

  void freeModeDevice(device_v *dHandle);
  void freeModeKernel(kernel_v *kHandle);
  void freeModeMemory(memory_v *mHandle);

  class mode_v {
  protected:
    std::string modeName;

  public:
    std::string& name() {
      return modeName;
    }

    virtual device_v* newDevice() = 0;
    virtual kernel_v* newKernel() = 0;
    virtual memory_v* newMemory() = 0;
  };

  template <class device_t,
            class kernel_t,
            class memory_t>
  class mode : public mode_v {

  public:
    mode(std::string modeName_) {
      modeName = modeName_;
      modeMap()[modeName] = this;
    }

    device_v* newDevice() {
      return new device_t();
    }

    kernel_v* newKernel() {
      return new kernel_t();
    }

    memory_v* newMemory() {
      return new memory_t();
    }
  };
  //====================================


  //---[ Memory ]-----------------------
  void memcpy(void *dest, void *src,
              const uintptr_t bytes,
              const int flags,
              const bool isAsync);

  void memcpy(void *dest, void *src,
              const uintptr_t bytes,
              const int flags = occa::autoDetect);

  void asyncMemcpy(void *dest, void *src,
                   const uintptr_t bytes,
                   const int flags = occa::autoDetect);

  void memcpy(memory dest,
              const void *src,
              const uintptr_t bytes = 0,
              const uintptr_t offset = 0);

  void memcpy(void *dest,
              memory src,
              const uintptr_t bytes = 0,
              const uintptr_t offset = 0);

  void memcpy(memory dest,
              memory src,
              const uintptr_t bytes = 0,
              const uintptr_t destOffset = 0,
              const uintptr_t srcOffset = 0);

  void asyncMemcpy(memory dest,
                   const void *src,
                   const uintptr_t bytes = 0,
                   const uintptr_t offset = 0);

  void asyncMemcpy(void *dest,
                   memory src,
                   const uintptr_t bytes = 0,
                   const uintptr_t offset = 0);

  void asyncMemcpy(memory dest,
                   memory src,
                   const uintptr_t bytes = 0,
                   const uintptr_t destOffset = 0,
                   const uintptr_t srcOffset = 0);


  //---[ Device Functions ]-------------
  extern device currentDevice;
  device getCurrentDevice();

  device host();
  void setDevice(device d);
  void setDevice(const std::string &infos);

  extern mutex_t deviceListMutex;
  extern std::vector<device> deviceList;

  std::vector<device>& getDeviceList();

  template <class TM>
  std::string getDeviceProperty(const std::string &info) {
    return currentDevice.getProperty<TM>(info);
  }

  template <class TM>
  void setDeviceProperty(const std::string &info, const TM &value) {
    currentDevice.setProperty(info, value);
  }

  void setCompiler(const std::string &compiler_);
  void setCompilerEnvScript(const std::string &compilerEnvScript_);
  void setCompilerFlags(const std::string &compilerFlags_);

  std::string getCompiler();
  std::string getCompilerEnvScript();
  std::string getCompilerFlags();

  void flush();
  void finish();

  void waitFor(streamTag tag);

  stream createStream();
  stream getStream();
  void setStream(stream s);
  stream wrapStream(void *handle_);

  streamTag tagStream();
  //====================================

  //---[ Kernel Functions ]-------------
  kernel buildKernel(const std::string &str,
                     const std::string &functionName,
                     const kernelInfo &info_ = defaultKernelInfo);

  kernel buildKernelFromString(const std::string &content,
                               const std::string &functionName,
                               const int language = usingOKL);

  kernel buildKernelFromString(const std::string &content,
                               const std::string &functionName,
                               const kernelInfo &info_ = defaultKernelInfo,
                               const int language = usingOKL);

  kernel buildKernelFromSource(const std::string &filename,
                               const std::string &functionName,
                               const kernelInfo &info_ = defaultKernelInfo);

  kernel buildKernelFromBinary(const std::string &filename,
                               const std::string &functionName);
  //====================================

  //---[ Memory Functions ]-------------
  memory wrapMemory(void *handle_,
                    const uintptr_t bytes);

  void wrapManagedMemory(void *handle_,
                         const uintptr_t bytes);

  memory malloc(const uintptr_t bytes,
                void *src = NULL);

  void* managedAlloc(const uintptr_t bytes,
                     void *src = NULL);

  memory mappedAlloc(const uintptr_t bytes,
                     void *src = NULL);

  void* managedMappedAlloc(const uintptr_t bytes,
                           void *src = NULL);
  //====================================

  //---[ Free Functions ]---------------
  void free(device d);
  void free(stream s);
  void free(kernel k);
  void free(memory m);
  //====================================

  void printAvailableDevices();

  //---[ Class Infos ]------------------
  class deviceInfo {
  public:
    std::string infos;

    deviceInfo();

    deviceInfo(const deviceInfo &dInfo);
    deviceInfo& operator = (const deviceInfo &dInfo);

    void append(const std::string &key,
                const std::string &value);
  };

  class kernelInfo {
  public:
    std::string header, flags;

    flags_t parserFlags;

    kernelInfo();

    kernelInfo(const kernelInfo &p);
    kernelInfo& operator = (const kernelInfo &p);

    kernelInfo& operator += (const kernelInfo &p);

    std::string salt() const;

    std::string getModeHeaderFilename() const;

    static bool isAnOccaDefine(const std::string &name);

    void addIncludeDefine(const std::string &filename);

    void addInclude(const std::string &filename);

    void removeDefine(const std::string &macro);

    template <class TM>
    void addDefine(const std::string &macro, const TM &value) {
      std::stringstream ss;

      if(isAnOccaDefine(macro))
        ss << "#undef " << macro << "\n";

      ss << "#define " << macro << " " << value << '\n';

      header = ss.str() + header;
    }

    void addSource(const std::string &content);

    void addCompilerFlag(const std::string &f);

    void addCompilerIncludePath(const std::string &path);

    flags_t& getParserFlags();
    const flags_t& getParserFlags() const;

    void addParserFlag(const std::string &flag,
                       const std::string &value = "");
  };

  template <> void kernelInfo::addDefine(const std::string &macro, const std::string &value);
  template <> void kernelInfo::addDefine(const std::string &macro, const float &value);
  template <> void kernelInfo::addDefine(const std::string &macro, const double &value);
  //====================================
}

#endif
