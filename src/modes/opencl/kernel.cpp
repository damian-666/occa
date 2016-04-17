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

#if OCCA_OPENCL_ENABLED

#include "occa/modes/opencl/kernel.hpp"
#include "occa/modes/opencl/device.hpp"
#include "occa/tools/io.hpp"
#include "occa/tools/sys.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace opencl {
    kernel::kernel(const occa::properties &properties_) :
      occa::kernel_v(properties_) {}

    kernel::~kernel(){}

    info_t kernel::makeCLInfo() {
      info_t info;
      info.clDeviceID = clDeviceID;
      info.clContext  = clContext;
      info.clProgram  = clProgram;
      info.clKernel   = clKernel;
      return info;
    }

    void* kernel::getHandle(const occa::properties &props) {
      const std::string type = props["type"];

      if (type == "kernel")
        return clKernel;
      if (type == "program")
        return clProgram;

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
          std::cout << "Found cached binary of [" << io::shortname(filename)
                    << "] in [" << io::shortname(binaryFile) << "]\n";

        return buildFromBinary(binaryFile, functionName);
      }

      std::stringstream ss;
      ss << "#include \"" << occaModeHeader() << "\"\n"
         << "#include \"" << sys::filename("[occa]/primitives.hpp") << "\"\n"
         << props["header"];

      // if (info.mode & (Serial | OpenMP | Pthreads | CUDA)) {
      //   fs << "#if defined(OCCA_IN_KERNEL) && !OCCA_IN_KERNEL\n"
      //      << "using namespace occa;\n"
      //      << "#endif\n";
      // }

      io::cacheFile(filename, hash, ss.str(), props["footer"]);

      std::string cFunction = io::read(sourceFile);
      info_t clInfo = makeCLInfo();
      opencl::buildKernelFromSource(clInfo,
                                    cFunction.c_str(), cFunction.size(),
                                    functionName,
                                    props["compilerFlags"],
                                    hash, sourceFile);
      clProgram = clInfo.clProgram;
      clKernel  = clInfo.clKernel;

      opencl::saveProgramBinary(clInfo, binaryFile, hash);

      io::releaseHash(hash, 0);
    }

    void kernel::buildFromBinary(const std::string &filename,
                                 const std::string &functionName){

      name = functionName;

      std::string cFile = io::read(filename);
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
      OCCA_CL_CHECK("Kernel (" + metadata.name + ") : Setting Kernel Argument [0]",
                    clSetKernelArg(clKernel, argc++, sizeof(void*), NULL));

      for(int i = 0; i < kArgc; ++i){
        for(int j = 0; j < kArgs[i].argc; ++j){
          OCCA_CL_CHECK("Kernel (" + metadata.name + ") : Setting Kernel Argument [" << (i + 1) << "]",
                        clSetKernelArg(clKernel, argc++, kArgs[i].args[j].size, kArgs[i].args[j].ptr()));
        }
      }

      OCCA_CL_CHECK("Kernel (" + metadata.name + ") : Kernel Run",
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
