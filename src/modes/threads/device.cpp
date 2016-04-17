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
    device::device(const occa::properties &properties_) :
      serial::device(properties_) {
      coreCount = sys::getCoreCount();

      std::vector<int> pinnedCores;

      threads  = properties.get<int>("threads", coreCount);

      if (properties.get("schedule", "compact") == "compact") {
        schedule = compact;
      }
      else {
        schedule = scatter;
      }

      if (properties.has("pinnedCores")){
        pinnedCores = properties.getVector<int>("pinnedCores");

        if (pinnedCores.size() != (size_t) threads){
          threads = (int) pinnedCores.size();
          std::cout << "[Threads]: Mismatch between thread count and pinned cores\n"
                    << "           Setting threads to " << threads << '\n';
        }

        for (size_t i = 0; i < pinnedCores.size(); ++i)
          if (pinnedCores[i] < 0){
            const int newPC = (((pinnedCores[i] % coreCount)
                                + pinnedCores[i]) % coreCount);

            std::cout << "Trying to pin thread on core ["
                      << pinnedCores[i] << "], changing it to ["
                      << newPC << "]\n";

            pinnedCores[i] = newPC;
          }
          else if (coreCount <= pinnedCores[i]){
            const int newPC = (pinnedCores[i] % coreCount);

            std::cout << "Trying to pin thread on core ["
                      << pinnedCores[i] << "], changing it to ["
                      << newPC << "]\n";

            pinnedCores[i] = newPC;
          }

        schedule = manual;
      }

      for (int t = 0; t < threads; ++t){
        workerData_t *args = new workerData_t;

        args->rank  = t;
        args->count = threads;

        // [-] Need to know number of sockets
        if (schedule & compact)
          args->pinnedCore = (t % coreCount);
        else if (schedule & scatter)
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

    device::~device(){}

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
    std::string device::binaryName(const std::string &filename){
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

      return mem;
    }

    memory_v* device::malloc(const uintptr_t bytes,
                             void *src){
      serial::memory *mem = new serial::memory();

      mem->dHandle = this;
      mem->size    = bytes;

      mem->handle = sys::malloc(bytes);

      if (src != NULL)
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
