#ifndef OCCA_TOOLS_ENV_HEADER
#define OCCA_TOOLS_ENV_HEADER

#include "occa/typedefs.hpp"

namespace occa {
  namespace env {
    extern bool isInitialized;

    extern std::string HOME, PWD;
    extern std::string PATH, LD_LIBRARY_PATH;

    extern std::string OCCA_DIR, OCCA_CACHE_DIR;
    extern size_t      OCCA_MEM_BYTE_ALIGN;
    extern strVector_t OCCA_INCLUDE_PATH;

    void initialize();
    void initCachePath();
    void initIncludePath();

    void signalExit(int sig);

    void endDirWithSlash(std::string &dir);

    class envInitializer_t {
    public: envInitializer_t();
    };
    extern envInitializer_t envInitializer;
  }
}

#endif
