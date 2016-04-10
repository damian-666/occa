#include "occa/kernel.hpp"

namespace occa {
  //---[ KernelArg ]--------------------

  kernelArg_t::kernelArg_t() {
    dHandle = NULL;
    mHandle = NULL;

    ::memset(&data, 0, sizeof(data));
    size = 0;
    info = kArgInfo::none;
  }

  kernelArg_t::kernelArg_t(const kernelArg_t &k) {
    *this = k;
  }

  kernelArg_t& kernelArg_t::operator = (const kernelArg_t &k) {
    dHandle = k.dHandle;
    mHandle = k.mHandle;

    ::memcpy(&data, &(k.data), sizeof(data));
    size = k.size;
    info = k.info;

    return *this;
  }

  kernelArg_t::~kernelArg_t() {}

  void* kernelArg_t::ptr() const {
    return ((info & kArgInfo::usePointer) ? data.void_ : (void*) &data);
  }

  kernelArg::kernelArg() {
    argc = 0;
  }

  kernelArg::~kernelArg() {}

  kernelArg::kernelArg(kernelArg_t &arg_) {
    argc = 1;

    args[0] = arg_;
  }

  kernelArg::kernelArg(const kernelArg &k) {
    argc = k.argc;

    args[0] = k.args[0];
    args[1] = k.args[1];
  }

  kernelArg& kernelArg::operator = (const kernelArg &k) {
    argc = k.argc;

    args[0] = k.args[0];
    args[1] = k.args[1];

    return *this;
  }

  template <>
  kernelArg::kernelArg(const occa::memory &m) {
    argc = 1;

    if (m.mHandle->dHandle->fakesUva()) {
      if(!m.isATexture()) {
        setupFrom(args[0], m.mHandle->handle, false, true);
      }
      else {
        setupFrom(args[0], m.mHandle->handle, false, true);
        setupFrom(args[1], m.mHandle->handle, false, true);

        args[0].data.void_ = m.textureArg1();
        args[1].data.void_ = m.textureArg2();

        args[0].info |= kArgInfo::hasTexture;
      }
    }
    else {
      setupFrom(args[0], m.mHandle->handle, false);
    }
  }

  template <> kernelArg::kernelArg(const int &arg_) {
    argc = 1; args[0].data.int_ = arg_; args[0].size = sizeof(int);
  }
  template <> kernelArg::kernelArg(const char &arg_) {
    argc = 1; args[0].data.char_ = arg_; args[0].size = sizeof(char);
  }
  template <> kernelArg::kernelArg(const short &arg_) {
    argc = 1; args[0].data.short_ = arg_; args[0].size = sizeof(short);
  }
  template <> kernelArg::kernelArg(const long &arg_) {
    argc = 1; args[0].data.long_ = arg_; args[0].size = sizeof(long);
  }

  template <> kernelArg::kernelArg(const unsigned int &arg_) {
    argc = 1; args[0].data.uint_ = arg_; args[0].size = sizeof(unsigned int);
  }
  template <> kernelArg::kernelArg(const unsigned char &arg_) {
    argc = 1; args[0].data.uchar_ = arg_; args[0].size = sizeof(unsigned char);
  }
  template <> kernelArg::kernelArg(const unsigned short &arg_) {
    argc = 1; args[0].data.ushort_ = arg_; args[0].size = sizeof(unsigned short);
  }

  template <> kernelArg::kernelArg(const float &arg_) {
    argc = 1; args[0].data.float_ = arg_; args[0].size = sizeof(float);
  }
  template <> kernelArg::kernelArg(const double &arg_) {
    argc = 1; args[0].data.double_ = arg_; args[0].size = sizeof(double);
  }

#if OCCA_64_BIT
  // 32 bit: uintptr_t == unsigned int
  template <> kernelArg::kernelArg(const uintptr_t &arg_) {
    argc = 1; args[0].data.uintptr_t_ = arg_; args[0].size = sizeof(uintptr_t);
  }
#endif

  occa::device kernelArg::getDevice() const {
    return occa::device(args[0].dHandle);
  }

  void kernelArg::setupForKernelCall(const bool isConst) const {
    occa::memory_v *mHandle = args[0].mHandle;

    if(mHandle                      &&
       mHandle->isManaged()         &&
       !mHandle->leftInDevice()     &&
       mHandle->dHandle->fakesUva() &&
       mHandle->dHandle->hasUvaEnabled()) {

      if(!mHandle->inDevice()) {
        mHandle->copyFrom(mHandle->uvaPtr);
        mHandle->memInfo |= uvaFlag::inDevice;
      }

      if(!isConst && !mHandle->isDirty()) {
        uvaDirtyMemory.push_back(mHandle);
        mHandle->memInfo |= uvaFlag::isDirty;
      }
    }
  }

  int kernelArg::argumentCount(const int kArgc, const kernelArg *kArgs) {
    int argc = 0;
    for(int i = 0; i < kArgc; ++i){
      argc += kArgs[i].argc;
    }
    return argc;
  }

  std::ostream& operator << (std::ostream &out, const argInfoMap &m) {
    std::map<std::string,std::string>::const_iterator it = m.iMap.begin();

    while(it != m.iMap.end()) {
      out << it->first << " = " << it->second << '\n';
      ++it;
    }

    return out;
  }
  //====================================

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