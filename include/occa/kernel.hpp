#ifndef OCCA_KERNEL_HEADER
#define OCCA_KERNEL_HEADER

#include <iostream>
#include <stdint.h>

#include "occa/defines.hpp"
#include "occa/tools.hpp"

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;

  static const bool useParser = true;

  static const int usingOKL    = (1 << 0);
  static const int usingOFL    = (1 << 1);
  static const int usingNative = (1 << 2);

  class kernelInfo;
  extern kernelInfo defaultKernelInfo;

  //---[ KernelArg ]--------------------
  namespace kArgInfo {
    static const char none       = 0;
    static const char usePointer = (1 << 0);
    static const char hasTexture = (1 << 1);
  }

  // [REFACTOR]
  union kernelArgData_t {
    uint8_t  uint8_;
    uint16_t uint16_;
    uint32_t uint32_;
    uint64_t uint64_;

    int8_t  int8_;
    int16_t int16_;
    int32_t int32_;
    int64_t int64_;

    float float_;
    double double_;

    void* void_;
  };

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
    kernelArg(const TM &arg_) {
      argc = 1;
      setupFrom(args[0], const_cast<TM*>(&arg_), sizeof(TM), false);
    }

    template <class TM>
    kernelArg(TM *arg_) {
      argc = 1;
      setupFrom(args[0], arg_);
    }

    template <class TM>
    kernelArg(const TM *arg_) {
      argc = 1;
      setupFrom(args[0], const_cast<TM*>(arg_));
    }

    void setupFrom(kernelArg_t &arg, void *arg_,
                   bool lookAtUva = true, bool argIsUva = false);

    void setupFrom(kernelArg_t &arg, void *arg_, size_t bytes,
                   bool lookAtUva = true, bool argIsUva = false);

    occa::device getDevice() const;

    void setupForKernelCall(const bool isConst) const;

    static int argumentCount(const int argc, const kernelArg *args);
  };

  template <> kernelArg::kernelArg(const occa::memory &m);

  template <> kernelArg::kernelArg(const uint8_t &arg_);
  template <> kernelArg::kernelArg(const uint16_t &arg_);
  template <> kernelArg::kernelArg(const uint32_t &arg_);
  template <> kernelArg::kernelArg(const uint64_t &arg_);

  template <> kernelArg::kernelArg(const int8_t &arg_);
  template <> kernelArg::kernelArg(const int16_t &arg_);
  template <> kernelArg::kernelArg(const int32_t &arg_);
  template <> kernelArg::kernelArg(const int64_t &arg_);

  template <> kernelArg::kernelArg(const float &arg_);
  template <> kernelArg::kernelArg(const double &arg_);
  //====================================


  //---[ kernel_v ]---------------------
  class kernel_v {
    friend class occa::kernel;
    friend class occa::device;

  private:
    occa::device_v *dHandle;

    std::string name;

    parsedKernelInfo metaInfo;

    int dims;
    dim inner, outer;

    std::vector<kernel> nestedKernels;
    std::vector<kernelArg> arguments;

  public:
    kernel_v();
    virtual ~kernel_v() = 0;

    virtual kernel_v* newKernel() = 0;

    virtual void* getKernelHandle() = 0;
    virtual void* getProgramHandle() = 0;

    virtual std::string fixBinaryName(const std::string &filename) = 0;

    virtual void buildFromSource(const std::string &filename,
                                 const std::string &functionName,
                                 const kernelInfo &info_ = defaultKernelInfo) = 0;

    virtual void buildFromBinary(const std::string &filename,
                                 const std::string &functionName) = 0;

    kernel* nestedKernelsPtr();
    int nestedKernelCount();

    kernelArg* argumentsPtr();
    int argumentCount();

    virtual int maxDims() = 0;
    virtual dim maxOuterDims() = 0;
    virtual dim maxInnerDims() = 0;

    virtual void runFromArguments(const int kArgc, const kernelArg *kArgs) = 0;

    virtual void free() = 0;
  };
  //====================================

  //---[ kernel ]-----------------------
  class kernel {
    friend class occa::device;

  private:
    kernel_v *kHandle;

  public:
    kernel();
    kernel(kernel_v *kHandle_);

    kernel(const kernel &k);
    kernel& operator = (const kernel &k);

    void checkIfInitialized() const;

    void* getKernelHandle();
    void* getProgramHandle();

    kernel_v* getKHandle();

    const std::string& mode();
    const std::string& name();

    occa::device getDevice();

    int maxDims();
    dim maxOuterDims();
    dim maxInnerDims();

    void setWorkingDims(int dims, dim inner, dim outer);

    void clearArgumentList();

    void addArgument(const int argPos,
                     const kernelArg &arg);

    void runFromArguments();

#include "occa/operators/declarations.hpp"

    void free();
  };
  //====================================
}

#endif