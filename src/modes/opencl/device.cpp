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

#include "occa/modes/opencl/device.hpp"
#include "occa/modes/opencl/kernel.hpp"
#include "occa/modes/opencl/memory.hpp"
#include "occa/modes/opencl/utils.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace opencl {
    device::device() : occa::device_v() {
      platformID = 0;
      deviceID   = 0;

      getEnvironmentVariables();
    }

    device::device(const device &d){
      *this = d;
    }

    device& device::operator = (const device &d) {
      initFrom(d);

      platformID = d.platformID;
      deviceID = d.deviceID;

      clPlatformID = d.clPlatformID;
      clDeviceID = d.clDeviceID;
      clContext = d.clContext;

      return *this;
    }

    device::~device(){}

    void* device::getContextHandle(){
      return (void*) clContext;
    }

    void device::setup(argInfoMap &aim){
      properties = aim;

      cl_int error;

      OCCA_CHECK(aim.has("platformID"),
                 "[OpenCL] device not given [platformID]");

      OCCA_CHECK(aim.has("deviceID"),
                 "[OpenCL] device not given [deviceID]");

      platformID = aim.get<int>("platformID");
      deviceID   = aim.get<int>("deviceID");

      clPlatformID = opencl::platformID(platformID);
      clDeviceID   = opencl::deviceID(platformID, deviceID);

      clContext = clCreateContext(NULL, 1, &clDeviceID, NULL, NULL, &error);
      OCCA_CL_CHECK("Device: Creating Context", error);
    }

    // [REFACTOR]
    void device::addOccaHeadersToInfo(kernelInfo &info_){
    }

    std::string device::getInfoSalt(const kernelInfo &info_){
      std::stringstream salt;

      salt << "OpenCL"
           << platformID << '-' << deviceID
           << info_.salt()
           << parserVersion
           << compilerFlags;

      return salt.str();
    }

    void device::getEnvironmentVariables(){
      if (properties.has("compilerFlags")) {
        compilerFlags = properties["compilerFlags"];
      }
      else if (env::var("OCCA_OPENCL_COMPILER_FLAGS").size()) {
        compilerFlags = env::var("OCCA_OPENCL_COMPILER_FLAGS");
      }
      else {
#if OCCA_DEBUG_ENABLED
        compilerFlags = "-cl-opt-disable";
#else
        compilerFlags = "";
#endif
      }

      properties["compilerFlags"] = compilerFlags;
    }

    void device::appendAvailableDevices(std::vector<occa::device> &dList){
      std::stringstream ss;

      int platformCount = occa::opencl::getPlatformCount();
      for(int p = 0; p < platformCount; ++p){
        int deviceCount = occa::opencl::getDeviceCountInPlatform(p);
        for(int d = 0; d < deviceCount; ++d){
          ss.str("");
          ss << "mode = OpenCL, platformID = " << p << " deviceID = " << d;
          dList.push_back(occa::device(ss.str()));
        }
      }
    }

    void device::flush(){
      OCCA_CL_CHECK("Device: Flush",
                    clFlush(*((cl_command_queue*) currentStream)));
    }

    void device::finish(){
      OCCA_CL_CHECK("Device: Finish",
                    clFinish(*((cl_command_queue*) currentStream)));
    }

    bool device::fakesUva(){
      return true;
    }

    void device::waitFor(streamTag tag){
      OCCA_CL_CHECK("Device: Waiting For Tag",
                    clWaitForEvents(1, &event(tag)));
    }

    stream_t device::createStream(){
      cl_int error;

      cl_command_queue *retStream = new cl_command_queue;

      *retStream = clCreateCommandQueue(clContext, clDeviceID, CL_QUEUE_PROFILING_ENABLE, &error);
      OCCA_CL_CHECK("Device: createStream", error);

      return retStream;
    }

    void device::freeStream(stream_t s){
      OCCA_CL_CHECK("Device: freeStream",
                    clReleaseCommandQueue( *((cl_command_queue*) s) ));

      delete (cl_command_queue*) s;
    }

    stream_t device::wrapStream(void *handle_){
      return handle_;
    }

    streamTag device::tagStream(){
      cl_command_queue &stream = *((cl_command_queue*) currentStream);

      streamTag ret;

#ifdef CL_VERSION_1_2
      OCCA_CL_CHECK("Device: Tagging Stream",
                    clEnqueueMarkerWithWaitList(stream, 0, NULL, &event(ret)));
#else
      OCCA_CL_CHECK("Device: Tagging Stream",
                    clEnqueueMarker(stream, &event(ret)));
#endif

      return ret;
    }

    double device::timeBetween(const streamTag &startTag, const streamTag &endTag){
      cl_ulong start, end;

      finish();

      OCCA_CL_CHECK ("Device: Time Between Tags (Start)",
                     clGetEventProfilingInfo(event(startTag),
                                             CL_PROFILING_COMMAND_END,
                                             sizeof(cl_ulong),
                                             &start, NULL) );

      OCCA_CL_CHECK ("Device: Time Between Tags (End)",
                     clGetEventProfilingInfo(event(endTag),
                                             CL_PROFILING_COMMAND_START,
                                             sizeof(cl_ulong),
                                             &end, NULL) );

      OCCA_CL_CHECK("Device: Time Between Tags (Freeing start tag)",
                    clReleaseEvent(event(startTag)));

      OCCA_CL_CHECK("Device: Time Between Tags (Freeing end tag)",
                    clReleaseEvent(event(endTag)));

      return (double) (1.0e-9 * (double)(end - start));
    }

    std::string device::fixBinaryName(const std::string &filename){
      return filename;
    }

    kernel_v* device::buildKernelFromSource(const std::string &filename,
                                            const std::string &functionName,
                                            const kernelInfo &info_){
      opencl::kernel *k = new opencl::kernel();

      k->dHandle = this;

      k->platformID = platformID;
      k->deviceID   = deviceID;

      k->clPlatformID = clPlatformID;
      k->clDeviceID   = clDeviceID;
      k->clContext    = clContext;

      k->buildFromSource(filename, functionName, info_);

      return k;
    }

    kernel_v* device::buildKernelFromBinary(const std::string &filename,
                                            const std::string &functionName){
      opencl::kernel *k = new opencl::kernel();

      k->dHandle = this;

      k->platformID = platformID;
      k->deviceID   = deviceID;

      k->clPlatformID = clPlatformID;
      k->clDeviceID   = clDeviceID;
      k->clContext    = clContext;

      k->buildFromBinary(filename, functionName);
      return k;
    }

    memory_v* device::wrapMemory(void *handle_,
                                 const uintptr_t bytes){
      memory *mem = new memory();
      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = new cl_mem;
      ::memcpy(mem->handle, handle_, sizeof(cl_mem));
      return mem;
    }

    memory_v* device::malloc(const uintptr_t bytes,
                             void *src){
      memory *mem = new memory();
      cl_int error;

      mem->dHandle = this;
      mem->handle  = new cl_mem;
      mem->size    = bytes;

      if(src == NULL){
        *((cl_mem*) mem->handle) = clCreateBuffer(clContext,
                                                  CL_MEM_READ_WRITE,
                                                  bytes, NULL, &error);
      }
      else{
        *((cl_mem*) mem->handle) = clCreateBuffer(clContext,
                                                  CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                                  bytes, src, &error);

        finish();
      }

      return mem;
    }

    memory_v* device::mappedAlloc(const uintptr_t bytes,
                                  void *src){

      cl_command_queue &stream = *((cl_command_queue*) currentStream);

      memory_v *mem = new memory;
      cl_int error;

      mem->dHandle  = this;
      mem->handle   = new cl_mem;
      mem->size     = bytes;

      mem->memInfo |= memFlag::isMapped;

      // Alloc pinned host buffer
      *((cl_mem*) mem->handle) = clCreateBuffer(clContext,
                                                CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                                bytes,
                                                NULL, &error);

      OCCA_CL_CHECK("Device: clCreateBuffer", error);

      if(src != NULL)
        mem->copyFrom(src);

      // Map memory to read/write
      mem->mappedPtr = clEnqueueMapBuffer(stream,
                                          *((cl_mem*) mem->handle),
                                          CL_TRUE,
                                          CL_MAP_READ | CL_MAP_WRITE,
                                          0, bytes,
                                          0, NULL, NULL,
                                          &error);

      OCCA_CL_CHECK("Device: clEnqueueMapBuffer", error);

      // Sync memory mapping
      finish();

      return mem;
    }

    uintptr_t device::memorySize(){
      return opencl::getDeviceMemorySize(clDeviceID);
    }

    void device::free(){
      OCCA_CL_CHECK("Device: Freeing Context",
                    clReleaseContext(clContext) );
    }
  }
}

#endif
