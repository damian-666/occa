#ifndef OCCA_THREADS_UTILS_HEADER
#define OCCA_THREADS_UTILS_HEADER

#include <iostream>
#include <queue>

#include "occa/modes/threads/headers.hpp"
#include "occa/device.hpp"

namespace occa {
  namespace threads {
    //---[ Types ]----------------------
    enum schedule_t {
      compact, scatter, manual
    };

    std::string toString(schedule_t s) {
      switch(s) {
      case compact: return "compact";
      case scatter: return "scatter";
      case manual : return "manual";
      }
      return "compact";
    }

    class job_t {
    public:
      int rank, count;
      schedule_t schedule;

      handleFunction_t handle;

      int dims;
      occa::dim inner, outer;

      std::vector<void*> args;

      job_t();
      job_t(const job_t &k);
      job_t& operator = (const job_t &k);
    };

    struct workerData_t {
      int rank, count;
      int pinnedCore;

      std::queue<job_t> *jobs;

      mutex_t *jobMutex, *kernelMutex;
    };
    //==================================

    //---[ Functions ]------------------
    void* limbo(void *args);
    void run(job_t &pArgs);
    //==================================
  }
}

#endif
