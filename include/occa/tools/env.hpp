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

    std::string var(const std::string &var);

    void signalExit(int sig);

    void endDirWithSlash(std::string &dir);

    class envInitializer_t {
    public: envInitializer_t();
    };
    extern envInitializer_t envInitializer;
  }
}

#endif
