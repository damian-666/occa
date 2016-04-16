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

#if (OCCA_OS & (LINUX_OS | OSX_OS))
#  if   (OCCA_OS == LINUX_OS)
#    include <sys/sysinfo.h>
#  elif (OCCA_OS == OSX_OS)
#    include <mach/mach.h>
#    include <mach/mach_host.h>
#  endif
#  if (OCCA_OS != WINUX_OS)
#    include <sys/sysctl.h>
#  endif
#  include <sys/wait.h>
#  include <dlfcn.h>
#else
#  include <windows.h>
#endif

#include "occa/sys.hpp"
#include "occa/tools/env.hpp"
#include "occa/parser/tools.hpp"

namespace occa {
  namespace flags {
    const int checkCacheDir = (1 << 0);
  }

  namespace sys {
    //---[ System Calls ]---------------
    int call(const std::string &cmdline) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      FILE *fp = popen(cmdline.c_str(), "r");
      return pclose(fp);
#else
      FILE *fp = _popen(cmdline.c_str(), "r");
      return _pclose(fp);
#endif
    }

    int call(const std::string &cmdline, std::string &output) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      FILE *fp = popen(cmdline.c_str(), "r");
#else
      FILE *fp = _popen(cmdline.c_str(), "r");
#endif

      size_t lineBytes = 512;
      char lineBuffer[512];

      while(fgets(lineBuffer, lineBytes, fp))
        output += lineBuffer;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return pclose(fp);
#else
      return _pclose(fp);
#endif
    }

    std::string expandEnvVariables(const std::string &str) {
      std::string ret;

      const char *cRoot = str.c_str();
      const char *c     = cRoot;

      while(*c != '\0') {
        const char C = c[0];

        if ((C == '$')     &&
           (c[1] != '\0') &&                   // Last $ doesn't expand
           ((cRoot == c) || (c[-1] != '\\'))) { // Escape the '$'

          ++c; // Skip $

          const bool hasBrace = (*c == '{');
          const char *c0 = (c + hasBrace);

          if (hasBrace)
            skipTo(c, '}');
          else
            skipToWhitespace(c);

          std::string envVar = env::var(std::string(c0, c - c0));

          ret += envVar;

          if (hasBrace)
            ++c;
        }
        else {
          ret += C;
          ++c;
        }
      }

      return ret;
    }

    void rmdir(const std::string &dir) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      ::rmdir(dir.c_str());
#else
      ::_rmdir(dir.c_str());
#endif
    }

    int mkdir(const std::string &dir) {
      errno = 0;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return ::mkdir(dir.c_str(), 0755);
#else
      return ::_mkdir(dir.c_str());
#endif
    }

    void mkpath(const std::string &dir) {
      strVector_t path;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      sys::absolutePathVec(dir, path);
#else
      // NBN: handle e.g. D:\my\path
      // [TODO] Need to make this work for relative paths
      std::string dir2 = dir;
      std::string::iterator itA = dir2.begin();
      itA += 4;
      std::replace(itA, dir2.end(), '\\', '/');
      sys::absolutePathVec(dir2, path);
#endif

      const int dirCount = (int) path.size();
      int makeFrom = -1;

      if (dirCount == 0)
        return;

      std::string sPath;

      for(int d = 0; d < dirCount; ++d) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
        sPath += '/';
#else
        // Don't want leading slash on absolute path
        if (d > 0)
          sPath += '/';
#endif
        sPath += path[d];

        if (!dirExists(sPath)) {
          makeFrom = d;
          break;
        }
      }

      if (0 < makeFrom) {
        sys::mkdir(sPath);

        for(int d = (makeFrom + 1); d < dirCount; ++d) {
          sPath += '/';
          sPath += path[d];

          sys::mkdir(sPath);
        }
      }
    }

    bool dirExists(const std::string &dir_) {
      std::string dir = expandEnvVariables(dir_);
      strip(dir);

      struct stat statInfo;

      return ((stat(dir.c_str(), &statInfo) == 0) &&
              (statInfo.st_mode &S_IFDIR));
    }

    bool fileExists(const std::string &filename_,
                    const int flags) {

      std::string filename = expandEnvVariables(filename_);
      strip(filename);

      if (flags & flags::checkCacheDir)
        return fileExists(getFilename(filename));

      struct stat statInfo;

      return (stat(filename.c_str(), &statInfo) == 0);
    }

    std::string getFilename(const std::string &filename) {
      std::string ret;

      strVector_t path;
      absolutePathVec(filename, path);

      const int dirCount = (int) path.size();

      if (dirCount == 0)
        return "";

      for(int dir = 0; dir < dirCount; ++dir) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
        ret += '/';
#else
        // Don't want leading slash on absolute path
        if (dir > 0)
          ret += '/';
#endif
        ret += path[dir];
      }

      return ret;
    }

    void absolutePathVec(const std::string &path_,
                         strVector_t &pathVec) {

      std::string path = expandEnvVariables(path_);
      strip(path);

      const int chars = (int) path.size();
      const char *c   = path.c_str();

      bool foundDir = false;

      if (chars == 0)
        return;

      // Starts at home
      if ((c[0] == '~') &&
         ((c[1] == '/') || (c[1] == '\0'))) {

        absolutePathVec(env::HOME, pathVec);

        if (c[1] == '\0')
          return;

        foundDir = true;
        c += 2;
      }
#if (OCCA_OS == WINDOWS_OS)
      else if(c[1] == ':'){

        const char *c0 = c;
        c += 3;
        skipTo(c, '/');
        pathVec.push_back(std::string(c0, c - c0));

        foundDir = true;
        ++c;
      }
#endif
      // OCCA path
      else if (c[0] == '[') {
        const char *c0 = (c + 1);
        skipTo(c, ']');

        if (c[0] == ']') {
          absolutePathVec(env::OCCA_CACHE_DIR, pathVec);

          pathVec.push_back("libraries");
          pathVec.push_back(std::string(c0, c - c0));

          foundDir = true;
          ++c;
        }
      }

      // Relative path
      if ((!foundDir) &&
         (c[0] != '/')) {

        strVector_t::iterator it = env::OCCA_INCLUDE_PATH.begin();

        while(it != env::OCCA_INCLUDE_PATH.end()) {
          if (it->size() && sys::fileExists(*it + path)) {
            absolutePathVec(*it, pathVec);
            foundDir = true;
            break;
          }
          ++it;
        }

        if (!foundDir)
          absolutePathVec(env::PWD, pathVec);
      }

      while(c[0] != '\0') {
        if (c[0] == '/') {
          ++c;
          continue;
        }

        const char *c0 = c;
        skipTo(c, '/');

        pathVec.push_back(std::string(c0, c - c0));

        if (c[0] != '\0')
          ++c;
      }
    }

    strVector_t absolutePathVec(const std::string &path){
      strVector_t pathVec;
      absolutePathVec(path, pathVec);
      return pathVec;
    }

    //---[ Processor Info ]-------------
    std::string getFieldFrom(const std::string &command,
                             const std::string &field){
#if (OCCA_OS & LINUX)
      std::string shellToolsFile = sys::getFilename("[occa]/scripts/shellTools.sh");

      if(!sys::fileExists(shellToolsFile)){
        sys::mkpath(dirname(shellToolsFile));

        std::ofstream fs2;
        fs2.open(shellToolsFile.c_str());

        fs2 << getCachedScript("shellTools.sh");

        fs2.close();
      }

      std::stringstream ss;

      ss << "echo \"(. " << shellToolsFile << "; " << command << " '" << field << "')\" | bash";

      std::string sCommand = ss.str();

      FILE *fp;
      fp = popen(sCommand.c_str(), "r");

      const int bufferSize = 4096;
      char *buffer = new char[bufferSize];

      ignoreResult( fread(buffer, sizeof(char), bufferSize, fp) );

      pclose(fp);

      int end;

      for(end = 0; end < bufferSize; ++end){
        if(buffer[end] == '\n')
          break;
      }

      std::string ret(buffer, end);

      delete [] buffer;

      return ret;
#else
      return "";
#endif
    }

    std::string getProcessorName(){
#if   (OCCA_OS & LINUX_OS)
      return getFieldFrom("getCPUINFOField", "model name");
#elif (OCCA_OS == OSX_OS)
      size_t bufferSize = 100;
      char buffer[100];

      sysctlbyname("machdep.cpu.brand_string",
                   &buffer, &bufferSize,
                   NULL, 0);

      return std::string(buffer);
#elif (OCCA_OS == WINDOWS_OS)
      char buffer[MAX_COMPUTERNAME_LENGTH + 1];
      int bytes;

      GetComputerName((LPSTR) buffer, (LPDWORD) &bytes);

      return std::string(buffer, bytes);
#endif
    }

    int getCoreCount(){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      return sysconf(_SC_NPROCESSORS_ONLN);
#elif (OCCA_OS == WINDOWS_OS)
      SYSTEM_INFO sysinfo;
      GetSystemInfo(&sysinfo);
      return sysinfo.dwNumberOfProcessors;
#endif
    }

    int getProcessorFrequency(){
#if   (OCCA_OS & LINUX_OS)
      std::stringstream ss;
      int freq;

      ss << getFieldFrom("getCPUINFOField", "cpu MHz");

      ss >> freq;

      return freq;
#elif (OCCA_OS == OSX_OS)
      uint64_t frequency = 0;
      size_t size = sizeof(frequency);

      int error = sysctlbyname("hw.cpufrequency", &frequency, &size, NULL, 0);

      OCCA_CHECK(error != ENOMEM,
                 "Error getting CPU Frequency.\n");

      return frequency/1.0e6;
#elif (OCCA_OS == WINDOWS_OS)
      LARGE_INTEGER performanceFrequency;
      QueryPerformanceFrequency(&performanceFrequency);

      return (int) (((double) performanceFrequency.QuadPart) / 1000.0);
#endif
    }

    std::string getProcessorCacheSize(int level){
#if   (OCCA_OS & LINUX_OS)
      std::stringstream field;

      field << 'L' << level;

      if(level == 1)
        field << 'd';

      field << " cache";

      return getFieldFrom("getLSCPUField", field.str());
#elif (OCCA_OS == OSX_OS)
      std::stringstream ss;
      ss << "hw.l" << level;

      if(level == 1)
        ss << 'd';

      ss << "cachesize";

      std::string field = ss.str();

      uint64_t cache = 0;
      size_t size = sizeof(cache);

      int error = sysctlbyname(field.c_str(), &cache, &size, NULL, 0);

      OCCA_CHECK(error != ENOMEM,
                 "Error getting L" << level << " Cache Size.\n");

      return stringifyBytes(cache);
#elif (OCCA_OS == WINDOWS_OS)
      std::stringstream ss;
      DWORD cache = 0;
      int bytes = 0;

      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;

      GetLogicalProcessorInformation(buffer, (LPDWORD) &bytes);

      OCCA_CHECK((GetLastError() == ERROR_INSUFFICIENT_BUFFER),
                 "[GetLogicalProcessorInformation] Failed");

      buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION) sys::malloc(bytes);

      bool passed = GetLogicalProcessorInformation(buffer, (LPDWORD) &bytes);

      OCCA_CHECK(passed,
                 "[GetLogicalProcessorInformation] Failed");

      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pos = buffer;
      int off = 0;
      int sk = sizeof(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION);

      while ((off + sk) <= bytes) {
        switch(pos->Relationship){
        case RelationCache:{
          CACHE_DESCRIPTOR info = pos->Cache;

          if (info.Level == level) {
            cache = info.Size;
            break;
          }
        }
        }
        ++pos;
        off += sk;
      }

      sys::free(buffer);

      return stringifyBytes(cache);
#endif
    }

    uintptr_t installedRAM(){
#if   (OCCA_OS & LINUX_OS)
      struct sysinfo info;

      const int error = sysinfo(&info);

      if(error != 0)
        return 0;

      return info.totalram;
#elif (OCCA_OS == OSX_OS)
      int64_t ram;

      int mib[2]   = {CTL_HW, HW_MEMSIZE};
      size_t bytes = sizeof(ram);

      sysctl(mib, 2, &ram, &bytes, NULL, 0);

      return ram;
#elif (OCCA_OS == WINDOWS_OS)
      return 0;
#endif
    }

    uintptr_t availableRAM(){
#if   (OCCA_OS & LINUX_OS)
      struct sysinfo info;

      const int error = sysinfo(&info);

      if(error != 0)
        return 0;

      return info.freeram;
#elif (OCCA_OS == OSX_OS)
      mach_msg_type_number_t infoCount = HOST_VM_INFO_COUNT;
      mach_port_t hostPort = mach_host_self();

      vm_statistics_data_t hostInfo;
      kern_return_t status;
      vm_size_t pageSize;

      status = host_page_size(hostPort, &pageSize);

      if(status != KERN_SUCCESS)
        return 0;

      status = host_statistics(hostPort,
                               HOST_VM_INFO,
                               (host_info_t) &hostInfo,
                               &infoCount);

      if(status != KERN_SUCCESS)
        return 0;

      return (hostInfo.free_count * pageSize);
#elif (OCCA_OS == WINDOWS_OS)
      return 0;
#endif
    }

    std::string getDeviceListInfo(){
      std::stringstream ss, ssFreq;

      ss << getCoreCount();

      uintptr_t ram      = installedRAM();
      std::string ramStr = stringifyBytes(ram);

      const int freq = getProcessorFrequency();

      if(freq < 1000)
        ssFreq << freq << " MHz";
      else
        ssFreq << (freq/1000.0) << " GHz";

      std::string l1 = getProcessorCacheSize(1);
      std::string l2 = getProcessorCacheSize(2);
      std::string l3 = getProcessorCacheSize(3);

      size_t maxSize = ((l1.size() < l2.size()) ? l2.size() : l1.size());
      maxSize        = ((maxSize   < l3.size()) ? l3.size() : maxSize  );

      if(maxSize){
        l1 = std::string(maxSize - l1.size(), ' ') + l1;
        l2 = std::string(maxSize - l2.size(), ' ') + l2;
        l3 = std::string(maxSize - l3.size(), ' ') + l3;
      }

      std::string tab[2];
      tab[0] = "   CPU Info   ";
      tab[1] = "              ";

      std::string processorName  = getProcessorName();
      std::string clockFrequency = ssFreq.str();
      std::string coreCount      = ss.str();

      ss.str("");

      // [P]rinted [S]omething
      bool ps = false;

      // << "==============o=======================o==========================================\n";
      if(processorName.size())
        ss << tab[ps]  << "|  Processor Name       | " << processorName                   << '\n'; ps = true;
      if(coreCount.size())
        ss << tab[ps]  << "|  Cores                | " << coreCount                       << '\n'; ps = true;
      if(ramStr.size())
        ss << tab[ps]  << "|  Memory (RAM)         | " << ramStr                          << '\n'; ps = true;
      if(clockFrequency.size())
        ss << tab[ps]  << "|  Clock Frequency      | " << clockFrequency                  << '\n'; ps = true;
      ss   << tab[ps]  << "|  SIMD Instruction Set | " << OCCA_VECTOR_SET                 << '\n'
           << tab[ps]  << "|  SIMD Width           | " << (32*OCCA_SIMD_WIDTH) << " bits" << '\n'; ps = true;
      if(l1.size())
        ss << tab[ps]  << "|  L1 Cache Size (d)    | " << l1                              << '\n'; ps = true;
      if(l2.size())
        ss << tab[ps]  << "|  L2 Cache Size        | " << l2                              << '\n'; ps = true;
      if(l3.size())
        ss << tab[ps]  << "|  L3 Cache Size        | " << l3                              << '\n';
      // << "==============o=======================o==========================================\n";

      return ss.str();
    }

    int compilerVendor(const std::string &compiler){
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      std::stringstream ss;
      int vendor_ = sys::vendor::notFound;

      const std::string safeCompiler = removeSlashes(compiler);
      const std::string &hash = safeCompiler;

      const std::string testFilename   = sys::getFilename("[occa]/testing/compilerVendorTest.cpp");
      const std::string binaryFilename = sys::getFilename("[occa]/testing/compilerVendor_" + safeCompiler);
      const std::string infoFilename   = sys::getFilename("[occa]/testing/compilerVendorInfo_" + safeCompiler);

      cacheFile(testFilename,
                io::read(env::OCCA_DIR + "/scripts/compilerVendorTest.cpp"),
                "compilerVendorTest");

      if(!haveHash(hash)){
        waitForHash(hash);
      } else {
        if(!sys::fileExists(infoFilename)){
          ss << compiler
             << ' '
             << testFilename
             << " -o "
             << binaryFilename
             << " > /dev/null 2>&1";

          const int compileError = system(ss.str().c_str());

          if(!compileError){
            int exitStatus = system(binaryFilename.c_str());
            int vendorBit  = WEXITSTATUS(exitStatus);

            if(vendorBit < sys::vendor::b_max)
              vendor_ = (1 << vendorBit);
          }

          ss.str("");
          ss << vendor_;

          io::write(infoFilename, ss.str());
          releaseHash(hash);

          return vendor_;
        }
        releaseHash(hash);
      }

      ss << io::read(infoFilename);
      ss >> vendor_;

      return vendor_;

#elif (OCCA_OS == WINDOWS_OS)
#  if OCCA_USING_VS
      return sys::vendor::VisualStudio;
#  endif

      if(compiler.find("cl.exe") != std::string::npos){
        return sys::vendor::VisualStudio;
      }
#endif
    }

    std::string compilerSharedBinaryFlags(const std::string &compiler){
      return compilerSharedBinaryFlags( sys::compilerVendor(compiler) );
    }

    std::string compilerSharedBinaryFlags(const int vendor_){
      if(vendor_ & (sys::vendor::GNU   |
                    sys::vendor::LLVM  |
                    sys::vendor::Intel |
                    sys::vendor::IBM   |
                    sys::vendor::PGI   |
                    sys::vendor::Cray  |
                    sys::vendor::Pathscale)){

        return "-x c++ -fPIC -shared"; // [-] -x c++ for now
      }
      else if(vendor_ & sys::vendor::HP){
        return "+z -b";
      }
      else if(vendor_ & sys::vendor::VisualStudio){
#if OCCA_DEBUG_ENABLED
        return "/TP /LD /MDd";
#else
        return "/TP /LD /MD";
#endif
      }

      return "";
    }

    void addSharedBinaryFlagsTo(const std::string &compiler, std::string &flags){
      addSharedBinaryFlagsTo(sys::compilerVendor(compiler), flags);
    }

    void addSharedBinaryFlagsTo(const int vendor_, std::string &flags){
      std::string sFlags = sys::compilerSharedBinaryFlags(vendor_);

      if(flags.size() == 0)
        flags = sFlags;

      if(flags.find(sFlags) == std::string::npos)
        flags = (sFlags + " " + flags);
    }

    //---[ Dynamic Methods ]------------
    void* malloc(uintptr_t bytes){
      void* ptr;

#if   (OCCA_OS & (LINUX_OS | OSX_OS))
      ignoreResult( posix_memalign(&ptr, env::OCCA_MEM_BYTE_ALIGN, bytes) );
#elif (OCCA_OS == WINDOWS_OS)
      ptr = ::malloc(bytes);
#endif

      return ptr;
    }

    void free(void *ptr){
      ::free(ptr);
    }

    void* dlopen(const std::string &filename,
                 const std::string &hash){

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      void *dlHandle = ::dlopen(filename.c_str(), RTLD_NOW);

      if((dlHandle == NULL) && (0 < hash.size())){
        releaseHash(hash, 0);

        OCCA_CHECK(false,
                   "Error loading binary [" << io::shortname(filename) << "] with dlopen");
      }
#else
      void *dlHandle = LoadLibraryA(filename.c_str());

      if((dlHandle == NULL) && (0 < hash.size())){
        releaseHash(hash, 0);

        OCCA_CHECK(dlHandle != NULL,
                   "Error loading dll [" << io::shortname(filename) << "] (WIN32 error: " << GetLastError() << ")");
      }
#endif

      return dlHandle;
    }

    handleFunction_t dlsym(void *dlHandle,
                           const std::string &functionName,
                           const std::string &hash){

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      void *sym = ::dlsym(dlHandle, functionName.c_str());

      char *dlError;

      if(((dlError = dlerror()) != NULL) && (0 < hash.size())){
        releaseHash(hash, 0);

        OCCA_CHECK(false,
                   "Error loading symbol from binary with dlsym (DL Error: " << dlError << ")");
      }
#else
      void *sym = GetProcAddress((HMODULE) dlHandle, functionName.c_str());

      if((sym == NULL) && (0 < hash.size())){

        OCCA_CHECK(false,
                   "Error loading symbol from binary with GetProcAddress");
      }
#endif

      handleFunction_t sym2;

      ::memcpy(&sym2, &sym, sizeof(sym));

      return sym2;
    }

    void dlclose(void *dlHandle) {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      ::dlclose(dlHandle);
#else
      FreeLibrary((HMODULE) (dlHandle));
#endif
    }

    void runFunction(handleFunction_t f,
                     const int *occaKernelInfoArgs,
                     int occaInnerId0, int occaInnerId1, int occaInnerId2,
                     int argc, void **args){

#include "operators/runFunctionFromArguments.cpp"
    }
  }

  mutex_t::mutex_t() {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    int error = pthread_mutex_init(&mutexHandle, NULL);

    OCCA_CHECK(error == 0,
               "Error initializing mutex");
#else
    mutexHandle = CreateMutex(NULL, FALSE, NULL);
#endif
  }

  void mutex_t::free() {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    int error = pthread_mutex_destroy(&mutexHandle);

    OCCA_CHECK(error == 0,
               "Error freeing mutex");
#else
    CloseHandle(mutexHandle);
#endif
  }

  void mutex_t::lock() {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    pthread_mutex_lock(&mutexHandle);
#else
    WaitForSingleObject(mutexHandle, INFINITE);
#endif
  }

  void mutex_t::unlock() {
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    pthread_mutex_unlock(&mutexHandle);
#else
    ReleaseMutex(mutexHandle);
#endif
  }
}
