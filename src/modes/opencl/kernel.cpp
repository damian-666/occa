#if OCCA_OPENCL_ENABLED

#include "occa/modes/opencl/kernel.hpp"

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

    info_ kernel::makeCLInfo() {
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

      std::string catFlags = info.flags + dHandle->compilerFlags;

      cl::buildKernelFromSource(makeCLInfo(),
                                cFunction.c_str(), cFunction.size(),
                                functionName,
                                catFlags,
                                hash, sourceFile);

      cl::saveProgramBinary(makeCLInfo(), binaryFile, hash);

      releaseHash(hash, 0);

      return this;
    }

    void kernel::buildFromBinary(const std::string &filename,
                                 const std::string &functionName){

      name = functionName;

      std::string cFile = readFile(filename);

      cl::buildKernelFromBinary(makeCLInfo(),
                                (const unsigned char*) cFile.c_str(),
                                cFile.size(),
                                functionName,
                                dHandle->compilerFlags);

      return this;
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
