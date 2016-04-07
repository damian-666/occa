#if OCCA_OPENCL_ENABLED

namespace occa {
  namespace opencl {
    namespace info {
      std::string deviceType(int type) {
        if(type & CPU)     return "CPU";
        if(type & GPU)     return "GPU";
        if(type & FPGA)    return "FPGA";
        if(type & XeonPhi) return "Xeon Phi";

        return "N/A";
      }

      std::string vendor(int type) {
        if(type & Intel)  return "Intel";
        if(type & AMD)    return "AMD";
        if(type & NVIDIA) return "NVIDIA";
        if(type & Altera) return "Altera";

        return "N/A";
      }
    }

    cl_device_type deviceType(int type){
      cl_device_type ret = 0;

      if(type & cl::info::CPU)     ret |= CL_DEVICE_TYPE_CPU;
      if(type & cl::info::GPU)     ret |= CL_DEVICE_TYPE_GPU;
      if(type & cl::info::FPGA)    ret |= CL_DEVICE_TYPE_ACCELERATOR;
      if(type & cl::info::XeonPhi) ret |= CL_DEVICE_TYPE_ACCELERATOR;

      return ret;
    }

    int getPlatformCount(){
      cl_uint platformCount;

      OCCA_CL_CHECK("OpenCL: Get Platform ID Count",
                    clGetPlatformIDs(0, NULL, &platformCount));

      return platformCount;
    }

    cl_platform_id platformID(int pID){
      cl_platform_id *platforms = new cl_platform_id[pID + 1];

      OCCA_CL_CHECK("OpenCL: Get Platform ID",
                    clGetPlatformIDs(pID + 1, platforms, NULL));

      cl_platform_id ret = platforms[pID];

      delete [] platforms;

      return ret;
    }

    int getDeviceCount(int type){
      int pCount = cl::getPlatformCount();
      int ret = 0;

      for(int p = 0; p < pCount; ++p)
        ret += getDeviceCountInPlatform(p, type);

      return ret;
    }

    int getDeviceCountInPlatform(int pID, int type){
      cl_uint dCount;

      cl_platform_id clPID = platformID(pID);

      OCCA_CL_CHECK("OpenCL: Get Device ID Count",
                    clGetDeviceIDs(clPID,
                                   deviceType(type),
                                   0, NULL, &dCount));

      return dCount;
    }

    cl_device_id deviceID(int pID, int dID, int type){
      cl_device_id *devices = new cl_device_id[dID + 1];

      cl_platform_id clPID = platformID(pID);

      OCCA_CL_CHECK("OpenCL: Get Device ID Count",
                    clGetDeviceIDs(clPID,
                                   deviceType(type),
                                   dID + 1, devices, NULL));

      cl_device_id ret = devices[dID];

      delete [] devices;

      return ret;
    }

    std::string deviceStrInfo(cl_device_id clDID,
                              cl_device_info clInfo){
      size_t bytes;

      OCCA_CL_CHECK("OpenCL: Getting Device String Info",
                    clGetDeviceInfo(clDID,
                                    clInfo,
                                    0, NULL, &bytes));

      char *buffer  = new char[bytes + 1];
      buffer[bytes] = '\0';

      OCCA_CL_CHECK("OpenCL: Getting Device String Info",
                    clGetDeviceInfo(clDID,
                                    clInfo,
                                    bytes, buffer, NULL));

      std::string ret = buffer;

      delete [] buffer;

      size_t firstNS = ret.size();
      size_t lastNS  = ret.size();

      size_t i;

      for(i = 0; i < ret.size(); ++i){
        if((ret[i] != ' ') &&
           (ret[i] != '\t') &&
           (ret[i] != '\n')){
          firstNS = i;
          break;
        }
      }

      if(i == ret.size())
        return "";

      for(i = (ret.size() - 1); i > firstNS; --i){
        if((ret[i] != ' ') &&
           (ret[i] != '\t') &&
           (ret[i] != '\n')){
          lastNS = i;
          break;
        }
      }

      if(i == firstNS)
        return "";

      return ret.substr(firstNS, (lastNS - firstNS + 1));
    }

    std::string deviceName(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);

      return deviceStrInfo(clDID, CL_DEVICE_NAME);
    }

    int deviceType(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);
      int ret = 0;

      cl_device_type clDeviceType;

      OCCA_CL_CHECK("OpenCL: Get Device Type",
                    clGetDeviceInfo(clDID,
                                    CL_DEVICE_TYPE,
                                    sizeof(clDeviceType), &clDeviceType, NULL));

      if(clDeviceType & CL_DEVICE_TYPE_CPU)
        ret |= cl::info::CPU;
      else if(clDeviceType & CL_DEVICE_TYPE_GPU)
        ret |= cl::info::GPU;

      return ret;
    }

    int deviceVendor(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);
      int ret = 0;

      std::string vendor = deviceStrInfo(clDID, CL_DEVICE_VENDOR);

      if(vendor.find("AMD")                    != std::string::npos ||
         vendor.find("Advanced Micro Devices") != std::string::npos ||
         vendor.find("ATI")                    != std::string::npos){

        ret |= cl::info::AMD;
      }
      else if(vendor.find("Intel") != std::string::npos){
        ret |= cl::info::Intel;
      }
      else if(vendor.find("Altera") != std::string::npos){
        ret |= cl::info::Altera;
      }
      else if(vendor.find("Nvidia") != std::string::npos ||
              vendor.find("NVIDIA") != std::string::npos){

        ret |= cl::info::NVIDIA;
      }

      return ret;
    }

    int deviceCoreCount(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);
      cl_uint ret;

      OCCA_CL_CHECK("OpenCL: Get Device Core Count",
                    clGetDeviceInfo(clDID,
                                    CL_DEVICE_MAX_COMPUTE_UNITS,
                                    sizeof(ret), &ret, NULL));

      return ret;
    }

    uintptr_t getDeviceMemorySize(cl_device_id dID){
      cl_ulong ret;

      OCCA_CL_CHECK("OpenCL: Get Device Available Memory",
                    clGetDeviceInfo(dID,
                                    CL_DEVICE_GLOBAL_MEM_SIZE,
                                    sizeof(ret), &ret, NULL));

      return ret;
    }

    uintptr_t getDeviceMemorySize(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);

      return getDeviceMemorySize(clDID);
    }

    std::string getDeviceListInfo(){
      std::stringstream ss;

      int platformCount = occa::cl::getPlatformCount();

      for(int pID = 0; pID < platformCount; ++pID){
        int deviceCount = occa::cl::getDeviceCountInPlatform(pID);

        for(int dID = 0; dID < deviceCount; ++dID){
          uintptr_t bytes      = getDeviceMemorySize(pID, dID);
          std::string bytesStr = stringifyBytes(bytes);

          if(pID || dID){
            ss << "              |-----------------------+------------------------------------------\n"
               << "              |  Device Name          | " << deviceName(pID, dID) << '\n';
          }
          else{
            ss << "    OpenCL    |  Device Name          | " << deviceName(pID, dID) << '\n';
          }

          ss << "              |  Driver Vendor        | " << cl::info::vendor(deviceVendor(pID,dID)) << '\n'
             << "              |  Platform ID          | " << pID      << '\n'
             << "              |  Device ID            | " << dID      << '\n'
             << "              |  Memory               | " << bytesStr << '\n';
        }
      }


      return ss.str();
    }

    void buildKernelFromSource(OpenCLKernelData_t &data_,
                               const char *content,
                               const size_t contentBytes,
                               const std::string &functionName,
                               const std::string &flags,
                               const std::string &hash,
                               const std::string &sourceFile){
      cl_int error;

      data_.program = clCreateProgramWithSource(data_.context, 1,
                                                (const char **) &content,
                                                &contentBytes,
                                                &error);

      if(error && hash.size())
        releaseHash(hash, 0);

      if(verboseCompilation_f){
        if(hash.size()){
          std::cout << "OpenCL compiling " << functionName
                    << " from [" << sourceFile << "]";

          if(flags.size())
            std::cout << " with flags [" << flags << "]";

          std::cout << '\n';
        }
        else
          std::cout << "OpenCL compiling " << functionName << " from [Library]\n";
      }

      OCCA_CL_CHECK("Kernel (" + functionName + ") : Constructing Program", error);

      error = clBuildProgram(data_.program,
                             1, &data_.deviceID,
                             flags.c_str(),
                             NULL, NULL);

      if(error){
        cl_int logError;
        char *log;
        uintptr_t logSize;

        clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

        if(logSize > 2){
          log = new char[logSize+1];

          logError = clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
          OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", logError);
          log[logSize] = '\0';

          printf("Kernel (%s): Build Log\n%s", functionName.c_str(), log);

          delete[] log;
        }

        if(hash.size())
          releaseHash(hash, 0);
      }

      OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", error);

      data_.kernel = clCreateKernel(data_.program, functionName.c_str(), &error);

      if(error && hash.size())
        releaseHash(hash, 0);

      OCCA_CL_CHECK("Kernel (" + functionName + "): Creating Kernel", error);

      if(verboseCompilation_f){
        if(sourceFile.size()){
          std::cout << "OpenCL compiled " << functionName << " from [" << sourceFile << "]";

          if(flags.size())
            std::cout << " with flags [" << flags << "]";

          std::cout << '\n';
        }
        else
          std::cout << "OpenCL compiled " << functionName << " from [Library]\n";
      }
    }

    void buildKernelFromBinary(OpenCLKernelData_t &data_,
                               const unsigned char *content,
                               const size_t contentBytes,
                               const std::string &functionName,
                               const std::string &flags){
      cl_int error, binaryError;

      data_.program = clCreateProgramWithBinary(data_.context,
                                                1, &(data_.deviceID),
                                                &contentBytes,
                                                (const unsigned char**) &content,
                                                &binaryError, &error);

      OCCA_CL_CHECK("Kernel (" + functionName + ") : Constructing Program", binaryError);
      OCCA_CL_CHECK("Kernel (" + functionName + ") : Constructing Program", error);

      error = clBuildProgram(data_.program,
                             1, &data_.deviceID,
                             flags.c_str(),
                             NULL, NULL);

      if(error){
        cl_int logError;
        char *log;
        uintptr_t logSize;

        clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

        if(logSize > 2){
          log = new char[logSize+1];

          logError = clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
          OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", logError);
          log[logSize] = '\0';

          printf("Kernel (%s): Build Log\n%s", functionName.c_str(), log);

          delete[] log;
        }
      }

      OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", error);

      data_.kernel = clCreateKernel(data_.program, functionName.c_str(), &error);
      OCCA_CL_CHECK("Kernel (" + functionName + "): Creating Kernel", error);
    }

    void saveProgramBinary(OpenCLKernelData_t &data_,
                           const std::string &binaryFile,
                           const std::string &hash){
      size_t binarySize;
      char *binary;

      cl_int error = clGetProgramInfo(data_.program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binarySize, NULL);

      if(error && hash.size())
        releaseHash(hash, 0);

      OCCA_CL_CHECK("saveProgramBinary: Getting Binary Sizes", error);

      binary = new char[binarySize + 1];

      error = clGetProgramInfo(data_.program, CL_PROGRAM_BINARIES, sizeof(char*), &binary, NULL);

      if(error && hash.size())
        releaseHash(hash, 0);

      OCCA_CL_CHECK("saveProgramBinary: Getting Binary", error);

      FILE *fp = fopen(binaryFile.c_str(), "wb");
      fwrite(binary, 1, binarySize, fp);
      fclose(fp);

      delete [] binary;
    }

    cl_event& event(streamTag tag) {
      return (cl_event&) tag.handle;
    }

    bool imageFormatIsSupported(cl_image_format &f,
                                cl_image_format *fs,
                                const int formatCount){

      for(int i = 0; i < formatCount; ++i){
        cl_image_format &f2 = fs[i];

        const bool orderSupported = (f.image_channel_order ==
                                     (f.image_channel_order &
                                      f2.image_channel_order));

        const bool typeSupported = (f.image_channel_data_type ==
                                    (f.image_channel_data_type &
                                     f2.image_channel_data_type));

        if(orderSupported && typeSupported)
          return true;
      }

      return false;
    }

    void printImageFormat(cl_image_format &imageFormat){
      std::cout << "---[ OpenCL Image Format ]--------------\n"
                << "Supported Channel Formats:\n";

#define OCCA_CL_PRINT_CHANNEL_INFO(X)                                   \
      if(imageFormat.image_channel_order & X) std::cout << "  " #X "\n"

      OCCA_CL_PRINT_CHANNEL_INFO(CL_R);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_Rx);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_A);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_INTENSITY);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_RG);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_RGx);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_RA);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_RGB);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_RGBx);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_RGBA);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_ARGB);
      OCCA_CL_PRINT_CHANNEL_INFO(CL_BGRA);

#undef OCCA_CL_PRINT_CHANNEL_INFO

      std::cout << "\nSupported Channel Types:\n";

#define OCCA_CL_PRINT_CHANNEL_TYPE(X)                                   \
      if(imageFormat.image_channel_data_type & X) std::cout << "  " #X "\n"

      OCCA_CL_PRINT_CHANNEL_TYPE(CL_SNORM_INT8);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_SNORM_INT16);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNORM_INT8);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNORM_INT16);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNORM_SHORT_565);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNORM_SHORT_555);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNORM_INT_101010);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_SIGNED_INT8);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_SIGNED_INT16);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_SIGNED_INT32);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNSIGNED_INT8);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNSIGNED_INT16);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_UNSIGNED_INT32);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_HALF_FLOAT);
      OCCA_CL_PRINT_CHANNEL_TYPE(CL_FLOAT);

#undef OCCA_CL_PRINT_CHANNEL_TYPE

      std::cout << "========================================\n";
    }

    std::string error(int e){
      switch(e){
      case CL_SUCCESS:                                   return "CL_SUCCESS";
      case CL_DEVICE_NOT_FOUND:                          return "CL_DEVICE_NOT_FOUND";
      case CL_DEVICE_NOT_AVAILABLE:                      return "CL_DEVICE_NOT_AVAILABLE";
      case CL_COMPILER_NOT_AVAILABLE:                    return "CL_COMPILER_NOT_AVAILABLE";
      case CL_MEM_OBJECT_ALLOCATION_FAILURE:             return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
      case CL_OUT_OF_RESOURCES:                          return "CL_OUT_OF_RESOURCES";
      case CL_OUT_OF_HOST_MEMORY:                        return "CL_OUT_OF_HOST_MEMORY";
      case CL_PROFILING_INFO_NOT_AVAILABLE:              return "CL_PROFILING_INFO_NOT_AVAILABLE";
      case CL_MEM_COPY_OVERLAP:                          return "CL_MEM_COPY_OVERLAP";
      case CL_IMAGE_FORMAT_MISMATCH:                     return "CL_IMAGE_FORMAT_MISMATCH";
      case CL_IMAGE_FORMAT_NOT_SUPPORTED:                return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
      case CL_BUILD_PROGRAM_FAILURE:                     return "CL_BUILD_PROGRAM_FAILURE";
      case CL_MAP_FAILURE:                               return "CL_MAP_FAILURE";
      case CL_MISALIGNED_SUB_BUFFER_OFFSET:              return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
      case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
      case CL_INVALID_VALUE:                             return "CL_INVALID_VALUE";
      case CL_INVALID_DEVICE_TYPE:                       return "CL_INVALID_DEVICE_TYPE";
      case CL_INVALID_PLATFORM:                          return "CL_INVALID_PLATFORM";
      case CL_INVALID_DEVICE:                            return "CL_INVALID_DEVICE";
      case CL_INVALID_CONTEXT:                           return "CL_INVALID_CONTEXT";
      case CL_INVALID_QUEUE_PROPERTIES:                  return "CL_INVALID_QUEUE_PROPERTIES";
      case CL_INVALID_COMMAND_QUEUE:                     return "CL_INVALID_COMMAND_QUEUE";
      case CL_INVALID_HOST_PTR:                          return "CL_INVALID_HOST_PTR";
      case CL_INVALID_MEM_OBJECT:                        return "CL_INVALID_MEM_OBJECT";
      case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:           return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
      case CL_INVALID_IMAGE_SIZE:                        return "CL_INVALID_IMAGE_SIZE";
      case CL_INVALID_SAMPLER:                           return "CL_INVALID_SAMPLER";
      case CL_INVALID_BINARY:                            return "CL_INVALID_BINARY";
      case CL_INVALID_BUILD_OPTIONS:                     return "CL_INVALID_BUILD_OPTIONS";
      case CL_INVALID_PROGRAM:                           return "CL_INVALID_PROGRAM";
      case CL_INVALID_PROGRAM_EXECUTABLE:                return "CL_INVALID_PROGRAM_EXECUTABLE";
      case CL_INVALID_KERNEL_NAME:                       return "CL_INVALID_KERNEL_NAME";
      case CL_INVALID_KERNEL_DEFINITION:                 return "CL_INVALID_KERNEL_DEFINITION";
      case CL_INVALID_KERNEL:                            return "CL_INVALID_KERNEL";
      case CL_INVALID_ARG_INDEX:                         return "CL_INVALID_ARG_INDEX";
      case CL_INVALID_ARG_VALUE:                         return "CL_INVALID_ARG_VALUE";
      case CL_INVALID_ARG_SIZE:                          return "CL_INVALID_ARG_SIZE";
      case CL_INVALID_KERNEL_ARGS:                       return "CL_INVALID_KERNEL_ARGS";
      case CL_INVALID_WORK_DIMENSION:                    return "CL_INVALID_WORK_DIMENSION";
      case CL_INVALID_WORK_GROUP_SIZE:                   return "CL_INVALID_WORK_GROUP_SIZE";
      case CL_INVALID_WORK_ITEM_SIZE:                    return "CL_INVALID_WORK_ITEM_SIZE";
      case CL_INVALID_GLOBAL_OFFSET:                     return "CL_INVALID_GLOBAL_OFFSET";
      case CL_INVALID_EVENT_WAIT_LIST:                   return "CL_INVALID_EVENT_WAIT_LIST";
      case CL_INVALID_EVENT:                             return "CL_INVALID_EVENT";
      case CL_INVALID_OPERATION:                         return "CL_INVALID_OPERATION";
      case CL_INVALID_GL_OBJECT:                         return "CL_INVALID_GL_OBJECT";
      case CL_INVALID_BUFFER_SIZE:                       return "CL_INVALID_BUFFER_SIZE";
      case CL_INVALID_MIP_LEVEL:                         return "CL_INVALID_MIP_LEVEL";
      case CL_INVALID_GLOBAL_WORK_SIZE:                  return "CL_INVALID_GLOBAL_WORK_SIZE";
      case CL_INVALID_PROPERTY:                          return "CL_INVALID_PROPERTY";
      default:                                           return "UNKNOWN ERROR";
      };
    }
  }

  const cl_channel_type clFormats[8] = {CL_UNSIGNED_INT8,
                                        CL_UNSIGNED_INT16,
                                        CL_UNSIGNED_INT32,
                                        CL_SIGNED_INT8,
                                        CL_SIGNED_INT16,
                                        CL_SIGNED_INT32,
                                        CL_HALF_FLOAT,
                                        CL_FLOAT};

  template <>
  void* formatType::format<occa::OpenCL>() const {
    return ((void*) &(clFormats[format_]));
  }
}

#endif