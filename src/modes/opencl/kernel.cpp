#if OCCA_OPENCL_ENABLED

#include "occa/modes/opencl/kernel.hpp"
#include "occa/modes/opencl/device.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace opencl {
    kernel::kernel() : occa::kernel_v() {}

    kernel::kernel(const kernel &k){
      *this = k;
    }

    kernel& kernel::operator = (const kernel &k){
      initFrom(k);

      platformID = k.platformID;
      deviceID   = k.deviceID;

      clPlatformID = k.clPlatformID;
      clDeviceID   = k.clDeviceID;
      clContext    = k.clContext;
      clProgram    = k.clProgram;
      clKernel     = k.clKernel;

      return *this;
    }

    kernel::~kernel(){}

    info_t kernel::makeCLInfo() {
      info_t info;
      info.clDeviceID = clDeviceID;
      info.clContext  = clContext;
      info.clProgram  = clProgram;
      info.clKernel   = clKernel;
      return info;
    }

    void* kernel::getKernelHandle(){
      return clKernel;
    }

    void* kernel::getProgramHandle(){
      return clProgram;
    }

    std::string kernel::fixBinaryName(const std::string &filename){
      return filename;
    }

    void kernel::buildFromSource(const std::string &filename,
                                 const std::string &functionName,
                                 const kernelInfo &info_){

      name = functionName;

      kernelInfo info = info_;
      dHandle->addOccaHeadersToInfo(info);

      const std::string hash = getFileContentHash(filename,
                                                  dHandle->getInfoSalt(info));

      const std::string hashDir    = hashDirFor(filename, hash);
      const std::string sourceFile = hashDir + kc::sourceFile;
      const std::string binaryFile = hashDir + fixBinaryName(kc::binaryFile);
      bool foundBinary = true;

      if (!haveHash(hash, 0))
        waitForHash(hash, 0);
      else if (sys::fileExists(binaryFile))
        releaseHash(hash, 0);
      else
        foundBinary = false;

      if (foundBinary) {
        if(verboseCompilation_f)
          std::cout << "Found cached binary of [" << compressFilename(filename) << "] in [" << compressFilename(binaryFile) << "]\n";

        return buildFromBinary(binaryFile, functionName);
      }

      createSourceFileFrom(filename, hashDir, info);

      std::string cFunction = readFile(sourceFile);
      std::string catFlags = info.flags + ((opencl::device*) dHandle)->compilerFlags;
      info_t clInfo = makeCLInfo();
      opencl::buildKernelFromSource(clInfo,
                                    cFunction.c_str(), cFunction.size(),
                                    functionName,
                                    catFlags,
                                    hash, sourceFile);
      clProgram = clInfo.clProgram;
      clKernel  = clInfo.clKernel;

      opencl::saveProgramBinary(clInfo, binaryFile, hash);

      releaseHash(hash, 0);
    }

    void kernel::buildFromBinary(const std::string &filename,
                                 const std::string &functionName){

      name = functionName;

      std::string cFile = readFile(filename);
      info_t clInfo = makeCLInfo();
      opencl::buildKernelFromBinary(clInfo,
                                    (const unsigned char*) cFile.c_str(),
                                    cFile.size(),
                                    functionName,
                                    ((opencl::device*) dHandle)->compilerFlags);
      clProgram = clInfo.clProgram;
      clKernel  = clInfo.clKernel;
    }

    int kernel::maxDims() {
      static cl_uint dims = 0;
      if (dims == 0) {
        size_t bytes;
        OCCA_CL_CHECK("Kernel: Max Dims",
                      clGetDeviceInfo(clDeviceID,
                                      CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                                      0, NULL, &bytes));
        OCCA_CL_CHECK("Kernel: Max Dims",
                      clGetDeviceInfo(clDeviceID,
                                      CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                                      bytes, &dims, NULL));
      }
      return (int) dims;
    }

    dim kernel::maxOuterDims() {
      static occa::dim outerDims(0);
      if (outerDims.x == 0) {
        int dims = maxDims();
        size_t *od = new size_t[dims];
        size_t bytes;
        OCCA_CL_CHECK("Kernel: Max Outer Dims",
                      clGetDeviceInfo(clDeviceID,
                                      CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                      0, NULL, &bytes));
        OCCA_CL_CHECK("Kernel: Max Outer Dims",
                      clGetDeviceInfo(clDeviceID,
                                      CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                      bytes, &od, NULL));
        for (int i = 0; i < dims; ++i) {
          outerDims[i] = od[i];
        }
        delete [] od;
      }
      return outerDims;
    }

    dim kernel::maxInnerDims() {
      static occa::dim innerDims(0);
      if (innerDims.x == 0) {
        size_t dims;
        size_t bytes;
        OCCA_CL_CHECK("Kernel: Max Inner Dims",
                      clGetKernelWorkGroupInfo(clKernel,
                                               clDeviceID,
                                               CL_KERNEL_WORK_GROUP_SIZE,
                                               0, NULL, &bytes));
        OCCA_CL_CHECK("Kernel: Max Inner Dims",
                      clGetKernelWorkGroupInfo(clKernel,
                                               clDeviceID,
                                               CL_KERNEL_WORK_GROUP_SIZE,
                                               bytes, &dims, NULL));
        innerDims.x = dims;
      }
      return innerDims;
    }

    void kernel::runFromArguments(const int kArgc, const kernelArg *kArgs){
      occa::dim fullOuter = outer*inner;

      int argc = 0;
      OCCA_CL_CHECK("Kernel (" + metaInfo.name + ") : Setting Kernel Argument [0]",
                    clSetKernelArg(clKernel, argc++, sizeof(void*), NULL));

      for(int i = 0; i < kArgc; ++i){
        for(int j = 0; j < kArgs[i].argc; ++j){
          OCCA_CL_CHECK("Kernel (" + metaInfo.name + ") : Setting Kernel Argument [" << (i + 1) << "]",
                        clSetKernelArg(clKernel, argc++, kArgs[i].args[j].size, kArgs[i].args[j].ptr()));
        }
      }

      OCCA_CL_CHECK("Kernel (" + metaInfo.name + ") : Kernel Run",
                    clEnqueueNDRangeKernel(*((cl_command_queue*) dHandle->currentStream),
                                           clKernel,
                                           (cl_int) dims,
                                           NULL,
                                           (uintptr_t*) &fullOuter,
                                           (uintptr_t*) &inner,
                                           0, NULL, NULL));
    }

    void kernel::free(){
      OCCA_CL_CHECK("Kernel: Free",
                    clReleaseKernel(clKernel));
    }
  }
}

#endif
