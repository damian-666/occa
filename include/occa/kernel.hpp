#ifndef OCCA_KERNEL_HEADER
#define OCCA_KERNEL_HEADER

namespace occa {
  class kernel_v; class kernel;
  class memory_v; class memory;
  class device_v; class device;

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
    virtual kernel_v();
    virtual ~kernel_v();

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

    // [REFACTOR]
    // virtual uintptr_t maximumInnerDimSize() = 0;
    // virtual int preferredDimSize() = 0;
    virtual int maxDims() = 0;
    virtual dim maxOuterDims() = 0;
    virtual dim maxInnerDims() = 0;

    virtual void runFromArguments(const int kArgc, const kernelArg *kArgs) = 0;

    virtual void free() = 0;
  };

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
}

#endif