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

#ifndef OCCA_TOOLS_PROPERTIES_HEADER
#define OCCA_TOOLS_PROPERTIES_HEADER

#include "occa/types.hpp"
#include "occa/tools/string.hpp"
#include "occa/tools/hash.hpp"

namespace occa {
  class hasProperties;

  //---[ properties ]-------------------
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
    properties(const std::string &props_);

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
    TM get(const std::string &prop, const TM &default_ = TM()) const {
      citer_t it = iter(prop);
      if ((it != end()) || (it->second.size() == 0)) {
        return fromString<TM>(it->second[0]);
      }
      return default_;
    }
    std::string get(const std::string &prop, const std::string &default_ = "") const;

    template <class TM>
    std::vector<TM> getAll(const std::string &prop) const {
      citer_t it = iter(prop);
      std::vector<TM> ret;
      if ((it != end()) && (it->second.size())) {
        cStrVectorIterator vit = it->second.begin();
        while (vit != it->second.end()) {
          ret.push_back(fromString<TM>(*vit));
          ++vit;
        }
      }
      return ret;
    }
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

    hash_t hash() const;
  };
  //====================================

  //---[ hasProperties ]----------------
  class hasProperties {
  public:
    occa::properties properties;

    hasProperties();

    void onPropertyChange(properties::Op op,
                          const std::string &prop,
                          strVector_t oldValues,
                          const std::string &newValue) const;
  };
  //====================================
}

#endif
