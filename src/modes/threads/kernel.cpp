#include "occa/modes/threads/kernel.hpp"
#include "occa/modes/threads/device.hpp"
#include "occa/modes/threads/utils.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace threads {
    kernel::kernel() : occa::kernel_v() {
      dlHandle = NULL;
      handle   = NULL;
    }

    kernel::kernel(const kernel &k){
      *this = k;
    }

    kernel& kernel::operator = (const kernel &k){
      initFrom(k);
      dlHandle = k.dlHandle;
      handle   = k.handle;

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

        return buildFromBinary(binaryFile, functionName);
      }

      createSourceFileFrom(filename, hashDir, info);

      std::stringstream command;
      occa::device device(dHandle);
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
      std::string occaLib = env::OCCA_DIR + "\\lib\\libocca_d.lib ";
#  else
      std::string occaLib = env::OCCA_DIR + "\\lib\\libocca.lib ";
#  endif

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

    void kernel::runFromArguments(const int kArgc, const kernelArg *kArgs){
      job_t job;

      job.count = threads();

      job.handle = handle;

      job.dims  = dims;
      job.inner = inner;
      job.outer = outer;

      const int argc = kernelArg::argumentCount(kArgc, kArgs);
      for (int i = 0; i < argc; ++i) {
        for(int j = 0; j < kArgs[i].argc; ++j){
          job.args.push_back(kArgs[i].args[j].ptr());
        }
      }

      for(int t = 0; t < threads(); ++t){
        job.rank = t;
        ((device*) dHandle)->addJob(job);
      }
    }

    void kernel::free(){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      dlclose(dlHandle);
#else
      FreeLibrary((HMODULE) dlHandle);
#endif
    }

    //---[ Custom ]---------------------
    int kernel::threads() {
      return ((device*) dHandle)->threads;
    }
    //==================================
  }
}
