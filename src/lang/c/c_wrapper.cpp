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

#include "occa/base.hpp"

#define OCCA_C_EXPORTS

#include "occa/lang/c/c_wrapper.hpp"

namespace occa {
  namespace c {
    enum type {
      memory_,

      int8_ , uint8_,
      int16_, uint16_,
      int32_, uint32_,
      int64_, uint64_,

      float_, double_,

      struct_,
      string_,

      device,
      kernel,
      stream,

      properties,
      kernelInfo,
    };
  }
}

OCCA_START_EXTERN_C

struct occaType_t {
  occa::c::type type;
  occa::kernelArg_t value;

  inline occaType_t() :
    type(),
    value() {}

  inline occaType_t(occa::c::type type_) :
    type(type_),
    value() {}
};

struct occaArgumentList_t {
  int argc;
  occaType_t *argv[100];
};

//---[ Globals & Flags ]----------------
occaKernelInfo occaNoKernelInfo = NULL;

const uintptr_t occaAutoSize = 0;
const uintptr_t occaNoOffset = 0;

const int occaUsingOKL    = occa::usingOKL;
const int occaUsingOFL    = occa::usingOFL;
const int occaUsingNative = occa::usingNative;

void OCCA_RFUNC occaSetVerboseCompilation(const int value) {
  occa::setVerboseCompilation((bool) value);
}
//======================================

OCCA_END_EXTERN_C


//---[ TypeCasting ]--------------------
namespace occa {
  namespace c {
    inline type& typeType(occaType t) {
      return t->ptr->type;
    }

    inline occa::kernelArg_t& typeValue(occaType t) {
      return t->ptr->value;
    }

    inline occa::memory typeToMemory(occaType t) {
      return occa::memory((occa::memory_v*) t->ptr->value.data.void_);
    }

    inline occaType newType(const occa::c::type type) {
      occaType ot = (occaType) new occaTypePtr_t;
      ot->ptr = new occaType_t();
      ot->ptr->type = type;
      return ot;
    }

    inline occaType createOccaType(void *ptr, size_t bytes, bool unsigned_) {
      occa::c::type type = occa::c::uint8_;
      switch(bytes) {
      case 8 : type = (unsigned_ ? occa::c::uint8_  : occa::c::int8_);
      case 16: type = (unsigned_ ? occa::c::uint16_ : occa::c::int16_);
      case 32: type = (unsigned_ ? occa::c::uint32_ : occa::c::int32_);
      case 64: type = (unsigned_ ? occa::c::uint64_ : occa::c::int64_);
      }
      return createOccaType(ptr, bytes, type);
    }

    inline occaType createOccaType(void *ptr, size_t bytes, occa::c::type type) {
      occaType ot = occa::c::newType(type);
      occa::kernelArg_t &kArg = occa::c::typeValue(ot);
      if ((type == struct_) || (type == string_)) {
        kArg.info = occa::kArgInfo::usePointer;
      }
      kArg.size = bytes;
      memcpy(&kArg.data, ptr, bytes);
      return ot;
    }

    inline std::string typeToStr(occaType value) {
      occa::kernelArg_t &value_ = occa::c::typeValue(value);
      const int valueType       = occa::c::typeType(value);

      switch(valueType) {
      case int8_  : return occa::toString(value_.data.int8_);
      case uint8_ : return occa::toString(value_.data.uint8_);
      case int16_ : return occa::toString(value_.data.int16_);
      case uint16_: return occa::toString(value_.data.uint16_);
      case int32_ : return occa::toString(value_.data.int32_);
      case uint32_: return occa::toString(value_.data.uint32_);
      case int64_ : return occa::toString(value_.data.int64_);
      case uint64_: return occa::toString(value_.data.uint64_);

      case float_  : return occa::toString(value_.data.float_);
      case double_ : return occa::toString(value_.data.double_);

      case string_ : return std::string((char*) value_.data.void_);
      default:
        std::cout << "Wrong type input in [occaKernelInfoAddDefine]\n";
      }

      return "";
    }
  }
}

OCCA_START_EXTERN_C

//  ---[ Known Types ]------------------
OCCA_LFUNC occaType OCCA_RFUNC occaInt8(int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::int8_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaUInt8(unsigned int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::uint8_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaInt16(int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::int16_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaUInt16(unsigned int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::uint16_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaInt32(int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::int32_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaUInt32(unsigned int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::uint32_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaInt64(int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::int64_);
}

OCCA_LFUNC occaType OCCA_RFUNC occaUInt64(unsigned int value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::uint64_);
}
//  ====================================

//  ---[ Ambiguous Types ]--------------
occaType OCCA_RFUNC occaInt(int value) {
  return occa::c::createOccaType(&value, sizeof(value), false);
}

occaType OCCA_RFUNC occaUInt(unsigned int value) {
  return occa::c::createOccaType(&value, sizeof(value), true);
}

occaType OCCA_RFUNC occaChar(char value) {
  return occa::c::createOccaType(&value, sizeof(value), false);
}

occaType OCCA_RFUNC occaUChar(unsigned char value) {
  return occa::c::createOccaType(&value, sizeof(value), true);
}

occaType OCCA_RFUNC occaShort(short value) {
  return occa::c::createOccaType(&value, sizeof(value), false);
}

occaType OCCA_RFUNC occaUShort(unsigned short value) {
  return occa::c::createOccaType(&value, sizeof(value), true);
}

occaType OCCA_RFUNC occaLong(long value) {
  return occa::c::createOccaType(&value, sizeof(value), false);
}

occaType OCCA_RFUNC occaULong(unsigned long value) {
  return occa::c::createOccaType(&value, sizeof(value), true);
}

occaType OCCA_RFUNC occaFloat(float value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::float_);
}

occaType OCCA_RFUNC occaDouble(double value) {
  return occa::c::createOccaType(&value, sizeof(value), occa::c::double_);
}

occaType OCCA_RFUNC occaStruct(void *value, uintptr_t bytes) {
  return occa::c::createOccaType(&value, bytes, occa::c::struct_);
}

occaType OCCA_RFUNC occaString(const char *value) {
  return occa::c::createOccaType(const_cast<char**>(&value), sizeof(value), occa::c::string_);
}
//  ====================================
//======================================


//---[ Background Device ]--------------
//  |---[ Device ]----------------------
void OCCA_RFUNC occaSetDevice(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  occa::setDevice(device_);
}

void OCCA_RFUNC occaSetDeviceFromInfo(const char *infos) {
  occa::setDevice(infos);
}

occaDevice OCCA_RFUNC occaGetCurrentDevice() {
  occa::device device = occa::getCurrentDevice();
  return (occaDevice) device.getDHandle();
}

void OCCA_RFUNC occaSetCompiler(const char *compiler_) {
  occa::setCompiler(compiler_);
}

void OCCA_RFUNC occaSetCompilerEnvScript(const char *compilerEnvScript_) {
  occa::setCompilerEnvScript(compilerEnvScript_);
}

void OCCA_RFUNC occaSetCompilerFlags(const char *compilerFlags_) {
  occa::setCompilerFlags(compilerFlags_);
}

const char* OCCA_RFUNC occaGetCompiler() {
  return occa::getCompiler().c_str();
}

const char* OCCA_RFUNC occaGetCompilerEnvScript() {
  return occa::getCompilerEnvScript().c_str();
}

const char* OCCA_RFUNC occaGetCompilerFlags() {
  return occa::getCompilerFlags().c_str();
}

void OCCA_RFUNC occaFlush() {
  occa::flush();
}

void OCCA_RFUNC occaFinish() {
  occa::finish();
}

void OCCA_RFUNC occaWaitFor(occaStreamTag tag) {
  occa::streamTag tag_;

  ::memcpy(&tag_, &tag, sizeof(tag_));

  occa::waitFor(tag_);
}

occaStream OCCA_RFUNC occaCreateStream() {
  occa::stream &newStream = *(new occa::stream(occa::createStream()));

  return (occaStream) &newStream;
}

occaStream OCCA_RFUNC occaGetStream() {
  occa::stream &currentStream = *(new occa::stream(occa::getStream()));

  return (occaStream) &currentStream;
}

void OCCA_RFUNC occaSetStream(occaStream stream) {
  occa::setStream(*((occa::stream*) stream));
}

occaStream OCCA_RFUNC occaWrapStream(void *handle_) {
  occa::stream &newStream = *(new occa::stream(occa::wrapStream(handle_)));

  return (occaStream) &newStream;
}

occaStreamTag OCCA_RFUNC occaTagStream() {
  occa::streamTag oldTag = occa::tagStream();
  occaStreamTag newTag;

  newTag.tagTime = oldTag.tagTime;

  ::memcpy(&(newTag.handle), &(oldTag.handle), sizeof(void*));

  return newTag;
}
//  |===================================

//  |---[ Kernel ]----------------------
occaKernel OCCA_RFUNC occaBuildKernel(const char *str,
                                      const char *functionName,
                                      occaKernelInfo info) {
  occa::kernel kernel;

  if(info != occaNoKernelInfo) {
    occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

    kernel = occa::buildKernel(str,
                               functionName,
                               info_);
  }
  else{
    kernel = occa::buildKernel(str,
                               functionName);
  }

  return (occaKernel) kernel.getKHandle();
}

occaKernel OCCA_RFUNC occaBuildKernelFromSource(const char *filename,
                                                const char *functionName,
                                                occaKernelInfo info) {
  occa::kernel kernel;

  if(info != occaNoKernelInfo) {
    occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

    kernel = occa::buildKernelFromSource(filename,
                                         functionName,
                                         info_);
  }
  else{
    kernel = occa::buildKernelFromSource(filename,
                                         functionName);
  }

  return (occaKernel) kernel.getKHandle();
}

occaKernel OCCA_RFUNC occaBuildKernelFromString(const char *str,
                                                const char *functionName,
                                                occaKernelInfo info,
                                                const int language) {
  occa::kernel kernel;

  if(info != occaNoKernelInfo) {
    occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

    kernel = occa::buildKernelFromString(str,
                                         functionName,
                                         info_,
                                         language);
  }
  else{
    kernel = occa::buildKernelFromString(str,
                                         functionName,
                                         language);
  }

  return (occaKernel) kernel.getKHandle();
}

occaKernel OCCA_RFUNC occaBuildKernelFromBinary(const char *filename,
                                                const char *functionName) {
  occa::kernel kernel;

  kernel = occa::buildKernelFromBinary(filename, functionName);

  return (occaKernel) kernel.getKHandle();
}
//  |===================================

//  |---[ Memory ]----------------------
void OCCA_RFUNC occaMemorySwap(occaMemory a, occaMemory b) {
  occaType_t *a_ptr = a->ptr;
  a->ptr = b->ptr;
  b->ptr = a_ptr;
}

occaMemory OCCA_RFUNC occaWrapMemory(void *handle_,
                                     const uintptr_t bytes) {

  occa::memory memory_ = occa::wrapMemory(handle_, bytes);

  occaMemory memory = occa::c::newType(occa::c::memory_);
  occa::c::typeValue(memory).data.void_ = memory_.getMHandle();
  return memory;
}

void OCCA_RFUNC occaWrapManagedMemory(void *handle_,
                                      const uintptr_t bytes) {
  occa::wrapManagedMemory(handle_, bytes);
}

occaMemory OCCA_RFUNC occaMalloc(const uintptr_t bytes,
                                 void *src) {
  occa::memory memory_ = occa::malloc(bytes, src);

  occaMemory memory = occa::c::newType(occa::c::memory_);
  memory->ptr->value.data.void_ = memory_.getMHandle();
  return memory;
}

void* OCCA_RFUNC occaManagedAlloc(const uintptr_t bytes,
                                  void *src) {

  return occa::managedAlloc(bytes, src);
}

occaMemory OCCA_RFUNC occaMappedAlloc(const uintptr_t bytes,
                                      void *src) {

  occa::memory memory_ = occa::mappedAlloc(bytes, src);

  occaMemory memory = occa::c::newType(occa::c::memory_);
  memory->ptr->value.data.void_ = memory_.getMHandle();
  return memory;
}

void* OCCA_RFUNC occaManagedMappedAlloc(const uintptr_t bytes,
                                        void *src) {

  return occa::managedMappedAlloc(bytes, src);
}
//  |===================================
//======================================


//---[ Device ]-------------------------
void OCCA_RFUNC occaPrintAvailableDevices() {
  occa::printAvailableDevices();
}

occaDeviceInfo OCCA_RFUNC occaCreateDeviceInfo() {
  occa::deviceInfo *info = new occa::deviceInfo();
  return (occaDeviceInfo) info;
}

void OCCA_RFUNC occaDeviceInfoAppend(occaDeviceInfo info,
                                     const char *key,
                                     const char *value) {

  occa::deviceInfo &info_ = *((occa::deviceInfo*) info);
  info_.append(key, value);
}

void OCCA_RFUNC occaDeviceInfoAppendType(occaDeviceInfo info,
                                         const char *key,
                                         occaType value) {

  occa::deviceInfo &info_ = *((occa::deviceInfo*) info);
  info_.append(key, occa::c::typeToStr(value));
  delete value;
}

void OCCA_RFUNC occaDeviceInfoFree(occaDeviceInfo info) {
  delete (occa::deviceInfo*) info;
}

occaDevice OCCA_RFUNC occaCreateDevice(const char *infos) {
  occa::device device(infos);
  return (occaDevice) device.getDHandle();
}

occaDevice OCCA_RFUNC occaCreateDeviceFromInfo(occaDeviceInfo dInfo) {
  occa::device device(*((occa::deviceInfo*) dInfo));
  return (occaDevice) device.getDHandle();
}

const char* OCCA_RFUNC occaDeviceMode(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.mode().c_str();
}

void OCCA_RFUNC occaDeviceSetCompiler(occaDevice device,
                                      const char *compiler) {
  occa::device device_((occa::device_v*) device);
  device_.setCompiler(compiler);
}

void OCCA_RFUNC occaDeviceSetCompilerEnvScript(occaDevice device,
                                               const char *compilerEnvScript_) {
  occa::device device_((occa::device_v*) device);
  device_.setCompilerEnvScript(compilerEnvScript_);
}

void OCCA_RFUNC occaDeviceSetCompilerFlags(occaDevice device,
                                           const char *compilerFlags) {
  occa::device device_((occa::device_v*) device);
  device_.setCompilerFlags(compilerFlags);
}

const char* OCCA_RFUNC occaDeviceGetCompiler(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.getCompiler().c_str();
}

const char* OCCA_RFUNC occaDeviceGetCompilerEnvScript(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.getCompilerFlags().c_str();
}

const char* OCCA_RFUNC occaDeviceGetCompilerFlags(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.getCompilerEnvScript().c_str();
}

uintptr_t OCCA_RFUNC occaDeviceMemorySize(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.memorySize();
}

uintptr_t OCCA_RFUNC occaDeviceMemoryAllocated(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.memoryAllocated();
}

// Old version of [occaDeviceMemoryAllocated()]
uintptr_t OCCA_RFUNC occaDeviceBytesAllocated(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  return device_.memoryAllocated();
}

occaKernel OCCA_RFUNC occaDeviceBuildKernel(occaDevice device,
                                            const char *str,
                                            const char *functionName,
                                            occaKernelInfo info) {
  occa::device device_((occa::device_v*) device);
  occa::kernel kernel;

  if(info != occaNoKernelInfo) {
    occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

    kernel = device_.buildKernel(str,
                                 functionName,
                                 info_);
  }
  else{
    kernel = device_.buildKernel(str,
                                 functionName);
  }

  return (occaKernel) kernel.getKHandle();
}

occaKernel OCCA_RFUNC occaDeviceBuildKernelFromSource(occaDevice device,
                                                      const char *filename,
                                                      const char *functionName,
                                                      occaKernelInfo info) {
  occa::device device_((occa::device_v*) device);
  occa::kernel kernel;

  if(info != occaNoKernelInfo) {
    occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

    kernel = device_.buildKernelFromSource(filename,
                                           functionName,
                                           info_);
  }
  else{
    kernel = device_.buildKernelFromSource(filename,
                                           functionName);
  }

  return (occaKernel) kernel.getKHandle();
}

occaKernel OCCA_RFUNC occaDeviceBuildKernelFromString(occaDevice device,
                                                      const char *str,
                                                      const char *functionName,
                                                      occaKernelInfo info,
                                                      const int language) {
  occa::device device_((occa::device_v*) device);
  occa::kernel kernel;

  if(info != occaNoKernelInfo) {
    occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

    kernel = device_.buildKernelFromString(str,
                                           functionName,
                                           info_,
                                           language);
  }
  else{
    kernel = device_.buildKernelFromString(str,
                                           functionName,
                                           language);
  }

  return (occaKernel) kernel.getKHandle();
}

occaKernel OCCA_RFUNC occaDeviceBuildKernelFromBinary(occaDevice device,
                                                      const char *filename,
                                                      const char *functionName) {
  occa::device device_((occa::device_v*) device);
  occa::kernel kernel;

  kernel = device_.buildKernelFromBinary(filename, functionName);

  return (occaKernel) kernel.getKHandle();
}

occaMemory OCCA_RFUNC occaDeviceMalloc(occaDevice device,
                                       uintptr_t bytes,
                                       void *src) {

  occa::device device_((occa::device_v*) device);
  occa::memory memory_ = device_.malloc(bytes, src);

  occaMemory memory = occa::c::newType(occa::c::memory_);
  memory->ptr->value.data.void_ = memory_.getMHandle();
  return memory;
}

void* OCCA_RFUNC occaDeviceManagedAlloc(occaDevice device,
                                        uintptr_t bytes,
                                        void *src) {

  occa::device device_((occa::device_v*) device);

  return device_.managedAlloc(bytes, src);
}

occaMemory OCCA_RFUNC occaDeviceMappedAlloc(occaDevice device,
                                            uintptr_t bytes,
                                            void *src) {

  occa::device device_((occa::device_v*) device);
  occa::memory memory_ = device_.mappedAlloc(bytes, src);

  occaMemory memory = occa::c::newType(occa::c::memory_);
  memory->ptr->value.data.void_ = memory_.getMHandle();
  return memory;
}

void* OCCA_RFUNC occaDeviceManagedMappedAlloc(occaDevice device,
                                              uintptr_t bytes,
                                              void *src) {

  occa::device device_((occa::device_v*) device);
  return device_.managedMappedAlloc(bytes, src);
}

void OCCA_RFUNC occaDeviceFlush(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  device_.flush();
}

void OCCA_RFUNC occaDeviceFinish(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  device_.finish();
}

occaStream OCCA_RFUNC occaDeviceCreateStream(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  occa::stream &newStream = *(new occa::stream(device_.createStream()));
  return (occaStream) &newStream;
}

occaStream OCCA_RFUNC occaDeviceGetStream(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  occa::stream &currentStream = *(new occa::stream(device_.getStream()));
  return (occaStream) &currentStream;
}

void OCCA_RFUNC occaDeviceSetStream(occaDevice device, occaStream stream) {
  occa::device device_((occa::device_v*) device);
  device_.setStream(*((occa::stream*) stream));
}

occaStream OCCA_RFUNC occaDeviceWrapStream(occaDevice device, void *handle_) {
  occa::device device_((occa::device_v*) device);
  occa::stream &newStream = *(new occa::stream(device_.wrapStream(handle_)));
  return (occaStream) &newStream;
}

occaStreamTag OCCA_RFUNC occaDeviceTagStream(occaDevice device) {
  occa::device device_((occa::device_v*) device);

  occa::streamTag oldTag = device_.tagStream();
  occaStreamTag newTag;

  ::memcpy(&newTag, &oldTag, sizeof(oldTag));

  return newTag;
}

void OCCA_RFUNC occaDeviceWaitFor(occaDevice device,
                                  occaStreamTag tag) {
  occa::device device_((occa::device_v*) device);

  occa::streamTag tag_;
  ::memcpy(&tag_, &tag, sizeof(tag_));

  device_.waitFor(tag_);
}

double OCCA_RFUNC occaDeviceTimeBetweenTags(occaDevice device,
                                            occaStreamTag startTag, occaStreamTag endTag) {
  occa::device device_((occa::device_v*) device);

  occa::streamTag startTag_, endTag_;
  ::memcpy(&startTag_, &startTag, sizeof(startTag_));
  ::memcpy(&endTag_  , &endTag  , sizeof(endTag_));

  return device_.timeBetween(startTag_, endTag_);
}

void OCCA_RFUNC occaGetStreamFree(occaStream stream) {
  delete (occa::stream*) stream;
}

void OCCA_RFUNC occaStreamFree(occaStream stream) {
  ((occa::stream*) stream)->free();
  delete (occa::stream*) stream;
}

void OCCA_RFUNC occaDeviceFree(occaDevice device) {
  occa::device device_((occa::device_v*) device);
  device_.free();
}
//======================================


//---[ Kernel ]-------------------------
occaDim OCCA_RFUNC occaCreateDim(uintptr_t x, uintptr_t y, uintptr_t z) {
  occaDim ret;
  ret.x = x;
  ret.y = y;
  ret.z = z;
  return ret;
}

const char* OCCA_RFUNC occaKernelMode(occaKernel kernel) {
  occa::kernel kernel_((occa::kernel_v*) kernel);
  return kernel_.mode().c_str();
}

const char* OCCA_RFUNC occaKernelName(occaKernel kernel) {
  occa::kernel kernel_((occa::kernel_v*) kernel);
  return kernel_.name().c_str();
}

occaDevice OCCA_RFUNC occaKernelGetDevice(occaKernel kernel) {
  occa::kernel kernel_((occa::kernel_v*) kernel);
  occa::device device = kernel_.getDevice();
  return (occaDevice) device.getDHandle();
}

void OCCA_RFUNC occaKernelSetWorkingDims(occaKernel kernel,
                                         int dims,
                                         occaDim items,
                                         occaDim groups) {

  occa::kernel kernel_((occa::kernel_v*) kernel);
  kernel_.setWorkingDims(dims,
                         occa::dim(items.x, items.y, items.z),
                         occa::dim(groups.x, groups.y, groups.z));
}

void OCCA_RFUNC occaKernelSetAllWorkingDims(occaKernel kernel,
                                            int dims,
                                            uintptr_t itemsX, uintptr_t itemsY, uintptr_t itemsZ,
                                            uintptr_t groupsX, uintptr_t groupsY, uintptr_t groupsZ) {

  occa::kernel kernel_((occa::kernel_v*) kernel);
  kernel_.setWorkingDims(dims,
                         occa::dim(itemsX, itemsY, itemsZ),
                         occa::dim(groupsX, groupsY, groupsZ));
}

occaArgumentList OCCA_RFUNC occaCreateArgumentList() {
  occaArgumentList list = (occaArgumentList) new occaArgumentList_t();
  list->argc = 0;
  return list;
}

void OCCA_RFUNC occaArgumentListClear(occaArgumentList list) {
  occaArgumentList_t &list_ = *list;
  for(int i = 0; i < list_.argc; ++i) {
    if(list_.argv[i]->type != occa::c::memory_)
      delete list_.argv[i];
  }
  list_.argc = 0;
}

void OCCA_RFUNC occaArgumentListFree(occaArgumentList list) {
  delete list;
}

void OCCA_RFUNC occaArgumentListAddArg(occaArgumentList list,
                                       int argPos,
                                       void *type) {

  occaArgumentList_t &list_ = *list;
  if(list_.argc < (argPos + 1)) {
    OCCA_CHECK(argPos < OCCA_MAX_ARGS,
               "Kernels can only have at most [" << OCCA_MAX_ARGS << "] arguments,"
               << " [" << argPos << "] arguments were set");

    list_.argc = (argPos + 1);
  }
  list_.argv[argPos] = (occaType_t*) type;
}

// Note the _
// [occaKernelRun] is reserved for a variadic macro which is more user-friendly
void OCCA_RFUNC occaKernelRun_(occaKernel kernel,
                               occaArgumentList list) {

  occaArgumentList_t &list_ = *((occaArgumentList_t*) list);
  occaKernelRunN(kernel, list_.argc, list_.argv);
}

void OCCA_RFUNC occaKernelRunN(occaKernel kernel, const int argc, occaType_t **args){
  occa::kernel kernel_((occa::kernel_v*) kernel);
  kernel_.clearArgumentList();

  for(int i = 0; i < argc; ++i){
    occaType_t &arg = *(args[i]);
    void *argPtr = arg.value.data.void_;

    if(arg.type == occa::c::memory_){
      occa::memory memory_((occa::memory_v*) argPtr);
      kernel_.addArgument(i, occa::kernelArg(occa::c::memory_));
    }
    else {
      kernel_.addArgument(i, occa::kernelArg(arg.value));
      delete (occaType_t*) args[i];
    }
  }

  kernel_.runFromArguments();
}

#include "operators/cKernelOperators.cpp"

void OCCA_RFUNC occaKernelFree(occaKernel kernel) {
  occa::kernel kernel_((occa::kernel_v*) kernel);

  kernel_.free();
}

occaKernelInfo OCCA_RFUNC occaCreateKernelInfo() {
  occa::kernelInfo *info = new occa::kernelInfo();

  return (occaKernelInfo) info;
}

void OCCA_RFUNC occaKernelInfoAddDefine(occaKernelInfo info,
                                        const char *macro,
                                        occaType value) {

  occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

  info_.addDefine(macro, occa::c::typeToStr(value));

  delete value;
}

void OCCA_RFUNC occaKernelInfoAddInclude(occaKernelInfo info,
                                         const char *filename) {

  occa::kernelInfo &info_ = *((occa::kernelInfo*) info);

  info_.addInclude(filename);
}

void OCCA_RFUNC occaKernelInfoFree(occaKernelInfo info) {
  delete (occa::kernelInfo*) info;
}
//======================================


//---[ Helper Functions ]---------------
int OCCA_RFUNC occaSysCall(const char *cmdline,
                           char **output) {
  if(output == NULL)
    return occa::sys::call(cmdline);

  std::string sOutput;
  int ret = occa::sys::call(cmdline, sOutput);

  const size_t chars = sOutput.size();
  *output = (char*) ::malloc(chars + 1);

  ::memcpy(*output, sOutput.c_str(), chars);
  output[chars] = 0;

  return ret;
}
//======================================


//---[ Wrappers ]-----------------------
// [REFORMAT] wrapDevice
occaMemory OCCA_RFUNC occaDeviceWrapMemory(occaDevice device,
                                           void *handle_,
                                           const uintptr_t bytes) {

  occa::device device_((occa::device_v*) device);
  occa::memory memory_ = device_.wrapMemory(handle_, bytes);

  occaMemory memory = occa::c::newType(occa::c::memory_);
  memory->ptr->value.data.void_ = memory_.getMHandle();
  return memory;
}
//======================================


//---[ Memory ]-------------------------
const char* OCCA_RFUNC occaMemoryMode(occaMemory memory) {
  occa::memory memory_((occa::memory_v*) occa::c::typeValue(memory).data.void_);
  return memory_.mode().c_str();
}

void* OCCA_RFUNC occaMemoryGetMemoryHandle(occaMemory memory) {
  occa::memory memory_((occa::memory_v*) occa::c::typeValue(memory).data.void_);
  return memory_.getMemoryHandle();
}

void* OCCA_RFUNC occaMemoryGetMappedPointer(occaMemory memory) {
  occa::memory memory_((occa::memory_v*) occa::c::typeValue(memory).data.void_);
  return memory_.getMappedPointer();
}

void OCCA_RFUNC occaMemcpy(void *dest, void *src,
                           const uintptr_t bytes) {

  occa::memcpy(dest, src, bytes, occa::autoDetect);
}

void OCCA_RFUNC occaAsyncMemcpy(void *dest, void *src,
                                const uintptr_t bytes) {

  occa::asyncMemcpy(dest, src, bytes, occa::autoDetect);
}

void OCCA_RFUNC occaCopyMemToMem(occaMemory dest, occaMemory src,
                                 const uintptr_t bytes,
                                 const uintptr_t destOffset,
                                 const uintptr_t srcOffset) {

  occa::memory src_ = occa::c::typeToMemory(src);
  occa::memory dest_ = occa::c::typeToMemory(dest);

  memcpy(dest_, src_, bytes, destOffset, srcOffset);
}

void OCCA_RFUNC occaCopyPtrToMem(occaMemory dest, const void *src,
                                 const uintptr_t bytes,
                                 const uintptr_t offset) {

  occa::memory dest_ = occa::c::typeToMemory(dest);

  memcpy(dest_, src, bytes, offset);
}

void OCCA_RFUNC occaCopyMemToPtr(void *dest, occaMemory src,
                                 const uintptr_t bytes,
                                 const uintptr_t offset) {

  occa::memory src_ = occa::c::typeToMemory(src);

  memcpy(dest, src_, bytes, offset);
}

void OCCA_RFUNC occaAsyncCopyMemToMem(occaMemory dest, occaMemory src,
                                      const uintptr_t bytes,
                                      const uintptr_t destOffset,
                                      const uintptr_t srcOffset) {

  occa::memory src_ = occa::c::typeToMemory(src);
  occa::memory dest_ = occa::c::typeToMemory(dest);

  asyncMemcpy(dest_, src_, bytes, destOffset, srcOffset);
}

void OCCA_RFUNC occaAsyncCopyPtrToMem(occaMemory dest, const void * src,
                                      const uintptr_t bytes,
                                      const uintptr_t offset) {

  occa::memory dest_ = occa::c::typeToMemory(dest);

  asyncMemcpy(dest_, src, bytes, offset);
}

void OCCA_RFUNC occaAsyncCopyMemToPtr(void *dest, occaMemory src,
                                      const uintptr_t bytes,
                                      const uintptr_t offset) {

  occa::memory src_ = occa::c::typeToMemory(src);

  asyncMemcpy(dest, src_, bytes, offset);
}

void OCCA_RFUNC occaMemoryFree(occaMemory memory) {
  occa::memory memory_ = occa::c::typeToMemory(memory);
  memory_.free();
  delete memory;
}
//======================================

OCCA_END_EXTERN_C
