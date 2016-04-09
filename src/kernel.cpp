#include "occa/kernel.hpp"

namespace occa {
  //---[ kernel_v ]---------------------
  kernel* kernel_v::nestedKernelsPtr() {
    return &(nestedKernels[0]);
  }

  int kernel_v::nestedKernelCount() {
    return (int) nestedKernels.size();
  }

  kernelArg* kernel_v::argumentsPtr() {
    return &(arguments[0]);
  }

  int kernel_v::argumentCount() {
    return (int) arguments.size();
  }
  //====================================

  //---[ kernel ]-----------------------
  kernel::kernel() :
    kHandle(NULL) {}

  kernel::kernel(kernel_v *kHandle_) :
    kHandle(kHandle_) {}

  kernel::kernel(const kernel &k) :
    kHandle(k.kHandle) {}

  kernel& kernel::operator = (const kernel &k) {
    kHandle = k.kHandle;
    return *this;
  }

  void kernel::checkIfInitialized() const {
    OCCA_CHECK(kHandle != NULL,
               "Kernel is not initialized");
  }

  void* kernel::getKernelHandle() {
    checkIfInitialized();
    return kHandle->getKernelHandle();
  }

  void* kernel::getProgramHandle() {
    checkIfInitialized();
    return kHandle->getProgramHandle();
  }

  kernel_v* kernel::getKHandle() {
    checkIfInitialized();
    return kHandle;
  }

  const std::string& kernel::mode() {
    checkIfInitialized();
    return kHandle->strMode;
  }

  const std::string& kernel::name() {
    checkIfInitialized();
    return kHandle->name;
  }

  occa::device kernel::getDevice() {
    checkIfInitialized();
    return occa::device(kHandle->dHandle);
  }

  void kernel::setWorkingDims(int dims, occa::dim inner, occa::dim outer) {
    checkIfInitialized();

    for(int i = 0; i < dims; ++i) {
      inner[i] += (inner[i] ? 0 : 1);
      outer[i] += (outer[i] ? 0 : 1);
    }

    for(int i = dims; i < 3; ++i)
      inner[i] = outer[i] = 1;

    if (kHandle->nestedKernelCount()) {
      for(int k = 0; k < kHandle->nestedKernelCount(); ++k)
        kHandle->nestedKernels[k].setWorkingDims(dims, inner, outer);
    } else {
      kHandle->dims  = dims;
      kHandle->inner = inner;
      kHandle->outer = outer;
    }
  }

  uintptr_t kernel::maximumInnerDimSize() {
    checkIfInitialized();
    return kHandle->maximumInnerDimSize();
  }

  int kernel::preferredDimSize() {
    checkIfInitialized();

    if(kHandle->nestedKernelCount())
      return 0;

    return kHandle->preferredDimSize();
  }

  void kernel::clearArgumentList() {
    checkIfInitialized();
    kHandle->arguments.clear();
  }

  void kernel::addArgument(const int argPos,
                           const kernelArg &arg) {
    checkIfInitialized();

    if(kHandle->argumentCount() <= argPos) {
      OCCA_CHECK(argPos < OCCA_MAX_ARGS,
                 "Kernels can only have at most [" << OCCA_MAX_ARGS << "] arguments,"
                 << " [" << argPos << "] arguments were set");

      kHandle->arguments.reserve(argPos + 1);
    }

    kHandle->arguments.insert(kHandle->arguments.begin() + argPos, arg);
  }

  void kernel::runFromArguments() {
    checkIfInitialized();

    // Add nestedKernels
    if (kHandle->nestedKernelCount())
      kHandle->arguments.insert(kHandle->arguments.begin(),
                                kHandle->nestedKernelsPtr());

    kHandle->runFromArguments(kHandle->argumentCount(),
                              kHandle->argumentsPtr());

    // Remove nestedKernels
    if (kHandle->nestedKernelCount())
      kHandle->arguments.erase(kHandle->arguments.begin());
  }

#include "operators/definitions.cpp"

  void kernel::free() {
    checkIfInitialized();

    if(kHandle->nestedKernelCount()) {
      for(int k = 0; k < kHandle->nestedKernelCount(); ++k)
        kHandle->nestedKernels[k].free();
    }

    kHandle->free();

    delete kHandle;
    kHandle = NULL;
  }
  //====================================
}