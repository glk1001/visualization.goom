#ifndef LIBS_UTILS_STREAM_CONVERT_H_
#define LIBS_UTILS_STREAM_CONVERT_H_

#include "utils/test_utils.hpp"
#include <string>
#include <cstring>
#include <stdexcept>
#include <sstream>


template <typename T> inline T convert_to(const std::string& str)
{
    std::istringstream s_str(str);
    T val;
    s_str >> val;
    if (s_str.fail()) {
        throw std::runtime_error("Invalid value: '" + str + "'");
    }
    return val;
}

template <> inline std::string convert_to(const std::string& str)
{
    return str;
}

template <> inline cstyle_str convert_to(const std::string& str)
{
    return const_cast<cstyle_str>(strdup(str.c_str()));
}

template <> inline const_cstyle_str convert_to(const std::string& str)
{
    return strdup(str.c_str());
}

template <typename T> inline std::string convert_from(const T& val)
{
    std::ostringstream s_str;
    s_str << val;
    if (s_str.fail()) {
        throw std::runtime_error("Could not convert value to string");
    }
    return s_str.str();
}

template <> inline std::string convert_from(const cstyle_str& val)
{
    return std::string(val);
}

template <> inline std::string convert_from(const const_cstyle_str& val)
{
    return std::string(val);
}

#endif
