#include "occa/tools/properties.hpp"

namespace occa {
  properties::properties() {}

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

  iter_t properties::iter(std::string prop) {
    return props.find(prop);
  }

  iter_t properties::end() {
    return props.end();
  }

  bool properties::has(std::string prop) {
    return (iter(prop) != end());
  }

  bool properties::hasMultiple(std::string prop) {
    iter_t it = iter(prop);
    return ((it != end()) && (it->second.size() > 1));
  }

  std::string& operator [] (std::string prop) {
    iter_t it = iter(prop);
    if ((it == end()) || (it->second.size() == 0)) {
      props[prop].push_back("");
      it = iter(prop);
    }
    return props[prop][0];
  }

  strVector_t properties::getAll(std::string prop) {
    iter_t it = iter(prop);
    if ((it != end()) && (it->second.size()))
      return it->second;
    return strVector_t();
  }
}
