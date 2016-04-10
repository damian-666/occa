#if OCCA_OPENCL_ENABLED

#include "occa/modes/opencl/device.hpp"

namespace occa {
  namespace opencl {
    template <>
    device_t<OpenCL>::device_t() {
      strMode = "OpenCL";

      data = NULL;

      uvaEnabled_ = false;

      bytesAllocated = 0;

      getEnvironmentVariables();
    }

    template <>
    device_t<OpenCL>::device_t(const device_t<OpenCL> &d){
      *this = d;
    }

    template <>
    device_t<OpenCL>& device_t<OpenCL>::operator = (const device_t<OpenCL> &d){
      modelID_ = d.modelID_;
      id_      = d.id_;

      data = d.data;

      uvaEnabled_    = d.uvaEnabled_;
      uvaMap         = d.uvaMap;
      uvaDirtyMemory = d.uvaDirtyMemory;

      compiler      = d.compiler;
      compilerFlags = d.compilerFlags;

      bytesAllocated = d.bytesAllocated;

      return *this;
    }

    template <>
    void* device_t<OpenCL>::getContextHandle(){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      return (void*) data_.context;
    }

    template <>
    void device_t<OpenCL>::setup(argInfoMap &aim){
      properties = aim;

      data = new OpenCLDeviceData_t;

      OCCA_EXTRACT_DATA(OpenCL, Device);
      cl_int error;

      OCCA_CHECK(aim.has("platformID"),
                 "[OpenCL] device not given [platformID]");

      OCCA_CHECK(aim.has("deviceID"),
                 "[OpenCL] device not given [deviceID]");

      data_.platform = aim.iGet("platformID");
      data_.device   = aim.iGet("deviceID");

      data_.platformID = cl::platformID(data_.platform);
      data_.deviceID   = cl::deviceID(data_.platform, data_.device);

      data_.context = clCreateContext(NULL, 1, &data_.deviceID, NULL, NULL, &error);
      OCCA_CL_CHECK("Device: Creating Context", error);
    }

    template <>
    void device_t<OpenCL>::addOccaHeadersToInfo(kernelInfo &info_){
      info_.mode = OpenCL;
    }

    template <>
    std::string device_t<OpenCL>::getInfoSalt(const kernelInfo &info_){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      std::stringstream salt;

      salt << "OpenCL"
           << data_.platform << '-' << data_.device
           << info_.salt()
           << parserVersion
           << compilerEnvScript
           << compiler
           << compilerFlags;

      return salt.str();
    }

    template <>
    deviceIdentifier device_t<OpenCL>::getIdentifier() const {
      deviceIdentifier dID;

      dID.mode_ = OpenCL;

      return dID;
    }

    template <>
    void device_t<OpenCL>::getEnvironmentVariables(){
      char *c_compilerFlags = getenv("OCCA_OPENCL_COMPILER_FLAGS");
      if(c_compilerFlags != NULL)
        compilerFlags = std::string(c_compilerFlags);
      else{
#if OCCA_DEBUG_ENABLED
        compilerFlags = "-cl-opt-disable";
#else
        compilerFlags = "";
#endif
      }
    }

    template <>
    void device_t<OpenCL>::appendAvailableDevices(std::vector<device> &dList){
      int platformCount = occa::cl::getPlatformCount();

      for(int p = 0; p < platformCount; ++p){
        int deviceCount = occa::cl::getDeviceCountInPlatform(p);

        for(int d = 0; d < deviceCount; ++d){
          device dev;
          dev.setup("OpenCL", p, d);

          dList.push_back(dev);
        }
      }
    }

    template <>
    void device_t<OpenCL>::setCompiler(const std::string &compiler_){
      compiler = compiler_;
    }

    template <>
    void device_t<OpenCL>::setCompilerEnvScript(const std::string &compilerEnvScript_){
      compilerEnvScript = compilerEnvScript_;
    }

    template <>
    void device_t<OpenCL>::setCompilerFlags(const std::string &compilerFlags_){
      compilerFlags = compilerFlags_;
    }

    template <>
    void device_t<OpenCL>::flush(){
      OCCA_CL_CHECK("Device: Flush",
                    clFlush(*((cl_command_queue*) currentStream)));
    }

    template <>
    void device_t<OpenCL>::finish(){
      OCCA_CL_CHECK("Device: Finish",
                    clFinish(*((cl_command_queue*) currentStream)));
    }

    template <>
    bool device_t<OpenCL>::fakesUva(){
      return true;
    }

    template <>
    void device_t<OpenCL>::waitFor(streamTag tag){
      OCCA_CL_CHECK("Device: Waiting For Tag",
                    clWaitForEvents(1, &cl::event(tag)));
    }

    template <>
    stream_t device_t<OpenCL>::createStream(){
      OCCA_EXTRACT_DATA(OpenCL, Device);
      cl_int error;

      cl_command_queue *retStream = new cl_command_queue;

      *retStream = clCreateCommandQueue(data_.context, data_.deviceID, CL_QUEUE_PROFILING_ENABLE, &error);
      OCCA_CL_CHECK("Device: createStream", error);

      return retStream;
    }

    template <>
    void device_t<OpenCL>::freeStream(stream_t s){
      OCCA_CL_CHECK("Device: freeStream",
                    clReleaseCommandQueue( *((cl_command_queue*) s) ));

      delete (cl_command_queue*) s;
    }

    template <>
    stream_t device_t<OpenCL>::wrapStream(void *handle_){
      return handle_;
    }

    template <>
    streamTag device_t<OpenCL>::tagStream(){
      cl_command_queue &stream = *((cl_command_queue*) currentStream);

      streamTag ret;

#ifdef CL_VERSION_1_2
      OCCA_CL_CHECK("Device: Tagging Stream",
                    clEnqueueMarkerWithWaitList(stream, 0, NULL, &cl::event(ret)));
#else
      OCCA_CL_CHECK("Device: Tagging Stream",
                    clEnqueueMarker(stream, &cl::event(ret)));
#endif

      return ret;
    }

    template <>
    double device_t<OpenCL>::timeBetween(const streamTag &startTag, const streamTag &endTag){
      cl_ulong start, end;

      finish();

      OCCA_CL_CHECK ("Device: Time Between Tags (Start)",
                     clGetEventProfilingInfo(cl::event(startTag),
                                             CL_PROFILING_COMMAND_END,
                                             sizeof(cl_ulong),
                                             &start, NULL) );

      OCCA_CL_CHECK ("Device: Time Between Tags (End)",
                     clGetEventProfilingInfo(cl::event(endTag),
                                             CL_PROFILING_COMMAND_START,
                                             sizeof(cl_ulong),
                                             &end, NULL) );

      OCCA_CL_CHECK("Device: Time Between Tags (Freeing start tag)",
                    clReleaseEvent(cl::event(startTag)));

      OCCA_CL_CHECK("Device: Time Between Tags (Freeing end tag)",
                    clReleaseEvent(cl::event(endTag)));

      return (double) (1.0e-9 * (double)(end - start));
    }

    template <>
    std::string device_t<OpenCL>::fixBinaryName(const std::string &filename){
      return filename;
    }

    template <>
    kernel_v* device_t<OpenCL>::buildKernelFromSource(const std::string &filename,
                                                      const std::string &functionName,
                                                      const kernelInfo &info_){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      kernel_v *k = new kernel_t<OpenCL>;

      k->dHandle = this;
      k->data    = new OpenCLKernelData_t;

      OpenCLKernelData_t &kData_ = *((OpenCLKernelData_t*) k->data);

      kData_.platform = data_.platform;
      kData_.device   = data_.device;

      kData_.platformID = data_.platformID;
      kData_.deviceID   = data_.deviceID;
      kData_.context    = data_.context;

      k->buildFromSource(filename, functionName, info_);

      return k;
    }

    template <>
    kernel_v* device_t<OpenCL>::buildKernelFromBinary(const std::string &filename,
                                                      const std::string &functionName){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      kernel_v *k = new kernel_t<OpenCL>;

      k->dHandle = this;
      k->data    = new OpenCLKernelData_t;

      OpenCLKernelData_t &kData_ = *((OpenCLKernelData_t*) k->data);

      kData_.platform = data_.platform;
      kData_.device   = data_.device;

      kData_.platformID = data_.platformID;
      kData_.deviceID   = data_.deviceID;
      kData_.context    = data_.context;

      k->buildFromBinary(filename, functionName);
      return k;
    }

    template <>
    void device_t<OpenCL>::cacheKernelInLibrary(const std::string &filename,
                                                const std::string &functionName,
                                                const kernelInfo &info_){
#if 0
      //---[ Creating shared library ]----
      kernel tmpK = occa::device(this).buildKernelFromSource(filename, functionName, info_);
      tmpK.free();

      kernelInfo info = info_;

      addOccaHeadersToInfo(info);

      std::string cachedBinary = getCachedName(filename, getInfoSalt(info));

      std::string prefix, name;
      getFilePrefixAndName(cachedBinary, prefix, name);

      std::string extension = getFileExtension(filename);

      const std::string iCachedBinary = prefix + "i_" + name;

      std::string contents = readFile(iCachedBinary);
      //==================================

      library::infoID_t infoID;

      infoID.modelID    = modelID_;
      infoID.kernelName = functionName;

      library::infoHeader_t &header = library::headerMap[infoID];

      header.fileID = -1;
      header.mode   = OpenCL;

      const std::string flatDevID = getIdentifier().flattenFlagMap();

      header.flagsOffset = library::addToScratchPad(flatDevID);
      header.flagsBytes  = flatDevID.size();

      header.contentOffset = library::addToScratchPad(contents);
      header.contentBytes  = contents.size();

      header.kernelNameOffset = library::addToScratchPad(functionName);
      header.kernelNameBytes  = functionName.size();
#endif
    }

    template <>
    kernel_v* device_t<OpenCL>::loadKernelFromLibrary(const char *cache,
                                                      const std::string &functionName){
#if 0
      OCCA_EXTRACT_DATA(OpenCL, Device);

      kernel_v *k = new kernel_t<OpenCL>;

      k->dHandle = this;
      k->data    = new OpenCLKernelData_t;

      OpenCLKernelData_t &kData_ = *((OpenCLKernelData_t*) k->data);

      kData_.platform = data_.platform;
      kData_.device   = data_.device;

      kData_.platformID = data_.platformID;
      kData_.deviceID   = data_.deviceID;
      kData_.context    = data_.context;

      k->loadFromLibrary(cache, functionName);
      return k;
#endif

      return NULL;
    }

    template <>
    memory_v* device_t<OpenCL>::wrapMemory(void *handle_,
                                           const uintptr_t bytes){
      memory_v *mem = new memory_t<OpenCL>;

      mem->dHandle = this;
      mem->size    = bytes;
      mem->handle  = handle_;

      mem->memInfo |= memFlag::isAWrapper;

      return mem;
    }

    template <>
    memory_v* device_t<OpenCL>::wrapTexture(void *handle_,
                                            const int dim, const occa::dim &dims,
                                            occa::formatType type, const int permissions){
#ifndef CL_VERSION_1_2
      if(dim == 1)
        return wrapMemory(handle_, dims.x * type.bytes());

      OCCA_EXTRACT_DATA(OpenCL, Device);

      memory_v *mem = new memory_t<OpenCL>;
      cl_int error;

      mem->dHandle = this;
      mem->size    = (dims.x * dims.y) * type.bytes();
      mem->handle  = handle_;

      mem->memInfo |= (memFlag::isATexture |
                       memFlag::isAWrapper);

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->textureInfo.bytesInEntry = type.bytes();

      mem->textureInfo.arg = new cl_sampler;

      *((cl_sampler*) mem->textureInfo.arg) = clCreateSampler(data_.context,
                                                              CL_FALSE,                 // Are args Normalized?
                                                              CL_ADDRESS_CLAMP_TO_EDGE, // Clamp edges
                                                              CL_FILTER_NEAREST,        // Point interpolation
                                                              &error);

      OCCA_CL_CHECK("Device: Creating texture sampler", error);

      return mem;
#else
      OCCA_EXTRACT_DATA(OpenCL, Device);
      cl_int error;

      memory_v *mem = new memory_t<OpenCL>;

      mem->dHandle = this;
      mem->size    = ((dim == 1) ? dims.x : (dims.x * dims.y)) * type.bytes();
      mem->handle  = handle_;

      mem->memInfo |= (memFlag::isATexture |
                       memFlag::isAWrapper);

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->textureInfo.bytesInEntry = type.bytes();

      mem->textureInfo.arg = new cl_sampler;

      *((cl_sampler*) mem->textureInfo.arg) = clCreateSampler(data_.context,
                                                              CL_FALSE,                 // Are args Normalized?
                                                              CL_ADDRESS_CLAMP_TO_EDGE, // Clamp edges
                                                              CL_FILTER_NEAREST,        // Point interpolation
                                                              &error);

      OCCA_CL_CHECK("Device: Creating texture sampler", error);

      return mem;
#endif
    }

    template <>
    memory_v* device_t<OpenCL>::malloc(const uintptr_t bytes,
                                       void *src){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      memory_v *mem = new memory_t<OpenCL>;
      cl_int error;

      mem->dHandle = this;
      mem->handle  = new cl_mem;
      mem->size    = bytes;

      if(src == NULL){
        *((cl_mem*) mem->handle) = clCreateBuffer(data_.context,
                                                  CL_MEM_READ_WRITE,
                                                  bytes, NULL, &error);
      }
      else{
        *((cl_mem*) mem->handle) = clCreateBuffer(data_.context,
                                                  CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                                  bytes, src, &error);

        finish();
      }

      return mem;
    }

    template <>
    memory_v* device_t<OpenCL>::textureAlloc(const int dim, const occa::dim &dims,
                                             void *src,
                                             occa::formatType type, const int permissions){
#ifndef CL_VERSION_1_2
      if(dim == 1)
        return malloc(dims.x * type.bytes(), src);

      OCCA_EXTRACT_DATA(OpenCL, Device);

      memory_v *mem = new memory_t<OpenCL>;
      cl_int error;

      mem->dHandle = this;
      mem->handle  = new cl_mem;
      mem->size    = (dims.x * dims.y) * type.bytes();

      mem->memInfo |= memFlag::isATexture;

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->textureInfo.bytesInEntry = type.bytes();

      cl_mem_flags flag = (CL_MEM_COPY_HOST_PTR |
                           ((permissions == occa::readWrite) ?
                            CL_MEM_READ_WRITE : CL_MEM_READ_ONLY));
      cl_image_format imageFormat;

      const int count = type.count();

      switch(count){
      case 1: imageFormat.image_channel_order = CL_R;    break;
      case 2: imageFormat.image_channel_order = CL_RG;   break;
      case 4: imageFormat.image_channel_order = CL_RGBA; break;
      };

      imageFormat.image_channel_data_type = *((cl_channel_type*) type.format<OpenCL>());

      *((cl_mem*) mem->handle) = clCreateImage2D(data_.context, flag,
                                                 &imageFormat,
                                                 dims.x,
                                                 dims.y,
                                                 0,
                                                 src, &error);

      OCCA_CL_CHECK("Device: Allocating texture", error);

      mem->textureInfo.arg = new cl_sampler;

      *((cl_sampler*) mem->textureInfo.arg) = clCreateSampler(data_.context,
                                                              CL_FALSE,                 // Are args Normalized?
                                                              CL_ADDRESS_CLAMP_TO_EDGE, // Clamp edges
                                                              CL_FILTER_NEAREST,        // Point interpolation
                                                              &error);

      OCCA_CL_CHECK("Device: Creating texture sampler", error);

      return mem;
#else
      OCCA_EXTRACT_DATA(OpenCL, Device);

      memory_v *mem = new memory_t<OpenCL>;
      cl_int error;

      mem->dHandle = this;
      mem->handle  = new cl_mem;
      mem->size    = ((dim == 1) ? dims.x : (dims.x * dims.y)) * type.bytes();

      mem->memInfo |= memFlag::isATexture;

      mem->textureInfo.dim  = dim;

      mem->textureInfo.w = dims.x;
      mem->textureInfo.h = dims.y;
      mem->textureInfo.d = dims.z;

      mem->textureInfo.bytesInEntry = type.bytes();

      cl_mem_flags flag;
      cl_image_format imageFormat;
      cl_image_desc imageDesc;

      flag  = CL_MEM_COPY_HOST_PTR;
      flag |= ((permissions == occa::readWrite) ? CL_MEM_READ_WRITE : CL_MEM_READ_ONLY);

      const int count = type.count();

      switch(count){
      case 1: imageFormat.image_channel_order = CL_R;    break;
      case 2: imageFormat.image_channel_order = CL_RG;   break;
      case 4: imageFormat.image_channel_order = CL_RGBA; break;
      };

      imageFormat.image_channel_data_type = *((cl_channel_type*) type.format<OpenCL>());

      imageDesc.image_type        = (dim == 1) ? CL_MEM_OBJECT_IMAGE1D : CL_MEM_OBJECT_IMAGE2D;
      imageDesc.image_width       = dims.x;
      imageDesc.image_height      = (dim < 2) ? 0 : dims.y;
      imageDesc.image_depth       = (dim < 3) ? 0 : dims.z;
      imageDesc.image_array_size  = 1;
      imageDesc.image_row_pitch   = 0;
      imageDesc.image_slice_pitch = 0;
      imageDesc.num_mip_levels    = 0;
      imageDesc.num_samples       = 0;
      imageDesc.buffer            = NULL;

      //--------------------------------------------
      cl_uint imageFormatCount;
      cl_image_format imageFormats[1024];

      clGetSupportedImageFormats(data_.context,
                                 flag,
                                 imageDesc.image_type,
                                 0, NULL,
                                 &imageFormatCount);

      clGetSupportedImageFormats(data_.context,
                                 flag,
                                 imageDesc.image_type,
                                 imageFormatCount, imageFormats,
                                 NULL);

      bool isCompatible = cl::imageFormatIsSupported(imageFormat,
                                                     imageFormats,
                                                     imageFormatCount);

      OCCA_CHECK(isCompatible,
                 "The specified image format is not compatible");
      //============================================

      *((cl_mem*) mem->handle) = clCreateImage(data_.context, flag,
                                               &imageFormat, &imageDesc,
                                               src, &error);

      OCCA_CL_CHECK("Device: Allocating texture", error);

      mem->textureInfo.arg = new cl_sampler;

      *((cl_sampler*) mem->textureInfo.arg) = clCreateSampler(data_.context,
                                                              CL_FALSE,                 // Are args Normalized?
                                                              CL_ADDRESS_CLAMP_TO_EDGE, // Clamp edges
                                                              CL_FILTER_NEAREST,        // Point interpolation
                                                              &error);

      OCCA_CL_CHECK("Device: Creating texture sampler", error);

      return mem;
#endif

      finish();
    }

    template <>
    memory_v* device_t<OpenCL>::mappedAlloc(const uintptr_t bytes,
                                            void *src){

      OCCA_EXTRACT_DATA(OpenCL, Device);

      cl_command_queue &stream = *((cl_command_queue*) currentStream);

      memory_v *mem = new memory_t<OpenCL>;
      cl_int error;

      mem->dHandle  = this;
      mem->handle   = new cl_mem;
      mem->size     = bytes;

      mem->memInfo |= memFlag::isMapped;

      // Alloc pinned host buffer
      *((cl_mem*) mem->handle) = clCreateBuffer(data_.context,
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

    template <>
    uintptr_t device_t<OpenCL>::memorySize(){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      return cl::getDeviceMemorySize(data_.deviceID);
    }

    template <>
    void device_t<OpenCL>::free(){
      OCCA_EXTRACT_DATA(OpenCL, Device);

      OCCA_CL_CHECK("Device: Freeing Context",
                    clReleaseContext(data_.context) );

      delete (OpenCLDeviceData_t*) data;
    }

    template <>
    int device_t<OpenCL>::simdWidth(){
      if(simdWidth_)
        return simdWidth_;

      OCCA_EXTRACT_DATA(OpenCL, Device);

      cl_device_type dBuffer;
      bool isGPU = false;

      const int bSize = 8192;
      char buffer[bSize + 1];
      buffer[bSize] = '\0';

      OCCA_CL_CHECK("Device: DEVICE_TYPE",
                    clGetDeviceInfo(data_.deviceID, CL_DEVICE_TYPE, sizeof(dBuffer), &dBuffer, NULL));

      OCCA_CL_CHECK("Device: DEVICE_VENDOR",
                    clGetDeviceInfo(data_.deviceID, CL_DEVICE_VENDOR, bSize, buffer, NULL));

      if(dBuffer & CL_DEVICE_TYPE_CPU)
        isGPU = false;
      else if(dBuffer & CL_DEVICE_TYPE_GPU)
        isGPU = true;
      else{
        OCCA_CHECK(false, "Can only find SIMD width for CPU and GPU devices at the momement\n");
      }

      if(isGPU){
        std::string vendor = buffer;
        if(vendor.find("NVIDIA") != std::string::npos)
          simdWidth_ = 32;
        else if((vendor.find("AMD")                    != std::string::npos) ||
                (vendor.find("Advanced Micro Devices") != std::string::npos))
          simdWidth_ = 64;
        else if(vendor.find("Intel") != std::string::npos)   // <> Need to check for Xeon Phi
          simdWidth_ = OCCA_SIMD_WIDTH;
        else{
          OCCA_CHECK(false, "simdWidth() only available for AMD, Intel and NVIDIA architectures");
        }
      }
      else
        simdWidth_ = OCCA_SIMD_WIDTH;

      return simdWidth_;
    }

    /**
     * info = { cl_platform_id*, cl_device_id*, cl_context* }
     */
    template <>
    occa::device wrapDevice<OpenCL>(void **info) {
      device_t<OpenCL> &dHandle   = *(new device_t<OpenCL>());
      OpenCLDeviceData_t &devData = *(new OpenCLDeviceData_t);

      //---[ Unwrap Args ]------
      cl_platform_id &platformID = *((cl_platform_id*) info[0]);
      cl_device_id &deviceID     = *((cl_device_id*) info[1]);
      cl_context &context        = *((cl_context*) info[2]);
      //========================

      //---[ Setup ]------------
      dHandle.data = &devData;

      devData.platform = -1;
      devData.device   = -1;

      devData.platformID = platformID;
      devData.deviceID   = deviceID;
      devData.context    = context;
      //========================

      dHandle.modelID_ = library::deviceModelID(dHandle.getIdentifier());
      dHandle.id_      = library::genDeviceID();

      dHandle.currentStream = dHandle.createStream();

      return occa::device(&dHandle);
    }
  }

#endif
