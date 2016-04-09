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

#include "occa/uva.hpp"
#include "occa/parser/tools.hpp"
#include "occa/texture.hpp"

namespace occa {
  class kernel_v;
  class kernel;

  class memory_v;
  class memory;

  class device_v;
  class device;

  class kernelInfo;
  class deviceInfo;

  //---[ Typedefs ]-----------------------
  typedef std::vector<int>          intVector_t;
  typedef std::vector<intVector_t>  intVecVector_t;
  typedef std::vector<std::string>  stringVector_t;
  //======================================


  //---[ Globals & Flags ]------------------------
  extern const int parserVersion;

  extern kernelInfo defaultKernelInfo;

  extern const int autoDetect;
  extern const int srcInUva, destInUva;

  extern bool uvaEnabledByDefault_f;
  extern bool verboseCompilation_f;

  void setVerboseCompilation(const bool value);

  // [REFACTOR]
  // hasModeEnabled(string mode)
  //==============================================


  //---[ Typedefs ]-------------------------------
  typedef void* stream_t;

  static const bool useParser = true;

  static const int usingOKL    = (1 << 0);
  static const int usingOFL    = (1 << 1);
  static const int usingNative = (1 << 2);
  //==============================================


  //---[ Helper Classes ]-------------------------
  class argInfoMap {
  public:
    std::map<std::string, std::string> iMap;

    argInfoMap();
    argInfoMap(const std::string &infos);
    argInfoMap(argInfoMap &aim);
    argInfoMap& operator = (argInfoMap &aim);

    std::string& operator [] (const std::string &info);

    bool has(const std::string &info);
    void remove(const std::string &info);

    template <class TM>
    void set(const std::string &info, const TM &value) {
      iMap[info] = toString(value);
    }

    template <class TM>
    TM get(const std::string &prop) {
      TM t;
      if (has(prop)) {
        std::stringstream ss;
        ss << iMap[prop];
        ss >> t;
      }
      return t;
    }

    template <class TM>
     std::vector<TM> getVector(const std::string &prop) {
      std::vector<TM> ret;
      TM t;
      if (has(prop)) {
        std::stringstream ss(iMap[prop].c_str());
        while(ss.peek() != '\0') {
          if (isWhitespace(c)) {
            ss.get(1);
          }
          else {
            if (ss.peek() == ',')
              ss.get(1);
            if (ss.peek() == '\0') {
              ss >> t;
              ret.push_back(t);
            }
          }
        }
      }
      return ret;
    }

    friend std::ostream& operator << (std::ostream &out, const argInfoMap &m);
  };

  class dim {
  public:
    uintptr_t x, y, z;

    dim();
    dim(uintptr_t x_);
    dim(uintptr_t x_, uintptr_t y_);
    dim(uintptr_t x_, uintptr_t y_, uintptr_t z_);

    dim(const dim &d);

    dim& operator = (const dim &d);

    dim operator + (const dim &d);
    dim operator - (const dim &d);
    dim operator * (const dim &d);
    dim operator / (const dim &d);

    bool hasNegativeEntries();

    uintptr_t& operator [] (int i);
    uintptr_t  operator [] (int i) const;
  };

  // [REFACTOR]
  union kernelArgData_t {
    uint8_t  uint8;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;

    int8_t  int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;

    void* void_;
  };

  namespace kArgInfo {
    static const char none       = 0;
    static const char usePointer = (1 << 0);
    static const char hasTexture = (1 << 1);
  }

  class kernelArg_t {
  public:
    occa::device_v *dHandle;
    occa::memory_v *mHandle;

    kernelArgData_t data;
    uintptr_t       size;
    char            info;

    kernelArg_t();
    kernelArg_t(const kernelArg_t &k);
    kernelArg_t& operator = (const kernelArg_t &k);
    ~kernelArg_t();

    void* ptr() const;
  };

  class kernelArg {
  public:
    int argc;
    kernelArg_t args[2];

    kernelArg();
    ~kernelArg();
    kernelArg(kernelArg_t &arg_);
    kernelArg(const kernelArg &k);
    kernelArg& operator = (const kernelArg &k);

    template <class TM>
    inline kernelArg(const TM &arg_) {
      argc = 1;

      args[0].data.void_ = const_cast<TM*>(&arg_);
      args[0].size       = sizeof(TM);
      args[0].info       = kArgInfo::usePointer;
    }

    template <class TM> inline kernelArg(TM *arg_);
    template <class TM> inline kernelArg(const TM *carg_);

    occa::device getDevice() const;

    void setupForKernelCall(const bool isConst) const;

    static int argumentCount(const int argc, const kernelArg *args);
  };

  template <> kernelArg::kernelArg(const int &arg_);
  template <> kernelArg::kernelArg(const char &arg_);
  template <> kernelArg::kernelArg(const short &arg_);
  template <> kernelArg::kernelArg(const long &arg_);

  template <> kernelArg::kernelArg(const unsigned int &arg_);
  template <> kernelArg::kernelArg(const unsigned char &arg_);
  template <> kernelArg::kernelArg(const unsigned short &arg_);

  template <> kernelArg::kernelArg(const float &arg_);
  template <> kernelArg::kernelArg(const double &arg_);

#if OCCA_64_BIT
  // 32 bit: uintptr_t == unsigned int
  template <> kernelArg::kernelArg(const uintptr_t  &arg_);
#endif
  //==============================================


  //---[ Memory ]---------------------------------
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

  //---[ Typedefs ]---------------------
  namespace memFlag {
    static const int none         = 0;
    static const int isATexture   = (1 << 0);
    static const int isManaged    = (1 << 1);
    static const int isMapped     = (1 << 2);
    static const int isAWrapper   = (1 << 3);
  }

  namespace uvaFlag {
    static const int inDevice     = (1 << 4);
    static const int leftInDevice = (1 << 5);
    static const int isDirty      = (1 << 6);
  }
  //====================================


  //---[ Device Functions ]-------------
  extern device currentDevice;
  device getCurrentDevice();

  device host();
  void setDevice(device d);
  void setDevice(const std::string &infos);

  extern mutex_t deviceListMutex;
  extern std::vector<device> deviceList;

  std::vector<device>& getDeviceList();

  // [REFACTOR]
  template <class TM>
  inline void setDeviceProperty(const std::string &info, const TM &value) {
    currentDevice.setProperty(info, value);
  }

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

  memory wrapTexture(void *handle_,
                     const int dim, const occa::dim &dims,
                     occa::formatType type, const int permissions);

  void wrapManagedTexture(void *handle_,
                          const int dim, const occa::dim &dims,
                          occa::formatType type, const int permissions);

  memory malloc(const uintptr_t bytes,
                void *src = NULL);

  void* managedAlloc(const uintptr_t bytes,
                     void *src = NULL);

  memory textureAlloc(const int dim, const occa::dim &dims,
                      void *src,
                      occa::formatType type, const int permissions = readWrite);

  void* managedTextureAlloc(const int dim, const occa::dim &dims,
                            void *src,
                            occa::formatType type, const int permissions = readWrite);

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

  //---[ KernelArg ]------------------------------
  template <class TM>
  inline kernelArg::kernelArg(TM *arg_) {
    ptrRangeMap_t::iterator it = uvaMap.find(arg_);

    if(it != uvaMap.end()) {
      occa::memory_v *mHandle = it->second;

      argc = 1;

      args[0].mHandle = mHandle;
      args[0].dHandle = mHandle->dHandle;

      args[0].data.void_ = mHandle->handle;
      args[0].size       = sizeof(void*);
      args[0].info       = kArgInfo::usePointer;
    }
    else {
      argc = 1;

      args[0].data.void_ = arg_;
      args[0].size       = sizeof(TM*);
      args[0].info       = kArgInfo::usePointer;
    }
  }

  template <class TM>
  inline kernelArg::kernelArg(const TM *carg_) {
    TM *arg_ = const_cast<TM*>(carg_);

    ptrRangeMap_t::iterator it = uvaMap.find(arg_);

    if(it != uvaMap.end()) {
      occa::memory_v *mHandle = it->second;

      argc = 1;

      args[0].mHandle = mHandle;
      args[0].dHandle = mHandle->dHandle;

      args[0].data.void_ = mHandle->handle;
      args[0].size       = sizeof(void*);
      args[0].info       = kArgInfo::usePointer;
    }
    else {
      argc = 1;

      args[0].data.void_ = arg_;
      args[0].size       = sizeof(TM*);
      args[0].info       = kArgInfo::usePointer;
    }
  }

  template <>
  inline kernelArg::kernelArg(const occa::memory &m) {
    if(m.mHandle->dHandle->fakesUva()) {
      if(!m.isATexture()) {
        argc = 1;

        args[0].mHandle = m.mHandle;
        args[0].dHandle = m.mHandle->dHandle;

        args[0].data.void_ = m.mHandle->handle;
        args[0].size       = sizeof(void*);
        args[0].info       = kArgInfo::usePointer;
      }
      else {
        argc = 2;

        args[0].mHandle = m.mHandle;
        args[0].dHandle = m.mHandle->dHandle;

        args[0].data.void_ = m.textureArg1();
        args[0].size       = sizeof(void*);
        args[0].info       = (kArgInfo::usePointer |
                              kArgInfo::hasTexture);

        args[1].mHandle = args[0].mHandle;
        args[1].dHandle = args[0].dHandle;

        args[1].data.void_ = m.textureArg2();
        args[1].size       = sizeof(void*);
        args[1].info       = kArgInfo::usePointer;
      }
    }
    else{
      argc = 1;

      args[0].data.void_ = m.mHandle->handle;
      args[0].size       = sizeof(void*);
      args[0].info       = kArgInfo::usePointer;
    }
  }
  //==============================================

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
    occa::mode mode;
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
    inline void addDefine(const std::string &macro, const TM &value) {
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
}

#endif
