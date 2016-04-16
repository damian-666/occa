#include "occa/tools/properties.hpp"

namespace occa {
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

  hasProperties::hasProperties() : properties(this) {}
}
