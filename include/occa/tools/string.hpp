#ifndef OCCA_TOOLS_STRING_HEADER
#define OCCA_TOOLS_STRING_HEADER

#include <iostream>
#include <iomanip>

namespace occa {
  template <class TM>
  inline std::string toString(const TM &t){
    std::stringstream ss;
    ss << t;
    return ss.str();
  }

  template <>
  inline std::string toString<std::string>(const std::string &t){
    return t;
  }

  template <>
  inline std::string toString<float>(const float &t){
    std::stringstream ss;
    ss << std::scientific << std::setprecision(8) << t << 'f';
    return ss.str();
  }

  template <>
  inline std::string toString<double>(const double &t){
    std::stringstream ss;
    ss << std::scientific << std::setprecision(16) << t;
    return ss.str();
  }

  template <class TM>
  inline TM fromString(const std::string &s){
    std::stringstream ss;
    TM t;
    ss << s;
    ss >> t;
    return t;
  }

  template <>
  inline std::string fromString(const std::string &s){
    return s;
  }

  uintptr_t atoi(const char *c);
  uintptr_t atoiBase2(const char *c);
  uintptr_t atoi(const std::string &str);

  double atof(const char *c);
  double atof(const std::string &str);

  double atod(const char *c);
  double atod(const std::string &str);

  std::string stringifyBytes(uintptr_t bytes);
}

#endif
