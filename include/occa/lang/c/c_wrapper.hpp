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

#ifndef OCCA_CBASE_HEADER
#define OCCA_CBASE_HEADER

#include "stdint.h"
#include "stdlib.h"

#include "occa/defines.hpp"

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_OSX_OS))
#  define OCCA_RFUNC
#  define OCCA_LFUNC
#else
#  define OCCA_RFUNC __stdcall
#  ifdef OCCA_C_EXPORTS
//   #define OCCA_LFUNC __declspec(dllexport)
#    define OCCA_LFUNC
#  else
//   #define OCCA_LFUNC __declspec(dllimport)
#    define OCCA_LFUNC
#  endif
#endif

OCCA_START_EXTERN_C

struct occaObject_t;
struct occaObject {
  struct occaObject_t *ptr;
};

typedef struct occaObject occaType;
typedef struct occaObject occaDevice;
typedef struct occaObject occaKernel;
typedef struct occaObject occaMemory;
typedef struct occaObject occaStream;
typedef struct occaObject occaProperties;
typedef struct occaObject occaArgumentList;

typedef struct occaStreamTag_t {
  double tagTime;
  void *handle;
} occaStreamTag;

typedef struct occaDim_t {
  uintptr_t x, y, z;
} occaDim;


//---[ Globals & Flags ]----------------
extern OCCA_LFUNC occaObject occaDefault;
//======================================


//---[ TypeCasting ]--------------------
//  ---[ Known Types ]------------------
OCCA_LFUNC occaObject OCCA_RFUNC occaPtr(void *value);

OCCA_LFUNC occaObject OCCA_RFUNC occaInt8(int value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUInt8(unsigned int value);

OCCA_LFUNC occaObject OCCA_RFUNC occaInt16(int value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUInt16(unsigned int value);

OCCA_LFUNC occaObject OCCA_RFUNC occaInt32(int value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUInt32(unsigned int value);

OCCA_LFUNC occaObject OCCA_RFUNC occaInt64(int value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUInt64(unsigned int value);
//  ====================================

//  ---[ Ambiguous Types ]--------------
OCCA_LFUNC occaObject OCCA_RFUNC occaInt(int value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUInt(unsigned int value);

OCCA_LFUNC occaObject OCCA_RFUNC occaChar(char value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUChar(unsigned char value);

OCCA_LFUNC occaObject OCCA_RFUNC occaShort(short value);
OCCA_LFUNC occaObject OCCA_RFUNC occaUShort(unsigned short value);

OCCA_LFUNC occaObject OCCA_RFUNC occaLong(long value);
OCCA_LFUNC occaObject OCCA_RFUNC occaULong(unsigned long value);

OCCA_LFUNC occaObject OCCA_RFUNC occaFloat(float value);
OCCA_LFUNC occaObject OCCA_RFUNC occaDouble(double value);

OCCA_LFUNC occaObject OCCA_RFUNC occaStruct(void *value, uintptr_t bytes);
OCCA_LFUNC occaObject OCCA_RFUNC occaString(const char *str);
//  ====================================
//======================================


//---[ Background Device ]--------------
//  |---[ Device ]----------------------
OCCA_LFUNC void OCCA_RFUNC occaSetDevice(occaDevice device);
OCCA_LFUNC void OCCA_RFUNC occaSetDeviceFromInfo(const char *infos);

OCCA_LFUNC void OCCA_RFUNC occaFlush();
OCCA_LFUNC void OCCA_RFUNC occaFinish();

OCCA_LFUNC void OCCA_RFUNC occaWaitFor(occaStreamTag tag);

OCCA_LFUNC occaStream OCCA_RFUNC occaCreateStream();
OCCA_LFUNC occaStream OCCA_RFUNC occaGetStream();
OCCA_LFUNC void OCCA_RFUNC occaSetStream(occaStream stream);
OCCA_LFUNC occaStream OCCA_RFUNC occaWrapStream(void *handle_);

OCCA_LFUNC occaStreamTag OCCA_RFUNC occaTagStream();
//  |===================================

//  |---[ Kernel ]----------------------
OCCA_LFUNC occaKernel OCCA_RFUNC occaBuildKernel(const char *str,
                                                 const char *functionName,
                                                 occaKernelInfo info);

OCCA_LFUNC occaKernel OCCA_RFUNC occaBuildKernelFromSource(const char *filename,
                                                           const char *functionName,
                                                           occaKernelInfo info);

OCCA_LFUNC occaKernel OCCA_RFUNC occaBuildKernelFromString(const char *str,
                                                           const char *functionName,
                                                           occaKernelInfo info,
                                                           occaLanguage language);

OCCA_LFUNC occaKernel OCCA_RFUNC occaBuildKernelFromBinary(const char *filename,
                                                           const char *functionName);
//  |===================================

//  |---[ Memory ]----------------------
OCCA_LFUNC void OCCA_RFUNC occaMemorySwap(occaMemory a, occaMemory b);

OCCA_LFUNC occaMemory OCCA_RFUNC occaWrapMemory(void *handle_,
                                                const uintptr_t bytes);

OCCA_LFUNC void OCCA_RFUNC occaWrapManagedMemory(void *handle_,
                                                 const uintptr_t bytes);

OCCA_LFUNC occaMemory OCCA_RFUNC occaMalloc(const uintptr_t bytes,
                                            void *src);

OCCA_LFUNC void* OCCA_RFUNC occaManagedAlloc(const uintptr_t bytes,
                                             void *src);

OCCA_LFUNC occaMemory OCCA_RFUNC occaMappedAlloc(const uintptr_t bytes,
                                                 void *src);

OCCA_LFUNC void* OCCA_RFUNC occaManagedMappedAlloc(const uintptr_t bytes,
                                                   void *src);
//  |===================================
//======================================


//---[ Device ]-------------------------
OCCA_LFUNC void OCCA_RFUNC occaPrintAvailableDevices();

OCCA_LFUNC occaDeviceInfo OCCA_RFUNC occaCreateDeviceInfo();

OCCA_LFUNC void OCCA_RFUNC occaDeviceInfoAppend(occaDeviceInfo info,
                                                const char *key,
                                                const char *value);

OCCA_LFUNC void OCCA_RFUNC occaDeviceInfoAppendType(occaDeviceInfo info,
                                                    const char *key,
                                                    occaObject value);

OCCA_LFUNC void OCCA_RFUNC occaDeviceInfoFree(occaDeviceInfo info);

OCCA_LFUNC occaDevice OCCA_RFUNC occaCreateDevice(const char *infos);

OCCA_LFUNC occaDevice OCCA_RFUNC occaCreateDeviceFromInfo(occaDeviceInfo dInfo);

OCCA_LFUNC const char* OCCA_RFUNC occaDeviceMode(occaDevice device);

OCCA_LFUNC void OCCA_RFUNC occaDeviceSetCompiler(occaDevice device,
                                                 const char *compiler);

OCCA_LFUNC void OCCA_RFUNC occaDeviceSetCompilerEnvScript(occaDevice device,
                                                          const char *compilerEnvScript);

OCCA_LFUNC void OCCA_RFUNC occaDeviceSetCompilerFlags(occaDevice device,
                                                      const char *compilerFlags);

OCCA_LFUNC const char* OCCA_RFUNC occaDeviceGetCompiler(occaDevice device);
OCCA_LFUNC const char* OCCA_RFUNC occaDeviceGetCompilerEnvScript(occaDevice device);
OCCA_LFUNC const char* OCCA_RFUNC occaDeviceGetCompilerFlags(occaDevice device);


OCCA_LFUNC uintptr_t OCCA_RFUNC occaDeviceMemorySize(occaDevice device);
OCCA_LFUNC uintptr_t OCCA_RFUNC occaDeviceMemoryAllocated(occaDevice device);
OCCA_LFUNC uintptr_t OCCA_RFUNC occaDeviceBytesAllocated(occaDevice device);

OCCA_LFUNC occaKernel OCCA_RFUNC occaDeviceBuildKernel(occaDevice device,
                                                       const char *str,
                                                       const char *functionName,
                                                       occaKernelInfo info);

OCCA_LFUNC occaKernel OCCA_RFUNC occaDeviceBuildKernelFromSource(occaDevice device,
                                                                 const char *filename,
                                                                 const char *functionName,
                                                                 occaKernelInfo info);

OCCA_LFUNC occaKernel OCCA_RFUNC occaDeviceBuildKernelFromString(occaDevice device,
                                                                 const char *str,
                                                                 const char *functionName,
                                                                 occaKernelInfo info);

OCCA_LFUNC occaKernel OCCA_RFUNC occaDeviceBuildKernelFromBinary(occaDevice device,
                                                                 const char *filename,
                                                                 const char *functionName);

OCCA_LFUNC occaMemory OCCA_RFUNC occaDeviceMalloc(occaDevice device,
                                                  uintptr_t bytes,
                                                  void *src);

OCCA_LFUNC void* OCCA_RFUNC occaDeviceManagedAlloc(occaDevice device,
                                                   uintptr_t bytes,
                                                   void *src);

OCCA_LFUNC occaMemory OCCA_RFUNC occaDeviceMappedAlloc(occaDevice device,
                                                       uintptr_t bytes,
                                                       void *src);

OCCA_LFUNC void* OCCA_RFUNC occaDeviceManagedMappedAlloc(occaDevice device,
                                                         uintptr_t bytes,
                                                         void *src);

OCCA_LFUNC void OCCA_RFUNC occaDeviceFlush(occaDevice device);
OCCA_LFUNC void OCCA_RFUNC occaDeviceFinish(occaDevice device);

OCCA_LFUNC occaStream OCCA_RFUNC occaDeviceCreateStream(occaDevice device);
OCCA_LFUNC occaStream OCCA_RFUNC occaDeviceGetStream(occaDevice device);
OCCA_LFUNC void       OCCA_RFUNC occaDeviceSetStream(occaDevice device, occaStream stream);
OCCA_LFUNC occaStream OCCA_RFUNC occaDeviceWrapStream(occaDevice device, void *handle_);

OCCA_LFUNC occaStreamTag OCCA_RFUNC occaDeviceTagStream(occaDevice device);
OCCA_LFUNC void OCCA_RFUNC occaDeviceWaitForTag(occaDevice device,
                                                occaStreamTag tag);
OCCA_LFUNC double OCCA_RFUNC occaDeviceTimeBetweenTags(occaDevice device,
                                                       occaStreamTag startTag, occaStreamTag endTag);

OCCA_LFUNC void OCCA_RFUNC occaGetStreamFree(occaStream stream);
OCCA_LFUNC void OCCA_RFUNC occaStreamFree(occaStream stream);
OCCA_LFUNC void OCCA_RFUNC occaDeviceFree(occaDevice device);
//======================================


//---[ Kernel ]-------------------------
OCCA_LFUNC const char* OCCA_RFUNC occaKernelMode(occaKernel kernel);
OCCA_LFUNC const char* OCCA_RFUNC occaKernelName(occaKernel kernel);

OCCA_LFUNC occaDevice OCCA_RFUNC occaKernelGetDevice(occaKernel kernel);

OCCA_LFUNC void OCCA_RFUNC occaKernelSetWorkingDims(occaKernel kernel,
                                                    int dims,
                                                    occaDim items,
                                                    occaDim groups);

OCCA_LFUNC void OCCA_RFUNC occaKernelSetAllWorkingDims(occaKernel kernel,
                                                       int dims,
                                                       uintptr_t itemsX, uintptr_t itemsY, uintptr_t itemsZ,
                                                       uintptr_t groupsX, uintptr_t groupsY, uintptr_t groupsZ);

OCCA_LFUNC occaArgumentList OCCA_RFUNC occaCreateArgumentList();

OCCA_LFUNC void OCCA_RFUNC occaArgumentListClear(occaArgumentList list);

OCCA_LFUNC void OCCA_RFUNC occaArgumentListFree(occaArgumentList list);

OCCA_LFUNC void OCCA_RFUNC occaArgumentListAddArg(occaArgumentList list,
                                                  int argPos,
                                                  void *type);

OCCA_LFUNC void OCCA_RFUNC occaKernelRun_(occaKernel kernel,
                                          occaArgumentList list);

OCCA_LFUNC void OCCA_RFUNC occaKernelRunN(occaKernel kernel,
                                          const int argc, struct occaObject_t **args);

#include "occa/operators/cKernelOperators.hpp"

OCCA_LFUNC void OCCA_RFUNC occaKernelFree(occaKernel kernel);

OCCA_LFUNC occaKernelInfo OCCA_RFUNC occaCreateKernelInfo();

OCCA_LFUNC void OCCA_RFUNC occaKernelInfoAddDefine(occaKernelInfo info,
                                                   const char *macro,
                                                   occaObject value);

OCCA_LFUNC void OCCA_RFUNC occaKernelInfoAddInclude(occaKernelInfo info,
                                                    const char *filename);

OCCA_LFUNC void OCCA_RFUNC occaKernelInfoFree(occaKernelInfo info);
//======================================


//---[ Wrappers ]-----------------------
// [REFORMAT] wrapDevice
OCCA_LFUNC occaMemory OCCA_RFUNC occaDeviceWrapMemory(occaDevice device,
                                                      void *handle_,
                                                      const uintptr_t bytes);
//======================================


//---[ Memory ]-------------------------
OCCA_LFUNC const char* OCCA_RFUNC occaMemoryMode(occaMemory memory);

OCCA_LFUNC void* OCCA_RFUNC occaMemoryGetMemoryHandle(occaMemory mem);
OCCA_LFUNC void* OCCA_RFUNC occaMemoryGetMappedPointer(occaMemory mem);

OCCA_LFUNC void OCCA_RFUNC occaMemcpy(void *dest, void *src,
                                      const uintptr_t bytes);

OCCA_LFUNC void OCCA_RFUNC occaAsyncMemcpy(void *dest, void *src,
                                           const uintptr_t bytes);

OCCA_LFUNC void OCCA_RFUNC occaCopyMemToMem(occaMemory dest, occaMemory src,
                                            const uintptr_t bytes,
                                            const uintptr_t destOffset,
                                            const uintptr_t srcOffset);

OCCA_LFUNC void OCCA_RFUNC occaCopyPtrToMem(occaMemory dest, const void *src,
                                            const uintptr_t bytes, const uintptr_t offset);

OCCA_LFUNC void OCCA_RFUNC occaCopyMemToPtr(void *dest, occaMemory src,
                                            const uintptr_t bytes, const uintptr_t offset);

OCCA_LFUNC void OCCA_RFUNC occaAsyncCopyMemToMem(occaMemory dest, occaMemory src,
                                                 const uintptr_t bytes,
                                                 const uintptr_t destOffset,
                                                 const uintptr_t srcOffset);

OCCA_LFUNC void OCCA_RFUNC occaAsyncCopyPtrToMem(occaMemory dest, const void *src,
                                                 const uintptr_t bytes, const uintptr_t offset);

OCCA_LFUNC void OCCA_RFUNC occaAsyncCopyMemToPtr(void *dest, occaMemory src,
                                                 const uintptr_t bytes, const uintptr_t offset);

OCCA_LFUNC void OCCA_RFUNC occaMemoryFree(occaMemory memory);
//======================================

OCCA_END_EXTERN_C

#endif
