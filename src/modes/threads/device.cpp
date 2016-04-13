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

#include "occa/modes/threads/device.hpp"
#include "occa/modes/threads/kernel.hpp"
#include "occa/modes/serial/memory.hpp"
#include "occa/modes/threads/utils.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace threads {
    device::device() : occa::device_v() {
      getEnvironmentVariables();
      sys::addSharedBinaryFlagsTo(compiler, compilerFlags);
    }

    device::device(const device &d){
      *this = d;
    }

    device& device::operator = (const device &d){
      initFrom(d);

      vendor = d.vendor;
      compiler = d.compiler;
      compilerFlags = d.compilerFlags;
      compilerEnvScript = d.compilerEnvScript;

      return *this;
    }

    device::~device(){}

    void* device::getContextHandle(){
      return NULL;
    }

    void device::setup(argInfoMap &aim){
      properties = aim;

      vendor = sys::compilerVendor(compiler);
      sys::addSharedBinaryFlagsTo(vendor, compilerFlags);

      coreCount = sys::getCoreCount();

      std::vector<int> pinnedCores;

      if(!aim.has("threads"))
        threads = coreCount;
      else
        threads = aim.get<int>("threads");

      if(!aim.has("schedule") ||
         (aim.get<std::string>("schedule") == "compact")){
        schedule = compact;
      }
      else{
        schedule = scatter;
      }

      if(aim.has("pinnedCores")){
        pinnedCores = aim.getVector<int>("pinnedCores");

        if(pinnedCores.size() != (size_t) threads){
          threads = (int) pinnedCores.size();
          std::cout << "[Threads]: Mismatch between thread count and pinned cores\n"
                    << "           Setting threads to " << threads << '\n';
        }

        for(size_t i = 0; i < pinnedCores.size(); ++i)
          if(pinnedCores[i] < 0){
            const int newPC = (((pinnedCores[i] % coreCount)
                                + pinnedCores[i]) % coreCount);

            std::cout << "Trying to pin thread on core ["
                      << pinnedCores[i] << "], changing it to ["
                      << newPC << "]\n";

            pinnedCores[i] = newPC;
          }
          else if(coreCount <= pinnedCores[i]){
            const int newPC = (pinnedCores[i] % coreCount);

            std::cout << "Trying to pin thread on core ["
                      << pinnedCores[i] << "], changing it to ["
                      << newPC << "]\n";

            pinnedCores[i] = newPC;
          }

        schedule = manual;
      }

      for(int t = 0; t < threads; ++t){
        workerData_t *args = new workerData_t;

        args->rank  = t;
        args->count = threads;

        // [-] Need to know number of sockets
        if(schedule & compact)
          args->pinnedCore = (t % coreCount);
        else if(schedule & scatter)
          args->pinnedCore = (t % coreCount);
        else // Manual
          args->pinnedCore = pinnedCores[t];

        args->jobs = &(jobs[t]);

        args->jobMutex = &(jobMutex);

#if (OCCA_OS & (LINUX_OS | OSX_OS))
        pthread_create(&tid[t], NULL, threads::limbo, args);
#else
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) threads::limbo, args, 0, &tid[t]);
#endif
      }
    }

    // [REFACTOR]
    void device::addOccaHeadersToInfo(kernelInfo &info_){
    }

    std::string device::getInfoSalt(const kernelInfo &info_){
      std::stringstream salt;

      salt << "Pthreads"
           << info_.salt()
           << parserVersion
           << compilerEnvScript
           << compiler
           << compilerFlags;

      return salt.str();
    }

    void device::getEnvironmentVariables(){
      char *c_compiler = getenv("OCCA_CXX");

      if(c_compiler != NULL){
        compiler = std::string(c_compiler);
      }
      else{
        c_compiler = getenv("CXX");

        if(c_compiler != NULL){
          compiler = std::string(c_compiler);
        }
        else{
#if (OCCA_OS & (LINUX_OS | OSX_OS))
          compiler = "g++";
#else
          compiler = "cl.exe";
#endif
        }
      }

      char *c_compilerFlags = getenv("OCCA_CXXFLAGS");

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      if(c_compilerFlags != NULL)
        compilerFlags = std::string(c_compilerFlags);
      else{
#  if OCCA_DEBUG_ENABLED
        compilerFlags = "-g";
#  else
        compilerFlags = "";
#  endif
      }
#else
#  if OCCA_DEBUG_ENABLED
      compilerFlags = " /Od";
#  else
      compilerFlags = " /O2";
#  endif

      std::string byteness;

      if(sizeof(void*) == 4)
        byteness = "x86 ";
      else if(sizeof(void*) == 8)
        byteness = "amd64";
      else
        OCCA_CHECK(false, "sizeof(void*) is not equal to 4 or 8");

#  if      (OCCA_VS_VERSION == 1800)
      char *visualStudioTools = getenv("VS120COMNTOOLS");   // MSVC++ 12.0 - Visual Studio 2013
#  elif    (OCCA_VS_VERSION == 1700)
      char *visualStudioTools = getenv("VS110COMNTOOLS");   // MSVC++ 11.0 - Visual Studio 2012
#  else // (OCCA_VS_VERSION == 1600)
      char *visualStudioTools = getenv("VS100COMNTOOLS");   // MSVC++ 10.0 - Visual Studio 2010
#  endif

      if(visualStudioTools != NULL){
        setCompilerEnvScript("\"" + std::string(visualStudioTools) + "..\\..\\VC\\vcvarsall.bat\" " + byteness);
      }
      else{
        std::cout << "WARNING: Visual Studio environment variable not found -> compiler environment (vcvarsall.bat) maybe not correctly setup." << std::endl;
      }
#endif

      properties["compiler"]          = compiler;
      properties["compilerFlags"]     = compilerFlags;
      properties["compilerEnvScript"] = compilerEnvScript;
    }

    void device::appendAvailableDevices(std::vector<occa::device> &dList){
      std::stringstream ss;
      ss << "mode = Threads, threads = " << sys::getCoreCount()
         << ", schedule = compact";
      dList.push_back(occa::device(ss.str()));
    }

    void device::flush(){}

    void device::finish(){
      bool done = false;
      while (!done) {
        done = true;
        for (int t = 0; t < threads; ++t) {
          if (jobs[t].size()) {
            done = false;
            break;
          }
        }
        OCCA_LFENCE;
      }
    }

    bool device::fakesUva(){
      return false;
    }

    void device::waitFor(streamTag tag){
      finish(); // [-] Not done
    }


    //---[ Stream ]---------------------
    stream_t device::createStream(){
      return NULL;
    }

    void device::freeStream(stream_t s){}

    stream_t device::wrapStream(void *handle_){
      return NULL;
    }

    streamTag device::tagStream(){
      streamTag ret;
      ret.tagTime = currentTime();
      return ret;
    }

    double device::timeBetween(const streamTag &startTag, const streamTag &endTag){
      return (endTag.tagTime - startTag.tagTime);
    }
    //==================================

    //---[ Kernel ]---------------------
    std::string device::fixBinaryName(const std::string &filename){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return filename;
#else
      return (filename + ".dll");
#endif
    }

    kernel_v* device::buildKernelFromSource(const std::string &filename,
                                            const std::string &functionName,
                                            const kernelInfo &info_){
      kernel *k = new kernel();
      k->dHandle  = this;
      k->buildFromSource(filename, functionName, info_);
      return k;
    }

    kernel_v* device::buildKernelFromBinary(const std::string &filename,
                                            const std::string &functionName){
      kernel *k = new kernel();
      k->dHandle  = this;
      k->buildFromBinary(filename, functionName);
      return k;
    }
    //==================================

    //---[ Memory ]---------------------
    memory_v* device::wrapMemory(void *handle_,
                                 const uintptr_t bytes){
      serial::memory *mem = new serial::memory();

      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = handle_;

      mem->memInfo |= memFlag::isAWrapper;

      return mem;
    }

    memory_v* device::malloc(const uintptr_t bytes,
                             void *src){
      serial::memory *mem = new serial::memory();

      mem->dHandle = this;
      mem->size    = bytes;

      mem->handle = sys::malloc(bytes);

      if(src != NULL)
        ::memcpy(mem->handle, src, bytes);

      return mem;
    }

    memory_v* device::mappedAlloc(const uintptr_t bytes,
                                  void *src){
      memory_v *mem = malloc(bytes, src);
      mem->mappedPtr = mem->handle;
      return mem;
    }

    uintptr_t device::memorySize(){
      return sys::installedRAM();
    }

    void device::free(){
      finish();
      jobMutex.free();
    }
    //==================================

    //---[ Custom ]---------------------
    void device::addJob(job_t &job) {
      jobMutex.lock();
      jobs[job.rank].push(job);
      jobMutex.unlock();
    }
    //==================================
  }
}
