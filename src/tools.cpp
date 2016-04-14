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

#if   (OCCA_OS & LINUX_OS)
#  include <sys/time.h>
#  include <unistd.h>
#  include <sys/types.h>
#elif (OCCA_OS & OSX_OS)
#  ifdef __clang__
#    include <CoreServices/CoreServices.h>
#    include <mach/mach_time.h>
#  else
#    include <mach/clock.h>
#    include <mach/mach.h>
#  endif
#  include <sys/types.h>
#else
#  ifndef NOMINMAX
#    define NOMINMAX     // NBN: clear min/max macros
#  endif
#  include <windows.h>
#endif

#include "occa/tools.hpp"

namespace occa {
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

  //---[ Dim ]--------------------------
  dim::dim() :
    x(1),
    y(1),
    z(1) {}

  dim::dim(uintptr_t x_) :
    x(x_),
    y(1),
    z(1) {}

  dim::dim(uintptr_t x_, uintptr_t y_) :
    x(x_),
    y(y_),
    z(1) {}

  dim::dim(uintptr_t x_, uintptr_t y_, uintptr_t z_) :
    x(x_),
    y(y_),
    z(z_) {}

  dim::dim(const dim &d) :
    x(d.x),
    y(d.y),
    z(d.z) {}

  dim& dim::operator = (const dim &d) {
    x = d.x;
    y = d.y;
    z = d.z;

    return *this;
  }

  dim dim::operator + (const dim &d) {
    return dim(x + d.x,
               y + d.y,
               z + d.z);
  }

  dim dim::operator - (const dim &d) {
    return dim(x - d.x,
               y - d.y,
               z - d.z);
  }

  dim dim::operator * (const dim &d) {
    return dim(x * d.x,
               y * d.y,
               z * d.z);
  }

  dim dim::operator / (const dim &d) {
    return dim(x / d.x,
               y / d.y,
               z / d.z);
  }

  bool dim::hasNegativeEntries() {
    return ((x & (1 << (sizeof(uintptr_t) - 1))) ||
            (y & (1 << (sizeof(uintptr_t) - 1))) ||
            (z & (1 << (sizeof(uintptr_t) - 1))));
  }

  uintptr_t& dim::operator [] (int i) {
    switch(i) {
    case 0 : return x;
    case 1 : return y;
    default: return z;
    }
  }

  uintptr_t dim::operator [] (int i) const {
    switch(i) {
    case 0 : return x;
    case 1 : return y;
    default: return z;
    }
  }
  //====================================

  //---[ Misc Functions ]---------------
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
  //====================================
}
