#if OCCA_CUDA_ENABLED

#include "occa/CUDA.hpp"

namespace occa {
  namespace cuda {
    template <>
    kernel_t<CUDA>::kernel_t(){
      strMode = "CUDA";

      data    = NULL;
      dHandle = NULL;

      dims  = 1;
      inner = occa::dim(1,1,1);
      outer = occa::dim(1,1,1);

      maximumInnerDimSize_ = 0;
      preferredDimSize_    = 0;
    }

    template <>
    kernel_t<CUDA>::kernel_t(const kernel_t<CUDA> &k){
      *this = k;
    }

    template <>
    kernel_t<CUDA>& kernel_t<CUDA>::operator = (const kernel_t<CUDA> &k){
      data    = k.data;
      dHandle = k.dHandle;

      metaInfo = k.metaInfo;

      dims  = k.dims;
      inner = k.inner;
      outer = k.outer;

      nestedKernels = k.nestedKernels;

      preferredDimSize_ = k.preferredDimSize_;

      return *this;
    }

    template <>
    kernel_t<CUDA>::~kernel_t(){}

    template <>
    void* kernel_t<CUDA>::getKernelHandle(){
      OCCA_EXTRACT_DATA(CUDA, Kernel);

      return data_.function;
    }

    template <>
    void* kernel_t<CUDA>::getProgramHandle(){
      OCCA_EXTRACT_DATA(CUDA, Kernel);

      return data_.module;
    }

    template <>
    std::string kernel_t<CUDA>::fixBinaryName(const std::string &filename){
      return filename;
    }

    template <>
    kernel_t<CUDA>* kernel_t<CUDA>::buildFromSource(const std::string &filename,
                                                    const std::string &functionName,
                                                    const kernelInfo &info_){

      name = functionName;

      OCCA_EXTRACT_DATA(CUDA, Kernel);
      kernelInfo info = info_;

      dHandle->addOccaHeadersToInfo(info);

      const std::string hash = getFileContentHash(filename,
                                                  dHandle->getInfoSalt(info));

      const std::string hashDir       = hashDirFor(filename, hash);
      const std::string sourceFile    = hashDir + kc::sourceFile;
      const std::string binaryFile    = hashDir + fixBinaryName(kc::binaryFile);
      const std::string ptxBinaryFile = hashDir + "ptxBinary.o";
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

      std::string archSM = "";

      if((dHandle->compilerFlags.find("-arch=sm_") == std::string::npos) &&
         (            info.flags.find("-arch=sm_") == std::string::npos)){

        std::stringstream archSM_;

        int major, minor;
        OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Getting CUDA Device Arch",
                        cuDeviceComputeCapability(&major, &minor, data_.device) );

        archSM_ << " -arch=sm_" << major << minor << ' ';

        archSM = archSM_.str();
      }

      std::stringstream command;

      if(verboseCompilation_f)
        std::cout << "Compiling [" << functionName << "]\n";

#if 0
      //---[ PTX Check Command ]----------
      if(dHandle->compilerEnvScript.size())
        command << dHandle->compilerEnvScript << " && ";

      command << dHandle->compiler
              << " -I."
              << " -I"  << env::OCCA_DIR << "include"
#  if (OCCA_OS == WINDOWS_OS)
              << " -D OCCA_OS=WINDOWS_OS -D _MSC_VER=1800"
#  endif
              << ' '          << dHandle->compilerFlags
              << archSM
              << " -Xptxas -v,-dlcm=cg"
              << ' '          << info.flags
              << " -x cu -c " << sourceFile
              << " -o "       << ptxBinaryFile;

      const std::string &ptxCommand = command.str();

      if(verboseCompilation_f)
        std::cout << "Compiling [" << functionName << "]\n" << ptxCommand << "\n";

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      ignoreResult( system(ptxCommand.c_str()) );
#else
      ignoreResult( system(("\"" +  ptxCommand + "\"").c_str()) );
#endif
#endif

      //---[ Compiling Command ]----------
      command.str("");

      command << dHandle->compiler
              << " -o "       << binaryFile
              << " -ptx -I."
              << " -I"  << env::OCCA_DIR << "include"
#  if (OCCA_OS == WINDOWS_OS)
              << " -D OCCA_OS=WINDOWS_OS -D _MSC_VER=1800"
#  endif
              << ' '          << dHandle->compilerFlags
              << archSM
              << ' '          << info.flags
              << " -x cu "    << sourceFile;

      const std::string &sCommand = command.str();

      if(verboseCompilation_f)
        std::cout << sCommand << '\n';

      const int compileError = system(sCommand.c_str());

      if(compileError){
        releaseHash(hash, 0);
        OCCA_CHECK(false, "Compilation error");
      }

      const CUresult moduleLoadError = cuModuleLoad(&data_.module,
                                                    binaryFile.c_str());

      if(moduleLoadError)
        releaseHash(hash, 0);

      OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Loading Module",
                      moduleLoadError);

      const CUresult moduleGetFunctionError = cuModuleGetFunction(&data_.function,
                                                                  data_.module,
                                                                  functionName.c_str());

      if(moduleGetFunctionError)
        releaseHash(hash, 0);

      OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Loading Function",
                      moduleGetFunctionError);

      releaseHash(hash, 0);

      return this;
    }

    template <>
    kernel_t<CUDA>* kernel_t<CUDA>::buildFromBinary(const std::string &filename,
                                                    const std::string &functionName){

      name = functionName;

      OCCA_EXTRACT_DATA(CUDA, Kernel);

      OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Loading Module",
                      cuModuleLoad(&data_.module, filename.c_str()));

      OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Loading Function",
                      cuModuleGetFunction(&data_.function, data_.module, functionName.c_str()));

      return this;
    }

    template <>
    kernel_t<CUDA>* kernel_t<CUDA>::loadFromLibrary(const char *cache,
                                                    const std::string &functionName){
      OCCA_EXTRACT_DATA(CUDA, Kernel);

      OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Loading Module",
                      cuModuleLoadData(&data_.module, cache));

      OCCA_CUDA_CHECK("Kernel (" + functionName + ") : Loading Function",
                      cuModuleGetFunction(&data_.function, data_.module, functionName.c_str()));

      return this;
    }

    template <>
    uintptr_t kernel_t<CUDA>::maximumInnerDimSize(){
      if(maximumInnerDimSize_)
        return maximumInnerDimSize_;

      OCCA_EXTRACT_DATA(CUDA, Kernel);

      int maxSize;

      OCCA_CUDA_CHECK("Kernel: Getting Maximum Inner-Dim Size",
                      cuFuncGetAttribute(&maxSize, CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, data_.function));

      maximumInnerDimSize_ = (uintptr_t) maxSize;

      return maximumInnerDimSize_;
    }

    template <>
    int kernel_t<CUDA>::preferredDimSize(){
      preferredDimSize_ = 32;
      return 32;
    }

    template <>
    void kernel_t<CUDA>::runFromArguments(const int kArgc, const kernelArg *kArgs){
      CUDAKernelData_t &data_ = *((CUDAKernelData_t*) data);
      CUfunction function_ = data_.function;

      int occaKernelInfoArgs = 0;
      int argc = 0;

      data_.vArgs = new void*[1 + kernelArg::argumentCount(kArgc, kArgs)];
      data_.vArgs[argc++] = &occaKernelInfoArgs;
      for(int i = 0; i < kArgc; ++i) {
        for(int j = 0; j < kArgs[i].argc; ++j){
          data_.vArgs[argc++] = kArgs[i].args[j].ptr();
        }
      }

      OCCA_CUDA_CHECK("Launching Kernel",
                      cuLaunchKernel(function_,
                                     outer.x, outer.y, outer.z,
                                     inner.x, inner.y, inner.z,
                                     0, *((CUstream*) dHandle->currentStream),
                                     data_.vArgs, 0));
      delete [] data_.vArgs;
    }

    template <>
    void kernel_t<CUDA>::free(){
      OCCA_EXTRACT_DATA(CUDA, Kernel);

      OCCA_CUDA_CHECK("Kernel (" + name + ") : Unloading Module",
                      cuModuleUnload(data_.module));

      delete (CUDAKernelData_t*) this->data;
    }
  }
}

#endif
