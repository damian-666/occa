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

#include "occa/tools/properties.hpp"

namespace occa {
  //---[ properties ]-------------------
  properties::properties(hasProperties *holder_) :
    holder(holder_) {}

  properties::properties(const std::string &props) {
    if(props.size() == 0)
      return;

    parserNS::expNode expRoot = parserNS::createOrganizedExpNodeFrom(props);
    parserNS::expNode &csvFlatRoot = *(expRoot.makeCsvFlatHandle());

    for(int i = 0; i < csvFlatRoot.leafCount; ++i) {
      parserNS::expNode &leaf = csvFlatRoot[i];
      std::string &info = (leaf.leafCount ? leaf[0].value : leaf.value);

      if(leaf.value != "=") {
        std::cout << "Flag [" << info << "] was not set, skipping it\n";
        continue;
      }

      iMap[info] = leaf[1].toString();
    }

    parserNS::expNode::freeFlatHandle(csvFlatRoot);
  }

  properties::properties(const properties &p) {
    *this = p;
  }

  properties& properties::operator = (const properties &p) {
    props = p.props;
    return *this;
  }

  iter_t properties::iter(const std::string &prop) {
    return props.find(prop);
  }

  citer_t properties::iter(const std::string &prop) const {
    return props.find(prop);
  }

  iter_t properties::end() {
    return props.end();
  }

  citer_t properties::end() const {
    return props.end();
  }

  bool properties::has(const std::string &prop) const {
    return (iter(prop) != end());
  }

  bool properties::hasMultiple(const std::string &prop) const {
    iter_t it = iter(prop);
    return ((it != end()) && (it->second.size() > 1));
  }

  std::string operator [] (const std::string &prop) const {
    return get(prop);
  }

  std::string get(const std::string &prop) const {
    return get<std::string>(prop);
  }

  strVector_t properties::getAll(const std::string &prop) const {
    iter_t it = iter(prop);
    if ((it != end()) && (it->second.size()))
      return it->second;
    return strVector_t();
  }

  void properties::clear(const std::string &prop) {
    strVector_t &oldValues = props[prop];
    onChange(Op::Clear, prop, oldValues, "");
    oldValues.clear();
  }

  void propertie::setHolder(hasProperties &holder_) {
    holder = &holder_;
  }

  void properties::onChange(properties::Op op,
                            const std::string &prop,
                            strVector_t oldValues,
                            const std::string &newValue) const {
    if (holder){
      holder->onChange(op, prop, oldValues, newValue);
    }
  }

  hash_t properties::hash() const {
    iter_t it = iter(prop);
    hash_t hash;
    while (it != end()) {
      hash ^= hash(it->first)
      hash ^= hash(it->second)
    }
    return hash;
  }
  //====================================

  //---[ hasProperties ]----------------
  hasProperties::hasProperties() : properties(this) {}
  //====================================
}
