#ifndef OCCA_TOOLS_PROPERTIES_HEADER
#define OCCA_TOOLS_PROPERTIES_HEADER

#include "occa/typedefs.hpp"

namespace occa {
  class properties {
    typedef strToStrsMapIterator iter_t;
    strToStrsMap props;

    properties();
    properties(const std::string &props);

    iter_t iter(std::string prop);
    iter_t end();

    bool has(std::string prop);
    bool hasMultiple(std::string prop);

    std::string& operator [] (std::string prop);

    template <class TM>
    TM get(std::string prop) {
      iter_t it = iter(prop);
      if ((it != end()) || (it->second.size() == 0)) {
        return fromString(it->second[0]);
      }
      return TM();
    }
    strVector_t getAll(std::string prop);

    template <class TM>
    void set(std::string prop, const TM &t) {
      set(prop, toString(t));
    }

    template <class TM>
    void append(std::string prop, const TM &t) {
      props[prop].push_back(toString(t));
    }

    std::string hash();
  };
}

#endif