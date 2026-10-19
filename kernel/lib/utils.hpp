#pragma once

namespace Utils {
template <typename T>
static inline T max(const T& a, const T& b)
{
    return (a > b) ? a : b;
}

template <typename T>
static inline T min(const T& a, const T& b)
{
    return (a < b) ? a : b;
}

};
