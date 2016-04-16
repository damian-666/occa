#ifndef OCCA_TOOLS_PROPERTIES_HEADER
#define OCCA_TOOLS_PROPERTIES_HEADER

#include "occa/typedefs.hpp"
#include "occa/tools/string.hpp"

namespace occa {
  class hasProperties;

  class properties {
  public:
    enum Op {
      Set, Append, Clear
    };

    typedef strToStrsMapIterator  iter_t;
    typedef cStrToStrsMapIterator citer_t;

  private:
    strToStrsMap_t props;
    hasProperties *holder;

  public:
    properties(hasProperties *holder_ = NULL);
    properties(const std::string &props);

    properties(const properties &p);
    properties& operator = (const properties &p);

    iter_t iter(const std::string &prop);
    citer_t iter(const std::string &prop) const;
    iter_t end();
    citer_t end() const;

    bool has(const std::string &prop) const;
    bool hasMultiple(const std::string &prop) const;

    std::string operator [] (const std::string &prop) const;

    template <class TM>
    TM get(const std::string &prop) const {
      citer_t it = iter(prop);
      if ((it != end()) || (it->second.size() == 0)) {
        return fromString<TM>(it->second[0]);
      }
      return TM();
    }
    std::string get(const std::string &prop) const;
    strVector_t getAll(const std::string &prop) const;

    template <class TM>
    void set(const std::string &prop, const TM &t) {
      iter_t it = iter(prop);
      std::string newValue = toString(t);

      if ((it != end()) && (it->second.size() > 0)) {
        strVector_t &oldValues = it->second;
        onChange(Set, prop, oldValues, newValue);
        it->second.clear();
        it->second.push_back(newValue);
      }
      else {
        strVector_t &oldValues = props[prop];
        onChange(Set, prop, oldValues, newValue);
        oldValues.push_back(newValue);
      }
    }

    template <class TM>
    void append(const std::string &prop, const TM &t) {
      strVector_t &oldValues = props[prop];
      std::string newValue   = toString(t);
      onChange(Append, prop, oldValues, newValue);
      oldValues.push_back(newValue);
    }

    void clear(const std::string &prop);

    void setOnChangeFunc(hasProperties &holder_);

    void onChange(properties::Op op,
                  const std::string &prop,
                  strVector_t oldValues,
                  const std::string &newValue) const;

    std::string hash() const;
  };

  class hasProperties {
  public:
    occa::properties properties;

    hasProperties();
    virtual void onChange(properties::Op op,
                          const std::string &prop,
                          strVector_t oldValues,
                          const std::string &newValue) const;
  };
}

#endif
