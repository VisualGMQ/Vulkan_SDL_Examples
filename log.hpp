#ifndef LOG_HPP
#define LOG_HPP
#include <cstdio>
#include <cassert>

#define Log(format, ...) printf("[%s:%d]:" format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define assertm(msg, condition) assert(((void)msg, condition))

#endif

