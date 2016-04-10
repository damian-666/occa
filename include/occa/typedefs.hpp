#ifndef OCCA_TYPEDEFS_HEADER
#define OCCA_TYPEDEFS_HEADER

#include <vector>
#include <map>

namespace occa {
  typedef std::vector<int>                   intVector_t;
  typedef std::vector<intVector_t>           intVecVector_t;
  typedef std::vector<std::string>           stringVector_t;

  typedef std::map<std::string, std::string> strToStrMap_t;
  typedef strToStrMap_t::iterator            strToStrMapIterator;
  typedef strToStrMap_t::const_iterator      cStrToStrMapIterator;

  typedef std::map<std::string, bool>        strToBoolMap_t;
  typedef strToBoolMap_t::iterator           strToBoolMapIterator;
  typedef strToBoolMap_t::const_iterator     cStrToBoolMapIterator;
}

#endif
