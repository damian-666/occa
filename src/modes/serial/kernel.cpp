#include "occa/modes/serial/kernel.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace serial {
    kernel::kernel(){
      dHandle = NULL;
      dlHandle = NULL;
      handle   = NULL;

      dims  = 1;
      inner = occa::dim(1,1,1);
      outer = occa::dim(1,1,1);
    }

    kernel::kernel(const kernel &k){
      *this = k;
    }

    kernel& kernel::operator = (const kernel &k){
      dHandle  = k.dHandle;
      dlHandle = k.dlHandle;
      handle   = k.handle;

      metaInfo = k.metaInfo;

      dims  = k.dims;
      inner = k.inner;
      outer = k.outer;

      nestedKernels = k.nestedKernels;

      for (int i = 0; i < 2*OCCA_MAX_ARGS; ++i) {
        vArgs[i] = k.vArgs[i];
      }

      return *this;
    }

    kernel::~kernel(){}

    void* kernel::getKernelHandle(){
      void *ret;
      ::memcpy(&ret, &handle, sizeof(void*));
      return ret;
    }

    void* kernel::getProgramHandle(){
      return dlHandle;
    }

    std::string kernel::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
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

        buildFromBinary(binaryFile, functionName);
      }

      createSourceFileFrom(filename, hashDir, info);

      occa::device device(dHandle);
      std::stringstream command;
      std::string compilerEnvScript = device.getCompilerEnvScript();
      if(compilerEnvScript.size())
        command << compilerEnvScript << " && ";

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      command << device.getCompiler()
              << ' '    << device.getCompilerFlags()
              << ' '    << info.flags
              << ' '    << sourceFile
              << " -o " << binaryFile
              << " -I"  << env::OCCA_DIR << "/include"
              << " -L"  << env::OCCA_DIR << "/lib -locca"
              << std::endl;
#else
#  if (OCCA_DEBUG_ENABLED)
      const std::string occaLib = env::OCCA_DIR + "/lib/libocca_d.lib ";
#  else
      const std::string occaLib = env::OCCA_DIR + "/lib/libocca.lib ";
#  endif

#  if OCCA_CUDA_ENABLED
      const std::string cupath = getenv("CUDA_PATH");
      const std::string cuInc  = cupath + "\\include";
      const std::string cuLib  = cupath + "\\lib\\x64\\cuda.lib ";
#  endif
#  if OCCA_OPENCL_ENABLED
      const std::string clpath = getenv("OPENCL_PATH");
      const std::string clLib  = cpath + "\\lib\\x64\\OpenCL.lib ";
#  endif

      command << dHandle->compiler
              << " /D MC_CL_EXE"
              << " /D OCCA_OS=WINDOWS_OS"
              << " /EHsc"
              << " /wd4244 /wd4800 /wd4804 /wd4018"
              << ' '    << dHandle->compilerFlags
              << ' '    << info.flags
              << " /I"  << env::OCCA_DIR << "/include"
#  if OCCA_CUDA_ENABLED
              << " /I"  << cuInc
#  endif
#  if OCCA_OPENCL_ENABLED
              << " /I"  << clInc
#  endif
              << ' '    << sourceFile
              << " /link " << occaLib
#  if OCCA_CUDA_ENABLED
              << " /link"  << cuLib
#  endif
#  if OCCA_OPENCL_ENABLED
              << " /link"  << clLib
#  endif
              << " /OUT:" << binaryFile
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

      dlHandle = sys::dlopen(binaryFile, hash);
      handle   = sys::dlsym(dlHandle, functionName, hash);

      releaseHash(hash, 0);
    }

    void kernel::buildFromBinary(const std::string &filename,
                                 const std::string &functionName){

      name = functionName;

      dlHandle = sys::dlopen(filename);
      handle   = sys::dlsym(dlHandle, functionName);
    }

    int kernel::maxDims() {
      return 3;
    }

    dim kernel::maxOuterDims() {
      return dim(-1,-1,-1);
    }

    dim kernel::maxInnerDims() {
      return dim(-1,-1,-1);
    }

    void kernel::runFromArguments(const int kArgc, const kernelArg *kArgs){
      int occaKernelArgs[6];

      occaKernelArgs[0] = outer.z; occaKernelArgs[3] = inner.z;
      occaKernelArgs[1] = outer.y; occaKernelArgs[4] = inner.y;
      occaKernelArgs[2] = outer.x; occaKernelArgs[5] = inner.x;

      int argc = 0;
      for(int i = 0; i < kArgc; ++i){
        for(int j = 0; j < kArgs[i].argc; ++j){
          vArgs[argc++] = kArgs[i].args[j].ptr();
        }
      }

      int occaInnerId0 = 0, occaInnerId1 = 0, occaInnerId2 = 0;

      sys::runFunction(handle,
                       occaKernelArgs,
                       occaInnerId0, occaInnerId1, occaInnerId2,
                       argc, vArgs);
    }

    void kernel::free(){
      sys::dlclose(dlHandle);
    }
  }
}
