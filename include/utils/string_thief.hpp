#pragma once 

#include <string> 

template <typename Function, Function func_ptr>
class string_thief {
 public:
  friend void string_set_length_hacker(std::string& self, std::size_t sz) {
#if defined(_MSVC_STL_VERSION)
    (self.*func_ptr)._Myval2._Mysize = sz;
#else
    (self.*func_ptr)(sz);
#endif
  }
};

#if defined(__GLIBCXX__)  // libstdc++
template class string_thief<decltype(&std::string::_M_set_length),
                            &std::string::_M_set_length>;
#elif defined(_LIBCPP_VERSION)
template class string_thief<decltype(&std::string::__set_size),
                            &std::string::__set_size>;
#elif defined(_MSVC_STL_VERSION)
template class string_thief<decltype(&std::string::_Mypair),
                            &std::string::_Mypair>;
#endif

void string_set_length_hacker(std::string&, std::size_t);

inline void string_resize(std::string& str, std::size_t sz) {
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION) || \
    defined(_MSVC_STL_VERSION)
  str.reserve(sz);
  string_set_length_hacker(str, sz);
  str[sz] = '\0';
#else
  str.resize(sz);
#endif
}



