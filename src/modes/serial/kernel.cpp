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

#include "occa/modes/serial/kernel.hpp"
#include "occa/tools/env.hpp"
#include "occa/tools/io.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace serial {
    kernel::kernel(const occa::properties &properties_) :
      occa::kernel_v(properties_) {
      dlHandle = NULL;
      handle   = NULL;
    }

    void* kernel::getHandle(const occa::properties &props) {
      const std::string type = props["type"];

      if (type == "dl_handle")
        return dlHandle;
      if (type == "function")
        return &handle;

      return NULL;
    }

    void kernel::buildFromSource(const std::string &filename,
                                 const std::string &functionName,
                                 const occa::properties &props) {

      name = functionName;

      hash_t hash = occa::hashFile(filename);
      hash ^= props.hash();

      const std::string sourceFile = sourceFilename(filename, hash);
      const std::string binaryFile = binaryFilename(filename, hash);
      bool foundBinary = true;

      if (!io::haveHash(hash, 0))
        io::waitForHash(hash, 0);
      else if (sys::fileExists(binaryFile))
        io::releaseHash(hash, 0);
      else
        foundBinary = false;

      if (foundBinary) {
        if(verboseCompilation_f)
          std::cout << "Found cached binary of [" << io::shortname(filename) << "] in [" << io::shortname(binaryFile) << "]\n";

        buildFromBinary(binaryFile, functionName);
      }

      std::stringstream ss;
      ss << "#include \"" << occaModeHeader() << "\"\n"
         << "#include \"" << sys::filename("[occa]/primitives.hpp") << "\"\n"
         << props["header"] << '\n'
         << "#if defined(OCCA_IN_KERNEL) && !OCCA_IN_KERNEL\n"
         << "using namespace occa;\n"
         << "#endif\n";

      io::cacheFile(filename, hash, ss.str(), props["footer"]);

      std::stringstream command;
      if (properties.has("compilerEnvScript"))
        command << properties["compilerEnvScript"] << " && ";

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      command << properties["compiler"]
              << ' '    << properties["compilerFlags"]
              << ' '    << sourceFile
              << " -o " << binaryFile
              << " -I"  << env::OCCA_DIR << "include"
              << " -L"  << env::OCCA_DIR << "lib -locca"
              << std::endl;
#else
#  if (OCCA_DEBUG_ENABLED)
      const std::string occaLib = env::OCCA_DIR + "lib/libocca_d.lib ";
#  else
      const std::string occaLib = env::OCCA_DIR + "lib/libocca.lib ";
#  endif

      command << properties["compiler"]
              << " /D MC_CL_EXE"
              << " /D OCCA_OS=WINDOWS_OS"
              << " /EHsc"
              << " /wd4244 /wd4800 /wd4804 /wd4018"
              << ' '       << properties["compilerFlags"]
              << " /I"     << env::OCCA_DIR << "/include"
              << ' '       << sourceFile
              << " /link " << occaLib
              << " /OUT:"  << binaryFile
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
        io::releaseHash(hash, 0);
        OCCA_CHECK(false, "Compilation error");
      }

      dlHandle = sys::dlopen(binaryFile, hash);
      handle   = sys::dlsym(dlHandle, functionName, hash);

      io::releaseHash(hash, 0);
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
