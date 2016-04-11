#include "occa/Serial.hpp"
#include "occa/Pthreads.hpp"

namespace occa {
  namespace threads {
    kernel_t<Pthreads>::kernel_t(){
      strMode = "Pthreads";

      data    = NULL;
      dHandle = NULL;

      dims  = 1;
      inner = occa::dim(1,1,1);
      outer = occa::dim(1,1,1);
    }

    kernel_t<Pthreads>::kernel_t(const kernel_t<Pthreads> &k){
      *this = k;
    }

    kernel_t<Pthreads>& kernel_t<Pthreads>::operator = (const kernel_t<Pthreads> &k){
      data    = k.data;
      dHandle = k.dHandle;

      metaInfo = k.metaInfo;

      dims  = k.dims;
      inner = k.inner;
      outer = k.outer;

      nestedKernels = k.nestedKernels;

      return *this;
    }

    kernel_t<Pthreads>::~kernel_t(){}

    void* kernel_t<Pthreads>::getKernelHandle(){
      OCCA_EXTRACT_DATA(Pthreads, Kernel);

      void *ret;

      ::memcpy(&ret, &data_.handle, sizeof(void*));

      return ret;
    }

    void* kernel_t<Pthreads>::getProgramHandle(){
      OCCA_EXTRACT_DATA(Pthreads, Kernel);

      return data_.dlHandle;
    }

    std::string kernel_t<Pthreads>::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    kernel_t<Pthreads>* kernel_t<Pthreads>::buildFromSource(const std::string &filename,
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

      data = new PthreadsKernelData_t;

      createSourceFileFrom(filename, hashDir, info);

      std::stringstream command;

      if(dHandle->compilerEnvScript.size())
        command << dHandle->compilerEnvScript << " && ";

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      command << dHandle->compiler
              << ' '    << dHandle->compilerFlags
              << ' '    << info.flags
              << ' '    << sourceFile
              << " -o " << binaryFile
              << " -I"  << env::OCCA_DIR << "/include"
              << " -L"  << env::OCCA_DIR << "/lib -locca"
              << std::endl;
#else
#  if (OCCA_DEBUG_ENABLED)
      std::string occaLib = env::OCCA_DIR + "\\lib\\libocca_d.lib ";
#  else
      std::string occaLib = env::OCCA_DIR + "\\lib\\libocca.lib ";
#  endif
      std::string ptLib   = env::OCCA_DIR + "\\lib\\pthreadVC2.lib ";

      command << dHandle->compiler
              << " /D MC_CL_EXE"
              << ' '    << dHandle->compilerFlags
              << ' '    << info.flags
              << " /I"  << env::OCCA_DIR << "\\include"
              << ' '    << sourceFile
              << " /link " << occaLib << ptLib << " /OUT:" << binaryFile
              << std::endl;
#endif

      const std::string &sCommand = command.str();

      if(verboseCompilation_f)
        std::cout << "Compiling [" << functionName << "]\n" << sCommand << "\n";

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      const int compileError = system(sCommand.c_str());
#else
      const int compileError = system(("\"" +  sCommand + "\"").c_str());
#endif

      if(compileError){
        releaseHash(hash, 0);
        OCCA_CHECK(false, "Compilation error");
      }

      OCCA_EXTRACT_DATA(Pthreads, Kernel);

      data_.dlHandle = sys::dlopen(binaryFile, hash);
      data_.handle   = sys::dlsym(data_.dlHandle, functionName, hash);

      PthreadsDeviceData_t &dData = *((PthreadsDeviceData_t*) ((device_t<Pthreads>*) dHandle)->data);

      data_.pThreadCount = dData.pThreadCount;

      data_.pendingJobs = &(dData.pendingJobs);

      for(int p = 0; p < 50; ++p)
        data_.pKernelInfo[p] = &(dData.pKernelInfo[p]);

      data_.pendingJobsMutex = &(dData.pendingJobsMutex);
      data_.kernelMutex      = &(dData.kernelMutex);

      releaseHash(hash, 0);

      return this;
    }

    kernel_t<Pthreads>* kernel_t<Pthreads>::buildFromBinary(const std::string &filename,
                                                            const std::string &functionName){

      name = functionName;

      data = new PthreadsKernelData_t;

      OCCA_EXTRACT_DATA(Pthreads, Kernel);

      data_.dlHandle = sys::dlopen(filename);
      data_.handle   = sys::dlsym(data_.dlHandle, functionName);

      PthreadsDeviceData_t &dData = *((PthreadsDeviceData_t*) ((device_t<Pthreads>*) dHandle)->data);

      data_.pThreadCount = dData.pThreadCount;

      data_.pendingJobs = &(dData.pendingJobs);

      for(int p = 0; p < 50; ++p)
        data_.pKernelInfo[p] = &(dData.pKernelInfo[p]);

      data_.pendingJobsMutex = &(dData.pendingJobsMutex);
      data_.kernelMutex      = &(dData.kernelMutex);

      return this;
    }

    kernel_t<Pthreads>* kernel_t<Pthreads>::loadFromLibrary(const char *cache,
                                                            const std::string &functionName){
      return buildFromBinary(cache, functionName);
    }

    uintptr_t kernel_t<Pthreads>::maximumInnerDimSize(){
      return ((uintptr_t) -1);
    }

    // [-] Missing
    int kernel_t<Pthreads>::preferredDimSize(){
      preferredDimSize_ = OCCA_SIMD_WIDTH;
      return OCCA_SIMD_WIDTH;
    }

    void kernel_t<Pthreads>::runFromArguments(const int kArgc, const kernelArg *kArgs){
      OCCA_EXTRACT_DATA(Pthreads, Kernel);

      const int pThreadCount = data_.pThreadCount;

      for(int p = 0; p < pThreadCount; ++p){
        // Allocated individually since each thread frees their
        //   own custom arg
        PthreadKernelInfo_t &pArgs = *(new PthreadKernelInfo_t);

        pArgs.rank  = p;
        pArgs.count = pThreadCount;

        pArgs.kernelHandle = data_.handle;

        pArgs.dims  = dims;
        pArgs.inner = inner;
        pArgs.outer = outer;

        int argc = 0;
        pArgs.argc = kernelArg::argumentCount(kArgc, kArgs);
        pArgs.args = new void*[pArgs.argc];
        for(int i = 0; i < pArgs.argc; ++i){
          for(int j = 0; j < kArgs[i].argc; ++j){
            pArgs.args[argc++] = kArgs[i].args[j].ptr();
          }
        }

        data_.kernelMutex->lock();
        data_.pKernelInfo[p]->push(&pArgs);
        data_.kernelMutex->unlock();
      }

      data_.pendingJobsMutex->lock();
      *(data_.pendingJobs) += data_.pThreadCount;
      data_.pendingJobsMutex->unlock();
    }

    void kernel_t<Pthreads>::free(){
      // [-] Fix later
      OCCA_EXTRACT_DATA(Pthreads, Kernel);

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      dlclose(data_.dlHandle);
#else
      FreeLibrary((HMODULE) (data_.dlHandle));
#endif
    }
  }
}
