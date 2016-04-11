#if OCCA_OPENMP_ENABLED

#include <omp.h>

#include "occa/modes/openmp/kernel.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace openmp {
    kernel::kernel(){
      strMode = "OpenMP";

      data    = NULL;
      dHandle = NULL;

      dims  = 1;
      inner = occa::dim(1,1,1);
      outer = occa::dim(1,1,1);
    }

    kernel::kernel(const kernel &k){
      *this = k;
    }

    kernel& kernel::operator = (const kernel &k){
      data    = k.data;
      dHandle = k.dHandle;

      metaInfo = k.metaInfo;

      dims  = k.dims;
      inner = k.inner;
      outer = k.outer;

      nestedKernels = k.nestedKernels;

      return *this;
    }

    kernel::~kernel(){}

    void* kernel::getKernelHandle(){
      OCCA_EXTRACT_DATA(OpenMP, Kernel);

      void *ret;

      ::memcpy(&ret, &data_.handle, sizeof(void*));

      return ret;
    }

    void* kernel::getProgramHandle(){
      OCCA_EXTRACT_DATA(OpenMP, Kernel);

      return data_.dlHandle;
    }

    std::string kernel::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    kernel* kernel::buildFromSource(const std::string &filename,
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

      data = new OpenMPKernelData_t;

      createSourceFileFrom(filename, hashDir, info);

      std::stringstream command;

      if(dHandle->compilerEnvScript.size())
        command << dHandle->compilerEnvScript << " && ";

      //---[ Check if compiler flag is added ]------
      OpenMPDeviceData_t &dData_ = *((OpenMPDeviceData_t*) dHandle->data);

      const std::string ompFlag = dData_.OpenMPFlag;

      if((dHandle->compilerFlags.find(ompFlag) == std::string::npos) &&
         (            info.flags.find(ompFlag) == std::string::npos)){

        info.flags += ' ';
        info.flags += ompFlag;
      }
      //============================================

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

      OCCA_EXTRACT_DATA(OpenMP, Kernel);

      data_.dlHandle = sys::dlopen(binaryFile, hash);
      data_.handle   = sys::dlsym(data_.dlHandle, functionName, hash);

      releaseHash(hash, 0);

      return this;
    }

    kernel* kernel::buildFromBinary(const std::string &filename,
                                                        const std::string &functionName){

      name = functionName;

      data = new OpenMPKernelData_t;

      OCCA_EXTRACT_DATA(OpenMP, Kernel);

      data_.dlHandle = sys::dlopen(filename);
      data_.handle   = sys::dlsym(data_.dlHandle, functionName);

      return this;
    }

    kernel* kernel::loadFromLibrary(const char *cache,
                                                        const std::string &functionName){
      return buildFromBinary(cache, functionName);
    }

    uintptr_t kernel::maximumInnerDimSize(){
      return ((uintptr_t) -1);
    }

    // [-] Missing
    int kernel::preferredDimSize(){
      preferredDimSize_ = OCCA_SIMD_WIDTH;
      return OCCA_SIMD_WIDTH;
    }


    void kernel::runFromArguments(const int kArgc, const kernelArg *kArgs){
      OpenMPKernelData_t &data_ = *((OpenMPKernelData_t*) data);
      handleFunction_t tmpKernel = (handleFunction_t) data_.handle;
      int occaKernelArgs[6];

      occaKernelArgs[0] = outer.z; occaKernelArgs[3] = inner.z;
      occaKernelArgs[1] = outer.y; occaKernelArgs[4] = inner.y;
      occaKernelArgs[2] = outer.x; occaKernelArgs[5] = inner.x;

      int argc = 0;
      for(int i = 0; i < kArgc; ++i){
        for(int j = 0; j < kArgs[i].argc; ++j){
          data_.vArgs[argc++] = kArgs[i].args[j].ptr();
        }
      }

      int occaInnerId0 = 0, occaInnerId1 = 0, occaInnerId2 = 0;

      sys::runFunction(tmpKernel,
                       occaKernelArgs,
                       occaInnerId0, occaInnerId1, occaInnerId2,
                       argc, data_.vArgs);
    }

    void kernel::free(){
      OCCA_EXTRACT_DATA(OpenMP, Kernel);

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      dlclose(data_.dlHandle);
#else
      FreeLibrary((HMODULE) (data_.dlHandle));
#endif
    }
  }
}

#endif
