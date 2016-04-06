#include <fstream>
#include <cstddef>

#include "occa/tools.hpp"
#include "occa/base.hpp"

#include "occa/parser/parser.hpp"

namespace occa {
  strToBoolMap_t fileLocks;

  //---[ Helper Info ]----------------
  namespace env {
    bool isInitialized = false;

    std::string HOME, PWD;
    std::string PATH, LD_LIBRARY_PATH;

    std::string OCCA_DIR, OCCA_CACHE_DIR;
    size_t OCCA_MEM_BYTE_ALIGN;
    stringVector_t OCCA_INCLUDE_PATH;

    void initialize() {
      if (isInitialized)
        return;

      ::signal(SIGTERM, env::signalExit);
      ::signal(SIGINT , env::signalExit);
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      ::signal(SIGKILL, env::signalExit);
      ::signal(SIGQUIT, env::signalExit);
#endif

      // Standard environment variables
#if (OCCA_OS & (LINUX_OS | OSX_OS))
      HOME            = env::var("HOME");
      PWD             = env::var("PWD");
      PATH            = env::var("PATH");
      LD_LIBRARY_PATH = env::var("LD_LIBRARY_PATH");

      endDirWithSlash(HOME);
      endDirWithSlash(PWD);
      endDirWithSlash(PATH);
#endif

      // OCCA environment variables
      OCCA_DIR = env::var("OCCA_DIR");
#ifdef OCCA_COMPILED_DIR
      if (OCCA_DIR.size() == 0) {
#  if (OCCA_OS & (LINUX_OS | OSX_OS))
        OCCA_DIR = OCCA_STRINGIFY(OCCA_COMPILED_DIR);
#  else
		OCCA_DIR = OCCA_COMPILED_DIR;
#  endif
      }
#endif

      OCCA_CHECK(0 < OCCA_DIR.size(),
                 "Environment variable [OCCA_DIR] is not set");

      initCachePath();
      initIncludePath();

      endDirWithSlash(OCCA_DIR);
      endDirWithSlash(OCCA_CACHE_DIR);

      OCCA_MEM_BYTE_ALIGN = OCCA_DEFAULT_MEM_BYTE_ALIGN;
      if(env::var("OCCA_MEM_BYTE_ALIGN").size() > 0){
        const size_t align = (size_t) std::atoi(env::var("OCCA_MEM_BYTE_ALIGN").c_str());

        if((align != 0) && ((align & (~align + 1)) == align)) {
          OCCA_MEM_BYTE_ALIGN = align;
        }
        else {
          std::cout << "Environment variable [OCCA_MEM_BYTE_ALIGN ("
                    << align << ")] is not a power of two, defaulting to "
                    << OCCA_DEFAULT_MEM_BYTE_ALIGN << '\n';
        }
      }

      isInitialized = true;
    }

    void initCachePath() {
      env::OCCA_CACHE_DIR = env::var("OCCA_CACHE_DIR");

      if (env::OCCA_CACHE_DIR.size() == 0) {
        std::stringstream ss;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
        ss << env::var("HOME") << "/._occa";
#else
        ss << env::var("USERPROFILE") << "\\AppData\\Local\\OCCA";

#  if OCCA_64_BIT
        ss << "_amd64";  // use different dir's fro 32 and 64 bit
#  else
        ss << "_x86";    // use different dir's fro 32 and 64 bit
#  endif
#endif
        env::OCCA_CACHE_DIR = ss.str();
      }

      const int chars = env::OCCA_CACHE_DIR.size();

      OCCA_CHECK(0 < chars,
                 "Path to the OCCA caching directory is not set properly, "
                 "unset OCCA_CACHE_DIR to use default directory [~/._occa]");

      env::OCCA_CACHE_DIR = sys::getFilename(env::OCCA_CACHE_DIR);

      if (!sys::dirExists(env::OCCA_CACHE_DIR))
        sys::mkpath(env::OCCA_CACHE_DIR);
    }

    void initIncludePath() {
      env::OCCA_INCLUDE_PATH.clear();
      std::string oip = env::var("OCCA_INCLUDE_PATH");

      const char *cStart = oip.c_str();
      const char *cEnd;

      stringVector_t tmpOIP;

      while(cStart[0] != '\0') {
        cEnd = cStart;
#if (OCCA_OS & (LINUX_OS | OSX_OS))
        skipTo(cEnd, ':');
#else
        skipTo(cEnd, ';');
#endif

        if (0 < (cEnd - cStart)) {
          std::string newPath(cStart, cEnd - cStart);
          newPath = sys::getFilename(newPath);
          endDirWithSlash(newPath);

          tmpOIP.push_back(newPath);
        }

        cStart = (cEnd + (cEnd[0] != '\0'));
      }

      tmpOIP.swap(env::OCCA_INCLUDE_PATH);
    }

    void signalExit(int sig) {
      clearLocks();
      ::exit(sig);
    }

    void endDirWithSlash(std::string &dir){
      if((0 < dir.size()) &&
         (dir[dir.size() - 1] != '/')){

        dir += '/';
      }
    }

    envInitializer_t::envInitializer_t() {
      env::initialize();
    }
    envInitializer_t envInitializer;
  }

  // Kernel Caching
  namespace kc {
    std::string sourceFile = "source.occa";
    std::string binaryFile = "binary";
  }
  //==================================

  fnvOutput_t::fnvOutput_t() {
    h[0] = 101527; h[1] = 101531;
    h[2] = 101533; h[3] = 101537;
    h[4] = 101561; h[5] = 101573;
    h[6] = 101581; h[7] = 101599;
  }

  bool fnvOutput_t::operator == (const fnvOutput_t &fo) {
    for(int i = 0; i < 8; ++i) {
      if (h[i] != fo.h[i])
        return false;
    }

    return true;
  }

  bool fnvOutput_t::operator != (const fnvOutput_t &fo) {
    for(int i = 0; i < 8; ++i) {
      if (h[i] != fo.h[i])
        return true;
    }

    return false;
  }

  void fnvOutput_t::mergeWith(const fnvOutput_t &fo) {
    for(int i = 0; i < 8; ++i)
      h[i] ^= fo.h[i];
  }

  fnvOutput_t::operator std::string () {
    std::stringstream ss;

    for(int i = 0; i < 8; ++i)
      ss << std::hex << h[i];

    return ss.str();
  }

  double currentTime() {
#if (OCCA_OS & LINUX_OS)

    timespec ct;
    clock_gettime(CLOCK_MONOTONIC, &ct);

    return (double) (ct.tv_sec + (1.0e-9 * ct.tv_nsec));

#elif (OCCA_OS == OSX_OS)
#  ifdef __clang__
    uint64_t ct;
    ct = mach_absolute_time();

    const Nanoseconds ct2 = AbsoluteToNanoseconds(*(AbsoluteTime *) &ct);

    return ((double) 1.0e-9) * ((double) ( *((uint64_t*) &ct2) ));
#  else
    clock_serv_t cclock;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);

    mach_timespec_t ct;
    clock_get_time(cclock, &ct);

    mach_port_deallocate(mach_task_self(), cclock);

    return (double) (ct.tv_sec + (1.0e-9 * ct.tv_nsec));
#  endif
#elif (OCCA_OS == WINDOWS_OS)
    static LARGE_INTEGER freq;
    static bool haveFreq = false;

    if (!haveFreq) {
      QueryPerformanceFrequency(&freq);
      haveFreq=true;
    }

    LARGE_INTEGER ct;

    QueryPerformanceCounter(&ct);

    return ((double) (ct.QuadPart)) / ((double) (freq.QuadPart));
#endif
  }

  //---[ File Functions ]-------------------------
  std::string getOnlyFilename(const std::string &filename) {
    std::string dir = getFileDirectory(filename);

    if (dir.size() < filename.size())
      return filename.substr(dir.size());

    return "";
  }

  std::string getPlainFilename(const std::string &filename) {
    std::string ext = getFileExtension(filename);
    std::string dir = getFileDirectory(filename);

    int start = (int) dir.size();
    int end = (int) filename.size();

    // For the [/] character
    if (0 < start)
      ++start;

    // For the [.ext] extension
    if (0 < ext.size())
      end -= (ext.size() - 1);

    return filename.substr(start, end - start);
  }

  std::string getFileDirectory(const std::string &filename) {
    const int chars = (int) filename.size();
    const char *c   = filename.c_str();

    int lastSlash = 0;

#if (OCCA_OS & (LINUX_OS | OSX_OS))
    for(int i = 0; i < chars; ++i)
      if (c[i] == '/')
        lastSlash = i;
#else
    for(int i = 0; i < chars; ++i)
      if ((c[i] == '/') || (c[i] == '\\'))
        lastSlash = i;
#endif

    if (lastSlash || (c[0] == '/'))
      ++lastSlash;

    return filename.substr(0, lastSlash);
  }

  std::string getFileExtension(const std::string &filename) {
    const char *c = filename.c_str();
    const char *i = NULL;

    while(*c != '\0') {
      if (*c == '.')
        i = c;

      ++c;
    }

    if (i != NULL)
      return filename.substr(i - filename.c_str() + 1);

    return "";
  }

  std::string compressFilename(const std::string &filename) {
    if (filename.find(env::OCCA_CACHE_DIR) != 0)
      return filename;

    const std::string libPath = env::OCCA_CACHE_DIR + "libraries/";
    const std::string kerPath = env::OCCA_CACHE_DIR + "kernels/";

    if (filename.find(libPath) == 0) {
      std::string libName = getLibraryName(filename);
      std::string theRest = filename.substr(libPath.size() + libName.size() + 1);

      return ("[" + libName + "]/" + theRest);
    }
    else if (filename.find(kerPath) == 0) {
      return filename.substr(kerPath.size());
    }

    return filename;
  }

  // NBN: handle binary mode and EOL chars on Windows
  std::string readFile(const std::string &filename, const bool readingBinary) {
    FILE *fp = NULL;

    if (!readingBinary) {
      fp = fopen(filename.c_str(), "r");
    }
    else{
      fp = fopen(filename.c_str(), "rb");
    }

    OCCA_CHECK(fp != 0,
               "Failed to open [" << compressFilename(filename) << "]");

    struct stat statbuf;
    stat(filename.c_str(), &statbuf);

    const size_t nchars = statbuf.st_size;

    char *buffer = (char*) calloc(nchars + 1, sizeof(char));
    size_t nread = fread(buffer, sizeof(char), nchars, fp);

    fclose(fp);
    buffer[nread] = '\0';

    std::string contents(buffer, nread);

    free(buffer);

    return contents;
  }

  void writeToFile(const std::string &filename,
                   const std::string &content) {

    sys::mkpath(getFileDirectory(filename));

    FILE *fp = fopen(filename.c_str(), "w");

    OCCA_CHECK(fp != 0,
               "Failed to open [" << compressFilename(filename) << "]");

    fputs(content.c_str(), fp);

    fclose(fp);
  }

  std::string getFileLock(const std::string &hash, const int depth) {
    std::string ret = (env::OCCA_CACHE_DIR + "locks/" + hash);

    ret += '_';
    ret += (char) ('0' + depth);

    return ret;
  }

  void clearLocks() {
    strToBoolMapIterator it = fileLocks.begin();
    while (it != fileLocks.end()) {
      releaseHash(it->first);
      ++it;
    }
    fileLocks.clear();
  }

  bool haveHash(const std::string &hash, const int depth) {
    std::string lockDir = getFileLock(hash, depth);

    sys::mkpath(env::OCCA_CACHE_DIR + "locks/");

    int mkdirStatus = sys::mkdir(lockDir);

    if (mkdirStatus && (errno == EEXIST))
      return false;

    fileLocks[lockDir] = true;

    return true;
  }

  void waitForHash(const std::string &hash, const int depth) {
    struct stat buffer;

    std::string lockDir   = getFileLock(hash, depth);
    const char *c_lockDir = lockDir.c_str();

    while(stat(c_lockDir, &buffer) == 0)
      ; // Do Nothing
  }

  void releaseHash(const std::string &hash, const int depth) {
    releaseHashLock(getFileLock(hash, depth));
  }

  void releaseHashLock(const std::string &lockDir) {
    sys::rmdir(lockDir);
    fileLocks.erase(lockDir);
  }

  bool fileNeedsParser(const std::string &filename) {
    std::string ext = getFileExtension(filename);

    return ((ext == "okl") ||
            (ext == "ofl") ||
            (ext == "cl") ||
            (ext == "cu"));
  }

  parsedKernelInfo parseFileForFunction(const std::string &deviceMode,
                                        const std::string &filename,
                                        const std::string &parsedFile,
                                        const std::string &functionName,
                                        const kernelInfo &info) {

    parser fileParser;

    const std::string extension = getFileExtension(filename);

    flags_t parserFlags = info.getParserFlags();

    parserFlags["mode"]     = deviceMode;
    parserFlags["language"] = ((extension != "ofl") ? "C" : "Fortran");

    if ((extension == "oak") ||
       (extension == "oaf")) {

      parserFlags["magic"] = "enabled";
    }

    std::string parsedContent = fileParser.parseFile(info.header,
                                                     filename,
                                                     parserFlags);

    if (!sys::fileExists(parsedFile)) {
      sys::mkpath(getFileDirectory(parsedFile));

      std::ofstream fs;
      fs.open(parsedFile.c_str());

      fs << parsedContent;

      fs.close();
    }

    kernelInfoIterator kIt = fileParser.kernelInfoMap.find(functionName);

    if (kIt != fileParser.kernelInfoMap.end())
      return (kIt->second)->makeParsedKernelInfo();

    OCCA_CHECK(false,
               "Could not find function ["
               << functionName << "] in file ["
               << compressFilename(filename    ) << "]");

    return parsedKernelInfo();
  }

  std::string removeSlashes(const std::string &str) {
    std::string ret = str;
    const size_t chars = str.size();

    for(size_t i = 0; i < chars; ++i) {
      if (ret[i] == '/')
        ret[i] = '_';
    }

    return ret;
  }

  std::string getOccaScriptFile(const std::string &filename) {
    return readFile(env::OCCA_DIR + "/scripts/" + filename);
  }

  void setupOccaHeaders(const kernelInfo &info) {
    cacheFile(sys::getFilename("[occa]/primitives.hpp"),
              readFile(env::OCCA_DIR + "/include/occa/defines/vector.hpp"),
              "vectorDefines");

    std::string mode = modeToStr(info.mode);
    cacheFile(info.getModeHeaderFilename(),
              readFile(env::OCCA_DIR + "/include/occa/defines/" + mode + ".hpp"),
              mode + "Defines");
  }

  void cacheFile(const std::string &filename,
                 std::string source,
                 const std::string &hash) {

    cacheFile(filename, source.c_str(), hash, false);
  }

  void cacheFile(const std::string &filename,
                 const char *source,
                 const std::string &hash,
                 const bool deleteSource) {
    if(!haveHash(hash)){
      waitForHash(hash);
    } else {
      if (!sys::fileExists(filename)) {
        sys::mkpath(getFileDirectory(filename));

        std::ofstream fs2;
        fs2.open(filename.c_str());
        fs2 << source;
        fs2.close();
      }
      releaseHash(hash);
    }
    if (deleteSource)
      delete [] source;
  }

  void createSourceFileFrom(const std::string &filename,
                            const std::string &hashDir,
                            const kernelInfo &info) {

    const std::string sourceFile = hashDir + kc::sourceFile;

    if (sys::fileExists(sourceFile))
      return;

    sys::mkpath(hashDir);

    setupOccaHeaders(info);

    std::ofstream fs;
    fs.open(sourceFile.c_str());

    fs << "#include \"" << info.getModeHeaderFilename() << "\"\n"
       << "#include \"" << sys::getFilename("[occa]/primitives.hpp") << "\"\n";

    if (info.mode & (Serial | OpenMP | Pthreads | CUDA)) {
      fs << "#if defined(OCCA_IN_KERNEL) && !OCCA_IN_KERNEL\n"
         << "using namespace occa;\n"
         << "#endif\n";
    }


    fs << info.header
       << readFile(filename);

    fs.close();
  }
  //==============================================


  //---[ Hash Functions ]-------------------------
  fnvOutput_t fnv(const void *ptr, uintptr_t bytes) {
    std::stringstream ss;

    const char *c = (char*) ptr;

    fnvOutput_t fo;
    int *h = fo.h;

    const int p[8] = {102679, 102701,
                      102761, 102763,
                      102769, 102793,
                      102797, 102811};

    for(uintptr_t i = 0; i < bytes; ++i) {
      for(int j = 0; j < 8; ++j)
        h[j] = (h[j] * p[j]) ^ c[i];
    }

    // int h2[8];

    // for(int i = 0; i < 8; ++i)
    //   h2[i] = ((h[0] & (0xFF << (8*i))) << (8*i + 0))
    //     |     ((h[1] & (0xFF << (8*i))) << (8*i + 1))
    //     |     ((h[2] & (0xFF << (8*i))) << (8*i + 2))
    //     |     ((h[3] & (0xFF << (8*i))) << (8*i + 3))
    //     |     ((h[4] & (0xFF << (8*i))) << (8*i + 4))
    //     |     ((h[5] & (0xFF << (8*i))) << (8*i + 5))
    //     |     ((h[6] & (0xFF << (8*i))) << (8*i + 6))
    //     |     ((h[7] & (0xFF << (8*i))) << (8*i + 7));

    return fo;
  }

  template <>
  fnvOutput_t fnv(const std::string &saltedString) {
    return fnv(saltedString.c_str(), saltedString.size());
  }

  std::string getContentHash(const std::string &content,
                             const std::string &salt) {

    std::string fo = fnv(content + salt);

    // Only taking the first 16 characters
    return fo.substr(0, 16);
  }

  std::string getFileContentHash(const std::string &filename,
                                 const std::string &salt) {

    return getContentHash(readFile(filename), salt);
  }

  std::string getLibraryName(const std::string &filename) {
    const std::string cacheLibraryPath = (env::OCCA_CACHE_DIR + "libraries/");

    if (filename.find(cacheLibraryPath) != 0)
      return "";

    const int chars = (int) filename.size();
    const char *c   = filename.c_str();

    int start = (int) cacheLibraryPath.size();
    int end;

    for(end = start; end < chars; ++end) {
      if (c[end] == '/')
        break;
    }

    return filename.substr(start, end - start);
  }

  std::string hashFrom(const std::string &filename) {
    std::string hashDir = hashDirFor(filename, "");

    const int chars = (int) filename.size();
    const char *c   = filename.c_str();

    int start = (int) hashDir.size();
    int end;

    for(end = (start + 1); end < chars; ++end) {
      if (c[end] == '/')
        break;
    }

    return filename.substr(start, end - start);
  }

  std::string hashDirFor(const std::string &filename,
                         const std::string &hash) {

    if (filename.size() == 0) {
      if (hash.size() != 0)
        return (env::OCCA_CACHE_DIR + "kernels/" + hash + "/");
      else
        return (env::OCCA_CACHE_DIR + "kernels/");
    }

    std::string occaLibName = getLibraryName(sys::getFilename(filename));

    if (occaLibName.size() == 0) {
      if (hash.size() != 0)
        return (env::OCCA_CACHE_DIR + "kernels/" + hash + "/");
      else
        return (env::OCCA_CACHE_DIR + "kernels/");
    }

    return (env::OCCA_CACHE_DIR + "libraries/" + occaLibName + "/" + hash + "/");
  }
  //==============================================


  //---[ String Functions ]-----------------------
  uintptr_t atoi(const char *c) {
    uintptr_t ret = 0;

    const char *c0 = c;

    bool negative  = false;
    bool unsigned_ = false;
    int longs      = 0;

    skipWhitespace(c);

    if ((*c == '+') || (*c == '-')) {
      negative = (*c == '-');
      ++c;
    }

    if (c[0] == '0')
      return atoiBase2(c0);

    while(('0' <= *c) && (*c <= '9')) {
      ret *= 10;
      ret += *(c++) - '0';
    }

    while(*c != '\0') {
      const char C = upChar(*c);

      if (C == 'L')
        ++longs;
      else if (C == 'U')
        unsigned_ = true;
      else
        break;

      ++c;
    }

    if (negative)
      ret = ((~ret) + 1);

    if (longs == 0) {
      if (!unsigned_)
        ret = ((uintptr_t) ((int) ret));
      else
        ret = ((uintptr_t) ((unsigned int) ret));
    }
    else if (longs == 1) {
      if (!unsigned_)
        ret = ((uintptr_t) ((long) ret));
      else
        ret = ((uintptr_t) ((unsigned long) ret));
    }
    // else it's already in uintptr_t form

    return ret;
  }

  uintptr_t atoiBase2(const char*c) {
    uintptr_t ret = 0;

    const char *c0 = c;

    bool negative     = false;
    int bits          = 3;
    int maxDigitValue = 10;
    char maxDigitChar = '9';

    skipWhitespace(c);

    if ((*c == '+') || (*c == '-')) {
      negative = (*c == '-');
      ++c;
    }

    if (*c == '0') {
      ++c;

      const char C = upChar(*c);

      if (C == 'X') {
        bits = 4;
        ++c;

        maxDigitValue = 16;
        maxDigitChar  = 'F';
      }
      else if (C == 'B') {
        bits = 1;
        ++c;

        maxDigitValue = 2;
        maxDigitChar  = '1';
      }
    }

    while(true) {
      if (('0' <= *c) && (*c <= '9')) {
        const char digitValue = *(c++) - '0';

        OCCA_CHECK(digitValue < maxDigitValue,
                   "Number [" << std::string(c0, c - c0)
                   << "...] must contain digits in the [0,"
                   << maxDigitChar << "] range");

        ret <<= bits;
        ret += digitValue;
      }
      else {
        const char C = upChar(*c);

        if (('A' <= C) && (C <= 'F')) {
          const char digitValue = 10 + (C - 'A');
          ++c;

          OCCA_CHECK(digitValue < maxDigitValue,
                     "Number [" << std::string(c0, c - c0)
                     << "...] must contain digits in the [0,"
                     << maxDigitChar << "] range");

          ret <<= bits;
          ret += digitValue;
        }
        else
          break;
      }
    }

    if (negative)
      ret = ((~ret) + 1);

    return ret;
  }

  uintptr_t atoi(const std::string &str){
    return occa::atoi((const char*) str.c_str());
  }

  double atof(const char *c){
    return ::atof(c);
  }

  double atof(const std::string &str){
    return ::atof(str.c_str());
  }

  double atod(const char *c){
    double ret;
#if (OCCA_OS & (LINUX_OS | OSX_OS))
    sscanf(c, "%lf", &ret);
#else
    sscanf_s(c, "%lf", &ret);
#endif
    return ret;
  }

  double atod(const std::string &str){
    return occa::atod(str.c_str());
  }

  std::string stringifyBytes(uintptr_t bytes) {
    if (0 < bytes) {
      std::stringstream ss;
	  uint64_t bigBytes = bytes;
      uint64_t big1 = 1;

      if (bigBytes < (big1 << 10))
        ss << bigBytes << " bytes";
      else if (bigBytes < (big1 << 20))
        ss << (bigBytes >> 10) << " KB";
      else if (bigBytes < (big1 << 30))
        ss << (bigBytes >> 20) << " MB";
      else if (bigBytes < (big1 << 40))
        ss << (bigBytes >> 30) << " GB";
      else if (bigBytes < (big1 << 50))
        ss << (bigBytes >> 40) << " TB";
      else
        ss << bigBytes << " bytes";

      return ss.str();
    }

    return "";
  }
  //==============================================


  //---[ Misc Functions ]-------------------------
  int maxBase2Bit(const int value){
    if(value <= 0)
      return 0;

    const int maxBits = 8 * sizeof(value);

    for(int i = 0; i < maxBits; ++i){
      if(value <= (1 << i))
        return i;
    }

    return 0;
  }

  int maxBase2(const int value){
    return (1 << maxBase2Bit(value));
  }

  uintptr_t ptrDiff(void *start, void *end){
    if(start < end)
      return (uintptr_t) (((char*) end) - ((char*) start));

    return (uintptr_t) (((char*) start) - ((char*) end));
  }

  void* ptrOff(void *ptr, uintptr_t offset){
    return (void*) (((char*) ptr) + offset);
  }
  //==============================================
}
